#ifndef PC_PLATFORM_H
#define PC_PLATFORM_H

// Shared helpers of the PC platform layer (pc/src). Not seen by game code.

#ifdef __cplusplus
#include <functional>

// Queue work to run as an "interrupt": on the interrupt thread, with
// interrupts disabled from the game's point of view.
void pc_irq_post(std::function<void()> fn);

// Run work as an "interrupt" right now, on the calling thread.
void pc_irq_run(const std::function<void()>& fn);
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Folder holding the extracted disc files (default: game/files).
const char* pc_data_dir(void);

// ARAM contents (audio RAM), addressed in bytes.
unsigned char* pc_aram_ptr(unsigned long addr);

// Movie playback support (audio.cpp, pc_gl.cpp).
void pc_audio_fmv_queue(const float* samples, int frames);
double pc_audio_fmv_queued(void);
void pc_audio_fmv_clear(void);
void pc_gl_fmv_frame(const unsigned char* rgba, int w, int h, int letterbox);

// Logging for the platform layer.
void pc_log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // PC_PLATFORM_H
