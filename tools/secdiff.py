#!/usr/bin/env python3
# Compares data sections of our object with the target object for a unit.
# Usage: python tools/secdiff.py <unit path, e.g. Game/zLight> <.section> [...]
import struct,sys
def secs(p):
    d=open(p,'rb').read()
    shoff=struct.unpack('>I',d[0x20:0x24])[0]; shnum=struct.unpack('>H',d[0x30:0x32])[0]; shstr=struct.unpack('>H',d[0x32:0x34])[0]
    sh=[struct.unpack('>IIIIIIIIII',d[shoff+i*40:shoff+i*40+40]) for i in range(shnum)]
    so=sh[shstr][4]
    r={}
    for s in sh:
        n=d[so+s[0]:d.index(b'\0',so+s[0])].decode()
        r[n]=d[s[4]:s[4]+s[5]] if s[1]!=8 else b''
    return r
u=sys.argv[1]
a=secs('build/GIHE78/src/%s.o'%u); b=secs('build/GIHE78/obj/%s.o'%u)
for n in sys.argv[2:]:
    x=a.get(n,b''); y=b.get(n,b'')
    print(n,len(x),len(y))
    for i in range(0,max(len(x),len(y)),16):
        if x[i:i+16]!=y[i:i+16]: print(hex(i), x[i:i+16].hex(), y[i:i+16].hex())
