// GameCube audio (AX voices + MIX volumes + AI output) on PC.
//
// On hardware the DSP runs a 5 ms audio frame: it reads every voice's parameter
// block (VPB) from main memory, decodes DSP-ADPCM samples out of ARAM, resamples
// them to 32 kHz and mixes them, then raises an interrupt in which the game's
// AX callback runs. The game reads and writes parts of the VPB directly (the
// sample-rate ratio, the current play address), so voices here use the same
// layout and the mixer treats the VPB as the source of truth, like the DSP.
//
// Frames are produced on the platform's audio thread (pc_audio_mix); with no
// audio device a timer thread produces them at the same rate so the game's
// frame counter still runs.
#include "pc_platform.h"

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <deque>
#include <mutex>
#include <chrono>
#include <string>
#include <thread>

namespace {

// Older Dolphin SDK parameter-block layout (the parts the game and mixer use).
struct AXPBADDR
{
    unsigned short loopFlag;
    unsigned short format;
    unsigned short loopAddressHi, loopAddressLo;
    unsigned short endAddressHi, endAddressLo;
    unsigned short currentAddressHi, currentAddressLo;
};

struct AXPBADPCM
{
    unsigned short a[8][2];
    unsigned short gain;
    unsigned short pred_scale;
    unsigned short yn1, yn2;
};

struct AXPB
{
    unsigned char pad0[0x6E];
    AXPBADDR addr;      // 0x6E
    AXPBADPCM adpcm;    // 0x7E
    unsigned short ratioHi, ratioLo; // 0xA6
    unsigned short currentAddressFrac;
    unsigned short last_samples[4];
    unsigned short loop_pred_scale, loop_yn1, loop_yn2;
};

struct AXVPB
{
    unsigned char pad0[0x1C];
    unsigned int sync;  // 0x1C
    unsigned char pad20[0x118];
    AXPB pb;            // 0x138
};
static_assert(offsetof(AXVPB, pb) == 0x138, "VPB layout");
static_assert(offsetof(AXPB, addr) == 0x6E, "PB layout");
static_assert(offsetof(AXPB, adpcm) == 0x7E, "PB layout");
static_assert(offsetof(AXPB, ratioHi) == 0xA6, "PB layout");

enum { MAX_VOICES = 64, FRAME_SAMPLES = 160, OUT_RATE = 32000 };
const unsigned int ARAM_BYTES = 16 * 1024 * 1024;

struct Voice
{
    AXVPB vpb;            // must stay first: the game holds AXVPB pointers
    bool used;
    bool running;
    unsigned short type;  // 0: normal (loop restarts ADPCM history), 1: stream
    unsigned int priority;
    void (*dropCallback)(void*);
    // Resampler state.
    double frac;
    float prev, cur;
    bool primed;
    // MIX state (0.1 dB units; pan 0..127, 64 = centre).
    int input, fader, pan;
    bool mute;
};

Voice s_voices[MAX_VOICES];
void (*s_frameCallback)();
bool s_inited;
bool s_stereo = true;
std::atomic<bool> s_timerRunning;

inline unsigned int addr32(unsigned short hi, unsigned short lo) { return (unsigned int)hi << 16 | lo; }

inline void set_current(AXPB& pb, unsigned int a)
{
    pb.addr.currentAddressHi = (unsigned short)(a >> 16);
    pb.addr.currentAddressLo = (unsigned short)a;
}

inline float db_to_gain(int db10)
{
    if (db10 <= -904)
    {
        return 0.0f;
    }
    return powf(10.0f, db10 / 200.0f);
}

// Debug: PC_LOG_AUDIO=1 logs voice starts and stops.
bool log_audio()
{
    static int on = -1;
    if (on < 0)
    {
        on = getenv("PC_LOG_AUDIO") ? 1 : 0;
    }
    return on != 0;
}

// Decodes the next ADPCM sample of a voice and advances its address
// (in nibbles; the first two nibbles of each 8-byte frame are the header).
float next_sample(Voice& v)
{
    AXPB& pb = v.vpb.pb;
    unsigned int a = addr32(pb.addr.currentAddressHi, pb.addr.currentAddressLo);
    if ((a >> 1) + 8 >= ARAM_BYTES)
    {
        static int warned;
        if (warned++ < 8)
        {
            pc_log("[audio] voice %d address %08x outside ARAM (loop %08x end %08x flag %d)", (int)(&v - s_voices), a,
                   addr32(pb.addr.loopAddressHi, pb.addr.loopAddressLo),
                   addr32(pb.addr.endAddressHi, pb.addr.endAddressLo), pb.addr.loopFlag);
        }
        v.running = false;
        return 0.0f;
    }
    if ((a & 15) == 0)
    {
        pb.adpcm.pred_scale = pc_aram_ptr(a >> 1)[0];
        a += 2;
    }
    const unsigned char b = pc_aram_ptr(a >> 1)[0];
    int nib = (a & 1) ? (b & 15) : (b >> 4);
    if (nib >= 8)
    {
        nib -= 16;
    }
    int ps = pb.adpcm.pred_scale;
    int c1 = (short)pb.adpcm.a[(ps >> 4) & 7][0], c2 = (short)pb.adpcm.a[(ps >> 4) & 7][1];
    int yn1 = (short)pb.adpcm.yn1, yn2 = (short)pb.adpcm.yn2;
    int s = ((nib << (ps & 15)) << 11) + c1 * yn1 + c2 * yn2;
    s = (s + 1024) >> 11;
    if (s > 32767) s = 32767;
    if (s < -32768) s = -32768;
    pb.adpcm.yn2 = (unsigned short)yn1;
    pb.adpcm.yn1 = (unsigned short)s;

    unsigned int end = addr32(pb.addr.endAddressHi, pb.addr.endAddressLo);
    if (a >= end)
    {
        if (pb.addr.loopFlag)
        {
            a = addr32(pb.addr.loopAddressHi, pb.addr.loopAddressLo);
            if (v.type == 0)
            {
                pb.adpcm.pred_scale = pb.loop_pred_scale;
                pb.adpcm.yn1 = pb.loop_yn1;
                pb.adpcm.yn2 = pb.loop_yn2;
            }
        }
        else
        {
            // Like the DSP: the play position moves to the loop address and the
            // voice stops. The game points one-shots' and streams' loop address at a
            // silent buffer and frees the voice once the position lands there.
            if (log_audio())
            {
                pc_log("[audio] voice %d ended at %08x (no loop)", (int)(&v - s_voices), a);
            }
            v.running = false;
            set_current(pb, addr32(pb.addr.loopAddressHi, pb.addr.loopAddressLo));
            return s / 32768.0f;
        }
    }
    else
    {
        a++;
    }
    set_current(pb, a);
    return s / 32768.0f;
}

void mix_frame(float* out)
{
    memset(out, 0, sizeof(float) * FRAME_SAMPLES * 2);
    for (int i = 0; i < MAX_VOICES; i++)
    {
        Voice& v = s_voices[i];
        if (!v.used || !v.running)
        {
            continue;
        }
        AXPB& pb = v.vpb.pb;
        if (pb.addr.format != 0)
        {
            continue; // only DSP-ADPCM is used by the game
        }
        double ratio = pb.ratioHi + pb.ratioLo / 65536.0;
        if (ratio <= 0)
        {
            continue;
        }
        float gain = v.mute ? 0.0f : db_to_gain(v.input + v.fader);
        float gl = gain, gr = gain;
        if (s_stereo)
        {
            float t = (v.pan < 0 ? 0 : v.pan > 127 ? 127 : v.pan) / 127.0f;
            gl *= cosf(t * 1.5707964f);
            gr *= sinf(t * 1.5707964f);
        }
        for (int n = 0; n < FRAME_SAMPLES && v.running; n++)
        {
            if (!v.primed)
            {
                v.prev = v.cur = next_sample(v);
                v.primed = true;
            }
            v.frac += ratio;
            while (v.frac >= 1.0 && v.running)
            {
                v.frac -= 1.0;
                v.prev = v.cur;
                v.cur = next_sample(v);
            }
            float s = v.prev + (v.cur - v.prev) * (float)v.frac;
            out[n * 2] += s * gl;
            out[n * 2 + 1] += s * gr;
        }
    }
}

// Movie soundtrack: interleaved stereo at 32 kHz, queued by the FMV player.
std::mutex s_fmvLock;
std::deque<float> s_fmvQueue;

void mix_fmv(float* out)
{
    std::lock_guard<std::mutex> lk(s_fmvLock);
    size_t n = s_fmvQueue.size() < FRAME_SAMPLES * 2 ? s_fmvQueue.size() : FRAME_SAMPLES * 2;
    for (size_t i = 0; i < n; i++)
    {
        out[i] += s_fmvQueue.front();
        s_fmvQueue.pop_front();
    }
}

// Debug: PC_STATS=1 logs voice usage every 10 seconds.
void log_voices()
{
    static int frames;
    static int on = -1;
    if (on < 0)
    {
        on = getenv("PC_STATS") ? 1 : 0;
    }
    if (!on || ++frames % 2000)
    {
        return;
    }
    int used = 0, running = 0;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        used += s_voices[i].used;
        running += s_voices[i].used && s_voices[i].running;
    }
    pc_log("[audio] voices used %d playing %d", used, running);
}

