# VirusTotal false-positive reports — v0.3.0

Pinned to the v0.3.0 archive; a later release needs new hashes and fresh
VirusTotal results. Nothing here is sent automatically — copy a section into the
vendor's form or email. Links and forms were checked against vendor or
VirusTotal documentation on 2026-11-24; no mail was sent, so the addresses
themselves are not delivery-tested.

VirusTotal report:
<https://www.virustotal.com/gui/file/c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752/detection>

| Engine | Detection | File | Channel source | Sent |
|---|---|---|---|---|
| AliCloud | `Trojan:Win/Babar.Gen` | `libwinpthread-1.dll` | VT list only | no |
| Gridinsoft | `Trojan.Win32.Gen.cl` | `libwinpthread-1.dll` | Vendor form (+ VT email) | no |
| Trend Micro (also HouseCall) | `Trojan.Win32.BABAR.TL0101FI26ZW` | the ZIP | VT list only (vendor form 404s) | no |
| Sophos | `Mal/Generic-S` | the ZIP | Vendor KB (email) | no |
| MaxSecure | `Trojan.Malware.691063983.susgen` | the ZIP | Vendor doc + VT list | no |
| SecureAge | `Malicious` | `scooby_launcher.exe` | Vendor form | no |

**Channel source:** `Vendor` means the company publishes that route itself
(form, knowledge-base article or its own documentation). `VT list` means the
address appears only in VirusTotal's false-positive contacts table and the
vendor publishes no working form; those are the weakest contacts.

## Shared facts

- **Project: Mystery Machine**, an open-source Windows PC port of
  *Scooby-Doo! Night of 100 Frights*: tag `v0.3.0`, commit
  `c50704ea6a7616c8f5ef22051a3a0b6bbdd621d0`, built locally with MSYS2 MINGW32
  by `pc/tools/package.py`.
- Archive `ProjectMysteryMachine.zip`, 3,666,051 bytes, SHA-256
  `c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752`: 11 files,
  no game data.
- Both executables are unsigned MinGW-w64 i686 builds. `strip --strip-debug`
  leaves the COFF symbol table after the last PE section, which VirusTotal shows
  as an `overlay`.
- Imports are `KERNEL32`, `USER32`, `GDI32`, `SHELL32`, `COMCTL32`, `COMDLG32`,
  `ole32`, `dwmapi`, `msimg32`, `msvcrt`, `SDL2` and `libwinpthread-1`; no
  WinINet/WinHTTP/URLMon and no sockets. The sandbox reports **Network comms:
  NOT FOUND**.
- The rundll32, T1055 and T1574 entries in the CAPE report come from the
  sandbox loading `libwinpthread-1.dll`'s exports through rundll32.
- Per-file results: `scooby.exe` 0/70, `SDL2.dll` 0/70,
  `libwinpthread-1.dll` 2/54, `scooby_launcher.exe` 1/70.

## AliCloud — `Trojan:Win/Babar.Gen` on `libwinpthread-1.dll`

