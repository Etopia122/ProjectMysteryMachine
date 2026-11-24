# Achievements

The PC port reports achievements to [RecompLauncher](https://recomplauncher.com)
through Recomp Achievements: the launcher shows the toast, stores the unlock,
counts it towards Beaten and Mastered, and puts it on the leaderboards. The game
only names what happened; the achievement list (names, descriptions, points) is
hosted with the game by RecompLauncher, so `recomplauncher.json` is not involved
and the list can be fixed without a new build.

## What is in the build now

| | |
|---|---|
| SDK | `pc/third_party/rl-achievements` (upstream `sdk/`, commit pinned in its README), built as the `rl_achievements` target and linked into `scooby.exe` |
| Transport | events: the game fires a name, the list maps it to an achievement |
| Start-up | `pc_achievements_init()` in `pc/src/main.cpp`, before the game boots |
| Per frame | `pc_achievements_frame()` from `pc_events_pump()` |
| Events fired | `new_game` (`zMain.cpp`, the new-game branch, not a load) and, from the watchers in `pc/src/achievements.cpp`, `first_snack`, `snacks_100`, `snacks_1000`, `area_perfect_1`, `areas_perfect_10`, `areas_perfect_all`, `first_token`, `tokens_half`, `tokens_all`, `first_clue`, `clues_10`, `first_warp`, `warps_all`, `first_upgrade`, `upgrades_all`, `all_cutscenes`, `beat_mastermind`, `deaths_10`, `scares_50` |
| Fairness | `rl_ach_save_loaded()` on a successful load (`zSaveLoad.cpp`); `rl_ach_casual()` when the debug menu opens (`debug_menu.cpp`), when a retail pad cheat runs (`zGame.cpp`: the warp gate, all powerups, credits and all movies codes), or when a `PC_*` switch that changes the game is set (`debug_hooks.cpp`: `PC_SCENE`, `PC_WARP`, `PC_PORTAL`, `PC_TELEPORT`, `PC_FORCESAVE`, `PC_ALLWARPS`, `PC_UPGRADES`, `PC_WARPSEEN`, `PC_EVENT`, `PC_BOSSDMG`, `PC_HURT`, `PC_MENU`, `PC_SNACKS`, `PC_TOKENS`, `PC_AREAS`, `PC_CLUES`, `PC_DEATHS`, `PC_SCARES`, `PC_MASTERMIND`) |

Everything is behind the SDK's own check for `RL_SESSION`: started by hand, or
by a launcher that has gone away, there is no channel, every call returns
straight away and the game plays normally. The session log records which of the
two happened (`[ach] connected to RecompLauncher` / `[ach] no RecompLauncher`).

Both are a one-way state for the session on the launcher's side, so the game
just says when it happens: `casual` when the debug menu opens or a pad cheat
runs, or once at start-up for a switch; `save_loaded` on each load. Switches that
only log (`PC_POSLOG`, `PC_PROBE`, `PC_SPHERE`, `PC_SURFDUMP`, `PC_ENTINFO`) are
not treated as cheats. The New 3DS build, which has no RecompLauncher, implements
the same functions as no-ops.

The watchers read ten counters and a playing flag from
`src/Game/zPCDebug.cpp` (`pc_ach_snacks`,
`pc_ach_tokens`, `pc_ach_clues`, `pc_ach_areas`, `pc_ach_warps`,
`pc_ach_upgrades`, `pc_ach_cutscenes`, `pc_ach_mastermind`, `pc_ach_deaths`,
`pc_ach_scares`, `pc_ach_playing`), so no game header leaks into the port code.
Each milestone fires once, when its counter reaches it; reading the counters
every frame without the once-only flag would send a message per frame.
`first_warp` and `first_upgrade` start from what play begins with, because that
state is not this session's doing: the hub's gate is open from the start, and
`SD2.INI` starts the player with five upgrades. The other counters are
progress the player made, saved or not, so a load catches up once (and
`save_loaded` keeps it off the leaderboards). `pc_ach_areas` is the progress
screen's own count of areas with every snack found (so `area 0`, whose totals
are 999, never counts).

Still to do before the list goes out:

