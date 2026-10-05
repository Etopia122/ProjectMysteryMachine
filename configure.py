#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GIHE78",  # 0
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-2"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
    # The retail entry point is a stack-fill routine in .text that calls
    # __start (see tools/prepare_dol.py), not __start itself.
    "-m fn_80016898",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = None

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]

# MetroTRK / Runtime / MSL flags, as in zeldaret/tww (whose copies are
# byte-identical to ours). Headers they need from TWW live in include/tww.
cflags_tww_base = [
    *cflags_base,
    "-i include/tww",
    "-i src",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C/PPC_EABI/Include",
    "-i src/PowerPC_EABI_Support/MSL/MSL_C++/MSL_Common/Include",
    "-i src/PowerPC_EABI_Support/Runtime/Inc",
    "-i src/PowerPC_EABI_Support/MetroTRK",
    "-DVERSION=2",  # TWW's VERSION_USA
    "-DSCOOBY_MSL",  # see math.h (sqrtf)
]

cflags_tww_runtime = [
    *cflags_tww_base,
    "-O4,p",
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-common off",
    "-inline deferred,auto",
    "-char signed",
]

cflags_tww_trk = [
    *cflags_tww_base,
    "-O4,p",
    "-use_lmw_stmw on",
    "-rostr",
    "-str reuse",
    "-common off",
    "-inline deferred,auto",
    "-char signed",
    "-sdata 0",
    "-sdata2 0",
]

cflags_tww_dolphin = [
    *cflags_tww_base,
    "-O4,p",
    "-fp_contract off",
]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

# Game code (i*, x*, z*): CodeWarrior for GameCube 1.3.2, see docs/compiler.md.
# -inline deferred makes CW emit a TU's functions in reverse source order,
# matching the DOL; chars are signed (extsb in xStrHash); float multiply-adds
# are contracted.
cflags_game = [
    *cflags_base,
    "-O4,s",  # size: loops are not unrolled (e.g. xStrTok's 32-byte clear)
    "-lang=c++",
    "-inline noauto",  # no automatic inlining (zConditional_Evaluate stays a call)
    "-inline deferred",
    "-fp_contract on",
    "-char signed",
    "-bool off",  # comparisons are int-valued (no clrlwi); no bool in the PS2 DWARF
    "-str reuse,readonly",  # no pooling: short literals go to .sdata2 (zAnimList)
    "-use_lmw_stmw on",
    "-i src/Core/x",
    "-i src/Core/gc",
    "-i src/Game",
]

config.linker_version = "GC/1.3.2"


# Helper function for game code
def GameLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_game,
        "progress_category": "game",
        "objects": objects,
    }


# Helper function for Dolphin libraries
# Dolphin SDK (sources and headers from bfbbdecomp/bfbb, CC0)
cflags_dolphin = [
    *cflags_base,
    "-lang=c",
    "-fp fmadd",
    "-fp_contract off",
    "-char signed",
    "-str reuse",
    "-common off",
    "-O4,p",
    "-i src/PowerPC_EABI_Support/include",
    "-i src/dolphin/include",
    "-i src/dolphin/src",
]


