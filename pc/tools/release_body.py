#!/usr/bin/env python3
# Assemble the body of a PC release: the checked-in notes
# (docs/releases/<tag>.md) followed by the "Verifying the download" block, with
# the archive's real byte size and SHA-256.
#
# The block is generated rather than written by hand: the archive is not
# reproducible, so its hash only exists once it has been built and is the one
# GitHub serves. When ARCHIVE.sha256 sits beside ARCHIVE (package.py writes it),
# the two are checked against each other first.
#
# usage: python pc/tools/release_body.py NOTES ARCHIVE OUT
import hashlib, sys
from pathlib import Path

USAGE = 'usage: python pc/tools/release_body.py NOTES ARCHIVE OUT'


def sha256(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


args = [Path(a) for a in sys.argv[1:]]
if len(args) != 3:
    raise SystemExit(USAGE)
notes, archive, out = args
if not notes.is_file():
    raise SystemExit('%s is missing: every release needs its notes checked in' % notes)
if not archive.is_file():
    raise SystemExit('%s is missing: package the release first' % archive)

digest = sha256(archive)
sidecar = Path(str(archive) + '.sha256')
if sidecar.is_file():
    recorded = sidecar.read_text(encoding='ascii').split()[0]
    if recorded != digest:
        raise SystemExit('%s does not match %s' % (sidecar, archive.name))

body = notes.read_text(encoding='utf-8').rstrip() + '\n'
body += (
    '\n**Verifying the download**\n\n'
    '`%s` is %s bytes with SHA-256\n'
    '`%s`. Compare\nyour download before running it:\n\n'
    '```powershell\n'
    'Get-FileHash .\\%s -Algorithm SHA256\n'
    '```\n\n'
    'The same value is attached to this release as\n'
    '`%s.sha256`.\n'
) % (archive.name, format(archive.stat().st_size, ','), digest, archive.name, archive.name)
out.write_text(body, encoding='utf-8', newline='\n')
print('%s: %s, %s bytes, SHA-256 %s' % (out, archive.name,
      format(archive.stat().st_size, ','), digest))
