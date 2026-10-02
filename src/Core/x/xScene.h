#ifndef XSCENE_H
#define XSCENE_H

#include "xBase.h"
#include "xMemMgr.h"

class _xEnt;
class _xEnv;

class _xScene
{
public:
    unsigned short flags;            // 0x0
    unsigned short num_ents;         // 0x2
    unsigned short num_trigs;        // 0x4
    unsigned short num_stats;        // 0x6
    unsigned short num_dyns;         // 0x8
    unsigned short num_npcs;         // 0xA
    unsigned short num_act_ents;     // 0xC
    unsigned short num_nact_ents;    // 0xE
    float gravity;                   // 0x10
    float drag;                      // 0x14
    float friction;                  // 0x18
    unsigned short num_ents_allocd;  // 0x1C
    unsigned short num_trigs_allocd; // 0x1E
    unsigned short num_stats_allocd; // 0x20
    unsigned short num_dyns_allocd;  // 0x22
    unsigned short num_npcs_allocd;  // 0x24
    _xEnt** trigs;                   // 0x28
    _xEnt** stats;                   // 0x2C
    _xEnt** dyns;                    // 0x30
    _xEnt** npcs;                    // 0x34
    _xEnt** act_ents;                // 0x38
    _xEnt** nact_ents;               // 0x3C
    _xEnv* env;                      // 0x40
    xMemPool mempool;                // 0x44
    xBase* (*resolvID)(unsigned int);   // 0x60
    char* (*base2Name)(xBase*);         // 0x64
    char* (*id2Name)(unsigned int);     // 0x68
};

#endif
