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

// Folder the program runs from, with a trailing slash (saves go under it).
const char* pc_base_path(void);

// Creates a folder; an existing one is fine.
void pc_mkdir(const char* path);

// Shuts the platform down and ends the program.
void pc_exit(int code);

// Starts a detached helper thread (VI retrace, interrupts, audio timer). On
// the 3DS these run above the game's priority: threads there aren't
// time-sliced, and the game busy-waits on them.
void pc_thread_start(void (*fn)(void), const char* name);

// PC_AUTOPAD test script: adds its buttons and stick to the pad state.
void pc_autopad(unsigned short* buttons, int* sx, int* sy);

// High-resolution timer for profiling.
unsigned long long pc_perf_counter(void);
double pc_perf_seconds(unsigned long long ticks);

// Long-frame diagnosis (PC_PROF): time a frame spent waiting, by kind; the
// renderer logs frames over 150 ms with their totals.
enum { PC_STALL_VI, PC_STALL_FILE, PC_STALL_PRESENT, PC_STALL_COUNT };
void pc_stall_note(int kind, unsigned long long ticks);

// Puts text on the system clipboard (where there is one).
void pc_set_clipboard(const char* text);

// Debug menu (pc/src/debug_menu.cpp), driven by the platform's input.
enum { PC_MENU_UP, PC_MENU_DOWN, PC_MENU_LEFT, PC_MENU_RIGHT, PC_MENU_OK, PC_MENU_BACK };
void pc_debug_menu_toggle(void);
int pc_debug_menu_key(int key);
int pc_debug_menu_active(void);
void pc_debug_menu_frame(void);
void pc_debug_coords_toggle(void);
void pc_debug_coords_save(void);
void pc_debug_coords_return(void);
// Environment-driven test hooks (pc/src/debug_hooks.cpp), once per frame.
void pc_debug_hooks_frame(void);

// The platform's key names for the menu's hints.
enum { PC_HINT_MENU, PC_HINT_SAVE, PC_HINT_RETURN };
const char* pc_debug_key_hint(int which);

// 1 on the New 3DS (its button names, the bottom screen), 0 on PC.
int pc_platform_3ds(void);

// The platform's changes to a texture's pixels as it loads (RGBA8), by name:
// the New 3DS draws its own buttons over the GameCube's. The renderer calls it
// for named textures; pc_raster_patch (rw layer) for ones the game finds by asset.
void pc_texture_patch(const char* name, unsigned char* rgba, int w, int h);

// The bottom screen's touch, in the HUD's 640x480 space: 1 while touched.
int pc_touch_read(int* x, int* y);

// Milliseconds since the program started.
unsigned int pc_ticks_ms(void);

// ARAM contents (audio RAM), addressed in bytes.
unsigned char* pc_aram_ptr(unsigned long addr);

// Audio output (per platform): opens a 16-bit stereo stream at 'rate' Hz
// that pulls samples with pc_audio_mix; false when there is no device.
int pc_audio_output_open(int rate);
void pc_audio_mix(short* out, int frames);

// Movie playback support (audio.cpp, pc_gl.cpp).
void pc_audio_fmv_queue(const float* samples, int frames);
double pc_audio_fmv_queued(void);
void pc_audio_fmv_clear(void);
void pc_render_fmv_frame(const unsigned char* rgba, int w, int h, int letterbox);

// Local time as seconds since 2000-01-01 (the GameCube clock's epoch).
unsigned long pc_local_seconds_since_2000(void);

// Logging for the platform layer.
void pc_log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // PC_PLATFORM_H
