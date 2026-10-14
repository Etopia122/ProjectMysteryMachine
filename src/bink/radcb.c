/*
 * RAD callback handlers: periodic timer alarms that poll a list of
 * registered callbacks (used for Bink's background IO and sound service).
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/rad.h"

typedef struct RADCB RADCB;
typedef S32 (*RADCBCHECK)(RADCB* cb, U32 ticks);
typedef void (*RADCBFUNC)(RADCB* cb, U32 ticks);

struct RADCB
{
    RADCB* next;     /* 0x00 */
    U32 user;        /* 0x04 */
    S32 result;      /* 0x08 */
    volatile S32 state;    /* 0x0C: 1 running, 2 suspended */
    volatile S32 suspends; /* 0x10 */
    RADCBCHECK check; /* 0x14 */
    RADCBFUNC func;  /* 0x18 */
};

typedef struct RADCBHANDLER
{
    volatile S32 state;    /* 0x00: 1 running, 2 suspended */
    volatile S32 suspends; /* 0x04 */
    OSAlarm alarm; /* 0x08 */
    RADCB* list;  /* 0x30 */
    U32 ticks;    /* 0x34 */
    S32 count;    /* 0x38 */
} RADCBHANDLER;

void RADCB_free_handler(RADCBHANDLER* h);

static S32 remove_cb_from_list(RADCBHANDLER* h, RADCB* cb)
{
    RADCB* c;

    if (h == 0)
        return 0;

    c = h->list;
    if (c == cb)
    {
        h->list = c->next;
        return 1;
    }
    while (c->next)
    {
        if (c->next == cb)
        {
            c->next = cb->next;
            return 1;
        }
        c = c->next;
    }
    return 0;
}

static void call_callbacks(RADCBHANDLER* h)
{
    RADCB* cb;

    h->ticks++;
    if (h->state == 1)
    {
        for (cb = h->list; cb; cb = cb->next)
        {
            if (cb->state == 1)
            {
                cb->result = cb->check(cb, h->ticks);
                if (cb->result)
                    cb->func(cb, h->ticks);
            }
        }
    }
}

static void RAD_callback_timer(OSAlarm* alarm, OSContext* context)
{
    RADCBHANDLER* h = (RADCBHANDLER*)((U8*)alarm - 8);

    if (h && h->list)
    {
        int level = OSEnableInterrupts();
        call_callbacks(h);
        OSRestoreInterrupts(level);
    }
}

RADCBHANDLER* RADCB_allocate_handler(U32 hz)
{
    RADCBHANDLER* h = (RADCBHANDLER*)radmalloc(sizeof(RADCBHANDLER));

    if (h)
    {
        memset(h, 0, sizeof(RADCBHANDLER));
        h->state = 0;
        h->ticks = 1;
        h->count = 0;
        h->suspends = 1;
        h->list = 0;
        OSSetPeriodicAlarm(&h->alarm, OSGetTime(), (1000 / hz) * (OS_TIMER_CLOCK / 1000), RAD_callback_timer);
    }
    return h;
}

void RADCB_register_callback(RADCBHANDLER* h, RADCB* cb, RADCBCHECK check, RADCBFUNC func)
{
    int level = OSDisableInterrupts();
    cb->state = 1;
    cb->check = check;
    cb->func = func;
    cb->suspends = 0;
    cb->next = h->list;
    h->list = cb;
    h->count++;
    OSRestoreInterrupts(level);
}

S32 RADCB_unregister_callback(RADCBHANDLER* h, RADCB* cb, U32 flags)
{
    S32 freed = 0;

    if (h && cb && cb->func && cb->check)
    {
        int level = OSDisableInterrupts();
        if (remove_cb_from_list(h, cb))
        {
            h->count--;
            if ((flags & 1) && h->list == 0)
            {
                freed = 1;
                RADCB_free_handler(h);
            }
        }
        OSRestoreInterrupts(level);
    }
    return freed;
}

S32 RADCB_unregister_2_callbacks(RADCBHANDLER* h1, RADCB* cb1, RADCBHANDLER* h2, RADCB* cb2, U32 flags)
{
    S32 r = RADCB_unregister_callback(h1, cb1, flags);
    return r | (RADCB_unregister_callback(h2, cb2, flags) << 1);
}

S32 RADCB_registered_count(RADCBHANDLER* h)
{
    return h->count;
}

S32 RADCB_try_to_suspend_handler(RADCBHANDLER* h)
{
    if (h)
    {
        int level = OSDisableInterrupts();
        if (h->state == 1)
            h->state = 2;
        h->suspends++;
        OSRestoreInterrupts(level);
        return (h->state == 1) ? 0 : 1;
    }
    return 1;
}

void RADCB_suspend_handler(RADCBHANDLER* h)
{
    while (!RADCB_try_to_suspend_handler(h))
        OSYieldThread();
}

void RADCB_resume_handler(RADCBHANDLER* h)
{
    if (h)
    {
        int level = OSDisableInterrupts();
        if (--h->suspends == 0)
            h->state = 1;
        OSRestoreInterrupts(level);
    }
}

S32 RADCB_try_to_suspend_callback(RADCBHANDLER* h, RADCB* cb)
{
    if (h && cb && cb->func && cb->check)
    {
        int level = OSDisableInterrupts();
        if (cb->state == 1)
            cb->state = 2;
        cb->suspends++;
        OSRestoreInterrupts(level);
        return (cb->state == 1) ? 0 : 1;
    }
    return 1;
}

void RADCB_suspend_callback(RADCBHANDLER* h, RADCB* cb)
{
    if (h && cb && cb->func && cb->check)
    {
        while (!RADCB_try_to_suspend_callback(h, cb))
            OSYieldThread();
    }
}

void RADCB_suspend_2_callbacks(RADCBHANDLER* h1, RADCB* cb1, RADCBHANDLER* h2, RADCB* cb2)
{
    RADCB_suspend_callback(h1, cb1);
    RADCB_suspend_callback(h2, cb2);
}

void RADCB_resume_callback(RADCBHANDLER* h, RADCB* cb)
{
    if (h && cb && cb->func && cb->check)
    {
        int level = OSDisableInterrupts();
        if (--cb->suspends == 0)
            cb->state = 1;
        OSRestoreInterrupts(level);
    }
}

void RADCB_suspend_2_handlers(RADCBHANDLER* h1, RADCBHANDLER* h2)
{
    RADCB_suspend_handler(h1);
    RADCB_suspend_handler(h2);
}

void RADCB_free_handler(RADCBHANDLER* h)
{
    if (h)
    {
        RADCB_suspend_handler(h);
        OSCancelAlarm(&h->alarm);
        radfree(h);
    }
}

void RADCB_free_2_handlers(RADCBHANDLER* h1, RADCBHANDLER* h2)
{
    RADCB_free_handler(h1);
    RADCB_free_handler(h2);
}

void RADCB_idle_on_callbacks(void)
{
}

U32 RADCB_callback_size(void)
{
    return sizeof(RADCB);
}
