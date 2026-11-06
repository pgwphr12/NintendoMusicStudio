"""Read PE imports and verify no third-party/runtime DLL is needed. No code execution."""
from pathlib import Path
import struct,json,sys
p=Path(sys.argv[1]);b=p.read_bytes()
u16=lambda i:struct.unpack_from('<H',b,i)[0]
u32=lambda i:struct.unpack_from('<I',b,i)[0]
pe=u32(0x3c);assert b[pe:pe+4]==b'PE\0\0';optional=pe+24
assert u16(optional)==0x20b
sections=[]
for n in range(u16(pe+6)):
 i=optional+u16(pe+20)+n*40;sections.append((u32(i+12),max(u32(i+8),u32(i+16)),u32(i+20)))
def offset(rva):
 for start,size,raw in sections:
  if start<=rva<start+size:return raw+rva-start
 raise ValueError('Unmapped PE RVA')
imports=[];i=offset(u32(optional+120))
while any(b[i:i+20]):
 at=offset(u32(i+12));imports.append(b[at:b.index(0,at)].decode('ascii'));i+=20
allowed={'kernel32.dll','user32.dll','gdi32.dll','shell32.dll','shlwapi.dll','advapi32.dll','ole32.dll','oleaut32.dll','comdlg32.dll','comctl32.dll','winmm.dll','dxgi.dll','dwmapi.dll','version.dll','imm32.dll','setupapi.dll','cfgmgr32.dll','avrt.dll','dsound.dll','msacm32.dll','powrprof.dll','ws2_32.dll','bcrypt.dll','ntdll.dll','ucrtbase.dll'}
assert all(n.lower() in allowed or n.lower().startswith('api-ms-win-') for n in imports),imports
assert not any(n.lower().startswith(('gme','sdl','msvcp','vcruntime','zlib','libgme')) for n in imports)
print(json.dumps({'single_exe':True,'imports':imports,'system_DLLs_only':True,'bytes':len(b)},indent=2))
