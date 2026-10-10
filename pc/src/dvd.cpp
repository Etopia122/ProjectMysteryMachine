// Dolphin DVD API on top of the extracted disc files (pc_data_dir()).
// "Entry numbers" index a table of every file found under the data folder.
// Synchronous reads happen immediately; asynchronous ones are read at once but
// complete (state + callback) on the interrupt thread, like a real drive.
#include "pc_platform.h"


#include <algorithm>
#include <ctype.h>
#include <dirent.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <vector>

extern "C" {

struct DVDFileInfo;
typedef void (*DVDCallback)(int result, DVDFileInfo* fileInfo);
typedef void (*DVDCBCallback)(int result, struct DVDCommandBlock* block);

struct DVDCommandBlock
{
    DVDCommandBlock* next;
    DVDCommandBlock* prev;
    unsigned int command;
    volatile int state; // 0 idle/done, 1 busy, -1 fatal error
    unsigned int offset;
    unsigned int length;
    void* addr;
    unsigned int currTransferSize;
    unsigned int transferredSize;
    void* id;
    void* callback;
    void* userData;
};

struct DVDFileInfo
{
    DVDCommandBlock cb;
    unsigned int startAddr; // PC: entry number
    unsigned int length;
    DVDCallback callback;
};

}

static_assert(sizeof(DVDCommandBlock) == 0x30, "DVDCommandBlock layout");
static_assert(sizeof(DVDFileInfo) == 0x3C, "DVDFileInfo layout");

struct Entry
{
    std::string key;  // lower-case path relative to the disc root, '/' separated
    std::string path; // host path
    unsigned int size; // SIZE_UNKNOWN until the file is first opened
    FILE* fp;
};

static const unsigned int SIZE_UNKNOWN = 0xFFFFFFFFu;

static std::vector<Entry> s_entries;
static std::mutex s_fileLock;
static bool s_ready;

