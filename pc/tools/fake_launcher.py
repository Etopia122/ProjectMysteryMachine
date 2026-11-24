"""Plays RecompLauncher's side of the achievements channel, on Windows.

The SDK's own ``sdk/tests/fake_launcher.py`` stands in for the launcher over a
Unix socket. The Windows build of the game opens the named pipe that
``RL_SESSION`` names instead, so this speaks the same protocol over that pipe
and checks the same kinds of message.

Seven runs, all of them the game starting a new game (``PC_AUTOPAD`` presses
Start then A, as the capture check does):

1. With a launcher: it must send ``hello``, answer the list, and then report the
   ``new_game`` event and the events the timed ``PC_*`` switches cause.
2. With only a give switch set, no ``PC_SCENE``: the session must be marked
   casual under that switch's own name.
3. With ``PC_MASTERMIND`` and no ``PC_SCENE``: the Mastermind is beaten through
   the game's own call, in a run of its own because that also sets the endgame
   flag.
4. With ``PC_SAVELOAD`` and a scratch card folder: a real save is written and
   read back, and the launcher hears ``save_loaded``.
5. With the pad: the hub's Mystery Machine save point opens the save screen
   (there is no Save in the menus). Slot 0 is saved into a scratch card folder,
   then saved over through its overwrite prompt (which starts on NO), and a
   third run at the title chooses Load Game and that slot; the level that comes
   back holds the snacks the second save had.
6. Without ``RL_SESSION``, as when a player starts scooby.exe by hand: no
   channel, the game runs normally.
7. With ``RL_SESSION`` naming a pipe nobody serves: the same, without waiting.

usage: python pc/tools/fake_launcher.py [exe] [--seconds N]
"""

import ctypes
import json
import os
import shutil
import subprocess
import sys
import threading
import time
import uuid
from ctypes import wintypes
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'build' / 'pc'
OUT = BUILD / 'achievements'
# The same autopad the capture check uses to get from the title screen into the
# game: Start at 2 s, A (New Game) at 3 s, release at 4 s.
AUTOPAD = '2:1000,3:0100,4:0000'
SCENE = 'I001'
# The Mystery Machine save point in the hub (H001's SAVEPOINT pickup, in front
# of the van; I001 has none): PC_TELEPORT moves Scooby onto it at 14 s, which
# opens the save screen.
HUB = 'H001'
HUB_SAVEPOINT = '14:2.23,1.58,3.67'

PIPE_ACCESS_DUPLEX = 0x00000003
PIPE_TYPE_BYTE = 0x00000000
PIPE_READMODE_BYTE = 0x00000000
PIPE_WAIT = 0x00000000
ERROR_PIPE_CONNECTED = 535

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
kernel32.CreateNamedPipeA.restype = wintypes.HANDLE
kernel32.CreateNamedPipeA.argtypes = [wintypes.LPCSTR, wintypes.DWORD, wintypes.DWORD,
                                      wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
                                      wintypes.DWORD, ctypes.c_void_p]
kernel32.ConnectNamedPipe.argtypes = [wintypes.HANDLE, ctypes.c_void_p]
kernel32.ReadFile.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD,
                              ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
kernel32.WriteFile.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]


def executable(override=None):
    if override:
        path = Path(override)
        if not path.is_absolute():
            path = ROOT / path
        if not path.exists():
            raise SystemExit('%s does not exist' % path)
        return path
    for name in ('scooby.exe', 'scooby_debug.exe'):
        path = BUILD / name
        if path.exists():
            return path
    raise SystemExit('%s is missing: build it first (sh pc/tools/build.sh)' % (BUILD / 'scooby.exe'))


def write_ini():
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / 'achievements.ini'
    path.write_text('; Written by pc/tools/fake_launcher.py for the achievement test.\n'
                    '[game]\n'
                    'skip_logos = 1\n'
                    '[display]\n'
                    'width = 640\n'
                    'height = 480\n'
                    'mode = windowed\n'
                    'aspect = 4:3\n'
                    'vsync = 0\n'
                    'msaa = 0\n'
                    '[graphics]\n'
                    'fps = 30\n')
    return path