// One 5 ms DSP frame: mix, then run the game's AX callback as an interrupt.
void run_frame(float* out)
{
    pc_irq_run([out]() {
        log_voices();
        mix_frame(out);
        mix_fmv(out);
        if (s_frameCallback)
        {
            s_frameCallback();
        }
    });
}

float s_pending[FRAME_SAMPLES * 2];
int s_pendingPos = FRAME_SAMPLES;

// Debug: PC_WAV=1 records the mixed output to audio.wav next to the program.
FILE* s_wav;
unsigned int s_wavBytes;

void wav_write(const int16_t* data, int bytes)
{
    static int state = -1;
    if (state < 0)
    {
        state = getenv("PC_WAV") ? 1 : 0;
        if (state)
        {
            std::string path = std::string(pc_base_path()) + "audio.wav";
            s_wav = fopen(path.c_str(), "wb");
            unsigned char hdr[44] = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,
                                      0x00, 0x7D, 0, 0, 0x00, 0xF4, 0x01, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0 };
            if (s_wav)
            {
                fwrite(hdr, 1, 44, s_wav);
            }
        }
    }
    if (!s_wav)
    {
        return;
    }
    fwrite(data, 1, bytes, s_wav);
    s_wavBytes += bytes;
    // Keep the header valid so the file is playable even if the game is killed.
    long pos = ftell(s_wav);
    unsigned int riff = s_wavBytes + 36;
    fseek(s_wav, 4, SEEK_SET);
    fwrite(&riff, 4, 1, s_wav);
    fseek(s_wav, 40, SEEK_SET);
    fwrite(&s_wavBytes, 4, 1, s_wav);
    fseek(s_wav, pos, SEEK_SET);
    fflush(s_wav);
}

} // namespace

