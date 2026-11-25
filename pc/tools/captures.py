#!/usr/bin/env python3
# Scripted capture check: starts the game in a scene, scripts the pad with
# PC_AUTOPAD, saves consecutive frames with PC_SHOT and measures them, so a
# visual regression is caught by one command before a release.
#
# Two kinds of check, both of which the "Fear on the Pier" lighthouse beams
# failed before the alpha-test fix (a shaft that vanished for one frame, then
# came back):
#
#   - stability: no frame may jump away from the one before it by more than the
#     scene's limit. A model that drops out for a frame, or geometry that
#     flickers, shows up here even though the first and last frames look fine.
#   - brightness: no frame may be nearly blank, and (once a baseline exists)
#     the scene's average brightness and worst jump must stay where the
#     baseline recorded them.
#
# usage:
#   python pc/tools/captures.py                   # check
#   python pc/tools/captures.py --update          # record a new baseline
#   python pc/tools/captures.py --only f010       # scenes whose name matches
#   python pc/tools/captures.py --env PC_DBG=nocull   # extra environment
#
# The run needs the game's own files (game/files beside the project, or
# SCOOBY_DATA). No image is kept by the project: the frames land in
# build/pc/shot_<frame>.bmp, and the baseline is numbers only, in
# build/pc/captures/baseline.json, rewritten per machine because the GPU and
# driver change the exact pixels.
import argparse
import json
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD = Path(os.environ.get('PC_BUILD_DIR') or ROOT / 'build' / 'pc')
# PC_SHOT always writes build/pc/shot_<frame>.bmp (pc/rw/gpu_gl.cpp).
SHOT_DIR = ROOT / 'build' / 'pc'
OUT = BUILD / 'captures'
CONFIG = Path(__file__).with_name('captures.json')
BASELINE = OUT / 'baseline.json'
# Coarse brightness grid: geometry that stops drawing changes many cells at
# once, while filtering or driver differences move each cell only a little.
GRID_W, GRID_H = 16, 12


def log_dir():
    appdata = os.environ.get('APPDATA')
    return Path(appdata) / 'ProjectMysteryMachine' / 'logs' if appdata else None


def crash_logs():
    d = log_dir()
    return set(d.glob('crash*.log')) if d and d.is_dir() else set()


def write_ini(cfg):
    OUT.mkdir(parents=True, exist_ok=True)
    w, h = cfg.get('resolution', [640, 480])
    path = OUT / 'captures.ini'
    path.write_text(
        '; Written by pc/tools/captures.py for the capture check.\n'
        '[game]\n'
        'skip_logos = 1\n'
        '[display]\n'
        'width = %d\n'
        'height = %d\n'
        'mode = windowed\n'
        'aspect = 4:3\n'
        'vsync = 0\n'
        'msaa = 0\n'
        '[graphics]\n'
        'fps = %d\n'
        'draw_distance = 4.00\n'
        'effects_distance = 4.00\n'
        'shadow_res = 1024\n' % (w, h, cfg.get('fps', 30)))
    return path


def executable(override=None):
    if override:
        path = Path(override)
        if not path.is_absolute():
            path = ROOT / path
        if not path.exists():
            raise SystemExit('%s does not exist' % path)
        return path
    for name in ('scooby_debug.exe', 'scooby.exe'):
        path = BUILD / name
        if path.exists():
            return path
    raise SystemExit('%s is missing: build it first (sh pc/tools/build.sh)' % (BUILD / 'scooby.exe'))


def pid_running(pid):
    """Whether a process with this pid still exists.

    The child handle alone is not enough: the point of checking is that the
    instance is really gone, not that a kill command was issued.
    """
    if os.name == 'nt':
        try:
            out = subprocess.run(['tasklist', '/FI', 'PID eq %d' % pid, '/NH'],
                                 capture_output=True, text=True, timeout=10)
        except (OSError, subprocess.SubprocessError):
            return True  # cannot tell: treat it as alive and keep trying
        return str(pid) in out.stdout
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except OSError:
        return True
    return True


