// Full-motion video on the New 3DS. The game's Bink movies are re-encoded once
// on a PC (n3ds/tools/convert_fmv.sh) to Ogg Theora at 320x240 with 32 kHz
// Vorbis, which the 3DS decodes in software (libtheora, Tremor); the picture
// goes straight into a GPU texture (n3ds_fmv_frame_yuv in gpu_c3d.cpp). Behaviour
// follows the PC player (pc/src/fmv.cpp): iFMVPlay(path, buttons, time) blocks
// until the movie ends, or until one of 'buttons' is pressed after 'time'
// seconds; the soundtrack goes through the AX mixer's movie queue.
//
// FMV/<name>.bik plays <fmv dir>/<name>.ogv (romfs:/fmv in a CIA built with the
// movies, else sdmc:/scooby/fmv); missing movies are skipped.
// Debug: PC_NOFMV=1 skips all movies.
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <tremor/ivorbiscodec.h>

#include "pc_platform.h"

void pc_events_pump();
extern "C" const char* n3ds_fmv_dir();
extern "C" void n3ds_fmv_frame_yuv(const unsigned char* y, int ys, const unsigned char* u, const unsigned char* v,
                                   int cs, int w, int h);

// Game side: pad polling as done by the original player.
signed int xPadUpdate(signed int idx, float time_passed);
extern unsigned char mPad[];
static unsigned int pad_pressed() { return *(unsigned int*)(mPad + 0x30); }

extern "C" unsigned char* sFMVWorkArea;
unsigned char* sFMVWorkArea;

namespace {

class Player
{
public:
    Player(FILE* f, unsigned int buttons, float skipTime, bool letterbox)
        : m_file(f), m_buttons(buttons), m_skipTime(skipTime)
    {
        ogg_sync_init(&m_sync);
        th_info_init(&m_ti);
        th_comment_init(&m_tc);
        vorbis_info_init(&m_vi);
        vorbis_comment_init(&m_vc);
    }

    ~Player()
    {
        if (m_td)
        {
            th_decode_free(m_td);
        }
        if (m_ts)
        {
            th_setup_free(m_ts);
        }
        if (m_vorbisReady)
        {
            vorbis_block_clear(&m_vb);
            vorbis_dsp_clear(&m_vd);
        }
        if (m_hasTheora)
        {
            ogg_stream_clear(&m_to);
        }
        if (m_hasVorbis)
        {
            ogg_stream_clear(&m_vo);
        }
        th_info_clear(&m_ti);
        th_comment_clear(&m_tc);
        vorbis_info_clear(&m_vi);
        vorbis_comment_clear(&m_vc);
        ogg_sync_clear(&m_sync);
        fclose(m_file);
        pc_audio_fmv_clear();
    }

    void run()
    {
        if (!read_headers())
        {
            pc_log("[fmv] not a Theora movie");
            return;
        }
        m_td = th_decode_alloc(&m_ti, m_ts);
        if (m_hasVorbis)
        {
            vorbis_synthesis_init(&m_vd, &m_vi);
            vorbis_block_init(&m_vd, &m_vb);
            m_vorbisReady = true;
        }
        pc_log("[fmv] %ux%u, %.2f fps, %s", m_ti.pic_width, m_ti.pic_height,
               (double)m_ti.fps_numerator / m_ti.fps_denominator, m_vorbisReady ? "with sound" : "silent");
        m_start = pc_perf_counter();
        bool frameReady = false;
        double frameTime = 0;
        while (!m_done)
        {
            // Keep about half a second of soundtrack queued ahead of playback.
            bool audioNeedsData = false;
            while (m_vorbisReady && pc_audio_fmv_queued() < 0.5)
            {
                if (!decode_audio())
                {
                    audioNeedsData = true;
                    break;
                }
            }
            if (!frameReady)
            {
                ogg_packet op;
                if (ogg_stream_packetout(&m_to, &op) > 0)
                {
                    ogg_int64_t gp = -1;
                    unsigned long long t0 = pc_perf_counter();
                    int r = th_decode_packetin(m_td, &op, &gp);
                    m_decodeTicks += pc_perf_counter() - t0;
                    if (r >= 0)
                    {
                        // th_granule_time gives the frame's end time.
                        frameTime = th_granule_time(m_td, gp) - (double)m_ti.fps_denominator / m_ti.fps_numerator;
                        frameReady = true;
                    }
                    continue;
                }
            }
            if (!frameReady || audioNeedsData)
            {
                if (!read_pages())
                {
                    if (!frameReady)
                    {
                        break; // end of the file
                    }
                    audioNeedsData = false;
                }
                if (!frameReady)
                {
                    continue;
                }
            }
            // Wait for the frame's time, keeping the pad and the system responsive.
            while (!m_done && clock() < frameTime)
            {
                poll_skip();
                svcSleepThread(1000000);
            }
            if (m_done)
            {
                break;
            }
            if (clock() > frameTime + 1.0 / 30)
            {
                m_late++;
            }
            unsigned long long t0 = pc_perf_counter();
            show_frame();
            m_showTicks += pc_perf_counter() - t0;
            m_frames++;
            frameReady = false;
            poll_skip();
        }
        if (m_frames)
        {
            pc_log("[fmv] %d frames in %.1f s, %d late; per frame: decode %.1f ms, events %.1f ms, convert+show %.1f ms", m_frames,
                   clock(), m_late, pc_perf_seconds(m_decodeTicks) * 1000.0 / m_frames,
                   pc_perf_seconds(m_showTicks - m_presentTicks) * 1000.0 / m_frames,
                   pc_perf_seconds(m_presentTicks) * 1000.0 / m_frames);
        }
        // Let the end of the soundtrack play unless the movie was skipped.
        while (!m_done && pc_audio_fmv_queued() > 0.0)
        {
            poll_skip();
            svcSleepThread(5000000);
        }
    }

private:
    double clock() const { return pc_perf_seconds(pc_perf_counter() - m_start); }

