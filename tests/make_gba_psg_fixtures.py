"""Own ARM drivers for audio-only PSG mirror and delayed-channel tests."""
from pathlib import Path
import struct,zlib,sys
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
def build(delayed):
 code=[];pool=[];fix=[]
 def literal(reg,value):fix.append((len(code),len(pool)));code.append(0xe59f0000|(reg<<12));pool.append(value)
 def writes(items):
  for a,v in items:literal(0,a);literal(1,v);code.append(0xe1c010b0)
 def wait():
  literal(2,3000000);code.extend([0xe2522001,0x1afffffd])
 writes([(0x4000084,0x80),(0x4000080,0xff77),(0x4000082,2)])
 if delayed:wait()
 writes([(0x4000070,0),(0x4000090,0x0123),(0x4000092,0x4567),(0x4000094,0x89ab),(0x4000096,0xcdef),(0x4000098,0xfedc),(0x400009a,0xba98),(0x400009c,0x7654),(0x400009e,0x3210),(0x4000070,0x80),(0x4000072,0x2000),(0x4000074,0x8700),(0x4000078,0xf000),(0x400007c,0x8003)])
 wait();writes([(0x4000082,0),(0x4000072,0x8000),(0x4000074,0x8600)]);wait();writes([(0x4000084,0),(0x4000084,0x80),(0x4000080,0xff77),(0x4000082,2),(0x4000078,0xa000),(0x400007c,0x8007)])
 code.append(0xeafffffe)
 for at,lit in fix:code[at]|=4*(len(code)+lit-at-2)
 rom=b''.join(struct.pack('<I',v) for v in code+pool);exe=struct.pack('<III',0x8000000,0,len(rom))+rom;data=zlib.compress(exe)
 return b'PSF'+bytes([0x22])+struct.pack('<III',0,len(data),zlib.crc32(data))+data+b'[TAG]title=Own wave noise power-cycle test\nlength=0:08\n'
for delayed,name in [(False,'psg-wave-noise.gsf'),(True,'psg-delayed.gsf')]: (out/name).write_bytes(build(delayed))
