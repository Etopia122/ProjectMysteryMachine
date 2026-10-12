// GameCube memory card (CARD API) on PC.
// Slot A is a 251-block card kept as a folder next to the executable
// (save/cardA): each card file is <name>.dat, its CARDStat lives in
// <name>.stat, and dir.txt maps file numbers to names (one per line, empty
// lines are free entries) so numbers stay stable like on a real card.
// Slot B has no card. Asynchronous calls finish at once; their callback runs
// as an interrupt, like the hardware's completion.
#include "pc_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <string>
#include <vector>

extern "C" {

typedef void (*CARDCallback)(long chan, long result);

struct CARDFileInfo
{
    long chan;
    long fileNo;
    long offset;
    long length;
    unsigned short iBlock;
    unsigned short padding;
};

struct CARDStat
{
    char fileName[32];
    unsigned long length;
    unsigned long time;
    unsigned char gameName[4];
    unsigned char company[2];
    unsigned char bannerFormat;
    unsigned char padding;
    unsigned long iconAddr;
    unsigned short iconFormat;
    unsigned short iconSpeed;
    unsigned long commentAddr;
    unsigned long offsetBanner;
    unsigned long offsetBannerTlut;
    unsigned long offsetIcon[8];
    unsigned long offsetIconTlut;
    unsigned long offsetData;
};

}

namespace {

enum
{
    READY = 0, BUSY = -1, NOCARD = -3, NOFILE = -4, IOERROR = -5, EXIST = -7, INSSPACE = -9,
    LIMIT = -11, NAMETOOLONG = -12,
};
const unsigned long SECTOR = 8192;
const long BLOCKS = 251;   // user blocks on a "Memory Card 251"
const long MAX_FILES = 127;
const long MEM_SIZE = 16;  // megabits
const char GAME_CODE[] = "GIHE"; // disc ID GIHE78
const char MAKER_CODE[] = "78";

struct Card
{
    bool mounted = false;
    long result = READY;
    long xferred = 0;
    std::vector<std::string> dir; // file number -> name ("" = free)
};

Card s_cards[2];

std::string card_dir()
{
    static std::string dir;
    if (dir.empty())
    {
        // PC_SAVE_DIR=<name> (under the base path) keeps test runs' saves apart
        // from the player's.
        const char* other = getenv("PC_SAVE_DIR");
        dir = std::string(pc_base_path()) + (other && *other ? other : "save");
        pc_mkdir(dir.c_str());
        dir += "/cardA";
        pc_mkdir(dir.c_str());
    }
    return dir;
}

std::string data_path(const std::string& name) { return card_dir() + "/" + name + ".dat"; }
std::string stat_path(const std::string& name) { return card_dir() + "/" + name + ".stat"; }

void load_dir(Card& c)
{
    c.dir.clear();
    FILE* f = fopen((card_dir() + "/dir.txt").c_str(), "r");
    if (f)
    {
        char line[64];
        while (fgets(line, sizeof(line), f))
        {
            line[strcspn(line, "\r\n")] = 0;
            c.dir.push_back(line);
        }
        fclose(f);
    }
}

void save_dir(const Card& c)
{
    FILE* f = fopen((card_dir() + "/dir.txt").c_str(), "w");
    if (f)
    {
        for (const std::string& n : c.dir)
        {
            fprintf(f, "%s\n", n.c_str());
        }
        fclose(f);
    }
}

long file_size(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 ? (long)st.st_size : -1;
}

long find_file(const Card& c, const char* name)
{
    for (size_t i = 0; i < c.dir.size(); i++)
    {
        if (c.dir[i] == name)
        {
            return (long)i;
        }
    }
    return -1;
}

long used_blocks(const Card& c)
{
    long blocks = 0;
    for (const std::string& n : c.dir)
    {
        if (!n.empty())
        {
            blocks += (file_size(data_path(n)) + SECTOR - 1) / SECTOR;
        }
    }
    return blocks;
}

long file_count(const Card& c)
{
    long n = 0;
    for (const std::string& s : c.dir)
    {
        n += !s.empty();
    }
    return n;
}

// Seconds since 2000-01-01 (local), the card's time base.
unsigned long card_time() { return pc_local_seconds_since_2000(); }

Card* card(long chan) { return chan == 0 ? &s_cards[0] : NULL; }

// Debug: PC_LOG_CARD=1 traces the CARD calls.
bool trace()
{
    static int on = -1;
    if (on < 0)
    {
        on = getenv("PC_LOG_CARD") ? 1 : 0;
    }
    return on != 0;
}
#define TRACE(...)     do     {         if (trace())             pc_log("[card] " __VA_ARGS__);     } while (0)

long finish(long chan, long result, CARDCallback cb)
{
    if (Card* c = card(chan))
    {
        c->result = result;
    }
    if (cb)
    {
        pc_irq_post([cb, chan, result]() { cb(chan, result); });
    }
    return result;
}

} // namespace

