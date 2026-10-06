#ifndef PC_PLATFORM_H
#define PC_PLATFORM_H

// Shared helpers of the PC platform layer (pc/src). Not seen by game code.

#ifdef __cplusplus
#include <functional>

// Queue work to run as an "interrupt": on the interrupt thread, with
// interrupts disabled from the game's point of view.
void pc_irq_post(std::function<void()> fn);
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Folder holding the extracted disc files (default: game/files).
const char* pc_data_dir(void);

// Logging for the platform layer.
void pc_log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // PC_PLATFORM_H
