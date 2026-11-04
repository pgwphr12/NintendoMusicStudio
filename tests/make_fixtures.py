"""Self-authored minimal music drivers; contains no commercial game music/ROM."""
from pathlib import Path
import struct, sys, math
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
def nsf(fds=False):
    code=bytearray([0x85,0x00])  # save track number
    def write(address,value):code.extend([0xa9,value,0x8d,address&255,address>>8])
    write(0x4015,0x0f)
    for address,value in [(0x4000,0xbf),(0x4002,0x40),(0x4003,1),(0x4004,0x7f),(0x4006,0xa0),(0x4007,0),(0x4008,0x81),(0x400a,0x90),(0x400b,1),(0x400c,0x10)]:write(address,value)
    code.extend([0xa5,0,0xf0,5]);write(0x4015,0x0e)  # track 2: pulse 1 off
    if fds:
        write(0x4023,2);write(0x4089,0x80)
        for i in range(64):write(0x4040+i,int(31+28*math.sin(i*2*math.pi/64)))
        for a,v in [(0x4089,0),(0x4080,0x9f),(0x4082,0x80),(0x4083,1)]:write(a,v)
    code.append(0x60);play=0x8000+len(code)
    code.extend([0xe6,1,0xa5,1,0x29,0x3f,0x09,0x40,0x8d,2,0x40,0x60])
    h=bytearray(128);h[:5]=b'NESM\x1a';h[5:8]=bytes([1,2,1]);struct.pack_into('<HHH',h,8,0x8000,0x8000,play)
    h[14:14+len(b'NGMV original channel test')]=b'NGMV original channel test';h[46:55]=b'NGMV Team'
    struct.pack_into('<H',h,0x6e,16666);struct.pack_into('<H',h,0x78,20000);h[0x7b]=4 if fds else 0
    return bytes(h+code),bytes(code),play
data,code,play=nsf();(out/'channels.nsf').write_bytes(data)
(out/'fds.nsf').write_bytes(nsf(True)[0])
def chunk(name,data):return struct.pack('<I',len(data))+name.encode('ascii')+data
nsfe=b'NSFE'+chunk('INFO',struct.pack('<HHHBBBB',0x8000,0x8000,play,0,0,2,0))+chunk('DATA',code)+chunk('auth',b'Original NGMV tests\0NGMV Team\0\0\0')+chunk('tlbl',b'Both pulses\0Pulse 2 only\0')+chunk('time',struct.pack('<ii',1000,1200))+chunk('NEND',b'')
(out/'channels.nsfe').write_bytes(nsfe)
gbs=bytearray(112);gbs[:3]=b'GBS';gbs[3:6]=bytes([1,2,1]);code=bytearray()
def gbwrite(a,v):code.extend([0x3e,v,0xea,a&255,a>>8])
for a,v in [(0xff26,0x80),(0xff24,0x77),(0xff25,0xff),(0xff10,0),(0xff11,0x80),(0xff12,0xf0),(0xff13,0xc0),(0xff14,0x83),(0xff16,0x40),(0xff17,0xa0),(0xff18,0x80),(0xff19,0x84)]:gbwrite(a,v)
for i in range(16):gbwrite(0xff30+i,((i%8)<<4)|((15-i)%8))
for a,v in [(0xff1a,0x80),(0xff1b,0),(0xff1c,0x20),(0xff1d,0x90),(0xff1e,0x87),(0xff20,0),(0xff21,0x60),(0xff22,0x34),(0xff23,0x80)]:gbwrite(a,v)
code.append(0xc9);play=0x400+len(code);code.append(0xc9)
struct.pack_into('<HHHHBB',gbs,6,0x400,0x400,play,0xfffe,0,0);title=b'NGMV original GB test';gbs[16:16+len(title)]=title
(out/'channels.gbs').write_bytes(gbs+code)
spc=bytearray(0x10200);signature=b'SNES-SPC700 Sound File Data v0.30';spc[:len(signature)]=signature;spc[0x21:0x25]=bytes([0x1a,0x1a,0x1a,30]);struct.pack_into('<H',spc,0x25,0x200);spc[0x2b]=0xef
spc[0x2e:0x2e+len(b'NGMV original SPC test')]=b'NGMV original SPC test';spc[0xa9:0xac]=b'003';spc[0xac:0xb1]=b'00000'
ram=memoryview(spc)[0x100:0x10100];ram[0x200:0x202]=bytes([0x2f,0xfe]);struct.pack_into('<HH',ram,0x1000,0x1100,0x1100);ram[0x1100:0x1109]=bytes([0x83,0x12,0x34,0x56,0x76,0x54,0x32,0x10,0xfe])
dsp=memoryview(spc)[0x10100:0x10180]
for ch,pitch in [(0,0x1000),(1,0x800)]:
    dsp[ch*16]=dsp[ch*16+1]=64;dsp[ch*16+2]=pitch&255;dsp[ch*16+3]=pitch>>8;dsp[ch*16+7]=127
