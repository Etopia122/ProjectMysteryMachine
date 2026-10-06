// Game data that lives in parts of the GameCube executable the decomp hasn't
// split into source files yet. Values come from the GIHE78 DOL.
#include <stdint.h>

// iAnim scratch buffer (0x80239028, 0x23A0 bytes on GameCube).
alignas(32) static unsigned char s_animScratch[0x23A0];
unsigned char* giAnimScratch = s_animScratch;

// iColor_tag constants (r, g, b, a).
unsigned char g_BLACK[4] = { 0x00, 0x00, 0x00, 0xFF };
unsigned char g_NEON_RED[4] = { 0xFF, 0x20, 0x00, 0xFF };
unsigned char g_NEON_GREEN[4] = { 0x20, 0xFF, 0x00, 0xFF };
unsigned char g_PIMP_GOLD[8] = { 0xD7, 0xDC, 0x13, 0xFF };

// Month and weekday names used by the save-game date display (iSystem).
const char* lbl_801AD940[12] = { "January ", "February ", "March ", "April ", "May ", "June ",
                                 "July ", "August ", "September ", "October ", "November ", "December " };
const char* lbl_801AD970[7] = { "Sunday ", "Monday ", "Tuesday ", "Wednesday ", "Thursday ", "Friday ", "Saturday " };
