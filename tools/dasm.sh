#!/bin/sh
# Dumps one function's asm with its .L_ labels kept.
# usage: tools/dasm.sh <asm file> <exact symbol name>
awk -v f="$2" '$1==".fn" && ($2==f"," || $2==f){p=1} p{print} p&&$1==".endfn"{exit}' "$1" | grep -v "^$\|^# " | sed -E 's/^\/\* [0-9A-F]+ [0-9A-F]+  [0-9A-F ]+ \*\/\t//'