dsp[0x0c]=dsp[0x1c]=100;dsp[0x5d]=0x10;dsp[0x4c]=3;dsp[0x6c]=0x20
(out/'channels.spc').write_bytes(spc)
# Rendering stress input: all eight voices are real active SPC signals.
for ch in range(8):
    pitch=0x400+ch*0x180
    dsp[ch*16]=dsp[ch*16+1]=40
    dsp[ch*16+2]=pitch&255;dsp[ch*16+3]=pitch>>8;dsp[ch*16+7]=127
dsp[0x4c]=255
(out/'scope-stress.spc').write_bytes(spc)
(out/'empty.nsf').write_bytes(b'');(out/'invalid.nsf').write_bytes(b'not a music file')
print('Generated original NSF, NSFE, FDS, GBS and SPC fixtures.')

# FDS memory regression: copy bank data into RAM, write/execute RAM,
# verify independent copies at $6000 and $E000, and switch both low banks.
base,original,old_play=nsf(True)
init=bytearray([0x85,2])
def emit_write(a,v):init.extend([0xa9,v,0x8d,a&255,a>>8])
def emit_check(a,v):
    init.extend([0xad,a&255,a>>8,0xc9,v,0xf0,1,0x60]) # failed check returns before sound init
emit_check(0x6000,0x85)
emit_write(0x6000,0xea);emit_check(0xe000,0x85)
emit_write(0x8000,0x60);init.extend([0x20,0,0x80])
for register,address,bank,value in [(0x5ff9,0x9000,3,0x55),(0x5ff9,0x9000,4,0x66),(0x5ff6,0x6000,5,0x77),(0x5ff7,0x7000,6,0x88)]:
    emit_write(register,bank);emit_check(address,value)
init.extend([0xa5,2]);init.extend(original[:old_play-0x8000])
play=0xe000+len(init);payload=bytearray(8*4096)
payload[:len(init)]=init;payload[len(init):len(init)+len(original[old_play-0x8000:])]=original[old_play-0x8000:]
for bank,value in [(3,0x55),(4,0x66),(5,0x77),(6,0x88)]:payload[bank*4096]=value
header=bytearray(base[:128]);struct.pack_into('<HHH',header,8,0x6000,0xe000,play)
header[112:120]=bytes([2,3,4,5,6,7,0,1])
(out/'fds-banked.nsf').write_bytes(header+payload)
nsfe=b'NSFE'+chunk('INFO',struct.pack('<HHHBBBB',0x6000,0xe000,play,0,4,2,0))+chunk('BANK',bytes([2,3,4,5,6,7,0,1]))+chunk('DATA',payload)+chunk('NEND',b'')
(out/'fds-banked.nsfe').write_bytes(nsfe)

# Original looping DMC sample for activity detection (no commercial samples).
base,original,old_play=nsf()
init=bytearray(original[:old_play-0x8000-1])
for address,value in [(0x4010,0x4f),(0x4012,0),(0x4013,1),(0x4015,0x1f)]:
    init.extend([0xa9,value,0x8d,address&255,address>>8])
