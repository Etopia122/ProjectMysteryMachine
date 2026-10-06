// Debug aid for x87 stack leaks (configure with -DPC_FPCHECK=ON).
// The game sources are compiled with -finstrument-functions; on every function
// entry the x87 register stack must be empty (i386 calling convention). A
// non-empty stack means the caller ignored a float return value through a
// declaration with the wrong return type: on GameCube the value just sits in
// f1, on x86 it stays on the FPU stack, and after eight of them every load
// produces NaN. Each offending call site is reported once and the stack is
// emptied so later sites are found too.
#include <stdio.h>
#include <windows.h>

struct X87Env
{
    unsigned int control, status, tag, ip, cs, dp, ds;
};

extern "C" {

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void* fn, void* site)
{
    X87Env env;
    __asm__ volatile("fnstenv %0" : "=m"(env));
    __asm__ volatile("fldenv %0" : : "m"(env));
    if ((env.tag & 0xffff) == 0xffff)
    {
        return;
    }
    static void* seen[512];
    static int nseen;
    int known = 0;
    for (int i = 0; i < nseen; i++)
    {
        known |= seen[i] == site;
    }
    if (!known && nseen < 512)
    {
        seen[nseen++] = site;
        // Addresses relative to the default image base, for addr2line.
        unsigned int base = (unsigned int)GetModuleHandleA(NULL);
        fprintf(stderr, "[fpcheck] x87 stack not empty (tag %04x) calling 0x%08x from 0x%08x\n",
                env.tag & 0xffff, (unsigned int)fn - base + 0x400000, (unsigned int)site - base + 0x400000);
    }
    env.tag = 0xffff;
    env.status &= ~0x3800u; // TOP = 0
    __asm__ volatile("fldenv %0" : : "m"(env));
}

__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void* fn, void* site) {}

}