def stop(proc, timeout=20.0):
    """Stops the game this run started, and returns only once it has exited.

    taskkill's result is checked rather than trusted, and the process handle is
    terminated directly if it is still there, because an instance that survives
    keeps running and writes shots into the folder the next scene measures.
    """
    if proc.poll() is not None:
        return
    deadline = time.time() + timeout
    asked_taskkill = False
    while True:
        if not asked_taskkill:
            if os.name == 'nt':
                # Only this run's process tree: taskkill by the exact pid.
                subprocess.run(['taskkill', '/F', '/T', '/PID', str(proc.pid)],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            else:
                proc.terminate()
            asked_taskkill = True
        else:
            # taskkill did not work: terminate the handle itself, which needs
            # no other process and cannot be refused.
            proc.kill()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            pass
        proc.poll()  # reap it if it has exited
        if not pid_running(proc.pid):
            return
        if time.time() > deadline:
            break
        time.sleep(0.25)
    raise RuntimeError('the game (pid %d) survived stop(): it would keep writing shots for '
                       'the next scene, so the run stops here' % proc.pid)


def run_scene(scene, cfg, ini, extra_env, exe):
    """Runs one scene and returns the frames it captured, in order."""
    for old in SHOT_DIR.glob('shot_*.bmp'):
        old.unlink()
    start, count = int(scene['start']), int(scene['frames'])
    env = dict(os.environ)
    env.update({
        'PC_SETTINGS': str(ini),
        'PC_SCENE': scene['scene'],
        'PC_NOFMV': '1',
        'PC_AUTOPAD': scene.get('autopad', ''),
        'PC_SHOT': '%d,1' % start,
    })
    data = Path(os.environ.get('SCOOBY_DATA') or ROOT / 'game' / 'files')
    if data.is_dir():
        env['SCOOBY_DATA'] = str(data)
    env.update(extra_env)

    OUT.mkdir(parents=True, exist_ok=True)
    run_log = OUT / ('run-%s.log' % scene['name'])
    with open(run_log, 'wb') as out:
        proc = subprocess.Popen([str(exe)], cwd=str(ROOT), env=env,
                                stdout=out, stderr=subprocess.STDOUT)
    wanted = [SHOT_DIR / ('shot_%d.bmp' % (start + i)) for i in range(count)]
    deadline = time.time() + float(scene.get('timeout', 180))
    try:
        while time.time() < deadline:
            if wanted[-1].exists() or proc.poll() is not None:
                break
            time.sleep(0.2)
        time.sleep(0.3)  # let the last screenshot finish being written
    finally:
        # Also on Ctrl-C or a timeout: the game must not outlive the scene.
        stop(proc)
    return wanted, run_log, proc.returncode


def frame_stats(path):
    """Mean and standard deviation of a screenshot's green channel.

    The green channel is a good enough brightness proxy for these checks and
    is a single strided slice per row, so no image library is needed.
    """
    data = path.read_bytes()
    offset = struct.unpack_from('<I', data, 10)[0]
    width, height = struct.unpack_from('<ii', data, 18)
    bpp = struct.unpack_from('<H', data, 28)[0]
    if bpp not in (24, 32):
        raise SystemExit('%s: unexpected %d-bit screenshot' % (path.name, bpp))
    stride = bpp // 8
    row = width * stride
    if height < 0:  # top-down bitmap
        height, top = -height, offset
    else:
        top = offset
    green = bytearray()
    for y in range(0, abs(height), 2):
        line = data[top + y * row: top + (y + 1) * row]
        green.extend(line[1::stride * 2])
    n = len(green) or 1
    mean = sum(green) / n
    var = sum(v * v for v in green) / n - mean * mean
    # The same samples, averaged into a GRID_W x GRID_H grid of brightnesses.
    grid = []
    rows = abs(height) // 2 or 1
    stride_g = len(green) // rows
    for gy in range(GRID_H):
        y0, y1 = gy * rows // GRID_H, (gy + 1) * rows // GRID_H
        for gx in range(GRID_W):
            x0, x1 = gx * stride_g // GRID_W, (gx + 1) * stride_g // GRID_W
            total, count = 0, 0
            for y in range(y0, y1):
                row = green[y * stride_g:(y + 1) * stride_g]
                total += sum(row[x0:x1])
                count += x1 - x0
            grid.append(total / count if count else 0.0)
    return green, mean, max(var, 0.0) ** 0.5, grid


def measure(paths):
    """Per-frame brightness, plus the jump from each frame to the next."""
    frames, prev = [], None
    for path in paths:
        green, mean, sd, grid = frame_stats(path)
        jump = None
        if prev is not None:
            jump = sum(abs(a - b) for a, b in zip(prev, green)) / len(green)
        frames.append({'mean': mean, 'sd': sd, 'jump': jump, 'grid': grid})
        prev = green
    return frames


def mean_grid(frames):
    """The window averaged into one grid, for a like-for-like comparison.

    The baseline records this average, and the check compares it against the
    same average, so neither side is one arbitrary frame's brightness.
    """
    return [round(sum(f['grid'][i] for f in frames) / len(frames), 1)
            for i in range(len(frames[0]['grid']))]


def check_scene(scene, frames, baseline, log_text, new_crashes):
    """Measures one scene: returns its numbers and what is wrong with them.

    Each problem is ('defect', text) for something a baseline must never
    accept (a blank frame, a frame that jumps, a stub call, a crash) or
    ('look', text) for a difference from the recorded baseline, which an
    intended visual change may make.
    """
    name = scene['name']
    limit = float(scene.get('max_diff', 6.0))
    means = [f['mean'] for f in frames]
    jumps = [f['jump'] for f in frames if f['jump'] is not None]
    mean = sum(means) / len(means)
    worst = max(jumps) if jumps else 0.0

    problems = []
    blank = [i for i, f in enumerate(frames) if f['mean'] < 4.0 or f['sd'] < 4.0]
    if blank:
        problems.append(('defect', '%d blank frame(s), first at frame %d' % (len(blank), blank[0])))
    if worst > limit:
        at = jumps.index(worst) + 1
        problems.append(('defect', 'frame %d jumps %.1f (limit %.1f)' % (at, worst, limit)))
    base = baseline.get(name)
    if base:
        if abs(mean - base['mean']) > max(2.0, 0.05 * base['mean']):
            problems.append(('look', 'brightness %.1f, baseline %.1f' % (mean, base['mean'])))
        if worst > base['maxDiff'] * 1.5 + 1.0:
            problems.append(('look', 'worst jump %.1f, baseline %.1f' % (worst, base['maxDiff'])))
        if 'grid' in base:
            # The window's average brightness per cell, against the same
            # average recorded by --update: a scene that stops drawing
            # (missing ground, sky or models) moves many cells at once.
            grid = mean_grid(frames)
            tol = float(scene.get('cell_tolerance', 12.0))
            off = sum(1 for a, b in zip(grid, base['grid']) if abs(a - b) > tol)
            allowed = max(2, len(grid) * 3 // 100)
            if off > allowed:
                problems.append(('look', '%d of %d grid cells differ by over %.0f'
                                 % (off, len(grid), tol)))
    stubs = log_text.count('[stub]')
    if stubs:
        problems.append(('defect', '%d stub call(s) in the run log' % stubs))
    if new_crashes:
        problems.append(('defect', '%d new crash log(s)' % len(new_crashes)))

    status = 'ok' if not problems else 'FAILED: ' + '; '.join(text for _, text in problems)
    print('%-12s %-4s %3d frames  mean %5.1f  jump %4.1f/%-4.1f  %s'
          % (name, scene['scene'], len(frames), mean, worst, limit, status))
    result = {'mean': mean, 'maxDiff': worst, 'frames': len(frames), 'grid': mean_grid(frames)}
    return result, problems


def main():
    parser = argparse.ArgumentParser(description='Scripted capture check for visual regressions.')
    parser.add_argument('--update', action='store_true',
                        help='record the baseline from this run (only if it passes)')
    parser.add_argument('--force-update', action='store_true',
                        help='record even though the scenes differ from the baseline (an intended change)')
    parser.add_argument('--only', help='only scenes whose name contains this text')
    parser.add_argument('--env', action='append', default=[], metavar='NAME=VALUE',
                        help='extra environment for the game (repeatable)')
    parser.add_argument('--exe', help='test another build (default: scooby_debug.exe)')
    args = parser.parse_args()
    args.update = args.update or args.force_update

    cfg = json.loads(CONFIG.read_text())
    scenes = [s for s in cfg['scenes'] if not args.only or args.only.lower() in s['name'].lower()]
    if not scenes:
        raise SystemExit('%s has no scene matching %r' % (CONFIG, args.only))
    exe = executable(args.exe)
    if not args.exe and not (BUILD / 'scooby.exe').exists() and not (BUILD / 'scooby_debug.exe').exists():
        raise SystemExit('%s has no game executable: build it first' % BUILD)
    data = Path(os.environ.get('SCOOBY_DATA') or ROOT / 'game' / 'files')
    if not data.is_dir() and not os.environ.get('SCOOBY_DATA'):
        raise SystemExit('game files not found in %s: extract your disc first, or set SCOOBY_DATA' % data)

    extra_env = {}
    for entry in args.env:
        key, _, value = entry.partition('=')
        extra_env[key] = value
    ini = write_ini(cfg)
    # --update compares against the old baseline too: a run may only replace it
    # when it does not look like a regression.
    baseline = {}
    if BASELINE.exists():
        baseline = json.loads(BASELINE.read_text()).get('scenes', {})

    print('%s, baseline %s' % (exe.name, 'yes' if baseline else 'none yet (--update)'))
    results, failures, kinds = {}, [], {}
    for scene in scenes:
        crashes_before = crash_logs()
        paths, run_log, code = run_scene(scene, cfg, ini, extra_env, exe)
        missing = [p for p in paths if not p.exists()]
        if missing:
            why = 'capture stopped at frame %s' % missing[0].stem.split('_')[-1]
            if code not in (None, 0):
                why += ', the game exited with %s (see %s)' % (code, run_log.name)
            print('%-12s %-4s %3d/%d frames  FAILED: %s'
                  % (scene['name'], scene['scene'], len(paths) - len(missing), len(paths), why))
            failures.append(scene['name'])
            kinds[scene['name']] = {'defect'}
            continue
        frames = measure(paths)
        log_text = run_log.read_text(errors='replace') if run_log.exists() else ''
        results[scene['name']], problems = check_scene(scene, frames, baseline, log_text,
                                                      crash_logs() - crashes_before)
        kinds[scene['name']] = {kind for kind, _ in problems}
        if problems:
            failures.append(scene['name'])

    if args.update:
        # A failing run describes a regression, not a new look: recording it
        # would make the check accept the breakage from then on.
        defects = [n for n in failures if 'defect' in kinds.get(n, ())]
        looks = [n for n in failures if 'defect' not in kinds.get(n, ())]
        if defects or len(results) != len(scenes):
            print('no baseline written: %d of %d scene(s) failed checks a baseline must not record'
                  % (len(failures), len(scenes)))
            return 1
        if looks and not args.force_update:
            print('no baseline written: %s now look(s) different from the baseline; if that change is '
                  'intended, repeat with --force-update' % ', '.join(looks))
            return 1
        OUT.mkdir(parents=True, exist_ok=True)
        # Keep the scenes this run did not cover (--only) at their old values.
        recorded = {}
        if BASELINE.exists():
            recorded = json.loads(BASELINE.read_text()).get('scenes', {})
        recorded.update(results)
        BASELINE.write_text(json.dumps({'scenes': recorded}, indent=2, sort_keys=True) + '\n')
        print('baseline written to %s (%d scene(s))' % (BASELINE, len(recorded)))
        return 0
    if failures:
        print('%d of %d scene(s) regressed; frames and logs are in %s'
              % (len(failures), len(scenes), OUT))
        return 1
    print('%d scene(s) ok' % len(scenes))
    return 0


if __name__ == '__main__':
    sys.exit(main())
