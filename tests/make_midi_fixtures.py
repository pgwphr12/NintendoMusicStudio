"""Own sine samples, SF2/DLS banks and SMF sequences; no system/game assets."""
from pathlib import Path
import math,struct,sys
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
def chunk(tag,data):return tag+struct.pack('<I',len(data))+data+(b'\0' if len(data)&1 else b'')
def listing(tag,*children):return chunk(b'LIST',tag+b''.join(children))
def name(s):return s.encode().ljust(20,b'\0')[:20]
def vlq(v):
    b=[v&127];v>>=7
    while v:b.insert(0,(v&127)|128);v>>=7
    return bytes(b)
def track(events):return chunk(b'MTrk',b''.join(vlq(t)+v for t,v in events)) # size fields below are big-endian
def smf(tracks,fmt=1,division=480):return b'MThd'+struct.pack('>IHHH',6,fmt,len(tracks),division)+b''.join(b'MTrk'+struct.pack('>I',len(t))+t for t in tracks)
def ev(events):return b''.join(vlq(t)+v for t,v in events)
tempo=ev([(0,b'\xff\x51\x03\x07\xa1\x20'),(480,b'\xff\x51\x03\x03\xd0\x90'),(480,b'\xff\x2f\0')])
notes=ev([(0,b'\xff\x03\x04TEST'),(0,b'\xc0\0'),(0,b'\x90\x3c\x64'),(480,b'\x3c\0'),(0,b'\x91\x43\x64'),(480,b'\x81\x43\0'),(0,b'\xff\x2f\0')])
song=smf([tempo,notes]);(out/'channels.mid').write_bytes(song);(out/'한글 음악.midi').write_bytes(song)
(out/'single.mid').write_bytes(smf([ev([(0,b'\x90\x3c\x64'),(480,b'\x80\x3c\0'),(0,b'\xff\x2f\0')])],0))
(out/'drums.mid').write_bytes(smf([ev([(0,b'\x99\x24\x64'),(240,b'\x89\x24\0'),(0,b'\xff\x2f\0')])],0))
(out/'program.mid').write_bytes(smf([ev([(0,b'\xc0\x28'),(0,b'\x90\x3c\x64'),(480,b'\x80\x3c\0'),(0,b'\xff\x2f\0')])],0))
for key,val in {'format2.mid':smf([notes],2),'bad-running.mid':smf([b'\0\x3c\x64\0\xff\x2f\0'],0),'bad-vlq.mid':smf([b'\x80'*5],0),'truncated.mid':song[:-1]}.items():(out/key).write_bytes(val)
# Two independent mono samples with safe tail guards.
rate=22050;count=2205
sample=[int(12000*math.sin(2*math.pi*440*i/rate)) for i in range(count)]
sample2=[int(12000*math.sin(2*math.pi*880*i/rate)) for i in range(count)]
pcm=struct.pack('<'+'h'*count,*sample)
wave=listing(b'wave',chunk(b'fmt ',struct.pack('<HHIIHH',1,1,rate,rate*2,2,16)),chunk(b'data',pcm),chunk(b'wsmp',struct.pack('<IHhiII',20,60,0,0,0,1)+struct.pack('<IIII',16,0,0,count)))
region=listing(b'rgn ',chunk(b'rgnh',struct.pack('<HHHHHH',0,127,0,127,0,0)),chunk(b'wlnk',struct.pack('<HHII',0,0,1,0)))
ins=listing(b'ins ',chunk(b'insh',struct.pack('<III',1,0,0)),listing(b'lrgn',region))
dls=chunk(b'RIFF',b'DLS '+chunk(b'colh',struct.pack('<I',1))+listing(b'lins',ins)+chunk(b'ptbl',struct.pack('<III',8,1,0))+listing(b'wvpl',wave))
(out/'own.dls').write_bytes(dls)
samples=sample+[0]*46+sample2+[0]*46
smpl=struct.pack('<'+'h'*len(samples),*samples)
phdr=b''.join(name(n)+struct.pack('<HHHIII',p,b,i,0,0,0) for i,(n,p,b) in enumerate([('Piano',0,0),('Strings',40,0),('Drums',0,128),('EOP',0,0)]))
pbag=b''.join(struct.pack('<HH',i,0) for i in range(4));pgen=b''.join(struct.pack('<HH',41,i) for i in [0,1,0])+b'\0'*4
inst=name('Sine440')+struct.pack('<H',0)+name('Sine880')+struct.pack('<H',1)+name('EOI')+struct.pack('<H',2)
igen=b''.join(struct.pack('<HH',op,v) for op,v in [(54,1),(53,0),(54,1),(53,1),(0,0)])
ibag=struct.pack('<HHHHHH',0,0,2,0,4,0)
def sh(n,start):return name(n)+struct.pack('<IIIIIBbHH',start,start+count,start,start+count,rate,60,0,0,1)
shdr=sh('sample440',0)+sh('sample880',count+46)+name('EOS')+b'\0'*26
pd=listing(b'pdta',chunk(b'phdr',phdr),chunk(b'pbag',pbag),chunk(b'pmod',b'\0'*10),chunk(b'pgen',pgen),chunk(b'inst',inst),chunk(b'ibag',ibag),chunk(b'imod',b'\0'*10),chunk(b'igen',igen),chunk(b'shdr',shdr))
sf=chunk(b'RIFF',b'sfbk'+listing(b'INFO',chunk(b'ifil',struct.pack('<HH',2,1)),chunk(b'INAM',b'Own sine test\0'))+listing(b'sdta',chunk(b'smpl',smpl))+pd)
(out/'own.sf2').write_bytes(sf)
bad=bytearray(sf);at=bad.index(b'shdr')+8+20;struct.pack_into('<I',bad,at,0xfffffffe);(out/'bad-sample.sf2').write_bytes(bad)
bad=bytearray(sf);at=bad.index(b'pgen')+8+2;struct.pack_into('<H',bad,at,65535);(out/'bad-index.sf2').write_bytes(bad)
(out/'bad-bank.sf2').write_bytes(sf[:-2])
print('Own MIDI/SF2/DLS fixtures generated')

(out/"simultaneous.mid").write_bytes(smf([ev([(0,b"\x90\x3c\x64"),(0,b"\x91\x43\x64"),(960,b"\x80\x3c\0"),(0,b"\x81\x43\0"),(0,b"\xff\x2f\0")])],0))
(out/"한글 음색.sf2").write_bytes(sf)

sixteen=[(0,bytes([0x90+c,48+c,90])) for c in range(16)]+[(480 if c==0 else 0,bytes([0x80+c,48+c,0])) for c in range(16)]+[(0,b"\xff\x2f\0")]
(out/"sixteen.mid").write_bytes(smf([ev(sixteen)],0))