// Fills n stereo frames of 16-bit output at 32 kHz (called by the platform's
// audio thread).
extern "C" void pc_audio_mix(int16_t* o, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (s_pendingPos >= FRAME_SAMPLES)
        {
            run_frame(s_pending);
            s_pendingPos = 0;
        }
        for (int c = 0; c < 2; c++)
        {
            float f = s_pending[s_pendingPos * 2 + c] * 32767.0f;
            o[i * 2 + c] = (int16_t)(f > 32767 ? 32767 : f < -32768 ? -32768 : f);
        }
        s_pendingPos++;
    }
    wav_write(o, n * 4);
}

namespace
{

void timer_thread()
{
    using namespace std::chrono;
    const microseconds period(5000);
    steady_clock::time_point next = steady_clock::now();
    float buf[FRAME_SAMPLES * 2];
    while (s_timerRunning)
    {
        next += period;
        std::this_thread::sleep_until(next);
        // PC_PROF=1: average mixing time per 5 ms audio frame.
        static bool prof = getenv("PC_PROF") != NULL;
        unsigned long long t0 = prof ? pc_perf_counter() : 0;
        run_frame(buf);
        if (prof)
        {
            static unsigned long long total;
            static int frames;
            total += pc_perf_counter() - t0;
            if (++frames == 200)
            {
                pc_log("[prof] audio mix %.2f ms per 5 ms frame", pc_perf_seconds(total) * 1000.0 / frames);
                total = 0;
                frames = 0;
            }
        }
    }
}

void start_output()
{
    if (s_inited)
    {
        return;
    }
    s_inited = true;
    if (pc_audio_output_open(OUT_RATE))
    {
        return;
    }
    pc_log("[audio] no output device; running silent");
    s_timerRunning = true;
    pc_thread_start(timer_thread, "audio timer");
}

} // namespace

