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
            Object(NonMatching, "Core/gc/iAnim.cpp"),
            Object(NonMatching, "Core/gc/iAnimSKB.cpp"),
            Object(NonMatching, "Core/gc/iCamera.cpp"),
            Object(NonMatching, "Core/gc/iCollide.cpp"),
            Object(NonMatching, "Core/gc/iCollideFast.cpp"),
            Object(NonMatching, "Core/gc/iCutscene.cpp"),
            Object(Matching, "Core/gc/iDraw.cpp"),
            Object(Matching, "Core/gc/iEnv.cpp"),
            Object(NonMatching, "Core/gc/iFile.cpp"),
            Object(NonMatching, "Core/gc/iFont.cpp"),
            Object(NonMatching, "Core/gc/iLight.cpp"),
            Object(NonMatching, "Core/gc/im3dpipe.c"),
            Object(Matching, "Core/gc/iMath.cpp"),
            Object(NonMatching, "Core/gc/iMath3.cpp"),
            Object(NonMatching, "Core/gc/iModel.cpp"),
            Object(NonMatching, "Core/gc/iMorph.cpp"),
            Object(NonMatching, "Core/gc/iPad.cpp"),
            Object(NonMatching, "Core/gc/iParMgr.cpp"),
            Object(NonMatching, "Core/gc/isavegame.cpp"),
            Object(NonMatching, "Core/gc/iScrFX.cpp"),
            Object(NonMatching, "Core/gc/iSystem.cpp"),
            Object(NonMatching, "Core/gc/itools.c"),
            Object(NonMatching, "Core/gc/iTRC.cpp"),
        ],
    ),
    GameLib(
        "Core/x",
        [
            Object(NonMatching, "Core/x/xAnim.cpp"),
            Object(Matching, "Core/x/xBase.cpp"),
            Object(NonMatching, "Core/x/xbinio.cpp"),
            Object(NonMatching, "Core/x/xBound.cpp"),
            Object(NonMatching, "Core/x/xCamera.cpp"),
            Object(NonMatching, "Core/x/xClimate.cpp"),
            Object(NonMatching, "Core/x/xCollide.cpp"),
            Object(NonMatching, "Core/x/xCollideFast.cpp"),
            Object(NonMatching, "Core/x/xDebug.cpp"),
            Object(NonMatching, "Core/x/xDraw.cpp"),
            Object(NonMatching, "Core/x/xEnt.cpp"),
            Object(NonMatching, "Core/x/xEntDrive.cpp"),
            Object(NonMatching, "Core/x/xEntMotion.cpp"),
            Object(Matching, "Core/x/xEnv.cpp"),
            Object(NonMatching, "Core/x/xFFX.cpp"),
            Object(NonMatching, "Core/x/xFont.cpp"),
            Object(NonMatching, "Core/x/xFX.cpp"),
            Object(NonMatching, "Core/x/xhipio.cpp"),
            Object(NonMatching, "Core/x/xIni.cpp"),
            Object(Matching, "Core/x/xLinkList.cpp"),
            Object(Matching, "Core/x/xMath.cpp"),
            Object(NonMatching, "Core/x/xMath3.cpp"),
            Object(NonMatching, "Core/x/xMemMgr.cpp"),
            Object(NonMatching, "Core/x/xModel.cpp"),
            Object(NonMatching, "Core/x/xMorph.cpp"),
            Object(NonMatching, "Core/x/xMovePoint.cpp"),
            Object(Matching, "Core/x/xordarray.cpp"),
            Object(NonMatching, "Core/x/xPad.cpp"),
            Object(Matching, "Core/x/xPar.cpp"),
            Object(NonMatching, "Core/x/xParGroup.cpp"),
            Object(NonMatching, "Core/x/xParMgr.cpp"),
            Object(NonMatching, "Core/x/xPartition.cpp"),
            Object(NonMatching, "Core/x/xpkrsvc.cpp"),
            Object(NonMatching, "Core/x/xQuickCull.cpp"),
            Object(NonMatching, "Core/x/xsavegame.cpp"),
            Object(NonMatching, "Core/x/xScene.cpp"),
            Object(NonMatching, "Core/x/xScrFx.cpp"),
            Object(NonMatching, "Core/x/xserializer.cpp"),
            Object(NonMatching, "Core/x/xShadow.cpp"),
            Object(NonMatching, "Core/x/xSnd.cpp"),
            Object(NonMatching, "Core/x/xSpline.cpp"),
            Object(NonMatching, "Core/x/xstransvc.cpp"),
            Object(NonMatching, "Core/x/xString.cpp"),
            Object(NonMatching, "Core/x/xSurface.cpp"),
            Object(NonMatching, "Core/x/xTRC.cpp"),
            Object(NonMatching, "Core/x/xutil.cpp"),
        ],
    ),
    GameLib(
        "Game",
        [
            Object(NonMatching, "Game/zActionLine.cpp"),
            Object(Matching, "Game/zAnimList.cpp"),
            Object(NonMatching, "Game/zAssetTypes.cpp"),
            Object(Matching, "Game/zCamera.cpp"),
            Object(NonMatching, "Game/zConditional.cpp"),
            Object(Matching, "Game/zCounter.cpp"),
            Object(NonMatching, "Game/zCutsceneMgr.cpp"),
            Object(NonMatching, "Game/zDispatcher.cpp"),
            Object(NonMatching, "Game/zEGenerator.cpp"),
            Object(NonMatching, "Game/zEnt.cpp"),
            Object(NonMatching, "Game/zEntBubble.cpp"),
            Object(NonMatching, "Game/zEntButton.cpp"),
            Object(NonMatching, "Game/zEntDestructObj.cpp"),
            Object(NonMatching, "Game/zEntHangable.cpp"),
            Object(NonMatching, "Game/zEntPickup.cpp"),
            Object(NonMatching, "Game/zEntPlayer.cpp"),
            Object(NonMatching, "Game/zEntProjectile.cpp"),
            Object(NonMatching, "Game/zEntShaggy.cpp"),
            Object(NonMatching, "Game/zEntSimpleObj.cpp"),
            Object(NonMatching, "Game/zEntTrigger.cpp"),
            Object(NonMatching, "Game/zEntVillain.cpp"),
            Object(NonMatching, "Game/zEnv.cpp"),
            Object(NonMatching, "Game/zFeet.cpp"),
            Object(Matching, "Game/zFog.cpp"),
            Object(NonMatching, "Game/zFX.cpp"),
            Object(NonMatching, "Game/zGame.cpp"),
            Object(NonMatching, "Game/zGameExtras.cpp"),
            Object(Matching, "Game/zGameState.cpp"),
            Object(Matching, "Game/zGroup.cpp"),
            Object(NonMatching, "Game/zGust.cpp"),
            Object(NonMatching, "Game/zHologram.cpp"),
            Object(NonMatching, "Game/zHud.cpp"),
            Object(NonMatching, "Game/zLaugh.cpp"),
            Object(NonMatching, "Game/zLight.cpp"),
            Object(NonMatching, "Game/zLightning.cpp"),
            Object(NonMatching, "Game/zLobMaster.cpp"),
            Object(NonMatching, "Game/zMain.cpp"),
            Object(NonMatching, "Game/zMenu.cpp"),
            Object(NonMatching, "Game/zMovePoint.cpp"),
            Object(NonMatching, "Game/zMusic.cpp"),
            Object(NonMatching, "Game/zParCmd.cpp"),
            Object(NonMatching, "Game/zParEmitter.cpp"),
            Object(NonMatching, "Game/zParSys.cpp"),
            Object(NonMatching, "Game/zPendulum.cpp"),
            Object(NonMatching, "Game/zPlatform.cpp"),
            Object(Matching, "Game/zPortal.cpp"),
            Object(Matching, "Game/zRenderState.cpp"),
            Object(Matching, "Game/zRumble.cpp"),
            Object(NonMatching, "Game/zSaveLoad.cpp"),
            Object(NonMatching, "Game/zScene.cpp"),
            Object(Matching, "Game/zScript.cpp"),
            Object(Matching, "Game/zSFX.cpp"),
            Object(NonMatching, "Game/zSurface.cpp"),
            Object(Matching, "Game/zTimer.cpp"),
            Object(NonMatching, "Game/zUI.cpp"),
            Object(NonMatching, "Game/zUIFont.cpp"),
            Object(NonMatching, "Game/zVar.cpp"),
            Object(NonMatching, "Game/zVolume.cpp"),
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
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "progress_category": "sdk",  # str | List[str]
        "objects": [
            Object(NonMatching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
            Object(NonMatching, "Runtime.PPCEABI.H/__init_cpp_exceptions.cpp"),
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