init.append(0x60);new_play=0x8000+len(init)
payload=init+original[old_play-0x8000:]
payload.extend(bytes(0x4000-len(payload)));payload.extend(bytes([0xf0,0x0f])*16)
header=bytearray(base[:128]);struct.pack_into('<H',header,12,new_play)
(out/'dmc.nsf').write_bytes(header+payload)

# PSF variants and original ARM fixtures share the same output directory.
import subprocess
subprocess.run([sys.executable,str(Path(__file__).with_name("make_psf_fixtures.py")),str(out)],check=True)
subprocess.run([sys.executable,str(Path(__file__).with_name("make_sega_fixtures.py")),str(out)],check=True)

# Own ordinary audio fixtures; compressed vectors contain only synthetic test tones.
import json,base64,wave
ordinary=out/'audio';ordinary.mkdir(exist_ok=True)
with wave.open(str(ordinary/'tone.wav'),'wb') as w:
    w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000)
    w.writeframes(b''.join(struct.pack('<hh',round(12000*math.sin(2*math.pi*(440 if i<48000 else 660)*i/48000)),round(8000*math.sin(2*math.pi*(880 if i<48000 else 990)*i/48000))) for i in range(96000)))
with wave.open(str(ordinary/'mono22050.wav'),'wb') as w:
    w.setnchannels(1);w.setsampwidth(2);w.setframerate(22050)
    w.writeframes(b''.join(struct.pack('<h',round(9000*math.sin(2*math.pi*330*i/22050))) for i in range(44100)))
for ext,value in json.loads((Path(__file__).parent/'audio_fixture_data.json').read_text())['base64'].items():
    (ordinary/('tone.'+ext)).write_bytes(base64.b64decode(value))
(ordinary/'한글 파일.mp3').write_bytes((ordinary/'tone.mp3').read_bytes())
(ordinary/'bad.wav').write_bytes(b'RIFF'+bytes(4)+b'WAVE'+bytes(64))
(ordinary/'bad.ogg').write_bytes(b'OggS'+bytes(64))
(ordinary/'bad.mp3').write_bytes(b'ID3'+bytes(64))

(ordinary/'한글 파일.ogg').write_bytes((ordinary/'tone.ogg').read_bytes())

float_data=b''.join(struct.pack('<ff',.25*math.sin(2*math.pi*330*i/96000),.2*math.sin(2*math.pi*550*i/96000)) for i in range(96000))
float_fmt=struct.pack('<HHIIHH',3,2,96000,96000*8,8,32)
float_body=b'WAVEfmt '+struct.pack('<I',len(float_fmt))+float_fmt+b'data'+struct.pack('<I',len(float_data))+float_data
(ordinary/'float96000.wav').write_bytes(b'RIFF'+struct.pack('<I',len(float_body))+float_body)

(ordinary/'padded.MP3').write_bytes(bytes(128)+(ordinary/'tone.mp3').read_bytes())

over=bytearray((ordinary/'tone.mp3').read_bytes());at=over.find(b'Info');at=at if at>=0 else over.find(b'Xing')
assert at>=0 and int.from_bytes(over[at+4:at+8],'big')&1
n=int.from_bytes(over[at+8:at+12],'big');over[at+8:at+12]=(n*10).to_bytes(4,'big')
(ordinary/'overstated.mp3').write_bytes(over)

# Own short extended-chip VGM for native repeat coverage.
es=bytearray(256);es[:4]=b'Vgm ';struct.pack_into('<I',es,8,0x171);struct.pack_into('<I',es,0x34,0xcc);struct.pack_into('<I',es,0x9c,2000000)
for i in range(32):es.extend([0xd2,0,i,int(110*math.sin(i*2*math.pi/32))&255])
for port,reg,val in [(1,0,140),(1,1,0),(2,0,15),(3,0,1)]:es.extend([0xd2,port,reg,val])
es.extend([0x61,0x4b,0x3c,0x66]);struct.pack_into('<I',es,4,len(es)-4);struct.pack_into('<I',es,0x18,15435)
(out/'short-scc.vgm').write_bytes(es)

(ordinary/'renamed-game.mp3').write_bytes((out/'channels.nsf').read_bytes())