    // Polls the pad like the original player; sets m_done when skipped.
    void poll_skip()
    {
        pc_events_pump();
        xPadUpdate(0, 0.0f);
        if (m_buttons && clock() >= m_skipTime && (pad_pressed() & m_buttons))
        {
            m_done = true;
        }
    }

    // Reads more of the file into the stream(s); false at the end.
    bool read_pages()
    {
        char* buf = ogg_sync_buffer(&m_sync, 65536);
        size_t n = fread(buf, 1, 65536, m_file);
        ogg_sync_wrote(&m_sync, (long)n);
        ogg_page og;
        while (ogg_sync_pageout(&m_sync, &og) > 0)
        {
            queue_page(&og);
        }
        return n > 0;
    }

    void queue_page(ogg_page* og)
    {
        if (m_hasTheora && ogg_page_serialno(og) == m_to.serialno)
        {
            ogg_stream_pagein(&m_to, og);
        }
        else if (m_hasVorbis && ogg_page_serialno(og) == m_vo.serialno)
        {
            ogg_stream_pagein(&m_vo, og);
        }
    }

    // Identifies the streams and reads their three header packets each.
    bool read_headers()
    {
        int theoraHeaders = 0, vorbisHeaders = 0;
        ogg_page og;
        ogg_packet op;
        // Beginning-of-stream pages name the streams.
        bool bos = true;
        while (bos)
        {
            if (ogg_sync_pageout(&m_sync, &og) <= 0)
            {
                char* buf = ogg_sync_buffer(&m_sync, 4096);
                size_t n = fread(buf, 1, 4096, m_file);
                if (n == 0)
                {
                    return false;
                }
                ogg_sync_wrote(&m_sync, (long)n);
                continue;
            }
            if (!ogg_page_bos(&og))
            {
                queue_page(&og);
                bos = false;
                break;
            }
            ogg_stream_state test;
            ogg_stream_init(&test, ogg_page_serialno(&og));
            ogg_stream_pagein(&test, &og);
            ogg_stream_packetout(&test, &op);
            if (!m_hasTheora && th_decode_headerin(&m_ti, &m_tc, &m_ts, &op) >= 0)
            {
                memcpy(&m_to, &test, sizeof(test));
                m_hasTheora = true;
                theoraHeaders = 1;
            }
            else if (!m_hasVorbis && vorbis_synthesis_headerin(&m_vi, &m_vc, &op) >= 0)
            {
                memcpy(&m_vo, &test, sizeof(test));
                m_hasVorbis = true;
                vorbisHeaders = 1;
            }
            else
            {
                ogg_stream_clear(&test);
            }
        }
        if (!m_hasTheora)
        {
            return false;
        }
        while ((theoraHeaders < 3) || (m_hasVorbis && vorbisHeaders < 3))
        {
            int r;
            while (theoraHeaders < 3 && (r = ogg_stream_packetout(&m_to, &op)) != 0)
            {
                if (r < 0 || th_decode_headerin(&m_ti, &m_tc, &m_ts, &op) < 0)
                {
                    return false;
                }
                theoraHeaders++;
            }
            while (m_hasVorbis && vorbisHeaders < 3 && (r = ogg_stream_packetout(&m_vo, &op)) != 0)
            {
                if (r < 0 || vorbis_synthesis_headerin(&m_vi, &m_vc, &op) < 0)
                {
                    return false;
                }
                vorbisHeaders++;
            }
            if (theoraHeaders < 3 || (m_hasVorbis && vorbisHeaders < 3))
            {
                if (!read_pages())
                {
                    return false;
                }
            }
        }
        return true;
    }

