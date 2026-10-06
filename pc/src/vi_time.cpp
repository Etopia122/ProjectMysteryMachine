// Video interface (retrace timing) and OS time on PC.
// A 60 Hz "retrace" thread advances the retrace counter and runs the game's
// pre-retrace callback as an interrupt, like the GameCube's VI.
#include "pc_platform.h"

#include <SDL.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <time.h>

typedef void (*VIRetraceCallback)(unsigned int retraceCount);
typedef long long OSTime;

// GameCube OS timer: bus clock / 4 = 40.5 MHz.
static const long long OS_TIMER_CLOCK = 40500000;

static std::atomic<unsigned int> s_retraceCount;
static VIRetraceCallback s_preCB;
static VIRetraceCallback s_postCB;
static std::mutex s_viLock;
static std::condition_variable s_viCv;
static std::chrono::steady_clock::time_point s_start = std::chrono::steady_clock::now();

static void vi_thread()
{
    using namespace std::chrono;
    const nanoseconds period(16683333); // 59.94 Hz
    steady_clock::time_point next = steady_clock::now() + period;
    for (;;)
    {
        std::this_thread::sleep_until(next);
        next += period;
        unsigned int count = ++s_retraceCount;
        VIRetraceCallback pre = s_preCB, post = s_postCB;
        if (pre || post)
        {
            pc_irq_post([pre, post, count]() {
                if (pre) pre(count);
                if (post) post(count);
            });
        }
        s_viCv.notify_all();
    }
}

extern "C" {

void VIInit()
{
    static bool started;
    if (!started)
    {
        started = true;
        std::thread(vi_thread).detach();
    }
}

unsigned int VIGetRetraceCount() { return s_retraceCount; }

void VIWaitForRetrace()
{
    std::unique_lock<std::mutex> lk(s_viLock);
    unsigned int c = s_retraceCount;
    s_viCv.wait_for(lk, std::chrono::milliseconds(50), [c] { return s_retraceCount != c; });
}

VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb)
{
    VIRetraceCallback old = s_preCB;
    s_preCB = cb;
    return old;
}

VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb)
{
    VIRetraceCallback old = s_postCB;
    s_postCB = cb;
    return old;
}

unsigned int VIGetNextField() { return 1; }
void VIFlush() {}
void VISetBlack(int black) {}
void VIConfigure(const void* rm) {}
void VISetNextFrameBuffer(void* fb) {}

OSTime OSGetTime()
{
    using namespace std::chrono;
    long long ns = duration_cast<nanoseconds>(steady_clock::now() - s_start).count();
    return (OSTime)((double)ns * (OS_TIMER_CLOCK / 1e9));
}

unsigned int OSGetTick() { return (unsigned int)OSGetTime(); }

struct OSCalendarTime
{
    int sec, min, hour, mday, mon, year, wday, yday, msec, usec;
};

// The game passes its own tick values (from OSGetTime); map them to wall-clock time.
void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* td)
{
    using namespace std::chrono;
    static const OSTime nowTicks = OSGetTime();
    static const time_t nowWall = time(NULL);
    long long secs = (long long)((ticks - nowTicks) / OS_TIMER_CLOCK);
    time_t t = nowWall + (time_t)secs;
    struct tm* lt = localtime(&t);
    td->sec = lt->tm_sec;
    td->min = lt->tm_min;
    td->hour = lt->tm_hour;
    td->mday = lt->tm_mday;
    td->mon = lt->tm_mon;
    td->year = lt->tm_year + 1900;
    td->wday = lt->tm_wday;
    td->yday = lt->tm_yday;
    long long sub = (ticks % OS_TIMER_CLOCK + OS_TIMER_CLOCK) % OS_TIMER_CLOCK;
    td->msec = (int)(sub * 1000 / OS_TIMER_CLOCK);
    td->usec = (int)(sub * 1000000 / OS_TIMER_CLOCK % 1000);
}

void OSInit() {}
unsigned int OSGetResetButtonState() { return 0; }
unsigned int OSGetSoundMode() { return 1; } // stereo
void OSSetSoundMode(unsigned int mode) {}

void OSResetSystem(int reset, unsigned int resetCode, int forceMenu)
{
    pc_log("[os] OSResetSystem(%d, %u, %d): exiting", reset, resetCode, forceMenu);
    SDL_Quit();
    exit(0);
}

void OSPanic(const char* file, int line, const char* msg, ...)
{
    pc_log("[os] PANIC %s:%d: %s", file, line, msg);
    SDL_Quit();
    exit(1);
}

// Cache maintenance has no meaning on PC.
void DCInvalidateRange(void* addr, unsigned int n) {}
void DCStoreRange(void* addr, unsigned int n) {}
void DCFlushRange(void* addr, unsigned int n) {}
void ICInvalidateRange(void* addr, unsigned int n) {}

}
