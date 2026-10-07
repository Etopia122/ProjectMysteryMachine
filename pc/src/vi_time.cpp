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
#include <string.h>
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

// Seconds since 2000-01-01 in local time: the GameCube clock's epoch (its RTC
// holds local time).
unsigned long pc_local_seconds_since_2000()
{
    time_t now = time(NULL);
    struct tm lt = *localtime(&now);
    return (unsigned long)(_mkgmtime(&lt) - 946684800);
}

// OS time: timer ticks since the 2000 epoch, like the hardware.
OSTime OSGetTime()
{
    using namespace std::chrono;
    static const OSTime base = (OSTime)pc_local_seconds_since_2000() * OS_TIMER_CLOCK;
    long long ns = duration_cast<nanoseconds>(steady_clock::now() - s_start).count();
    return base + (OSTime)((double)ns * (OS_TIMER_CLOCK / 1e9));
}

unsigned int OSGetTick() { return (unsigned int)OSGetTime(); }

struct OSCalendarTime
{
    int sec, min, hour, mday, mon, year, wday, yday, msec, usec;
};

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime* td)
{
    long long secs = ticks / OS_TIMER_CLOCK;
    long long sub = ticks % OS_TIMER_CLOCK;
    if (sub < 0)
    {
        sub += OS_TIMER_CLOCK;
        secs--;
    }
    time_t t = (time_t)(946684800 + secs);
    struct tm* gt = gmtime(&t);
    if (!gt)
    {
        memset(td, 0, sizeof(*td));
        td->year = 2000;
        td->mday = 1;
        return;
    }
    td->sec = gt->tm_sec;
    td->min = gt->tm_min;
    td->hour = gt->tm_hour;
    td->mday = gt->tm_mday;
    td->mon = gt->tm_mon;
    td->year = gt->tm_year + 1900;
    td->wday = gt->tm_wday;
    td->yday = gt->tm_yday;
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
