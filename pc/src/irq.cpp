// Interrupt emulation.
// On GameCube, hardware completions (disc, audio, video) run callbacks from
// interrupt handlers, and the game guards shared state with
// OSDisableInterrupts/OSRestoreInterrupts. On PC, "interrupt" work runs on one
// background thread and the interrupt state is a recursive lock: while the game
// has interrupts disabled, no callback can run, as on hardware.
#include "pc_platform.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

static std::recursive_mutex s_irqLock;
static thread_local int s_disableDepth;

static std::mutex s_queueLock;
static std::condition_variable s_queueCv;
static std::deque<std::function<void()> > s_queue;

static void irq_thread()
{
    for (;;)
    {
        std::function<void()> fn;
        {
            std::unique_lock<std::mutex> lk(s_queueLock);
            s_queueCv.wait(lk, [] { return !s_queue.empty(); });
            fn = s_queue.front();
            s_queue.pop_front();
        }
        std::lock_guard<std::recursive_mutex> irq(s_irqLock);
        fn();
    }
}

void pc_irq_post(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> lk(s_queueLock);
        static bool started;
        if (!started)
        {
            started = true;
            pc_thread_start(irq_thread, "irq");
        }
        s_queue.push_back(fn);
    }
    s_queueCv.notify_one();
}

// Runs fn now on the calling thread as an "interrupt" (game interrupts held off).
void pc_irq_run(const std::function<void()>& fn)
{
    std::lock_guard<std::recursive_mutex> irq(s_irqLock);
    fn();
}

extern "C" {

int OSDisableInterrupts()
{
    if (s_disableDepth > 64)
    {
        pc_log("[irq] OSDisableInterrupts nested %d deep without restore", s_disableDepth);
        __builtin_trap();
    }
    s_irqLock.lock();
    return s_disableDepth++ == 0;
}

int OSEnableInterrupts()
{
    // Only used to undo a disable at the outermost level.
    int wasEnabled = s_disableDepth == 0;
    while (s_disableDepth > 0)
    {
        s_disableDepth--;
        s_irqLock.unlock();
    }
    return wasEnabled;
}

int OSRestoreInterrupts(int level)
{
    if (s_disableDepth > 0)
    {
        s_disableDepth--;
        s_irqLock.unlock();
    }
    return s_disableDepth == 0;
}

}
