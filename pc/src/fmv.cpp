// Full-motion video on PC: replaces the GameCube iFMV.cpp (Bink SDK + GX) with
// FFmpeg, which decodes Bink 1 video and audio. The game-level behaviour is kept:
// iFMVPlay(path, buttons, time) blocks until the movie ends, or until one of
// 'buttons' is pressed after 'time' seconds (the game's pad state is polled the
// same way the original does). The soundtrack is mixed into the AX output.
//
// Debug: PC_NOFMV=1 skips all movies.
#include "pc_platform.h"

#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

void pc_events_pump();

// Game side: pad polling as done by the original player.
signed int xPadUpdate(signed int idx, float time_passed);
extern unsigned char mPad[];
static unsigned int pad_pressed() { return *(unsigned int*)(mPad + 0x30); }

extern "C" unsigned char* sFMVWorkArea;
unsigned char* sFMVWorkArea;

namespace {

struct Decoder
{
    AVCodecContext* ctx = NULL;
    int stream = -1;

    bool open(AVFormatContext* fmt, AVMediaType type)
    {
        const AVCodec* codec = NULL;
        stream = av_find_best_stream(fmt, type, -1, -1, &codec, 0);
        if (stream < 0 || !codec)
        {
            if (type == AVMEDIA_TYPE_VIDEO)
            {
                char err[128];
                av_strerror(stream, err, sizeof(err));
                pc_log("[fmv] find stream: %d (%s), codec %p, streams %u", stream, err, codec, fmt->nb_streams);
            }
            stream = -1;
            return false;
        }
        ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(ctx, fmt->streams[stream]->codecpar);
        if (avcodec_open2(ctx, codec, NULL) < 0)
        {
            avcodec_free_context(&ctx);
            stream = -1;
            return false;
        }
        return true;
    }

    ~Decoder() { avcodec_free_context(&ctx); }
};

class Player
{
public:
    Player(const char* path, unsigned int buttons, float skipTime, bool letterbox)
        : m_path(path), m_buttons(buttons), m_skipTime(skipTime), m_letterbox(letterbox)
    {
    }

    ~Player()
    {
        av_frame_free(&m_frame);
        av_packet_free(&m_packet);
        sws_freeContext(m_sws);
        swr_free(&m_swr);
        if (m_fmt)
        {
            avformat_close_input(&m_fmt);
        }
        pc_audio_fmv_clear();
    }

    void run()
    {
        if (avformat_open_input(&m_fmt, m_path.c_str(), NULL, NULL) < 0)
        {
            pc_log("[fmv] can't open %s", m_path.c_str());
            return;
        }
        avformat_find_stream_info(m_fmt, NULL);
        if (!m_video.open(m_fmt, AVMEDIA_TYPE_VIDEO))
        {
            pc_log("[fmv] no video stream in %s", m_path.c_str());
            return;
        }
        m_audio.open(m_fmt, AVMEDIA_TYPE_AUDIO);
        if (m_audio.ctx)
        {
            AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            swr_alloc_set_opts2(&m_swr, &stereo, AV_SAMPLE_FMT_FLT, 32000, &m_audio.ctx->ch_layout,
                                m_audio.ctx->sample_fmt, m_audio.ctx->sample_rate, 0, NULL);
            if (!m_swr || swr_init(m_swr) < 0)
            {
                swr_free(&m_swr);
            }
        }
        pc_log("[fmv] %s: %dx%d, %s", m_path.c_str(), m_video.ctx->width, m_video.ctx->height,
               m_swr ? "with sound" : "silent");

        m_frame = av_frame_alloc();
        m_packet = av_packet_alloc();
        m_start = SDL_GetPerformanceCounter();
        while (!m_done && av_read_frame(m_fmt, m_packet) >= 0)
        {
            if (m_packet->stream_index == m_video.stream)
            {
                decode_video(m_packet);
            }
            else if (m_packet->stream_index == m_audio.stream && m_swr)
            {
                decode_audio(m_packet);
            }
            av_packet_unref(m_packet);
        }
        if (!m_done)
        {
            decode_video(NULL); // flush
        }
        // Let the end of the soundtrack play unless the movie was skipped.
        while (!m_done && pc_audio_fmv_queued() > 0.0)
        {
            poll_skip();
            SDL_Delay(5);
        }
    }

private:
    double clock() const
    {
        return (double)(SDL_GetPerformanceCounter() - m_start) / SDL_GetPerformanceFrequency();
    }

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

    void decode_video(AVPacket* pkt)
    {
        if (avcodec_send_packet(m_video.ctx, pkt) < 0)
        {
            return;
        }
        while (!m_done && avcodec_receive_frame(m_video.ctx, m_frame) == 0)
        {
            AVRational tb = m_fmt->streams[m_video.stream]->time_base;
            double t = m_frame->best_effort_timestamp == AV_NOPTS_VALUE
                           ? m_lastPts + 1.0 / 30
                           : m_frame->best_effort_timestamp * av_q2d(tb);
            m_lastPts = t;
            int w = m_frame->width, h = m_frame->height;
            m_sws = sws_getCachedContext(m_sws, w, h, (AVPixelFormat)m_frame->format, w, h, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, NULL, NULL, NULL);
            m_rgba.resize((size_t)w * h * 4);
            uint8_t* dst[1] = { m_rgba.data() };
            int dstStride[1] = { w * 4 };
            sws_scale(m_sws, m_frame->data, m_frame->linesize, 0, h, dst, dstStride);
            // Wait for the frame's time, keeping the pad and window responsive.
            while (!m_done && clock() < t)
            {
                poll_skip();
                SDL_Delay(1);
            }
            if (m_done)
            {
                break;
            }
            pc_render_fmv_frame(m_rgba.data(), w, h, m_letterbox);
            poll_skip();
        }
    }

    void decode_audio(AVPacket* pkt)
    {
        if (avcodec_send_packet(m_audio.ctx, pkt) < 0)
        {
            return;
        }
        while (avcodec_receive_frame(m_audio.ctx, m_frame) == 0)
        {
            int out = swr_get_out_samples(m_swr, m_frame->nb_samples);
            m_pcm.resize((size_t)out * 2);
            uint8_t* dst[1] = { (uint8_t*)m_pcm.data() };
            int n = swr_convert(m_swr, dst, out, (const uint8_t**)m_frame->extended_data, m_frame->nb_samples);
            if (n > 0)
            {
                pc_audio_fmv_queue(m_pcm.data(), n);
            }
        }
    }

    std::string m_path;
    unsigned int m_buttons;
    float m_skipTime;
    bool m_letterbox;
    bool m_done = false;
    AVFormatContext* m_fmt = NULL;
    Decoder m_video, m_audio;
    AVFrame* m_frame = NULL;
    AVPacket* m_packet = NULL;
    SwsContext* m_sws = NULL;
    SwrContext* m_swr = NULL;
    std::vector<uint8_t> m_rgba;
    std::vector<float> m_pcm;
    Uint64 m_start = 0;
    double m_lastPts = 0;
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
    std::string path = std::string(pc_data_dir()) + "/" + filename;
    Player player(path.c_str(), buttons, time, strncmp(filename, "FMV/cin", 7) == 0);
    player.run();
    return 0;
}
