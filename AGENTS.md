# Project instructions

## Releases

`python pc/tools/package.py` prints the archive's SHA-256 and writes
`build/dist/ProjectMysteryMachine.zip.sha256`. Every release needs:

1. that `.sha256` uploaded as a release asset beside the zip;
2. a **Verifying the download** block in the release body with the hash and
   byte size (see the v0.3.0 release body);
3. a matching bullet in the `Releases` section of `README.md`.

The checksum is how downloaders verify the archive: the unsigned MinGW builds
draw generic antivirus flags (see `docs/false-positive-reports.md`).
