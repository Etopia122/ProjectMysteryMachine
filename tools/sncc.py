#!/usr/bin/env python3

###
# Runs the SN Systems ProDG compiler (ngccc, GCC 2.95 based) used for the
# RAD Game Tools libraries (Bink). ngccc needs SN_NGC_PATH to point at the
# directory holding its sn.ini, so this wrapper sets it.
#
# Usage:
#   python tools/sncc.py <compilers dir> <ProDG/version> [ngccc args ...]
###

import os
import subprocess
import sys


def main() -> None:
    compilers, version = sys.argv[1], sys.argv[2]
    cdir = os.path.abspath(os.path.join(compilers, version))
    env = dict(os.environ, SN_NGC_PATH=cdir)
    cmd = [os.path.join(cdir, "ngccc.exe")]
    if os.name != "nt":
        cmd.insert(0, "wine")
    result = subprocess.run(cmd + sys.argv[3:], env=env)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
