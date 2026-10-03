#!/usr/bin/env python3
# Prints the .text function order of an object file.
# Usage: python tools/fnorder.py build/GIHE78/src/<unit>.o
import struct,sys
p=sys.argv[1]
d=open(p,'rb').read()
shoff=struct.unpack('>I',d[0x20:0x24])[0]; shnum=struct.unpack('>H',d[0x30:0x32])[0]; shstr=struct.unpack('>H',d[0x32:0x34])[0]
sh=[struct.unpack('>IIIIIIIIII',d[shoff+i*40:shoff+i*40+40]) for i in range(shnum)]
so=sh[shstr][4]
names=[d[so+s[0]:d.index(b'\0',so+s[0])].decode() for s in sh]
ti=names.index('.text')
sym=[s for s in sh if s[1]==2][0]; strt=sh[sym[6]]
out=[]
for i in range(sym[5]//16):
    n,v,sz,info,oth,shn=struct.unpack('>IIIBBH',d[sym[4]+i*16:sym[4]+i*16+16])
    if shn==ti and (info&0xf)==2:
        out.append((v,d[strt[4]+n:d.index(b'\0',strt[4]+n)].decode()))
for v,n in sorted(out): print(hex(v),n)