extern "C" {

// The ports' own settings for each save slot, beside the saves (the game's
// save data stays as it is): port_flags.txt, one number per slot.
int pc_save_flags_get(int slot)
{
    int flags[3] = { 0, 0, 0 };
    if (FILE* f = fopen((card_dir() + "/port_flags.txt").c_str(), "r"))
    {
        if (fscanf(f, "%d %d %d", &flags[0], &flags[1], &flags[2]) != 3)
        {
            flags[0] = flags[1] = flags[2] = 0;
        }
        fclose(f);
    }
    return slot >= 0 && slot < 3 ? flags[slot] : 0;
}

void pc_save_flags_set(int slot, int value)
{
    if (slot < 0 || slot >= 3)
    {
        return;
    }
    int flags[3];
    for (int i = 0; i < 3; i++)
    {
        flags[i] = i == slot ? value : pc_save_flags_get(i);
    }
    if (FILE* f = fopen((card_dir() + "/port_flags.txt").c_str(), "w"))
    {
        fprintf(f, "%d %d %d\n", flags[0], flags[1], flags[2]);
        fclose(f);
    }
}

void CARDInit() {}

long CARDProbeEx(long chan, long* memSize, long* sectorSize)
{
    if (!card(chan))
    {
        return NOCARD;
    }
    if (memSize)
    {
        *memSize = MEM_SIZE;
    }
    if (sectorSize)
    {
        *sectorSize = (long)SECTOR;
    }
    return READY;
}

long CARDMount(long chan, void* workArea, CARDCallback detachCallback)
{
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    load_dir(*c);
    c->mounted = true;
    c->result = READY;
    return READY;
}

long CARDUnmount(long chan)
{
    if (Card* c = card(chan))
    {
        c->mounted = false;
        return READY;
    }
    return NOCARD;
}

long CARDCheckEx(long chan, long* xferBytes)
{
    if (xferBytes)
    {
        *xferBytes = 0;
    }
    return card(chan) ? READY : NOCARD;
}

long CARDGetSectorSize(long chan, unsigned long* size)
{
    if (size)
    {
        *size = SECTOR;
    }
    return card(chan) ? READY : NOCARD;
}

long CARDGetEncoding(long chan, unsigned short* encode)
{
    if (encode)
    {
        *encode = 0; // ANSI
    }
    return card(chan) ? READY : NOCARD;
}

long CARDGetResultCode(long chan)
{
    Card* c = card(chan);
    TRACE("resultcode %ld -> %ld", chan, c ? c->result : (long)NOCARD);
    return c ? c->result : NOCARD;
}

long CARDGetXferredBytes(long chan)
{
    Card* c = card(chan);
    return c ? c->xferred : 0;
}

long CARDFreeBlocks(long chan, long* bytesNotUsed, long* filesNotUsed)
{
    TRACE("freeblocks %ld", chan);
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    if (bytesNotUsed)
    {
        *bytesNotUsed = (BLOCKS - used_blocks(*c)) * (long)SECTOR;
    }
    if (filesNotUsed)
    {
        *filesNotUsed = MAX_FILES - file_count(*c);
    }
    return READY;
}

long CARDFormat(long chan)
{
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    for (const std::string& n : c->dir)
    {
        if (!n.empty())
        {
            remove(data_path(n).c_str());
            remove(stat_path(n).c_str());
        }
    }
    c->dir.clear();
    save_dir(*c);
    return READY;
}

long CARDOpen(long chan, const char* fileName, CARDFileInfo* info)
{
    TRACE("open %ld '%s'", chan, fileName);
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    long no = find_file(*c, fileName);
    if (no < 0)
    {
        return NOFILE;
    }
    info->chan = chan;
    info->fileNo = no;
    info->offset = 0;
    info->length = file_size(data_path(fileName));
    info->iBlock = 0;
    return READY;
}

long CARDCreate(long chan, const char* fileName, unsigned long size, CARDFileInfo* info)
{
    TRACE("create %ld '%s' %lu", chan, fileName, size);
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    if (strlen(fileName) >= 32)
    {
        return NAMETOOLONG;
    }
    if (find_file(*c, fileName) >= 0)
    {
        return EXIST;
    }
    size = (size + SECTOR - 1) / SECTOR * SECTOR;
    if (used_blocks(*c) + (long)(size / SECTOR) > BLOCKS)
    {
        return INSSPACE;
    }
    if (file_count(*c) >= MAX_FILES)
    {
        return LIMIT;
    }
    FILE* f = fopen(data_path(fileName).c_str(), "wb");
    if (!f)
    {
        return IOERROR;
    }
    std::vector<char> zero(size);
    fwrite(zero.data(), 1, size, f);
    fclose(f);
    CARDStat st = {};
    strncpy(st.fileName, fileName, sizeof(st.fileName) - 1);
    // The SDK stamps new files with the disc's game and maker codes; the game
    // only accepts saves carrying its own (GIHE / 78).
    memcpy(st.gameName, GAME_CODE, 4);
    memcpy(st.company, MAKER_CODE, 2);
    st.length = size;
    st.time = card_time();
    f = fopen(stat_path(fileName).c_str(), "wb");
    if (f)
    {
        fwrite(&st, sizeof(st), 1, f);
        fclose(f);
    }
    long no = -1;
    for (size_t i = 0; i < c->dir.size() && no < 0; i++)
    {
        if (c->dir[i].empty())
        {
            no = (long)i;
        }
    }
    if (no < 0)
    {
        no = (long)c->dir.size();
        c->dir.push_back("");
    }
    c->dir[no] = fileName;
    save_dir(*c);
    return CARDOpen(chan, fileName, info);
}

long CARDClose(CARDFileInfo* info) { return READY; }

long CARDDelete(long chan, const char* fileName)
{
    TRACE("delete %ld '%s'", chan, fileName);
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    long no = find_file(*c, fileName);
    if (no < 0)
    {
        return NOFILE;
    }
    remove(data_path(fileName).c_str());
    remove(stat_path(fileName).c_str());
    c->dir[no] = "";
    save_dir(*c);
    return READY;
}

static long transfer(CARDFileInfo* info, void* buf, long length, long offset, bool write)
{
    TRACE("%s file %ld len %ld off %ld", write ? "write" : "read", info->fileNo, length, offset);
    Card* c = card(info->chan);
    if (!c)
    {
        return NOCARD;
    }
    if (info->fileNo < 0 || info->fileNo >= (long)c->dir.size() || c->dir[info->fileNo].empty())
    {
        return NOFILE;
    }
    FILE* f = fopen(data_path(c->dir[info->fileNo]).c_str(), write ? "r+b" : "rb");
    if (!f)
    {
        return IOERROR;
    }
    fseek(f, offset, SEEK_SET);
    size_t n = write ? fwrite(buf, 1, (size_t)length, f) : fread(buf, 1, (size_t)length, f);
    fclose(f);
    c->xferred += (long)n;
    return n == (size_t)length ? READY : IOERROR;
}

long CARDRead(CARDFileInfo* info, void* addr, long length, long offset)
{
    return finish(info->chan, transfer(info, addr, length, offset, false), NULL);
}

long CARDReadAsync(CARDFileInfo* info, void* addr, long length, long offset, CARDCallback cb)
{
    long r = transfer(info, addr, length, offset, false);
    finish(info->chan, r, cb);
    return r;
}

long CARDWriteAsync(CARDFileInfo* info, const void* addr, long length, long offset, CARDCallback cb)
{
    long r = transfer(info, (void*)addr, length, offset, true);
    finish(info->chan, r, cb);
    return r;
}

long CARDGetStatus(long chan, long fileNo, CARDStat* stat)
{
    TRACE("getstatus %ld %ld", chan, fileNo);
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    if (fileNo < 0 || fileNo >= (long)c->dir.size() || c->dir[fileNo].empty())
    {
        return NOFILE;
    }
    const std::string& name = c->dir[fileNo];
    memset(stat, 0, sizeof(*stat));
    FILE* f = fopen(stat_path(name).c_str(), "rb");
    if (f)
    {
        fread(stat, sizeof(*stat), 1, f);
        fclose(f);
    }
    strncpy(stat->fileName, name.c_str(), sizeof(stat->fileName) - 1);
    stat->length = (unsigned long)file_size(data_path(name));
    if (!stat->gameName[0])
    {
        memcpy(stat->gameName, GAME_CODE, 4);
        memcpy(stat->company, MAKER_CODE, 2);
    }
    return READY;
}

long CARDSetStatus(long chan, long fileNo, CARDStat* stat)
{
    TRACE("setstatus %ld %ld", chan, fileNo);
    Card* c = card(chan);
    if (!c)
    {
        return NOCARD;
    }
    if (fileNo < 0 || fileNo >= (long)c->dir.size() || c->dir[fileNo].empty())
    {
        return NOFILE;
    }
    CARDStat st = *stat;
    st.time = card_time();
    FILE* f = fopen(stat_path(c->dir[fileNo]).c_str(), "wb");
    if (!f)
    {
        return IOERROR;
    }
    fwrite(&st, sizeof(st), 1, f);
    fclose(f);
    return READY;
}

long CARDSetAttributes(long chan, long fileNo, unsigned char attr) { return card(chan) ? READY : NOCARD; }

}
