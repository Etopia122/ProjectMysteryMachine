# Project instructions

## Releases

`python pc/tools/package.py` prints the archive's SHA-256 and writes
`build/dist/ProjectMysteryMachine.zip.sha256`. Every release needs:

1. that `.sha256` uploaded as a release asset beside the zip;
2. a **Verifying the download** block in the release body with the hash and
   byte size (see the v0.3.0 release body);
3. a matching bullet in the `Releases` section of `README.md`.

The checksum is how downloaders verify the archive: the MinGW builds draw
generic antivirus flags (see `docs/false-positive-reports.md`).

## Signing

Releases sign `scooby_launcher.exe` and `libwinpthread-1.dll` with Authenticode
before the archive is built. `python pc/tools/package.py --sign` needs either
`PC_SIGN_SHA1` (a certificate thumbprint already in the Windows store) or
`PC_SIGN_PFX` (a PFX file, with `PC_SIGN_PASSWORD` if it has one); it timestamps
with DigiCert and verifies both signatures. Never substitute a generated
self-signed certificate for a release: it is not a trusted publisher.
