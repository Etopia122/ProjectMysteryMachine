// PC audio output: an SDL stream that pulls mixed samples from audio.cpp.
#include "pc_platform.h"

#include <SDL.h>

static void SDLCALL sdl_callback(void*, Uint8* stream, int len)
{
    pc_audio_mix((short*)stream, len / 4);
}

extern "C" int pc_audio_output_open(int rate)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        return 0;
    }
    SDL_AudioSpec want = {}, have = {};
    want.freq = rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    want.callback = sdl_callback;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!dev)
    {
        pc_log("[audio] SDL: %s", SDL_GetError());
        return 0;
    }
    pc_log("[audio] %d Hz, %d channels", have.freq, have.channels);
    SDL_PauseAudioDevice(dev, 0);
    return 1;
}
