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

Patterns that have mattered for matching:

- Floats tested by truthiness (`if (f)`, `if (a || b)`) load the variable
  before the 0.0f constant; `f != 0.0f` loads the constant first.
- `x ? 1 : 0` keeps the branches; `if (x) return 1; return 0;` becomes a
  branchless `mfcr` sequence (and vice versa for some shapes, so try both).
- Byte flags cleared with a literal mask (`flags &= 0xE7`) give `andi.`;
  `&= ~0x18` gives `rlwinm`.
- A decrement and the test of the result are often separate statements
  (`n--; if (n == 0)`), which reloads the value.
- A function that is both emitted and inlined needs `#pragma auto_inline on`
  around its definition. With `-inline deferred` it can be defined after its
  callers, which is how to keep the emitted order right.
- Loading all of a vector's components before storing them usually means an
  inline helper with float parameters, or a struct copy.
- Two calls in one expression are evaluated right to left: `sin(a) * exp(b)`
  calls `exp` first and keeps `sin`'s result as the first `fmuls` operand.
- A recursive static function under `#pragma auto_inline on` is emitted once
  and inlined into itself several levels deep (`xFFXApplyOne`).
- Precomputed values such as `float rsq = r * r;` at the top of a function
  show up as early `fmuls` into callee-saved registers.
- A three-way compare (-1/1/0) keeps all its branches only when it assigns
  a result variable and returns it once; early returns turn the last test
  into a branchless `subfc/subfe/neg` (`xSER_ord_compare`).
- Pointer locals like `xsd = &g_xserdata` decide which accesses share one
  `lis/addi` pair; the original sometimes mixes the local and the global in
  one function (`xSerial_svgame_register`).
- **Const parameters change scheduling.** MWCC lets loads through a `const T*`
  *function parameter* move above stores to locals (all three `lfs` before
  the `stfs`); a non-const parameter keeps each load after the previous
  store. Const on an inline helper's parameter or on a local pointer does
  not do it. The PS2 dump drops const, so when the original hoists such
  loads, give the function const parameters and rename the symbol
  (`xRayHitsBoundFast__FPC6_xRay3PC7_xBound`); BFBB's signatures are a good
  guide.
- Static data in the same section is pooled: if the original addresses a
  static as `base + offset` from another static's register, both must be in
  that section, so a function-local `static char buf[64];` that sits in
  `.data` is written `= { 0 }` (`xstransvc.cpp`).
- An otherwise empty function that stores its parameter to the stack
  (`stwu; stw r3, 8(r1)`) has a `volatile` local initialised from it.
- The PS2 DWARF dump omits unused parameters. If a GameCube function reads
  its first argument from r4, it has an unused leading parameter
  (`zCutsceneMgrSave(_zCutsceneMgr*, xSerial*)`).
- A `switch` on one character compiles to a different compare tree than an
  `if` chain; `xIniParse` uses `if (*line != '#' && *line != '\0')`.
- A few units were built with `-bool on` (`zEntTrigger.cpp`): there a
  comparison stored to an `int` (`collide = (isect.penned <= 0.0f);`) is
  computed in r0 and then copied with `mr` into the variable's register.
  Under the project-wide `-bool off` it goes straight to that register. Give
  such units `extra_cflags=["-bool on"]`.
- Register assignment follows the order locals are declared (and the order
  independent statements are written) more than anything semantic. When only
  registers differ, run `tools/perm_decls.py` on a `// PERM_BEGIN` /
  `// PERM_END` block of declarations (iMix `MIXUpdateSettings` went from 94.7%
  to 100%).
- Stack layout reflects how locals were introduced: every inline expansion
  adds its own locals. zParEmitter `Emit` only got the original frame once each
  `switch` case called its own inline helper, as the original did.
- Initialized local aggregates (`_xVec3 v = {0, 1, 0};`) copy from a `.rodata`
  pool that is emitted once per function *definition*: an inline helper
  expanded six times shares one pool, so separate pools mean separate helpers.
- RenderWare-style setters are macros, not inline functions: their arguments
  are evaluated in place (loads interleaved with stores), unlike an inline
  call whose arguments are evaluated first, right to left.
- MWCC emits `.data` with 8-byte alignment. A jump table (or any `.data`) that
  starts at a 4-aligned address cannot begin its own unit: the code shares a
  unit with the data before it (the TRC code after iVSync is part of iSystem).
- A store-then-reload of a global after a loop (`x = f(); ...; y = x;` where the
  target reloads `x`) means the global is `volatile` (zMenu retrace counter).
