// New 3DS audio output: one ndsp channel streaming the mixed 32 kHz stereo
// output of audio.cpp, refilled by a thread on the second CPU core.
#include <3ds.h>
#include <string.h>

#include "pc_platform.h"

enum { BUFFERS = 4, FRAMES_PER_BUFFER = 640 }; // 4 x 20 ms

static ndspWaveBuf s_wave[BUFFERS];
static s16* s_samples;
static volatile bool s_run;
static LightEvent s_event;

static void ndsp_callback(void*)
{
    LightEvent_Signal(&s_event);
}

static void refill_thread(void*)
{
    while (s_run)
    {
        for (int i = 0; i < BUFFERS; i++)
        {
            if (s_wave[i].status == NDSP_WBUF_DONE || s_wave[i].status == NDSP_WBUF_FREE)
            {
                pc_audio_mix((short*)s_wave[i].data_pcm16, FRAMES_PER_BUFFER);
                DSP_FlushDataCache(s_wave[i].data_pcm16, FRAMES_PER_BUFFER * 4);
                ndspChnWaveBufAdd(0, &s_wave[i]);
            }
        }
        LightEvent_Wait(&s_event);
    }
}

extern "C" int pc_audio_output_open(int rate)
{
    if (R_FAILED(ndspInit()))
    {
        pc_log("[audio] ndspInit failed (missing DSP firmware dump?)");
        return 0;
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, (float)rate);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    float mix[12] = { 1.0f, 1.0f };
    ndspChnSetMix(0, mix);
    s_samples = (s16*)linearAlloc(BUFFERS * FRAMES_PER_BUFFER * 4);
    memset(s_samples, 0, BUFFERS * FRAMES_PER_BUFFER * 4);
    memset(s_wave, 0, sizeof(s_wave));
    for (int i = 0; i < BUFFERS; i++)
    {
        s_wave[i].data_pcm16 = s_samples + i * FRAMES_PER_BUFFER * 2;
        s_wave[i].nsamples = FRAMES_PER_BUFFER;
        s_wave[i].status = NDSP_WBUF_FREE;
    }
    LightEvent_Init(&s_event, RESET_ONESHOT);
    ndspSetCallback(ndsp_callback, NULL);
    s_run = true;
    // Core 2 is the New 3DS's extra application core; fall back to core 1.
    Thread t = threadCreate(refill_thread, NULL, 32 * 1024, 0x18, 2, true);
    if (!t)
    {
        t = threadCreate(refill_thread, NULL, 32 * 1024, 0x18, 1, true);
    }
    pc_log("[audio] ndsp %d Hz stereo, refill thread %s", rate, t ? "started" : "failed");
    return t != NULL;
}
