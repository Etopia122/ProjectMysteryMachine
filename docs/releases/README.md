# Release notes

One file per PC release, named exactly after its tag: `vX.Y.Z.md`. Pushing that
tag builds the port in `.github/workflows/pc-release.yml`, which posts the file
as the body of a **draft** GitHub release.

Keep to what the published bodies do:

- the version headline ("**Project: Mystery Machine vX.Y.Z**");
- what the release is and the "**This release contains no game data.**" line;
- what's new since the previous tag;
- how to play, and what is in the archive;
- anything specific to that build (for example the VirusTotal paragraph in
  v0.3.0).

**Do not write a checksum.** The workflow appends a **Verifying the download**
block with the archive's real byte size and SHA-256 via
`pc/tools/release_body.py`: only the CI-built hash is the one GitHub serves, and
it does not exist until the build has run.

The workflow fails if the file for the pushed tag is missing, so land it before
the tag. Earlier bodies are on the
[releases page](https://github.com/Etopia122/ProjectMysteryMachine/releases).