extern "C" {

// ---------------------------------------------------------------- AI / AX
void AIInit(unsigned char* stack) {}

void AXInit()
{
    memset(s_voices, 0, sizeof(s_voices));
    start_output();
}

void AXQuit()
{
    pc_irq_run([]() {
        s_frameCallback = NULL;
        for (int i = 0; i < MAX_VOICES; i++)
        {
            s_voices[i].running = false;
            s_voices[i].used = false;
        }
    });
}

typedef void (*AXCallback)();
AXCallback AXRegisterCallback(AXCallback cb)
{
    AXCallback old = s_frameCallback;
    s_frameCallback = cb;
    return old;
}

AXVPB* AXAcquireVoice(unsigned int priority, void (*callback)(void*), unsigned int userContext)
{
    Voice* found = NULL;
    pc_irq_run([&]() {
        for (int i = 0; i < MAX_VOICES && !found; i++)
        {
            if (!s_voices[i].used)
            {
                found = &s_voices[i];
            }
        }
        if (found)
        {
            memset(found, 0, sizeof(*found));
            found->used = true;
            found->priority = priority;
            found->dropCallback = callback;
            found->pan = 64;
            found->input = found->fader = -904;
            found->mute = true;
        }
    });
    if (!found)
    {
        static int warned;
        if (!warned++)
        {
            pc_log("[audio] out of voices");
        }
        return NULL;
    }
    return &found->vpb;
}

static Voice* voice_of(AXVPB* p) { return (Voice*)p; }

void AXFreeVoice(AXVPB* p)
{
    if (p)
    {
        pc_irq_run([p]() {
            voice_of(p)->running = false;
            voice_of(p)->used = false;
        });
    }
}

void AXSetVoiceState(AXVPB* p, unsigned short state)
{
    if (!p)
    {
        return;
    }
    pc_irq_run([p, state]() {
        Voice* v = voice_of(p);
        if (log_audio())
        {
            AXPB& pb = p->pb;
            pc_log("[audio] voice %d state %d: addr %08x loop %08x end %08x loopflag %d format %d ratio %d.%05d",
                   (int)(v - s_voices), state, addr32(pb.addr.currentAddressHi, pb.addr.currentAddressLo),
                   addr32(pb.addr.loopAddressHi, pb.addr.loopAddressLo),
                   addr32(pb.addr.endAddressHi, pb.addr.endAddressLo), pb.addr.loopFlag, pb.addr.format, pb.ratioHi,
                   pb.ratioLo);
        }
        if (state && !v->running)
        {
            v->frac = 0;
            v->primed = false;
        }
        v->running = state != 0;
    });
}

void AXSetVoiceType(AXVPB* p, unsigned short type)
{
    if (p)
    {
        voice_of(p)->type = type;
    }
}

void AXSetVoiceAddr(AXVPB* p, AXPBADDR* addr)
{
    if (p)
    {
        pc_irq_run([p, addr]() { p->pb.addr = *addr; });
    }
}

void AXSetVoiceAdpcm(AXVPB* p, AXPBADPCM* adpcm)
{
    if (p)
    {
        pc_irq_run([p, adpcm]() {
            p->pb.adpcm = *adpcm;
            p->pb.loop_pred_scale = adpcm->pred_scale;
            p->pb.loop_yn1 = 0;
            p->pb.loop_yn2 = 0;
        });
    }
}

void AXSetVoiceSrcType(AXVPB* p, unsigned int type) {}

void AXSetVoiceLoop(AXVPB* p, unsigned short loop)
{
    if (p)
    {
        p->pb.addr.loopFlag = loop;
    }
}

void AXSetVoiceLoopAddr(AXVPB* p, unsigned int addr)
{
    if (p)
    {
        pc_irq_run([p, addr]() {
            p->pb.addr.loopAddressHi = (unsigned short)(addr >> 16);
            p->pb.addr.loopAddressLo = (unsigned short)addr;
        });
    }
}

void AXSetVoiceEndAddr(AXVPB* p, unsigned int addr)
{
    if (p)
    {
        pc_irq_run([p, addr]() {
            p->pb.addr.endAddressHi = (unsigned short)(addr >> 16);
            p->pb.addr.endAddressLo = (unsigned short)addr;
        });
    }
}

// ---------------------------------------------------------------- MIX
void MIXInit() {}

void MIXInitChannel(AXVPB* p, unsigned int mode, int input, int auxA, int auxB, int pan, int span, int fader)
{
    if (!p)
    {
        return;
    }
    Voice* v = voice_of(p);
    v->input = input;
    v->fader = fader;
    v->pan = pan;
    v->mute = (mode & 4) != 0;
}

void MIXReleaseChannel(AXVPB* p) {}
void MIXUnMute(AXVPB* p)
{
    if (p)
    {
        voice_of(p)->mute = false;
    }
}
void MIXMute(AXVPB* p)
{
    if (p)
    {
        voice_of(p)->mute = true;
    }
}
void MIXUpdateSettings() {}

int MIXGetInput(AXVPB* p) { return p ? voice_of(p)->input : 0; }
int MIXGetFader(AXVPB* p) { return p ? voice_of(p)->fader : 0; }
int MIXGetPan(AXVPB* p) { return p ? voice_of(p)->pan : 64; }

void MIXAdjustInput(AXVPB* p, int db)
{
    if (p)
    {
        Voice* v = voice_of(p);
        v->input = v->input + db > 60 ? 60 : v->input + db;
    }
}

void MIXAdjustFader(AXVPB* p, int db)
{
    if (p)
    {
        Voice* v = voice_of(p);
        v->fader = v->fader + db > 60 ? 60 : v->fader + db;
    }
}

void MIXAdjustPan(AXVPB* p, int pan)
{
    if (p)
    {
        Voice* v = voice_of(p);
        v->pan = v->pan + pan;
    }
}

void pc_audio_set_stereo(int stereo) { s_stereo = stereo != 0; }

// FMV soundtrack: append interleaved stereo samples at 32 kHz.
void pc_audio_fmv_queue(const float* samples, int frames)
{
    std::lock_guard<std::mutex> lk(s_fmvLock);
    s_fmvQueue.insert(s_fmvQueue.end(), samples, samples + frames * 2);
}

// Seconds of FMV audio queued but not yet played.
double pc_audio_fmv_queued()
{
    std::lock_guard<std::mutex> lk(s_fmvLock);
    return s_fmvQueue.size() / 2.0 / OUT_RATE;
}

void pc_audio_fmv_clear()
{
    std::lock_guard<std::mutex> lk(s_fmvLock);
    s_fmvQueue.clear();
}

}