def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_dolphin,
        "progress_category": "sdk",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    GameLib(
        "Core/gc",
        [
            Object(Matching, "Core/gc/iAnim.cpp"),
            Object(Equivalent, "Core/gc/iAnimSKB.cpp"),
            Object(Equivalent, "Core/gc/iCamera.cpp"),
            Object(Equivalent, "Core/gc/iCollide.cpp", extra_cflags=["-bool on"]),
            Object(Matching, "Core/gc/iCollideFast.cpp"),
            Object(Matching, "Core/gc/iCutscene.cpp"),
            Object(Matching, "Core/gc/iDraw.cpp"),
            Object(Equivalent, "Core/gc/iEnv.cpp"),
            Object(Equivalent, "Core/gc/iFile.cpp"),
            Object(Matching, "Core/gc/iFont.cpp"),
            Object(Matching, "Core/gc/iLight.cpp"),
            Object(Matching, "Core/gc/iMath.cpp"),
            Object(Equivalent, "Core/gc/iMath3.cpp", extra_cflags=["-bool on"]),
            Object(Matching, "Core/gc/iMemMgr.cpp"),
            Object(Equivalent, "Core/gc/iModel.cpp"),
            Object(Equivalent, "Core/gc/iMorph.cpp"),
            Object(
                Equivalent,
                "Core/gc/iPad.cpp",
                # Test builds can script controller input (tools/make_test_iso.py --autoplay)
                extra_cflags=(
                    ["-DSCOOB_AUTOPLAY", "-i build/test"]
                    if config.non_matching and __import__("os").environ.get("SCOOB_AUTOPLAY")
                    else []
                ),
            ),
            Object(Equivalent, "Core/gc/iParMgr.cpp"),
            Object(Equivalent, "Core/gc/isavegame.cpp"),
            Object(Matching, "Core/gc/iScrFX.cpp"),
            Object(Equivalent, "Core/gc/iShadow.cpp"),
            Object(Equivalent, "Core/gc/iSystem.cpp"),
            # GameCube-only units linked after the game code (names by role)
            Object(Equivalent, "Core/gc/iSnd.cpp"),
            Object(Equivalent, "Core/gc/iFMV.cpp"),
            Object(Equivalent, "Core/gc/iRad3D.c"),  # Bink RAD 3D image helpers
            Object(Equivalent, "Core/gc/iFX.cpp"),
            Object(Matching, "Core/gc/iMix.c"),  # older Dolphin MIX library
            Object(Matching, "Core/gc/iTime.cpp"),
        ],
    ),
    GameLib(
        "Core/x",
        [
            Object(Equivalent, "Core/x/xAnim.cpp", extra_cflags=["-bool on"]),
            Object(Matching, "Core/x/xBase.cpp"),
            Object(Matching, "Core/x/xbinio.cpp"),
            Object(Matching, "Core/x/xBound.cpp"),
            Object(Equivalent, "Core/x/xCamera.cpp"),
            Object(Equivalent, "Core/x/xClimate.cpp"),
            Object(Matching, "Core/x/xCollide.cpp"),
            Object(Matching, "Core/x/xCollideFast.cpp"),
            Object(Matching, "Core/x/xDebug.cpp"),
            Object(Equivalent, "Core/x/xDraw.cpp"),
            Object(Equivalent, "Core/x/xEnt.cpp"),
            Object(Matching, "Core/x/xEntDrive.cpp"),
            Object(Equivalent, "Core/x/xEntMotion.cpp"),
            Object(Matching, "Core/x/xEnv.cpp"),
            Object(Matching, "Core/x/xFFX.cpp"),
            Object(Matching, "Core/x/xFont.cpp"),
            Object(Equivalent, "Core/x/xFX.cpp"),
            Object(Matching, "Core/x/xhipio.cpp"),
            Object(Matching, "Core/x/xIni.cpp"),
            Object(Matching, "Core/x/xLinkList.cpp"),
            Object(Matching, "Core/x/xMath.cpp"),
            Object(Equivalent, "Core/x/xMath3.cpp"),
            Object(Equivalent, "Core/x/xMemMgr.cpp"),
            Object(Matching, "Core/x/xModel.cpp"),
            Object(Matching, "Core/x/xMorph.cpp"),
            Object(Matching, "Core/x/xMovePoint.cpp"),
            Object(Matching, "Core/x/xordarray.cpp"),
            Object(Matching, "Core/x/xPad.cpp"),
            Object(Matching, "Core/x/xPar.cpp"),
            Object(Matching, "Core/x/xParGroup.cpp"),
            Object(Equivalent, "Core/x/xParMgr.cpp"),
            Object(Equivalent, "Core/x/xPartition.cpp"),
            Object(Equivalent, "Core/x/xpkrsvc.cpp"),
            Object(Equivalent, "Core/x/xQuickCull.cpp"),
            Object(Equivalent, "Core/x/xsavegame.cpp"),
            Object(Matching, "Core/x/xScene.cpp"),
            Object(Equivalent, "Core/x/xScrFx.cpp"),
            Object(Equivalent, "Core/x/xserializer.cpp"),
            Object(Equivalent, "Core/x/xShadow.cpp"),
            Object(Equivalent, "Core/x/xSnd.cpp"),
            Object(Equivalent, "Core/x/xSpline.cpp"),
            Object(Equivalent, "Core/x/xstransvc.cpp"),
            Object(Equivalent, "Core/x/xString.cpp"),
            Object(Matching, "Core/x/xSurface.cpp"),
            Object(Equivalent, "Core/x/xTRC.cpp"),
            Object(Matching, "Core/x/xutil.cpp"),
        ],
    ),
    GameLib(
        "Game",
        [
            Object(Equivalent, "Game/zActionLine.cpp"),
            Object(Matching, "Game/zAnimList.cpp"),
            Object(Equivalent, "Game/zAssetTypes.cpp"),
            Object(Matching, "Game/zCamera.cpp"),
            Object(Equivalent, "Game/zConditional.cpp"),
            Object(Matching, "Game/zCounter.cpp"),
            Object(Matching, "Game/zCutsceneMgr.cpp"),
            Object(Equivalent, "Game/zDispatcher.cpp"),
            Object(Matching, "Game/zEGenerator.cpp"),
            Object(Equivalent, "Game/zEnt.cpp"),
            Object(Equivalent, "Game/zEntBubble.cpp"),
            Object(Equivalent, "Game/zEntButton.cpp"),
            Object(Matching, "Game/zEntDestructObj.cpp"),
            Object(Equivalent, "Game/zEntHangable.cpp"),
            Object(Equivalent, "Game/zEntPickup.cpp"),
            Object(Equivalent, "Game/zEntPlayer.cpp", extra_cflags=["-bool on"]),
            Object(Equivalent, "Game/zEntProjectile.cpp"),
            Object(Equivalent, "Game/zEntShaggy.cpp", extra_cflags=["-bool on"]),
            Object(Matching, "Game/zEntSimpleObj.cpp"),
            Object(Matching, "Game/zEntTrigger.cpp", extra_cflags=["-bool on"]),
            Object(Equivalent, "Game/zEntVillain.cpp"),
            Object(Matching, "Game/zEntVillainHolo.cpp"),
            Object(Matching, "Game/zEnv.cpp"),
            Object(Equivalent, "Game/zFeet.cpp"),
            Object(Matching, "Game/zFog.cpp"),
            Object(Matching, "Game/zFX.cpp"),
            Object(Equivalent, "Game/zGame.cpp"),
            Object(Equivalent, "Game/zGameExtras.cpp"),
            Object(Matching, "Game/zGameState.cpp"),
            Object(Matching, "Game/zGroup.cpp"),
            Object(Matching, "Game/zGust.cpp"),
            Object(Matching, "Game/zHologram.cpp"),
            Object(Equivalent, "Game/zHud.cpp"),
            Object(Matching, "Game/zLaugh.cpp"),
            Object(Matching, "Game/zLight.cpp"),
            Object(Matching, "Game/zLightEffect.cpp"),
            Object(Equivalent, "Game/zLightning.cpp"),
            Object(Equivalent, "Game/zLobMaster.cpp"),
            Object(Equivalent, "Game/zMain.cpp"),
            Object(Matching, "Game/zMenu.cpp"),
            Object(Equivalent, "Game/zMovePoint.cpp"),
            Object(Matching, "Game/zMusic.cpp", extra_cflags=["-bool on"]),
            Object(Equivalent, "Game/zParCmd.cpp"),
            Object(Equivalent, "Game/zParEmitter.cpp"),
            Object(Matching, "Game/zParSys.cpp"),
            Object(Matching, "Game/zPendulum.cpp"),
            Object(Equivalent, "Game/zPlatform.cpp"),
            Object(Matching, "Game/zPortal.cpp"),
            Object(Matching, "Game/zRenderState.cpp"),
            Object(Matching, "Game/zRumble.cpp"),
            Object(Equivalent, "Game/zSaveLoad.cpp"),
            Object(Equivalent, "Game/zScene.cpp"),
            Object(Matching, "Game/zScript.cpp"),
            Object(Matching, "Game/zSFX.cpp"),
            Object(Matching, "Game/zSurface.cpp"),
            Object(Matching, "Game/zTimer.cpp"),
            Object(Equivalent, "Game/zUI.cpp"),
            Object(Matching, "Game/zUIFont.cpp"),
            Object(Equivalent, "Game/zVar.cpp"),
        ],
    ),
    DolphinLib(
        "amcstubs",
        [
            Object(Matching, "dolphin/src/amcstubs/AmcExi2Stubs.c"),
        ],
    ),
    DolphinLib(
        "si",
        [
            Object(Matching, "dolphin/src/si/SISamplingRate.c"),
        ],
    ),
    DolphinLib(
        "os",
        [
            Object(Matching, "dolphin/src/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/src/os/OSTime.c"),
            Object(Matching, "dolphin/src/os/OSAlloc.c"),
            Object(Matching, "dolphin/src/os/OSResetSW.c"),
        ],
    ),
    DolphinLib(
        "dvd",
        [
            Object(Matching, "dolphin/src/dvd/dvdqueue.c"),
            Object(Matching, "dolphin/src/dvd/dvderror.c"),
            Object(NonMatching, "dolphin/src/dvd/dvdfs.c"),
            Object(Matching, "dolphin/src/dvd/emu_level2/fstload.c"),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(Matching, "dolphin/src/dsp/dsp_task.c"),
        ],
    ),
    DolphinLib(
        "ax",
        [
            Object(NonMatching, "dolphin/src/ax/AXSPB.c"),
        ],
    ),
    DolphinLib(
        "OdemuExi2",
        [
            Object(NonMatching, "dolphin/src/OdemuExi2/DebuggerDriver.c"),
        ],
    ),
    # More Dolphin SDK units from bfbbdecomp/bfbb (CC0).
    DolphinLib(
        "bfbb_ax",
        [
            Object(NonMatching, "dolphin/src/ax/AXAlloc.c"),
            Object(NonMatching, "dolphin/src/ax/AXCL.c"),
            Object(NonMatching, "dolphin/src/ax/AXOut.c"),
            Object(Matching, "dolphin/src/ax/AXProf.c"),
        ],
    ),
    DolphinLib(
        "bfbb_gx",
        [
            Object(NonMatching, "dolphin/src/gx/GXFifo.c"),
            Object(NonMatching, "dolphin/src/gx/GXAttr.c"),
            Object(NonMatching, "dolphin/src/gx/GXMisc.c"),
            Object(NonMatching, "dolphin/src/gx/GXGeometry.c"),
            Object(NonMatching, "dolphin/src/gx/GXFrameBuf.c"),
            Object(NonMatching, "dolphin/src/gx/GXLight.c"),
            Object(NonMatching, "dolphin/src/gx/GXTexture.c"),
            Object(NonMatching, "dolphin/src/gx/GXBump.c"),
            Object(NonMatching, "dolphin/src/gx/GXTransform.c"),
        ],
    ),
    DolphinLib(
        "bfbb_os",
        [
            Object(NonMatching, "dolphin/src/os/OSAlarm.c"),
            Object(Matching, "dolphin/src/os/OSArena.c"),
            Object(NonMatching, "dolphin/src/os/OSMemory.c"),
        ],
    ),
    # Dolphin SDK units from zeldaret/tww (CC0) whose code matches ours.
    {
        "lib": "tww_exi",
        "mw_version": "GC/1.2.5n",
        "cflags": [*cflags_tww_base],
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "dolphin/exi/EXIUart.c", extra_cflags=['-O4,p']),
        ],
    },
    {
        "lib": "tww_base",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "dolphin/base/PPCArch.c"),
        ],
    },
    {
        "lib": "tww_os",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "dolphin/os/OSContext.c"),
            Object(Matching, "dolphin/os/OSCache.c"),
            Object(Matching, "dolphin/os/OSFont.c"),
            Object(NonMatching, "dolphin/os/OSInterrupt.c"),
            Object(Matching, "dolphin/os/OSLink.c"),
            Object(Matching, "dolphin/os/OSMutex.c"),
            Object(Matching, "dolphin/os/OSReset.c"),
            Object(Matching, "dolphin/os/OSRtc.c"),
            Object(Matching, "dolphin/os/OSSync.c"),
            Object(Matching, "dolphin/os/OSThread.c"),
            Object(Matching, "dolphin/os/__ppc_eabi_init.cpp"),
        ],
    },
    {
        "lib": "tww_db",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "dolphin/db/db.c"),
        ],
    },
    {
        "lib": "tww_mtx",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "dolphin/mtx/mtx.c"),
        ],
    },
    {
        "lib": "tww_dvd",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "dolphin/dvd/dvdlow.c"),
        ],
    },
    {
        "lib": "tww_pad",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(NonMatching, "dolphin/pad/Padclamp.c"),
            Object(Matching, "dolphin/pad/Pad.c"),
        ],
    },
    {
        "lib": "tww_si",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "dolphin/si/SIBios.c"),
        ],
    },
    {
        "lib": "tww_ai",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "dolphin/ai/ai.c"),
            Object(Matching, "dolphin/ar/arq.c"),
        ],
    },
    {
        "lib": "tww_card",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "dolphin/card/CARDOpen.c"),
        ],
    },
    {
        "lib": "tww_gx",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_tww_dolphin,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "dolphin/gx/GXDisplayList.c"),
            Object(NonMatching, "dolphin/gx/GXPerf.c"),
        ],
    },
    # MetroTRK, Runtime and MSL: byte-identical to The Wind Waker's copies.
    # Sources and flags from zeldaret/tww (CC0); see tools/place_ref_objects.py.
    {
        "lib": "TRK_MINNOW_DOLPHIN",
        "mw_version": "GC/1.3.2",
        "cflags": cflags_tww_trk,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "TRK_MINNOW_DOLPHIN/Portable/mainloop.c", extra_cflags=["-enum min"]),
            Object(Matching, "TRK_MINNOW_DOLPHIN/Portable/nubevent.c", extra_cflags=["-enum min"]),
            Object(Matching, "TRK_MINNOW_DOLPHIN/Portable/usr_put.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/Portable/mutex_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/ppc/Generic/flush_cache.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/ppc/Generic/mpc_7xx_603e.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/Os/dolphin/targcont.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/Os/dolphin/target_options.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/Portable/main_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/MetroTRK/Export/mslsupp.c"),
        ],
    },
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": "GC/1.3",
        "cflags": cflags_tww_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "PowerPC_EABI_Support/Runtime/Src/__va_arg.c"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/Src/global_destructor_chain.c"),
            Object(NonMatching, "PowerPC_EABI_Support/Runtime/Src/runtime.c"),
        ],
    },
    {
        "lib": "MSL_C",
        "mw_version": "GC/1.3",
        "cflags": cflags_tww_runtime,
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/ctype.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/abort_exit.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/arith.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mbstring.c", extra_cflags=["-inline noauto"]),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/printf.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/string.c"),
            Object(NonMatching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/strtoul.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Src/uart_console_io.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/direct_io.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/file_io.c", extra_cflags=["-inline noauto"]),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/FILE_POS.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mem.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/mem_funcs.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/misc_io.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/scanf.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Src/wchar_io.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_acos.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_asin.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_atan2.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_fmod.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_pow.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_rem_pio2.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_cos.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_rem_pio2.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_sin.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/k_tan.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_atan.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_cos.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_floor.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_frexp.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ldexp.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_sin.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_tan.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_copysign.c"),
            Object(Matching, "PowerPC_EABI_Support/MSL/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_modf.c"),
        ],
    },
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
]
config.progress_each_module = args.verbose
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]

if args.mode == "configure":
    # Extract main.dol and write the entry-point-patched copy used for analysis
    import subprocess

    subprocess.run(
        [sys.executable, "tools/prepare_dol.py", f"orig/{config.version}"], check=True
    )
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
