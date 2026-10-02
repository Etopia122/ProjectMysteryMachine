# Compiler and flags (GIHE78)

Summary: game code was built with **CodeWarrior for GameCube 1.3.2**
(`GC/1.3.2`, Ver 2.4.2 build 81) using `-O4,s`, `-inline deferred`, signed
chars and FP contraction. See `cflags_game` in `configure.py`.

## Evidence

Probed with `tools/compiler_probe/probe.py`, which compiles a test source with
every compiler/flag combination and compares each function against the DOL
(relocated fields masked). Test sources are in `tools/compiler_probe/tests/`.

| Finding | Evidence |
| --- | --- |
| `-inline deferred` | Functions of a TU appear in reverse source order in the DOL (same as the PS2 build). With `-inline deferred` CW emits them in that order; with `-inline auto` they come out reversed relative to the DOL. |
| Compiler >= 1.3.2 | 1.0-1.2.5 use an older prologue (`xBaseLoad`/`xBaseSave` mismatch); 1.3 compiles `xStrHash` to 0x3C bytes instead of 0x40. |
| Not 2.0p1 | BFBB's patched compiler mismatches `xMat3x3RotY` (0xA8 vs 0xA4) and `xAngleClamp`. |
| 1.3.2 over 2.0-2.7 | Codegen for the probed functions is identical across 1.3.2 and 2.0-2.7, so the decision rests on the libraries shipped with the toolchain: MetroTRK matches Wind Waker's (a 1.3.2 game) function-for-function (172/178 exact sizes across TRK/Runtime/MSL, every TRK TU exact) versus 166/184 for BFBB's (2.0) copy, where several TRK TUs and MSL `mem_funcs.c` (0/4) differ. Not yet confirmed by a codegen difference. |
| `-fp_contract on` | `xQuatMul`, `xMat3x3RMulVec`, `xMat4x3MoveLocalAt` use `fmadds`/`fnmsubs`; without contraction they don't match. |
| `-char signed` | `xStrHash` sign-extends characters (`extsb`); with `-char unsigned` it no longer matches (BFBB uses unsigned). |
| `-bool off` | `zConditional_Evaluate` returns comparison results with no byte mask (`clrlwi`); with `bool` enabled every comparison is truncated to 8 bits. The PS2 DWARF contains no `bool` type at all. |
| `-inline noauto` + `-inline deferred` | `zConditionalEventCB` calls `zConditional_Evaluate` instead of inlining it, which auto-inlining does regardless of how the function is written. Some functions are auto-inlined anyway (`xMath`, `iMath`, `zGameStateSwitch` into `zGameStateSwitchEvent` while `zGameModeSwitch` in the same TU is not), so auto-inlining was switched on selectively. `#pragma auto_inline on`/`off` around individual definitions reproduces this: a function is an auto-inline candidate if it was defined while the pragma was on, and it is still emitted out of line (unlike `inline`, which suppresses the out-of-line copy). |
| `-use_lmw_stmw on` | Game functions save registers with `stmw`/`lmw` (e.g. 0x80016A30). |
| `-str reuse,readonly` (no `pool`) | String literals are separate objects; short ones (<= 8 bytes) go to `.sdata2` like other small constants (zAnimList's "stop0"... at 0x802603C0). With `pool` they'd be merged into one `.rodata` blob. |
| `-Cpp_exceptions off` | No `extab`/`extabindex` sections. |
| `-O4,s` | Optimized for size: `xStrTok`/`xStrTokBuffer` clear a 32-byte array with a counted `bdnz` loop, which `-O4,p` fully unrolls (identical across all compiler versions, so it's the flag, not the version). `-O3` does not match. |

## Writing source

Because of `-inline deferred`, CW emits a TU's functions **last to first**.
Write functions in the reverse of their DOL order (equivalently, in the PS2
DWARF dump's order reversed). objdiff scores functions individually, so a
wrong order still shows 100% there but breaks the DOL hash check.

## Open questions

- A function that compiles differently under 1.3.2 and 2.x would settle the
  version. Larger functions (loops, register pressure) are the best bet.
- `-common on/off` and `-sdata`/`-sdata2` thresholds still need a discriminating test.
- Library objects (SDK, MSL, Runtime, MetroTRK, RenderWare, Bink) use their own
  compilers; pick per library when splitting (SDK is usually `GC/1.2.5n`).

## Linker

`mw_comment_version: 10` (CodeWarrior for GameCube 1.3.2+) and
`config.linker_version = "GC/1.3.2"`. The DOL entry point is the stack-fill
function at 0x80016898, not `__start` (see `tools/prepare_dol.py`); the link
must set it.