Submit: antivirus@alibabacloud.com, the address in [VirusTotal's false-positive
contacts](https://docs.virustotal.com/docs/false-positive-contacts). Alibaba Cloud
publishes no dedicated false-positive form, so if the mail bounces, open a
support ticket at <https://www.alibabacloud.com/support> with the same text.

Subject: False positive - Trojan:Win/Babar.Gen - libwinpthread-1.dll, SHA-256 1096bf215c08ac858ab2c03e0712525c0db1e02f524c54e7f7b7295faa51287f

```
Hello,

AliCloud reports Trojan:Win/Babar.Gen for libwinpthread-1.dll, SHA-256
1096bf215c08ac858ab2c03e0712525c0db1e02f524c54e7f7b7295faa51287f (82,783
bytes), which we ship unmodified in an open-source Windows game port.

The DLL identifies itself as "POSIX WinThreads for Windows", "Copyright (C)
MinGW-W64 Project Members 2010-2023", version 1.0.0.0, compiled 2026-06-10. It
imports only KERNEL32.dll and msvcrt.dll and exports the standard pthread API.
pc/tools/package.py copies it from the MSYS2 mingw32 toolchain, so it is the
same runtime that accompanies many unrelated MinGW builds.

It is bundled in ProjectMysteryMachine.zip (SHA-256
c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752, 3,666,051
bytes):
https://github.com/Etopia122/ProjectMysteryMachine/releases/tag/v0.3.0

VirusTotal:
https://www.virustotal.com/gui/file/1096bf215c08ac858ab2c03e0712525c0db1e02f524c54e7f7b7295faa51287f

Please review the hash and remove the detection, or tell us which code path
triggers it. Thank you.
```

## Gridinsoft — `Trojan.Win32.Gen.cl` on `libwinpthread-1.dll`

Submit: <https://gridinsoft.com/incorrect-detection> (file/app detection,
False-Positive, 50 MB max), or virus@gridinsoft.com; a support request at
<https://support.gridinsoft.com/portal/en/newticket> is the fallback. Upload the
DLL itself; it is under the limit.

```
Gridinsoft flags libwinpthread-1.dll as Trojan.Win32.Gen.cl. It is the stock
MinGW-w64 winpthreads runtime, not our code:

SHA-256 1096bf215c08ac858ab2c03e0712525c0db1e02f524c54e7f7b7295faa51287f
Size    82,783 bytes
Version POSIX WinThreads for Windows, Copyright (C) MinGW-W64 Project Members
        2010-2023, 1.0.0.0, compiled 2026-06-10
Imports KERNEL32.dll, msvcrt.dll only

Bundle: ProjectMysteryMachine.zip from
https://github.com/Etopia122/ProjectMysteryMachine/releases/tag/v0.3.0
(archive SHA-256 c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752)

VirusTotal: https://www.virustotal.com/gui/file/1096bf215c08ac858ab2c03e0712525c0db1e02f524c54e7f7b7295faa51287f

Please reclassify the hash or say what triggers the generic detection.
```

## Trend Micro / HouseCall — `Trojan.Win32.BABAR.TL0101FI26ZW` on the archive

