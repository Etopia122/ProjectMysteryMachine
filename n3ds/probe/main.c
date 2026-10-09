// New 3DS bring-up probe: checks the toolchain, the GPU path (citro3d), the
// memory budget and SD card access to the game files before the real port.
#include <3ds.h>
#include <citro3d.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float x, y, z; } vertex;

static const vertex sTri[] = {
    { 200.0f, 200.0f, 0.5f },
    { 100.0f, 40.0f, 0.5f },
    { 300.0f, 40.0f, 0.5f },
};

// Prints to the bottom screen and to the emulator log.
static void report(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s\n", buf);
    svcOutputDebugString(buf, strlen(buf));
}

static void probe()
{
    bool n3ds = false;
    APT_CheckNew3DS(&n3ds);
    report("[probe] new3ds %d", n3ds);
    osSetSpeedupEnable(true);
    report("[probe] heap free %lu KB, linear free %lu KB",
           (unsigned long)(osGetMemRegionFree(MEMREGION_APPLICATION) / 1024),
           (unsigned long)(linearSpaceFree() / 1024));
    report("[probe] app memory %lu KB", (unsigned long)(osGetMemRegionSize(MEMREGION_APPLICATION) / 1024));

    // Largest single heap block malloc can hand out.
    size_t lo = 0, hi = 512u << 20;
    while (hi - lo > (1u << 20))
    {
        size_t mid = lo + (hi - lo) / 2;
        void* p = malloc(mid);
        if (p)
        {
            free(p);
            lo = mid;
        }
        else
        {
            hi = mid;
        }
    }
    report("[probe] largest malloc %u MB", (unsigned)(lo >> 20));

    const char* path = "sdmc:/scooby/files/O0/o008.HIP";
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        report("[probe] cannot open %s", path);
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char magic[5] = { 0 };
    fread(magic, 1, 4, f);
    u64 t0 = svcGetSystemTick();
    static char buf[256 * 1024];
    size_t total = 0, n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
    {
        total += n;
    }
    double secs = (double)(svcGetSystemTick() - t0) / SYSCLOCK_ARM11;
    fclose(f);
    report("[probe] %s: %ld bytes, magic %s, read %.2f MB/s", path, size, magic, total / 1048576.0 / (secs > 0 ? secs : 1));
}

int main()
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    DVLB_s* dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgram_s program;
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &dvlb->DVLE[0]);
    C3D_BindProgram(&program);
    int uProjection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");

    C3D_AttrInfo* attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 3);
    AttrInfo_AddFixed(attr, 1);
    C3D_FixedAttribSet(1, 1.0f, 0.8f, 0.2f, 1.0f);

    C3D_Mtx projection;
    Mtx_OrthoTilt(&projection, 0.0f, 400.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);

    void* vbo = linearAlloc(sizeof(sTri));
    memcpy(vbo, sTri, sizeof(sTri));
    C3D_BufInfo* bufs = C3D_GetBufInfo();
    BufInfo_Init(bufs);
    BufInfo_Add(bufs, vbo, sizeof(vertex), 1, 0x0);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    probe();

    int frames = 0;
    u64 t0 = svcGetSystemTick();
    while (aptMainLoop())
    {
        hidScanInput();
        if (hidKeysDown() & KEY_START)
        {
            break;
        }
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x203040FF, 0);
        C3D_FrameDrawOn(target);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProjection, &projection);
        C3D_DrawArrays(GPU_TRIANGLES, 0, 3);
        C3D_FrameEnd(0);
        if (++frames == 300)
        {
            report("[probe] 300 frames in %.2f s", (double)(svcGetSystemTick() - t0) / SYSCLOCK_ARM11);
        }
    }

    linearFree(vbo);
    shaderProgramFree(&program);
    DVLB_Free(dvlb);
    C3D_Fini();
    gfxExit();
    return 0;
}