- A callee that truncates its result to a byte (`rlwinm ..., 24, 31`) while the
  caller compares the full word (`cmplwi r3, 0`, no `clrlwi.`): the function
  returns `unsigned int` with a byte cast inside, `return (unsigned char)(e);`.
- Constant operands: `x * k` with a literal `k` is emitted constant-first; when
  the target has the variable first, the factor came in as a variable, usually
  through an inline helper's parameter (`xVec3SMul(&o, &v, -0.25f)`).
- Uninitialized `char buf[5] = { 0};` copies from a zero pool in `.sbss2`;
  give the unit an `.sbss2` split for it.
- A function under `#pragma auto_inline on` is not emitted at its own source
  position: its out-of-line copy follows the function that first needed it.
  Move the definition around (zParSys needed it right after its caller) and
  check the order with `build/tmp/fnorder.py <obj>`.
- A recursive `auto_inline` function is inlined into itself several levels deep
  (zParGroupUpdateR: five copies, then a real call); other callers still call it.
- A loop duplicated in two functions can come from one inline helper in one and
  hand-written code in the other (zParSysUpdate vs zParGroupUpdateR): try both.
- `.sdata2` constants are pooled in code-generation (DOL) order. A unit whose pool
  starts with constants its first float-using function doesn't load first had a
  function the linker dead-stripped: add a global stand-in that uses them, in the
  right DOL position (xModel's `xModelScreenSize`); the linker strips it again.
- A function with a `switch` big enough for a jump table needs its `.data`
  split, or the table appears twice and `.data` grows (0x40 for 16 cases).
- Loading struct fields into locals first (`float x = sr->x; float y = sr->y;`)
  lets later loads move ahead of stores; it reproduced xModelRender2D's
  schedule without making the pointer `const` (which hoisted too much).
- Imported names of statics often carry the wrong signature (PS2 vs GC). Fix the
  mangled name in symbols.txt, or check_unit can't pair the function.

- A call result assigned to a local inside a tiny inline helper
  (`float ret = iurand(); return ret;`, zLightEffect's `leGetRandom`) colors
  registers differently from calling `iurand()` directly in the expression.
- Clamp helpers written with `if (x < y) x = y; return x;` avoid the extra
  branches that `return (x < y) ? y : x;` produces in each arm.
- Struct assignment is member-wise: a nested 4-byte `RwRGBA` member is copied
  as one word, separate `unsigned char` members byte by byte. iFont's
  `RwIm2DVertex` copies match only with the color as four byte members.
- A literal added with the variable first (`fadds f1, v, k`) came in through a
  local (`float d = 0.49999991f; v + d`), like the multiply case above.
- A loop counter that is truncated (`clrlwi`) at its use was a wider integer
  than the `unsigned char` it is stored into; a pointer difference
  (`s - characters`) gets strength-reduced into the same counter but with a
  different schedule.
- An inline helper's parameter can keep its own saved register when its body is
  a separate block of statements (xFX `xFXanimUVAtomicSetup`'s apply loop).
- `.sdata2`/`.rodata` pools are padded to 8 bytes at the end; secdiff shows the
  trailing zero word as a size difference, which is harmless.
- Copy-initialising a 4-byte struct (`iColor_tag c = box->color;`) copies one
  word; assigning it (`c = box->color;`) copies member by member (bytes).
- Constants are not folded through a one-iteration loop: xFont's single-digit
  parse (`n = 0; for (i = 0; i < 1; i++) n = n * 10 + *s++ - '0';`) keeps its
  `mulli` by zero, like the three-digit loops next to it.
- `s = end = text;` keeps the copy chain (`mr end, text; mr s, end`), where two
  statements get copy-propagated (`mr s, text`).
- Splitting `dx = a * b;` into `dx = a; dx *= b;` builds the value in `dx`'s
  register.
- A `(float)` of `byte + 3` that converts as unsigned (magic `0x4330000000000000`,
  no `xoris`) was cast: `(unsigned int)(w + 3)`.
- A zero-initialised global that sits in `.data` needs an explicit `= { 0 }`
  (xFont `gFontGlobals`); without it MWCC puts it in `.bss`.
- A struct field loaded once before a loop (`lwz` in the preheader) while the
  loop stores through the same base pointer was copied to a local in the source
  (`_zEnt** bubbles = sc->bubbles;`); MWCC does not hoist it on its own.
- A `switch` compare tree's shape gives away the case set: zEntBubble's
  villain response tests 2 first, which needs a fourth (empty) case.
- `switch` case bodies are emitted in source order; a case placed out of
  numeric order in the DOL was written that way.
- Large struct copies (`_xMat4x3`, even `_xVec3` in places) that show up as
  `memcpy` calls were written as `memcpy` (or an inline that calls it).

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