Submit: the official [False Detection and Re-Classify
Requests](https://www.trendmicro.com/en_us/about/legal/detection-reevaluation.html)
page is the non-customer route, but the File Re-classify Request form it links to
returned 404 when checked on 2026-11-24. Use virus_doctor@trendmicro.com
(VirusTotal-listed) with this text. Trend customers go through the Business
Success Portal instead (New Case → Threat Issue → File False Positive,
KA-0001177).

```
Product: Trend Micro (VirusTotal engines "TrendMicro" and "TrendMicro-HouseCall")
Detection: Trojan.Win32.BABAR.TL0101FI26ZW
File: ProjectMysteryMachine.zip
SHA-256: c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752
Size: 3,666,051 bytes
Source: https://github.com/Etopia122/ProjectMysteryMachine/releases/tag/v0.3.0
VirusTotal: https://www.virustotal.com/gui/file/c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752

The archive is an open-source, asset-free Windows port of a 2002 GameCube game:
scooby.exe (0/70), scooby_launcher.exe (1/70), SDL2.dll (0/70),
libwinpthread-1.dll (2/54), a JSON descriptor and licence texts. There is no
network code and the sandbox reports no network communication; the launcher
copies files from the player's own disc image.

"Babar" matches the generic "Babar.Gen" label AliCloud applies to the bundled
MinGW-w64 winpthreads runtime, so we believe the name is inherited from that
DLL rather than a family match.

Please review the archive and remove the detection.
```

## Sophos — `Mal/Generic-S` on the archive

Submit: email samples@sophos.com with the zip in a password-protected archive
using the password `infected` — the documented non-customer route
(KBA-000048894); no feedback is promised. Intelix (<https://intelix.sophos.com/>)
is the supported analysis portal, but its "Disagree" escalation only works for
validated Support Portal profiles, so it cannot clear a detection for a
non-customer.

```
Sophos reports Mal/Generic-S for ProjectMysteryMachine.zip, SHA-256
c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752 (3,666,051
bytes):
https://github.com/Etopia122/ProjectMysteryMachine/releases/tag/v0.3.0

It is an unsigned MSYS2/MinGW-w64 build of an open-source Windows game port.
The archive holds scooby.exe, scooby_launcher.exe (both i686 GUI programs),
SDL2.dll, libwinpthread-1.dll and licence files; no game data. VirusTotal has
scooby.exe at 0/70 and SDL2.dll at 0/70, and the binaries import no
WinINet/WinHTTP/URLMon. The PE files carry a MinGW COFF symbol table after
their last section, which scanners report as an appended overlay.

Please re-analyse the sample and clear the generic detection, or say which
component triggers it.
```

## MaxSecure — `Trojan.Malware.691063983.susgen` on the archive

Submit: tech@maxpcsecure.com (in [VirusTotal's
list](https://docs.virustotal.com/docs/false-positive-contacts) and in Max
Secure's own administrator guide), or the contact form at
<https://www.maxpcsecure.com/contact.htm>.
The old `maxsecureantivirus.com/submit_aFalse_Positive.htm` URL now just serves
the homepage; there is no public false-positive form, so attach the zip or
include the links in the message.

```
Detection: Trojan.Malware.691063983.susgen ("susgen" = suspicious, generic)
File: ProjectMysteryMachine.zip
SHA-256: c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752
Size: 3,666,051 bytes (11 files, 10.5 MB unpacked)
Download: https://github.com/Etopia122/ProjectMysteryMachine/releases/tag/v0.3.0
VirusTotal: https://www.virustotal.com/gui/file/c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752

Contents: scooby.exe and scooby_launcher.exe (unsigned MinGW-w64 i686 builds;
the launcher extracts files from the user's own game disc and starts the game),
SDL2.dll, libwinpthread-1.dll (stock MinGW-w64 runtime), recomplauncher.json,
README.txt and five licence texts. The source is public at the URL above.

No component imports WinINet/WinHTTP/URLMon or uses sockets, and the sandbox
reports no network communication. The executables are unsigned and carry a
MinGW symbol table after the last section, which is likely what the generic
heuristic reacts to.

Please review the hash and clear the detection.
```

## SecureAge — `Malicious` on `scooby_launcher.exe`

Submit: <https://uav.secureage.com/falsepositive> (SecureAge's form: 30 MB max,
email and description required), or the "Submit false positive" option at
<https://www.secureage.com/contact-us>.

```
SecureAge reports "Malicious" (no family name) for scooby_launcher.exe,
SHA-256 3ab1ef47da4bbdf659efc7c0469b8791b1e3a37143b3b27d977fc099473a69ce,
1,386,366 bytes.

It is the unsigned 32-bit MinGW-w64 launcher of an open-source Windows port of
Scooby-Doo! Night of 100 Frights. It shows a window, copies the game's files
from the player's own disc image, writes scooby.ini and starts scooby.exe
(CreateProcessW). Version resource: "Project: Mystery Machine - Scooby-Doo!
Night of 100 Frights Launcher", 0.3.0.0. Imports are the normal Win32 set
(KERNEL32, USER32, GDI32, SHELL32, COMCTL32, COMDLG32, ole32, dwmapi, msimg32,
msvcrt, SDL2, libwinpthread-1), with no WinINet/WinHTTP/URLMon.

VirusTotal (SecureAge is the only detection, 1/70):
https://www.virustotal.com/gui/file/3ab1ef47da4bbdf659efc7c0469b8791b1e3a37143b3b27d977fc099473a69ce

It ships in ProjectMysteryMachine.zip (SHA-256
c410f5c54b0b9c77a64e7e1f934b3823de431829119a0b78625526ad697dd752):
https://github.com/Etopia122/ProjectMysteryMachine/releases/tag/v0.3.0

Please review the file and clear the verdict.
```

After a vendor replies, use **Reanalyze** on the VirusTotal report: a cleared
detection only disappears after a fresh analysis.