def log_path():
    appdata = os.environ.get('APPDATA')
    return Path(appdata) / 'ProjectMysteryMachine' / 'logs' / 'scooby.log' if appdata else None


def log_size(path):
    try:
        return path.stat().st_size
    except OSError:
        return 0


def log_since(path, offset):
    try:
        with open(path, 'rb') as f:
            f.seek(offset)
            return f.read().decode('utf-8', 'replace')
    except OSError:
        return ''


def stop(proc):
    if proc.poll() is not None:
        return
    # Only this run's process tree: taskkill by the exact pid.
    subprocess.run(['taskkill', '/F', '/T', '/PID', str(proc.pid)],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        proc.wait(timeout=20)
    except subprocess.TimeoutExpired:
        proc.kill()


class Launcher:
    """The launcher's end of the pipe: records what the game sends, answers."""

    def __init__(self):
        self.name = 'scooby-ach-%d-%s' % (os.getpid(), uuid.uuid4().hex[:8])
        self.path = '\\\\.\\pipe\\' + self.name
        self.handle = kernel32.CreateNamedPipeA(
            self.path.encode(), PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1, 64 * 1024, 64 * 1024, 0, None)
        if self.handle == wintypes.HANDLE(-1).value:
            raise SystemExit('CreateNamedPipeA failed: %d' % ctypes.get_last_error())
        self.messages = []
        self.connected = False
        self.done = False
        self._lock = threading.Lock()
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()

    def _send(self, obj):
        line = (json.dumps(obj) + '\n').encode()
        written = wintypes.DWORD()
        kernel32.WriteFile(self.handle, line, len(line), ctypes.byref(written), None)

    def _serve(self):
        if not kernel32.ConnectNamedPipe(self.handle, None):
            if ctypes.get_last_error() != ERROR_PIPE_CONNECTED:
                return
        self.connected = True
        rx = b''
        buf = ctypes.create_string_buffer(64 * 1024)
        while not self.done:
            n = wintypes.DWORD()
            if not kernel32.ReadFile(self.handle, buf, len(buf), ctypes.byref(n), None):
                break
            rx += buf.raw[:n.value]
            while b'\n' in rx:
                line, rx = rx.split(b'\n', 1)
                if not line.strip():
                    continue
                try:
                    message = json.loads(line)
                except ValueError:
                    print('launcher <- not JSON: %r' % line)
                    continue
                with self._lock:
                    self.messages.append(message)
                print('launcher <- %s' % json.dumps(message, sort_keys=True))
                if message.get('t') == 'hello':
                    self._send({'t': 'quiet'})
                    # A list in the events format: the launcher maps the name
                    # of a code event to an achievement. The list is hosted with
                    # the game, not compiled in (pc/docs/achievements.md).
                    self._send({'t': 'list', 'achievements': [
                        {'id': 1, 'title': 'The Mystery Begins', 'points': 5,
                         'type': 'event', 'event': 'new_game'}]})

    def kinds(self):
        with self._lock:
            return [m.get('t') for m in self.messages]

    def events(self):
        with self._lock:
            return [m.get('name') for m in self.messages if m.get('t') == 'event']

    def wait_for(self, kind, timeout):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if kind in self.kinds():
                return True
            time.sleep(0.1)
        return False

    def wait_event(self, name, timeout):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if name in self.events():
                return True
            time.sleep(0.1)
        return False

    def close(self):
        self.done = True
        kernel32.CloseHandle(self.handle)


def start(exe, ini, extra_env, scene=True, autopad=AUTOPAD):
    env = dict(os.environ)
    env.update({
        'PC_SETTINGS': str(ini),
        'PC_NOFMV': '1',
        'PC_AUTOPAD': autopad,
    })
    if scene:
        env['PC_SCENE'] = SCENE
    data = Path(os.environ.get('SCOOBY_DATA') or ROOT / 'game' / 'files')
    if data.is_dir():
        env['SCOOBY_DATA'] = str(data)
    env.update(extra_env)
    OUT.mkdir(parents=True, exist_ok=True)
    return subprocess.Popen([str(exe)], cwd=str(ROOT), env=env,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def run_with_launcher(exe, ini, seconds):
    launcher = Launcher()
    log = log_path()
    offset = log_size(log) if log else 0
    print('run 1: %s with RL_SESSION=%s' % (exe.name, launcher.name))
    # Counters are driven well into play (the run reaches gameplay at about 8 s),
    # so the watchers have to notice a change rather than state that was already
    # true: PC_SNACKS collects a snack through the game's own pickup path,
    # PC_TOKENS collects all 21 monster tokens the same way, PC_CLUES gives 10
    # clues as a clue pickup does, PC_DEATHS and PC_SCARES count ten deaths and
    # fifty scares the way losing a life and a fright do, PC_AREAS finishes every
    # real area's snacks (which also proves the 100 and 1,000 snack totals),
    # PC_CUTSCENES marks all 14 cutscenes seen, PC_ALLWARPS opens every gate, and
    # PC_UPGRADES gives the debug menu's every-upgrade item.
    proc = start(exe, ini, {
        'RL_SESSION': launcher.name,
        'PC_SNACKS': '12:1',
        'PC_TOKENS': '13:21',
        'PC_CLUES': '14:10',
        'PC_ALLWARPS': '15',
        'PC_UPGRADES': '16',
        'PC_DEATHS': '17:10',
        'PC_AREAS': '18:29',
        'PC_SCARES': '19:50',
        'PC_CUTSCENES': '20:14',
    })
    launcher.wait_event('all_cutscenes', seconds)  # the last switch to run
    time.sleep(0.5)  # let anything queued arrive
    stop(proc)
    types = launcher.kinds()
    events = launcher.events()
    casual = [m.get('reason') for m in launcher.messages if m.get('t') == 'casual']
    launcher.close()
    text = log_since(log, offset) if log else ''

    print('messages: %s' % ', '.join(str(t) for t in types))
    print('events:   %s' % (', '.join(events) or '(none)'))
    print('casual:   %s' % (', '.join(casual) or '(none)'))
    ok = (launcher.connected and types[:1] == ['hello'] and 'list_ok' in types
          and 'new_game' in events)
    if not ok:
        print('FAIL: the launcher did not see hello, list_ok and the new_game event')
    # The counter watchers: a snack collected at 12 s must report first_snack, the
    # 21 tokens at 13 s must report tokens_half and tokens_all (and first_token),
    # 10 clues at 14 s must report first_clue and clues_10, all 26 gates opening
    # at 15 s must report warps_all and first_warp (the gates opened are more than
    # the hub's lone one), the 15 upgrades at 16 s must report first_upgrade (more
    # than the five SD2.INI starts with) and upgrades_all, ten deaths at 17 s must
    # report deaths_10, finishing all 29 areas at 18 s must report the snack
    # totals and all three area milestones, fifty scares at 19 s must report
    # scares_50, and all 14 cutscenes at 20 s must report all_cutscenes.
    for wanted in ('first_snack', 'snacks_100', 'snacks_1000', 'first_token',
                   'tokens_half', 'tokens_all', 'first_clue', 'clues_10',
                   'warps_all', 'first_warp', 'first_upgrade', 'upgrades_all',
                   'deaths_10', 'area_perfect_1', 'areas_perfect_10',
                   'areas_perfect_all', 'scares_50', 'all_cutscenes'):
        if wanted not in events:
            print('FAIL: the counters moved but the watcher did not report %s' % wanted)
            ok = False
    # This run starts with PC_SCENE, a cheat switch: the session must be marked
    # casual as well, so the wiring for Ranked is exercised too.
    if not casual:
        print('FAIL: PC_SCENE is a cheat switch but the run was not marked casual')
        ok = False
    if log and '[ach] connected to RecompLauncher' not in text:
        print('FAIL: the game did not log a connection')
        ok = False
    return ok


def run_casual_switch(exe, ini, seconds):
    """A give switch is a cheat, so the session must be marked casual.

    Every other run starts with PC_SCENE, which the switch scan reports first and
    stops at, so this one leaves PC_SCENE out: the reason must name PC_CLUES. The
    report happens on the first frame, so the run need not reach gameplay.
    """
    launcher = Launcher()
    print('run: %s, PC_CLUES without PC_SCENE' % exe.name)
    proc = start(exe, ini, {'RL_SESSION': launcher.name, 'PC_CLUES': '600:1'}, scene=False)
    launcher.wait_for('casual', min(seconds, 30))
    stop(proc)
    reasons = [m.get('reason') for m in launcher.messages if m.get('t') == 'casual']
    launcher.close()
    print('casual:   %s' % (', '.join(reasons) or '(none)'))
    if 'PC_CLUES' in reasons:
        return True
    print('FAIL: PC_CLUES gives progress but the run was not marked casual for it')
    return False


def run_mastermind(exe, ini, seconds):
    """The Mastermind is beaten through the game's own call.

    It gets a run of its own because zVarDefeatMasterMind also sets the endgame
    flag, which the menus and credits read: the counter run keeps ordinary play.
    No PC_SCENE here, so the casual report must name PC_MASTERMIND itself (the
    switch hands over the fight), and the new game starts from the title screen.
    """
    launcher = Launcher()
    log = log_path()
    offset = log_size(log) if log else 0
    print('run: %s, PC_MASTERMIND without PC_SCENE' % exe.name)
    proc = start(exe, ini, {'RL_SESSION': launcher.name, 'PC_MASTERMIND': '20'}, scene=False)
    beat = launcher.wait_event('beat_mastermind', seconds)
    time.sleep(0.5)
    stop(proc)
    events = launcher.events()
    casual = [m.get('reason') for m in launcher.messages if m.get('t') == 'casual']
    launcher.close()
    text = log_since(log, offset) if log else ''
    print('events:   %s' % (', '.join(events) or '(none)'))
    print('casual:   %s' % (', '.join(casual) or '(none)'))
    ok = beat
    if not beat:
        print('FAIL: the Mastermind was beaten but the watcher did not report beat_mastermind')
    if 'PC_MASTERMIND' not in casual:
        print('FAIL: PC_MASTERMIND beats the Mastermind but the run was not marked casual for it')
        ok = False
    if log and '[debug] 1 Mastermind beaten' not in text:
        print('FAIL: the switch never ran (no "Mastermind beaten" line in the log)')
        ok = False
    return ok


def run_save_load(exe, ini, seconds):
    """A real save is written and read back, and the launcher hears about it.

    PC_SAVELOAD calls the save and load screens' own functions
    (zSaveLoad_SaveGame/zSaveLoad_LoadGame) with slot 0, so the notification
    comes from a file on disk rather than from the menu around it. The card
    folder is a scratch one of its own (PC_SAVE_DIR), emptied first, and the
    snacks given before the save are what the file holds.
    """
    name = 'achievements-save'
    card = BUILD / name / 'cardA'
    shutil.rmtree(BUILD / name, ignore_errors=True)
    launcher = Launcher()
    log = log_path()
    offset = log_size(log) if log else 0
    print('run: %s, PC_SAVELOAD with PC_SAVE_DIR=%s' % (exe.name, name))
    proc = start(exe, ini, {'RL_SESSION': launcher.name, 'PC_SAVE_DIR': name,
                            'PC_SNACKS': '12:3', 'PC_SAVELOAD': '15'})
    loaded = launcher.wait_for('save_loaded', seconds)
    time.sleep(0.5)
    stop(proc)
    types = launcher.kinds()
    events = launcher.events()
    launcher.close()
    text = log_since(log, offset) if log else ''
    saves = sorted(card.glob('*.dat')) if card.is_dir() else []
    print('messages: %s' % ', '.join(str(t) for t in types))
    print('events:   %s' % (', '.join(events) or '(none)'))
    print('card:     %s' % (', '.join('%s %d bytes' % (p.name, p.stat().st_size) for p in saves)
                            or '(no files)'))
    ok = loaded
    if not loaded:
        print('FAIL: the game saved and loaded but the launcher did not see save_loaded')
    if types.count('save_loaded') != 1:
        print('FAIL: save_loaded arrived %d times (one save, one load)' % types.count('save_loaded'))
        ok = False
    if events.count('new_game') != 1:
        print('FAIL: the reload reported new_game %d times (a load is not a new game)'
              % events.count('new_game'))
        ok = False
    if log:
        if '[debug] 1 save written and read back' not in text:
            print('FAIL: PC_SAVELOAD never ran (no "save written and read back" line)')
            ok = False
        if '[ach] save loaded' not in text:
            print('FAIL: the game did not log the load')
            ok = False
    if not saves:
        print('FAIL: PC_SAVELOAD left no card file in %s' % card)
        ok = False
    elif b'I001' not in saves[0].read_bytes():
        print('FAIL: the save file does not hold the scene it was written in')
        ok = False
    shutil.rmtree(BUILD / name, ignore_errors=True)
    return ok


def run_save_screen_load(exe, ini, seconds):
    """Saving and loading the way a player does them: with the pad.

    The save screen has no menu entry: it opens at a Mystery Machine save point,
    which fires the game's own save event (the SAVEPOINT pickup). Each save
    starts a new game in the hub, collects its snacks, moves Scooby onto the
    hub's save point, and the pad picks slot 0. The first save lands on an empty
    slot; the second one saves over it, which brings up the overwrite prompt
    (it starts on NO, so the pad goes down to YES and confirms). A last process
    starts at the title, chooses Load Game with the pad and picks slot 0; the
    level that comes back must hold the snacks the second save was written
    with. That is the claim pc/docs/achievements.md makes about the reload,
    checked on the loaded game rather than only on the file.
    """
    name = 'achievements-pad'
    card = BUILD / name / 'cardA'
    save = card / 'ScoobyDoo00.dat'
    shutil.rmtree(BUILD / name, ignore_errors=True)
    log = log_path()

    def save_at_point(snacks, pad, wait):
        """Play, collect `snacks`, touch the save point and work the pad."""
        offset = log_size(log) if log else 0
        before = save.read_bytes() if save.is_file() else None
        # Start is pressed at 2 s and New Game (A) at 3 s, as every run does;
        # the snacks arrive at 12 s and the save point at 14 s opens the screen.
        print('run: %s, save point with %d snacks' % (exe.name, snacks))
        proc = start(exe, ini, {'PC_SAVE_DIR': name, 'PC_SNACKS': '12:%d' % snacks,
                                'PC_SCENE': HUB, 'PC_TELEPORT': HUB_SAVEPOINT}, scene=False,
                     autopad='2:1000,3:0100,4:0000,' + pad)
        deadline = time.time() + wait
        while time.time() < deadline:
            if save.is_file() and save.read_bytes() != before:
                break
            time.sleep(0.2)
        time.sleep(0.5)
        stop(proc)
        return save.is_file() and save.read_bytes() != before

    # Phase 1: an empty card, so A on slot 0 saves it.
    saved = save_at_point(3, '16:0100,16.15:0000', seconds)
    if not saved:
        print('FAIL: the save point opened the save screen but the pad wrote no card file')
        shutil.rmtree(BUILD / name, ignore_errors=True)
        return False
    size = save.stat().st_size
    print('card:     %s %d bytes' % (save.name, size))
    ok = size == 40960 and HUB.encode() in save.read_bytes()
    if not ok:
        print('FAIL: the card file is not the 40 KiB save of the hub it was written in')

    # Phase 2: the same slot again. Its overwrite prompt starts on NO, so Down
    # moves to YES and A confirms it.
    overwritten = save_at_point(5, '16:0100,16.15:0000,19:0004,19.15:0000,21:0100,21.15:0000',
                                seconds)
    if not overwritten:
        print('FAIL: slot 0 kept its old save: the overwrite prompt was not confirmed')
        ok = False

    # Phase 3: title -> Load Game (Start, one Down, A) -> the load screen's slot
    # 0 (A). Both screens start on slot 0, and only a load logs the line below, so
    # it also says the pad chose Load Game rather than New Game.
    offset = log_size(log) if log else 0
    print('run: %s, Load Game from the title with the pad' % exe.name)
    proc = start(exe, ini, {'PC_SAVE_DIR': name}, scene=False,
                 autopad='6:1000,6.15:0000,8:0004,8.15:0000,10:0100,10.15:0000,18:0100,18.15:0000')
    loaded = '[debug] loaded %s: 5 snacks' % HUB
    text = ''
    deadline = time.time() + seconds
    while log and time.time() < deadline and loaded not in text:
        text = log_since(log, offset)
        time.sleep(0.2)
    stop(proc)
    if not log:
        print('note: no %APPDATA%, so the game log was not checked')
    elif loaded not in text:
        print('FAIL: the loaded game did not carry the snacks the second save held')
        ok = False
    shutil.rmtree(BUILD / name, ignore_errors=True)
    return ok


def run_without_launcher(exe, ini, seconds, session):
    log = log_path()
    offset = log_size(log) if log else 0
    label = 'no RL_SESSION' if session is None else 'RL_SESSION=%s (nobody there)' % session
    print('run: %s, %s' % (exe.name, label))
    extra = {} if session is None else {'RL_SESSION': session}
    proc = start(exe, ini, extra)
    deadline = time.time() + seconds
    while time.time() < deadline and proc.poll() is None:
        time.sleep(0.2)
    alive = proc.poll() is None
    stop(proc)
    text = log_since(log, offset) if log else ''
    ok = alive
    if not alive:
        print('FAIL: the game exited by itself (code %s)' % proc.returncode)
    if log:
        if '[ach] no RecompLauncher' not in text:
            print('FAIL: no "no RecompLauncher" line in the log')
            ok = False
        if '[ach] connected' in text:
            print('FAIL: the game thought it had a launcher')
            ok = False
    else:
        print('note: no %APPDATA%, so the game log was not checked')
    return ok


def main():
    args = [a for a in sys.argv[1:]]
    seconds = 120
    if '--seconds' in args:
        i = args.index('--seconds')
        seconds = int(args[i + 1])
        del args[i:i + 2]
    exe = executable(args[0] if args else None)
    ini = write_ini()

    results = []
    results.append(('with a launcher', run_with_launcher(exe, ini, seconds)))
    results.append(('give switch casual', run_casual_switch(exe, ini, seconds)))
    results.append(('beat the Mastermind', run_mastermind(exe, ini, seconds)))
    results.append(('save and reload', run_save_load(exe, ini, seconds)))
    results.append(('save screen and load', run_save_screen_load(exe, ini, seconds)))
    results.append(('without a launcher', run_without_launcher(
        exe, ini, 20, None)))
    results.append(('launcher gone', run_without_launcher(
        exe, ini, 20, 'scooby-ach-missing-%s' % uuid.uuid4().hex[:8])))

    print()
    for name, ok in results:
        print('%-18s %s' % (name, 'PASS' if ok else 'FAIL'))
    raise SystemExit(0 if all(ok for _, ok in results) else 1)


if __name__ == '__main__':
    main()