- `rl_ach_set_info(game_id, version)` once the game has its RecompLauncher
  listing (the launcher only notes a hello without an id).

## Testing

`pc/tools/fake_launcher.py` plays the launcher's side of the channel over the
Windows named pipe and checks five runs: with a launcher (it must see `hello`,
answer the list, the `new_game` event, `first_snack` once `PC_SNACKS` collects a
snack mid-game, `first_token`, `tokens_half` and `tokens_all` once `PC_TOKENS`
gives the 21 tokens, `first_clue` and `clues_10` once `PC_CLUES` gives 10 clues,
`warps_all` and `first_warp` once `PC_ALLWARPS` opens the 26 gates,
`area_perfect_1`, `areas_perfect_10` and `areas_perfect_all` once `PC_AREAS`
finishes all 29 real areas' snacks (which also takes the collected snack count
to the game's 6,748, so `snacks_100` and `snacks_1000` come with it),
`first_upgrade` and `upgrades_all` once `PC_UPGRADES` gives every upgrade, which
is more than the five `SD2.INI` starts with, `deaths_10` once `PC_DEATHS` counts
ten deaths the way losing a life counts them, `scares_50` once `PC_SCARES` counts
fifty scares the same way, and `casual` because the run itself starts with the
`PC_SCENE` cheat switch);
with only `PC_CLUES` set, which must mark the session casual under its own name
(the other runs stop at `PC_SCENE`, the first switch in the list, so they cannot
show that a give switch is listed); with only `PC_MASTERMIND` set, which must
report `beat_mastermind` and also name itself as the casual switch; without
`RL_SESSION`; and with `RL_SESSION` naming a pipe nobody is serving. The same
run without those switches reports only `new_game`, which is what shows the
watchers react to the counters rather than to state that was already true.
Loading a save is not scripted by the harness, so `save_loaded` is wired but not
exercised there. `all_cutscenes` is the same (the run plays no cutscene): its
getter runs every frame but never sees its threshold.

`beat_mastermind` gets a run of its own because the game's own call,
`zVarDefeatMasterMind()` (the 0x127 dispatcher event), sets the endgame flag as
well as the defeated one, and the menus and credits read that flag: the counter
run keeps ordinary play. That run starts from the title screen rather than with
`PC_SCENE`, so its casual report names `PC_MASTERMIND` itself.

`upgrades_all` needs all 15 upgrades held at once, so it is worth knowing that
a level change keeps them: a warp does not reset the player; `zEntPlayerReset`
(`zSceneReset`, on a death or a scene restart) does, and that path writes the
player block before the reset and reads it back with `zSceneLoad` afterwards.
Checked by hand
with one warp per run (`PC_UPGRADES=10` plus `PC_WARP=14:I020`, then
`PC_WARP=14:E001`): the count read 15 after both changes, one within the area
and one into another. A death was not scripted, so that path is code reading,
not a test.

```
python pc/tools/fake_launcher.py build/pc/scooby.exe
```

The SDK's own `sdk/tests/fake_launcher.py` does the same over a Unix socket, so
it does not apply to the Windows build; the checks here mirror it.

## Rules for the list

- **Fire on transitions, not on state.** An event unlock is permanent, and the
  SDK's "false for a frame" guard only exists for memory rules. Each watcher
  fires a milestone once, when its counter reaches it, so a session sends a few
  messages rather than one per frame, and a loaded save cannot re-grant what it
  already holds (the crossing is reported, and `save_loaded` marks it Casual).
- **Points are 5, 10, 25 or 50.** Achievements marked bonus count for points but
  not for Beaten or Mastered.
- **Beaten** is the Mastermind. **Mastered** is every non-bonus achievement.

## The list (draft, from state the port already reads)

Every row reads state the game already exposes, through the same calls the HUD,
the map screen and the save file use. `snacks_tot`, `Inv_Mtokens` and
`Inv_Specials` are in `src/Game/zGlobals.h`; the helpers are in `zVar.cpp`,
`zScene.cpp`, `zEntPickup.cpp` and `zCutsceneMgr.cpp`.

