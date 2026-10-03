#!/bin/sh
# usage: dasm.sh <asm file> <fn name prefix>
awk -v f="$2" 'index($0, ".fn " f)==1{p=1} p{print} p&&index($0, ".endfn " f)==1{exit}' "$1" | grep -v "^$\|^# " | sed -E 's/^\/\* [0-9A-F]+ [0-9A-F]+  [0-9A-F ]+ \*\/\t//'