    // Decodes some soundtrack into the movie queue; false if it needs more data.
    bool decode_audio()
    {
        ogg_int32_t** pcm;
        int n = vorbis_synthesis_pcmout(&m_vd, &pcm);
        if (n > 0)
        {
            int channels = m_vi.channels;
            m_pcm.resize((size_t)n * 2);
            for (int i = 0; i < n; i++)
            {
                // Tremor outputs 24-bit fixed point.
                float l = pcm[0][i] * (1.0f / 16777216.0f);
                float r = channels > 1 ? pcm[1][i] * (1.0f / 16777216.0f) : l;
                m_pcm[i * 2] = l;
                m_pcm[i * 2 + 1] = r;
            }
            pc_audio_fmv_queue(m_pcm.data(), n);
            vorbis_synthesis_read(&m_vd, n);
            return true;
        }
        ogg_packet op;
        if (ogg_stream_packetout(&m_vo, &op) > 0)
        {
            if (vorbis_synthesis(&m_vb, &op) == 0)
            {
                vorbis_synthesis_blockin(&m_vd, &m_vb);
            }
            return true;
        }
        return false;
    }

    // Shows the decoded YCbCr 4:2:0 picture (converted straight into the GPU
    // texture by the citro3d backend).
    void show_frame()
    {
        th_ycbcr_buffer yuv;
        th_decode_ycbcr_out(m_td, yuv);
        int w = (int)m_ti.pic_width & ~1, h = (int)m_ti.pic_height & ~1;
        int ox = (int)m_ti.pic_x & ~1, oy = (int)m_ti.pic_y & ~1;
        unsigned long long t0 = pc_perf_counter();
        n3ds_fmv_frame_yuv(yuv[0].data + oy * yuv[0].stride + ox, yuv[0].stride,
                           yuv[1].data + (oy >> 1) * yuv[1].stride + (ox >> 1),
                           yuv[2].data + (oy >> 1) * yuv[2].stride + (ox >> 1), yuv[1].stride, w, h);
        m_presentTicks += pc_perf_counter() - t0;
        pc_events_pump();
    }

    FILE* m_file;
    unsigned int m_buttons;
    float m_skipTime;
    bool m_done = false;
    ogg_sync_state m_sync;
    ogg_stream_state m_to, m_vo;
    bool m_hasTheora = false, m_hasVorbis = false, m_vorbisReady = false;
    th_info m_ti;
    th_comment m_tc;
    th_setup_info* m_ts = NULL;
    th_dec_ctx* m_td = NULL;
    vorbis_info m_vi;
    vorbis_comment m_vc;
    vorbis_dsp_state m_vd;
    vorbis_block m_vb;
    std::vector<float> m_pcm;
    unsigned long long m_start = 0;
    int m_frames = 0, m_late = 0;
    unsigned long long m_decodeTicks = 0, m_showTicks = 0, m_presentTicks = 0;
};

} // namespace

// filename is the disc path, e.g. "FMV/Intro.bik".
signed int iFMVPlay(char* filename, unsigned int buttons, float time)
{
    if (filename == NULL)
    {
        return 1;
    }
    if (getenv("PC_NOFMV"))
    {
        return 0;
    }
    std::string name = filename;
    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
    {
        name = name.substr(slash + 1);
    }
    size_t dot = name.rfind('.');
    if (dot != std::string::npos)
    {
        name = name.substr(0, dot);
    }
    std::string path = std::string(n3ds_fmv_dir()) + name + ".ogv";
    FILE* f = fopen(path.c_str(), "rb");
    if (!f)
    {
        pc_log("[fmv] %s not found (convert the movies with n3ds/tools/convert_fmv.sh); skipping", path.c_str());
        return 0;
    }
    pc_log("[fmv] playing %s", path.c_str());
    Player player(f, buttons, time, strncmp(filename, "FMV/cin", 7) == 0);
    player.run();
    return 0;
}
