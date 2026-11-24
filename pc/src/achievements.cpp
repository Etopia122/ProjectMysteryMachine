// Recomp Achievements (pc/third_party/rl-achievements): tells RecompLauncher
// what the player earned. The launcher shows the toast, stores the unlock,
// counts it for Beaten/Mastered and puts it on the leaderboards; the game only
// names what happened.
//
// Started without RecompLauncher there is no channel to open, the SDK's calls
// return straight away and this module does nothing, so it ships in every
// build. The list of achievements is hosted with the game (not compiled in):
// see pc/docs/achievements.md.
#include "pc_platform.h"

#include <rl_achievements.h>

// The counters the watchers read each frame (src/Game/zPCDebug.cpp).
extern "C" unsigned int pc_ach_snacks();
extern "C" unsigned int pc_ach_tokens();
extern "C" unsigned int pc_ach_clues();
extern "C" unsigned int pc_ach_areas();
extern "C" unsigned int pc_ach_warps();
extern "C" unsigned int pc_ach_upgrades();
extern "C" unsigned int pc_ach_cutscenes();
extern "C" unsigned int pc_ach_mastermind();
extern "C" unsigned int pc_ach_deaths();
extern "C" unsigned int pc_ach_scares();
extern "C" int pc_ach_playing();

namespace {

// A counter and the events it fires as it climbs, lowest first. Each fires
// once, when the counter reaches it: a message per frame would flood the
// launcher, and an achievement unlocks only once anyway.
struct Milestone
{
    unsigned int at;
    const char* event;
    bool fired;
};

template <int N>
void watch(unsigned int value, Milestone (&list)[N])
{
    for (int i = 0; i < N; i++)
    {
        if (!list[i].fired && value >= list[i].at)
        {
            list[i].fired = true;
            pc_achievements_event(list[i].event);
        }
    }
}

} // namespace

extern "C" void pc_achievements_init()
{
    static bool started;
    if (started)
    {
        return;
    }
    started = true;
    // No catalog id yet: the launcher only notes a hello without one. Add it
    // (rl_ach_set_info) once the game has its RecompLauncher listing.
    if (rl_ach_init("", NULL, NULL))
    {
        pc_log("[ach] connected to RecompLauncher");
    }
    else
    {
        pc_log("[ach] no RecompLauncher: achievements do nothing");
    }
}

// Once per presented frame (pc_events_pump): reads what the launcher sent, then
// the counters the milestones watch. Each milestone keeps its own flag, so a
// counter that sits past it every frame still sends one message.
extern "C" void pc_achievements_frame()
{
    rl_ach_frame();

    static Milestone snacks[] = { { 1, "first_snack", false },
                                  { 100, "snacks_100", false },
                                  { 1000, "snacks_1000", false } };
    static Milestone tokens[] = { { 1, "first_token", false },
                                  { 11, "tokens_half", false },
                                  { 21, "tokens_all", false } };
    static Milestone clues[] = { { 1, "first_clue", false }, { 10, "clues_10", false } };
    static Milestone areas[] = { { 1, "area_perfect_1", false },
                                 { 10, "areas_perfect_10", false },
                                 { 29, "areas_perfect_all", false } };
    static Milestone warps[] = { { 26, "warps_all", false } };
    static Milestone upgrades[] = { { 15, "upgrades_all", false } };
    static Milestone cutscenes[] = { { 14, "all_cutscenes", false } };
    static Milestone mastermind[] = { { 1, "beat_mastermind", false } };
    static Milestone deaths[] = { { 10, "deaths_10", false } };
    static Milestone scares[] = { { 50, "scares_50", false } };
    watch(pc_ach_snacks(), snacks);
    watch(pc_ach_tokens(), tokens);
    watch(pc_ach_clues(), clues);
    watch(pc_ach_areas(), areas);
    watch(pc_ach_warps(), warps);
    watch(pc_ach_upgrades(), upgrades);
    watch(pc_ach_cutscenes(), cutscenes);
    watch(pc_ach_mastermind(), mastermind);
    watch(pc_ach_deaths(), deaths);
    watch(pc_ach_scares(), scares);

    // What the game starts play with is not this session's doing: the hub's warp
    // gate is open from the start, and SD2.INI starts the player with five
    // upgrades, so these two fire on the change after play begins, not on the
    // state. (A loaded save's counters, upgrades included, count as progress:
    // the launcher is told about the load.)
    static bool playBegan, firstWarp, firstUpgrade;
    static unsigned int warpsAtStart, upgradesAtStart;
    if (!playBegan && pc_ach_playing())
    {
        playBegan = true;
        warpsAtStart = pc_ach_warps();
        upgradesAtStart = pc_ach_upgrades();
    }
    if (playBegan && !firstWarp && pc_ach_warps() > warpsAtStart)
    {
        firstWarp = true;
        pc_achievements_event("first_warp");
    }
    if (playBegan && !firstUpgrade && pc_ach_upgrades() > upgradesAtStart)
    {
        firstUpgrade = true;
        pc_achievements_event("first_upgrade");
    }
}

// A code event: the launcher maps the name to an achievement from the list.
// Firing it twice is fine; an achievement unlocks once.
extern "C" void pc_achievements_event(const char* name)
{
    if (!name || !*name)
    {
        return;
    }
    if (rl_ach_connected())
    {
        pc_log("[ach] event %s", name);
    }
    rl_ach_event(name);
}

// A save was loaded. The launcher keeps unlocks from here on, but does not
// count them on the leaderboards: the save may already hold the work.
extern "C" void pc_achievements_save_loaded()
{
    if (rl_ach_connected())
    {
        pc_log("[ach] save loaded");
    }
    rl_ach_save_loaded();
}

// A cheat was used: the debug menu opened, or a switch that changes the game
// was set at start-up. The launcher turns Ranked off for the rest of the
// session, so sending it again does no harm.
extern "C" void pc_achievements_casual(const char* reason)
{
    if (rl_ach_connected())
    {
        pc_log("[ach] casual: %s", reason);
    }
    rl_ach_casual(reason);
}
