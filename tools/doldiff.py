#!/usr/bin/env python3
# Prints the first differing words between the built DOL and the original.
# Usage: python tools/doldiff.py
import struct,sys
def load(p):
    d=open(p,'rb').read()
    offs=struct.unpack('>18I',d[0:72]); addrs=struct.unpack('>18I',d[72:144]); sizes=struct.unpack('>18I',d[144:216])
    return d,offs,addrs,sizes
a=load('build/GIHE78/main.dol'); b=load('orig/GIHE78/sys/main.dol')
print('entry', hex(struct.unpack('>I',a[0][0xE0:0xE4])[0]), hex(struct.unpack('>I',b[0][0xE0:0xE4])[0]))
for i in range(18):
    if a[3][i]!=b[3][i] or a[2][i]!=b[2][i]: print('sec',i,hex(a[2][i]),hex(a[3][i]),hex(b[2][i]),hex(b[3][i]))
    da=a[0][a[1][i]:a[1][i]+a[3][i]]; db=b[0][b[1][i]:b[1][i]+b[3][i]]
    n=0
    for j in range(0,min(len(da),len(db)),4):
        if da[j:j+4]!=db[j:j+4]:
            print('diff', hex(a[2][i]+j), da[j:j+4].hex(), db[j:j+4].hex()); n+=1
            if n>8: break
