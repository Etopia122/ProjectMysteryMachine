#!/usr/bin/env python3

###
# Rejects game data in the repository. Both builds read the game's files from
# the player's own dump, so nothing from the disc may be committed: an ISO is
# both a legal problem and a blob that would have to be purged from history by
# hand. Checks the tracked tree, and with --history every blob reachable from
# any ref.
#
# Usage:
#   python tools/check_no_game_assets.py [--history] [--max-size MIB]
###

import argparse
import re
import subprocess
import sys

DISC_SUFFIXES = (
    '.iso', '.gcm', '.rvz', '.gcz', '.wia', '.ciso', '.nkit', '.chd',
    '.cue', '.bin', '.img', '.mdf', '.mds', '.nrg',
)
ROM_SUFFIXES = ('.dol', '.rel', '.elf', '.3dsx', '.cia', '.ncsd', '.cci', '.cxi')
ASSET_SUFFIXES = ('.hip', '.bik', '.aw', '.txd', '.dsp', '.bnr', '.thp', '.vag')
ARCHIVE_SUFFIXES = ('.zip', '.7z', '.rar', '.tar', '.gz', '.xz', '.zst')

SUFFIX_LABELS = {}
for _suffix in DISC_SUFFIXES:
    SUFFIX_LABELS[_suffix] = 'disc image'
for _suffix in ROM_SUFFIXES:
    SUFFIX_LABELS[_suffix] = 'executable or ROM'
for _suffix in ASSET_SUFFIXES:
    SUFFIX_LABELS[_suffix] = 'extracted game asset'
for _suffix in ARCHIVE_SUFFIXES:
    SUFFIX_LABELS[_suffix] = 'archive'

# Disc IDs such as GIHE78 (USA) / GIHP78 (PAL), and the files an extracted dump
# leaves behind.
DISC_ID = re.compile(r'^gih[a-z0-9]{3}(?:\.|$)')
GAME_NAMES = {'main.dol', 'boot.dol', 'start.dol', 'boot.bin', 'apploader.img', 'banner.bnr'}
GAME_DIRS = {'game', 'orig', 'ref', 'assets/raw'}

DEFAULT_MAX_SIZE = 4.0  # MiB


def git(*args, stdin=None):
    try:
        proc = subprocess.run(('git',) + args, input=stdin, capture_output=True)
    except FileNotFoundError:
        raise SystemExit('git is not on PATH')
    if proc.returncode:
        raise SystemExit('git %s failed: %s'
                         % (' '.join(args), proc.stderr.decode('utf-8', 'replace').strip()))
    return proc.stdout


def blob_sizes(shas):
    """Map blob sha -> (type, size) using one git cat-file --batch-check call."""
    if not shas:
        return {}
    query = ''.join(sha + '\n' for sha in shas).encode()
    out = git('cat-file', '--batch-check=%(objectname) %(objecttype) %(objectsize)', stdin=query)
    sizes = {}
    for line in out.decode('utf-8', 'replace').splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2].isdigit():
            sizes[fields[0]] = (fields[1], int(fields[2]))
    return sizes


def tracked_blobs():
    out = git('ls-files', '-s', '-z')
    blobs = {}
    for entry in out.split(b'\0'):
        if not entry:
            continue
        meta, path = entry.split(b'\t', 1)
        fields = meta.split()
        if len(fields) < 2:
            continue
        blobs[path.decode('utf-8', 'replace')] = fields[1].decode()
    return blobs


def history_objects():
    out = git('rev-list', '--objects', '--all')
    objects = {}
    for line in out.decode('utf-8', 'replace').splitlines():
        fields = line.split(' ', 1)
        path = fields[1] if len(fields) > 1 else ''
        if path:
            objects.setdefault(fields[0], path)
    return objects


def problems(path, size, max_size):
    lowered = path.lower()
    parts = lowered.split('/')
    base = parts[-1]
    reasons = []
    # Placeholders such as orig/GIHE78/.gitkeep are expected to live in the
    # ignored reference folders; the files themselves are still judged by name.
    if not base.startswith('.'):
        for suffix, label in SUFFIX_LABELS.items():
            if lowered.endswith(suffix):
                reasons.append(label)
                break
        if DISC_ID.match(base):
            reasons.append('disc image')
        if base in GAME_NAMES:
            reasons.append('game file')
        if parts[0] in GAME_DIRS or '/'.join(parts[:2]) in GAME_DIRS:
            reasons.append('game folder')
    if size > max_size:
        reasons.append('%.1f MiB blob, over the %.1f MiB cap'
                       % (size / (1 << 20), max_size / (1 << 20)))
    return reasons


def main() -> None:
    parser = argparse.ArgumentParser(description='Reject game data in the repository.')
    parser.add_argument('--history', action='store_true',
                        help='also scan every blob reachable from any ref')
    parser.add_argument('--max-size', type=float, default=DEFAULT_MAX_SIZE, metavar='MIB',
                        help='reject larger blobs (default: %(default)s)')
    args = parser.parse_args()
    max_size = int(args.max_size * (1 << 20))

    head = tracked_blobs()
    head_sizes = blob_sizes(sorted(set(head.values())))
    found = {}
    largest = ('', 0)

    for path, sha in head.items():
        size = head_sizes.get(sha, ('', 0))[1]
        if size > largest[1]:
            largest = (path, size)
        found[(path, sha)] = problems(path, size, max_size)

    scanned = len(head)
    if args.history:
        objects = history_objects()
        sizes = blob_sizes(sorted(objects))
        for sha, path in objects.items():
            kind, size = sizes.get(sha, ('', 0))
            if kind != 'blob' or (path, sha) in found:
                continue
            scanned += 1
            found[(path, sha)] = problems(path, size, max_size)

    bad = sorted((path, sha, reasons) for (path, sha), reasons in found.items() if reasons)
    if bad:
        for path, sha, reasons in bad:
            where = '' if head.get(path) == sha else ' [history]'
            print('%s: %s%s' % (path, ', '.join(reasons), where))
        print('\n%d problem(s); game data must never be committed. Remove the file'
              ' and keep it out of the index.' % len(bad))
        raise SystemExit(1)

    print('checked %d file(s), largest %s (%.1f KiB), cap %.1f MiB: no game data'
          % (scanned, largest[0] or '-', largest[1] / 1024, args.max_size))


if __name__ == '__main__':
    main()
