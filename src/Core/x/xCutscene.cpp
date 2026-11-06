// The GameCube build has no cutscene player; only these stubs remain.

class xCutscene;

// Functions are in reverse address order (-inline deferred).

void xCutscene_Init(void* toc)
{
}

signed int xCutscene_Destroy(xCutscene* csn)
{
    return 1;
}
