# Project instructions

## Releases

The PC release is built by `.github/workflows/pc-release.yml`: pushing a `v*`
tag builds the port with the MSYS2 mingw32 toolchain, packages it with
`python pc/tools/package.py --no-captures`, verifies the archive and opens a
**draft** release with `ProjectMysteryMachine.zip` and
`ProjectMysteryMachine.zip.sha256`. Nothing is public until the draft is
reviewed and published by hand. `workflow_dispatch` runs the build and
verification without creating a release.

Every release needs:

1. `docs/releases/vX.Y.Z.md`, the release notes, checked in **before** the tag
   is pushed (CI fails without it). It holds the prose only: the workflow
   appends a **Verifying the download** block with the archive's real hash and
   byte size, so never write a checksum there.
2. `pc/launcher/launcher.rc` bumped to the same version: CI refuses a tag whose
   version does not match `FILEVERSION` (`v0.3.0` ↔ `0,3,0,0`).
3. a matching bullet in the `Releases` section of `README.md`, with the hash
   from the draft body. The hash only exists once CI has built the archive, so
   this is a follow-up commit. The archive is reproducible from the same tag and
   the same MSYS2 toolchain: the programs carry no build timestamp and
   `package.py` writes fixed zip entries, so a rebuild reproduces the published
   hash. A `workflow_dispatch` run builds it twice (`build/pc-repro`) and fails
   if the two archives differ.

The capture check (`pc/tools/captures.py`) needs the extracted disc and a GPU,
so CI always passes `--no-captures`: run the visual check locally before
tagging.

`python pc/tools/package.py` still builds and packages the archive by hand (it
prints the SHA-256 and writes the `.sha256` beside it), which is how a local
test build is checked.

The checksum is how downloaders verify the archive: the unsigned MinGW builds
draw generic antivirus flags (see `docs/false-positive-reports.md`).
