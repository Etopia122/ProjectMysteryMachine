Recomp Achievements SDK (vendored)
==================================

The achievement client `scooby.exe` reports to RecompLauncher with. This is the
`sdk/` folder of [RecompAchievements](https://github.com/elripalda/RecompAchievements)
at commit `faf094acdf1efb57d8db7c8187b4b47aa1501805` (2026-11-24), unchanged.

- `src/rl_achievements.c`, `include/rl_achievements.h`: the SDK itself.
- `third_party/rcheevos`, `third_party/cjson`: its dependencies, under MIT
  (licence texts kept beside them).
- `LICENSE`: the Recomp Achievements License 1.0 (free for any game; launchers
  and front-ends need written permission).
- `tests/fake_launcher.py` is the upstream test harness. It stands in for the
  launcher over a Unix socket, so on Windows use `pc/tools/fake_launcher.py`
  instead, which speaks the same protocol over a named pipe.

`pc/CMakeLists.txt` builds `sdk/CMakeLists.txt` as the `rl_achievements` target
and links it into the game. Update by replacing this folder with the `sdk/`
folder from a newer upstream commit and updating the commit above.