const char* pc_data_dir(void)
{
    static std::string dir;
    if (dir.empty())
    {
        // SCOOBY_DATA, else game/files under the working folder, next to the
        // exe, or at the repository root when run from build/pc.
        const char* env = getenv("SCOOBY_DATA");
        if (env)
        {
            dir = env;
        }
        else
        {
            std::string exe = pc_base_path();
            const std::string candidates[] = { "game/files", exe + "game/files", exe + "../../game/files" };
            dir = candidates[0];
            for (const std::string& c : candidates)
            {
                struct stat st;
                if (stat(c.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                {
                    dir = c;
                    break;
                }
            }
        }
    }
    return dir.c_str();
}

void pc_log(const char* fmt, ...)
{
#ifdef N3DS_PORT
    fprintf(stderr, "%6.2f ", pc_ticks_ms() / 1000.0);
#endif
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

static std::string normalize(const char* p)
{
    std::string s;
    for (; *p; p++)
    {
        char c = *p == '\\' ? '/' : (char)tolower((unsigned char)*p);
        if (c == '/' && (s.empty() || s.back() == '/'))
        {
            continue;
        }
        s += c;
    }
    return s;
}

static void scan(const std::string& host, const std::string& rel)
{
    DIR* d = opendir(host.c_str());
    if (!d)
    {
        return;
    }
    while (dirent* e = readdir(d))
    {
        std::string name = e->d_name;
        if (name == "." || name == "..")
        {
            continue;
        }
        std::string hp = host + "/" + name;
        std::string rp = rel.empty() ? name : rel + "/" + name;
        // The entry type, without a stat per file when the platform reports it
        // (stat is slow on the 3DS SD card); sizes are read on first open.
        bool dir;
#ifdef DT_DIR
        if (e->d_type != DT_UNKNOWN)
        {
            dir = e->d_type == DT_DIR;
        }
        else
#endif
        {
            struct stat st;
            if (stat(hp.c_str(), &st) != 0)
            {
                continue;
            }
            dir = S_ISDIR(st.st_mode);
        }
        if (dir)
        {
            scan(hp, rp);
        }
        else
        {
            Entry en;
            en.key = normalize(rp.c_str());
            en.path = hp;
            en.size = SIZE_UNKNOWN;
            en.fp = NULL;
            s_entries.push_back(en);
        }
    }
    closedir(d);
}

static void ensure_ready()
{
    if (s_ready)
    {
        return;
    }
    s_ready = true;
    scan(pc_data_dir(), "");
    if (s_entries.empty())
    {
        pc_log("[dvd] no game files found in '%s' (extract your disc with pc/tools/extract_disc.py "
               "or set SCOOBY_DATA)", pc_data_dir());
    }
    else
    {
        pc_log("[dvd] %d files in '%s'", (int)s_entries.size(), pc_data_dir());
    }
}

// Reads from an entry; returns bytes read or -1.
// Opens an entry's file (once) and learns its size; false if it can't be opened.
static bool open_entry(Entry& e)
{
    if (!e.fp)
    {
        e.fp = fopen(e.path.c_str(), "rb");
        if (!e.fp)
        {
            return false;
        }
        fseek(e.fp, 0, SEEK_END);
        e.size = (unsigned int)ftell(e.fp);
    }
    return true;
}

static int read_entry(unsigned int entry, void* addr, int length, int offset)
{
    std::lock_guard<std::mutex> lk(s_fileLock);
    if (entry >= s_entries.size())
    {
        return -1;
    }
    Entry& e = s_entries[entry];
    if (!open_entry(e))
    {
        return -1;
    }
    if (offset < 0 || (unsigned int)offset > e.size)
    {
        return -1;
    }
    static bool prof = getenv("PC_PROF") != NULL;
    unsigned long long t0 = prof ? pc_perf_counter() : 0;
    fseek(e.fp, offset, SEEK_SET);
    size_t n = fread(addr, 1, (size_t)length, e.fp);
    if (prof)
    {
        pc_stall_note(PC_STALL_FILE, pc_perf_counter() - t0);
    }
    // The drive transfers whole 32-byte blocks; zero the tail past end of file.
    if ((int)n < length)
    {
        memset((char*)addr + n, 0, (size_t)length - n);
    }
    return (int)n;
}

extern "C" {

void DVDInit() { ensure_ready(); }

int DVDConvertPathToEntrynum(const char* path)
{
    ensure_ready();
    std::string k = normalize(path);
    for (size_t i = 0; i < s_entries.size(); i++)
    {
        if (s_entries[i].key == k)
        {
            return (int)i;
        }
    }
    char hex[64] = "";
    for (int i = 0; i < 12 && path[i]; i++)
    {
        sprintf(hex + i * 3, "%02X ", (unsigned char)path[i]);
    }
    pc_log("[dvd] file not found: %s (%s)", path, hex);
    return -1;
}

int DVDFastOpen(int entrynum, DVDFileInfo* fi)
{
    ensure_ready();
    if (entrynum < 0 || (size_t)entrynum >= s_entries.size())
    {
        return 0;
    }
    // Debug: PC_LOG_FILES=1 logs every file the game opens (a progress trace).
    static bool logFiles = getenv("PC_LOG_FILES") != NULL;
    if (logFiles)
    {
        pc_log("[dvd] %.1fs open %s", pc_ticks_ms() / 1000.0, s_entries[entrynum].key.c_str());
    }
    memset(fi, 0, sizeof(*fi));
    fi->startAddr = (unsigned int)entrynum;
    {
        std::lock_guard<std::mutex> lk(s_fileLock);
        if (!open_entry(s_entries[entrynum]))
        {
            return 0;
        }
    }
    fi->length = s_entries[entrynum].size;
    return 1;
}

int DVDOpen(const char* path, DVDFileInfo* fi)
{
    int n = DVDConvertPathToEntrynum(path);
    return n >= 0 && DVDFastOpen(n, fi);
}

int DVDClose(DVDFileInfo* fi) { return 1; }

int DVDReadPrio(DVDFileInfo* fi, void* addr, int length, int offset, int prio)
{
    fi->cb.state = 1;
    int r = read_entry(fi->startAddr, addr, length, offset);
    fi->cb.transferredSize = r < 0 ? 0 : (unsigned int)r;
    fi->cb.state = r < 0 ? -1 : 0;
    return r < 0 ? -1 : r;
}

int DVDReadAsyncPrio(DVDFileInfo* fi, void* addr, int length, int offset, DVDCallback cb, int prio)
{
    fi->cb.state = 1;
    fi->cb.addr = addr;
    fi->cb.length = (unsigned int)length;
    fi->cb.offset = (unsigned int)offset;
    fi->callback = cb;
    int r = read_entry(fi->startAddr, addr, length, offset);
    pc_irq_post([fi, r, cb]() {
        if (fi->cb.state != 1)
        {
            return; // cancelled
        }
        fi->cb.transferredSize = r < 0 ? 0 : (unsigned int)r;
        fi->cb.state = r < 0 ? -1 : 0;
        if (cb)
        {
            cb(r < 0 ? -1 : r, fi);
        }
    });
    return 1;
}

int DVDSeekAsyncPrio(DVDFileInfo* fi, int offset, DVDCallback cb, int prio)
{
    fi->cb.state = 1;
    pc_irq_post([fi, cb]() {
        if (fi->cb.state != 1)
        {
            return;
        }
        fi->cb.state = 0;
        if (cb)
        {
            cb(0, fi);
        }
    });
    return 1;
}

int DVDGetCommandBlockStatus(const DVDCommandBlock* block) { return block->state; }
int DVDGetDriveStatus() { return 0; }
int DVDCheckDisk() { return 1; }

int DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback cb)
{
    block->state = 10; // canceled
    if (cb)
    {
        pc_irq_post([block, cb]() { cb(0, block); });
    }
    return 1;
}

int DVDCancelAll() { return 0; }

}
