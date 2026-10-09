#!/bin/sh
# Runs a .3dsx in Azahar under gdb, interrupts it after <seconds> and prints
# every thread's backtrace (Azahar's GDB stub must be enabled: use_gdbstub=true).
# usage: sh n3ds/tools/stall.sh <app.elf> <app.3dsx> <seconds>
AZAHAR=${AZAHAR:-/c/Users/Ethan/Azahar/azahar-windows-msvc-2126.2}
GDB=/c/msys64/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb.exe
elf=$1
app=$(cygpath -w "$2")
secs=$3
taskkill //IM azahar.exe //F >/dev/null 2>&1
("$AZAHAR/azahar.exe" "$app" >/dev/null 2>&1 &)
sleep 3
cat > build/n3ds/stall.gdb <<EOF
set pagination off
set confirm off
target remote localhost:24689
continue &
shell ping -n $((secs + 1)) 127.0.0.1 >NUL
interrupt
shell ping -n 8 127.0.0.1 >NUL
info threads
thread apply all bt 25
detach
EOF
"$GDB" -batch -x build/n3ds/stall.gdb "$elf" 2>&1 | grep -vE "^\[New Thread|warning: (Remote|Unable)"
taskkill //IM azahar.exe //F >/dev/null 2>&1