| # | Event | Title | Points | Fires when | Reads |
|---|---|---|---|---|---|
| 1 | `new_game` | The Mystery Begins | 5 | a new game starts (not a load) | the new-game branch of `zMain.cpp` — **wired** |
| 2 | `first_snack` | Scooby Snack! | 5 | the first snack | `globals.areaStats[].snacks_tot` summed `>= 1` — **wired** |
| 3 | `snacks_100` | Snack Attack | 10 | 100 snacks | the same sum `>= 100` — **wired** |
| 4 | `snacks_1000` | Snack Hoarder | 25 | 1,000 snacks | the same sum `>= 1000` — **wired** |
| 5 | `area_perfect_1` | Every Last Crumb | 10 | one area fully cleared | `pc_dbg_area_perfect(1..29)` count `>= 1` — **wired** |
| 6 | `areas_perfect_10` | Neighbourhood Watch | 25 | ten areas fully cleared | the same count `>= 10` — **wired** |
| 7 | `areas_perfect_all` | Snack Sense | 50 | all 29 areas cleared | the same count `>= 29` (the loop never includes area 0, the 999-snack sentinel) — **wired** |
| 8 | `first_token` | Monster Token | 5 | the first monster token | bits set in `globals.player.Inv_Mtokens[0]` `>= 1` — **wired** |
| 9 | `tokens_half` | Rogues' Gallery | 25 | 11 of the 21 tokens | the same count `>= 11` — **wired** |
| 10 | `tokens_all` | Collector of Culprits | 50 | all 21 monster tokens | the same count `>= 21` — **wired** |
| 11 | `first_clue` | A Clue! | 5 | the first clue | `globals.player.Inv_Clue` `>= 1` — **wired** |
| 12 | `clues_10` | Meddling Kids | 10 | 10 clues | the same count `>= 10` (saved with the player) — **wired** |
| 13 | `first_warp` | Shortcut | 10 | a warp gate opens | count of `zEntPickup_WarpIsOpen(i)`, `i < 26`, above the count when play began (the hub's gate is open from the start) — **wired** |
| 14 | `warps_all` | Master of Shortcuts | 25 | all 26 warp gates | the same count reaching 26 — **wired** |
| 15 | `first_upgrade` | Well Equipped | 5 | more upgrades than the game starts with | bits set in `globals.player.Inv_Specials` above the count when play began — **wired** |
| 16 | `upgrades_all` | Fully Kitted | 25 | every upgrade | the same count `>= 15` (15 bits; 5 start set from `SD2.INI`) — **wired** |
| 17 | `all_cutscenes` | Home Theatre | 25 | all 14 cutscenes seen | count of `zCutSceneNamesTable_getStatus(1..14)` `>= 14` — **wired** |
| 18 | `beat_mastermind` | Unmasked! | 50 | the Mastermind is beaten (**Beaten**) | `zVarEntryCB_eVareVarDefeatedMM() >= 1` — **wired** |
| 19 | `deaths_10` | Ruh-Roh! | 5 | ten lives lost in one area (bonus) | `globals.stats.deaths_sum >= 10`, the tally kept with the area — **wired** |
| 20 | `scares_50` | Jump Scare Veteran | 10 | fifty scares in one area (bonus) | `globals.stats.scares_sum >= 50`, the same way — **wired** |

Wired today: rows 1–20, through `new_game` in `zMain.cpp` and the watchers in
`pc/src/achievements.cpp`.

Notes on the numbers: 21 monster tokens (`Inv_Mtokens[0]` bits 0–20), 26 warp
gates (`zWarpPointTable`), 29 real areas (`sAreaTable`), 14 cutscenes
(`zCutSceneNamesTable`), 15 upgrades (`Inv_Specials` bits 0–14), 6,748 snacks in
the whole game (the progress screen's figure).

The snack achievements count only snacks the player has actually collected:
`areaStats[].snacks_tot` is incremented by the pickup path, and the watcher sums
the 29 real areas only. Area 0 in `sAreaTable` is a sentinel with 999 snacks, so
including it would hand out `snacks_1000` after one snack and could count a scene
the area table does not name as a perfect area. A snack hidden out of bounds or
somewhere unreachable is never collected, so it never reaches the counter or
unlocks an area by itself. Area perfection compares that collected tally with
the game's own per-area total, the same check the HUD's all-snacks message and
the progress screen use; the 29 areas add up to the game's 6,748-snack figure.

The token, clue and upgrade counters are saved with the player and only reset
when a session starts, not on a scene change, so they can be treated as
progress. `areaStats[]` is the per-area tally that survives loading, which is
why the snack watcher sums `areaStats[].snacks_tot` instead of
`stats.snacks_tot`. `stats.deaths_sum` and `stats.scares_sum` are per-area too:
`zSceneSave` writes them into the area's own block (`setClient(ent->sceneID)`)
and `zSceneLoad` restores them on re-entry, zeroing them for an area with no
saved block, so those two achievements count within one area, not across the
game.

### Left out until the game exposes them

Per-boss achievements (Black Knight, Green Ghost, Redbeard) need one small
`PC_PORT` hook where a villain dies in `zEntVillain.cpp`; only the Mastermind's
defeat is readable today, through the `zVar` table. Puzzle pieces are readable
one letter at a time (13 `zVar` callbacks), so they make a poor single
achievement and are better as a list of 13 or left out.

## The list RecompLauncher hosts (format)

Events lists are the format in the SDK's `docs/SPEC.md`: one entry per
achievement, with `event` naming the code event. The entries for what is wired
today, to paste into the full list:

```json
{
  "format": 1,
  "game": "<catalog id>",
  "source": "events",
  "console": "",
  "copies": 1,
  "achievements": [
    { "id": 1, "title": "The Mystery Begins", "description": "Start a new game.", "points": 5, "event": "new_game" },
    { "id": 2, "title": "Scooby Snack!", "description": "Find your first snack.", "points": 5, "event": "first_snack" },
    { "id": 3, "title": "Snack Attack", "description": "Collect 100 snacks.", "points": 10, "event": "snacks_100" },
    { "id": 4, "title": "Snack Hoarder", "description": "Collect 1,000 snacks.", "points": 25, "event": "snacks_1000" },
    { "id": 5, "title": "Every Last Crumb", "description": "Find every snack in one area.", "points": 10, "event": "area_perfect_1" },
    { "id": 6, "title": "Neighbourhood Watch", "description": "Find every snack in ten areas.", "points": 25, "event": "areas_perfect_10" },
    { "id": 7, "title": "Snack Sense", "description": "Find every snack in all 29 areas.", "points": 50, "event": "areas_perfect_all" },
    { "id": 8, "title": "Monster Token", "description": "Find your first monster token.", "points": 5, "event": "first_token" },
    { "id": 9, "title": "Rogues' Gallery", "description": "Collect 11 of the 21 monster tokens.", "points": 25, "event": "tokens_half" },
    { "id": 10, "title": "Collector of Culprits", "description": "Collect all 21 monster tokens.", "points": 50, "event": "tokens_all" },
    { "id": 11, "title": "A Clue!", "description": "Find your first clue.", "points": 5, "event": "first_clue" },
    { "id": 12, "title": "Meddling Kids", "description": "Collect 10 clues.", "points": 10, "event": "clues_10" },
    { "id": 13, "title": "Shortcut", "description": "Open a warp gate.", "points": 10, "event": "first_warp" },
    { "id": 14, "title": "Master of Shortcuts", "description": "Open all 26 warp gates.", "points": 25, "event": "warps_all" },
    { "id": 15, "title": "Well Equipped", "description": "Find an upgrade after the ones you start with.", "points": 5, "event": "first_upgrade" },
    { "id": 16, "title": "Fully Kitted", "description": "Hold all 15 upgrades at once.", "points": 25, "event": "upgrades_all" },
    { "id": 17, "title": "Home Theatre", "description": "See all 14 cutscenes.", "points": 25, "event": "all_cutscenes" },
    { "id": 18, "title": "Unmasked!", "description": "Defeat the Mastermind.", "points": 50, "event": "beat_mastermind" },
    { "id": 19, "title": "Ruh-Roh!", "description": "Lose ten lives in one area.", "points": 5, "event": "deaths_10" },
    { "id": 20, "title": "Jump Scare Veteran", "description": "Be scared fifty times in one area.", "points": 10, "event": "scares_50" }
  ]
}
```
