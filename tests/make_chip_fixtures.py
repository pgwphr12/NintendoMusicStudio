"""Own ES5506 sine ROM/host sequences and AY8930 register patterns."""
from pathlib import Path
import math,struct,sys
out=Path(sys.argv[1]);out.mkdir(exist_ok=True,parents=True)
def header():
 b=bytearray(256);b[:4]=b'Vgm ';struct.pack_into('<I',b,8,0x171);struct.pack_into('<I',b,0x34,0xcc);return b
def finish(b):
 b+=bytes([0x61,0x44,0xac,0x66]);struct.pack_into('<I',b,4,len(b)-4);struct.pack_into('<I',b,0x18,44100);return b
def es(active=31,clock=16000000,only_last=False):
 b=header();struct.pack_into('<I',b,0xd0,0x80000000|clock);
 if only_last:struct.pack_into('<I',b,0x2c,7670454)
 def reg(r,value):
  for i in range(4):b.extend([0xbe,r*4+i,(value>>(24-8*i))&255])
 data=bytes(int(110*math.sin(i*2*math.pi/128))&255 for i in range(256))
 b.extend(b'\x67\x66\x90'+struct.pack('<III',8+len(data),4096,0x80000000)+data)
 reg(15,0x20);reg(10,1);reg(11,17);reg(12,32)
 for v in range(active+1):
  if only_last and v!=active:continue
  reg(15,0x20+v);reg(1,0);reg(2,127<<11);reg(3,0)
  reg(15,v);reg(11,active);reg(12,8);reg(1,0x800+v*61);reg(2,0xffff);reg(4,0xffff);reg(7,0xffff);reg(9,0xffff);reg(0,0x308|((v%6)<<10))
 return finish(b)
for n,a,c,last in [('es-many.vgm',31,16000000,False),('es-last.vgm',31,16000000,True),('es-high-rate.vgm',4,16000000,False),('es-exact-rate.vgm',15,12288000,False)]:(out/n).write_bytes(es(a,c,last))
def ay(duty=4,expanded=True,flags=1):
 b=header();struct.pack_into('<I',b,0x74,2000000);b[0x78]=3;b[0x79]=flags
 def reg(r,v):b.extend([0xa0,r,v])
 reg(13,0xa0 if expanded else 0)
 if expanded:reg(13,0xb0);reg(6,duty);reg(13,0xa0)
 reg(0,100);reg(1,0);reg(7,0x3e);reg(8,31 if expanded else 15)
 return finish(b)
for n,d,e,f in [('ay-own.vgm',4,True,1),('ay-narrow.vgm',0,True,1),('ay-classic.vgm',4,False,1),('ay-half.vgm',4,True,17)]: (out/n).write_bytes(ay(d,e,f))

# A genuine direct bus write has no voice source; preserve as explicit residual.
b=header();struct.pack_into('<I',b,0xd0,0x80000000|16000000)
def regbus(r,v):
 for i in range(4):b.extend([0xbe,r*4+i,(v>>(24-8*i))&255])
regbus(15,0x20);regbus(10,1);regbus(11,17);regbus(15,0);regbus(12,8);regbus(15,0x40);regbus(0,0x40000);regbus(1,0xfffe0000)
(out/'es-direct-bus.vgm').write_bytes(finish(b))
