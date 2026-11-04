"""Own offscreen player process only; no interaction with existing user apps."""
from pathlib import Path
import ctypes,os,subprocess,sys,time,json
from ctypes import wintypes as W
exe=Path(sys.argv[1]).resolve();fixture=Path(sys.argv[2]).resolve();out=Path(sys.argv[3]).resolve();out.mkdir(exist_ok=True,parents=True)
u=ctypes.WinDLL('user32',use_last_error=True)
CB=ctypes.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
u.EnumWindows.argtypes=[CB,W.LPARAM];u.GetWindowThreadProcessId.argtypes=[W.HWND,ctypes.POINTER(W.DWORD)]
u.GetClassNameW.argtypes=[W.HWND,W.LPWSTR,ctypes.c_int];u.GetWindowTextW.argtypes=[W.HWND,W.LPWSTR,ctypes.c_int]
u.PostMessageW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM]
env=dict(os.environ,SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='player1415-ui')
def windows(pid):
 found=[]
 @CB
 def visit(h,_):
  p=W.DWORD();u.GetWindowThreadProcessId(h,ctypes.byref(p))
  if p.value==pid:
   c=ctypes.create_unicode_buffer(256);t=ctypes.create_unicode_buffer(512)
   u.GetClassNameW(h,c,256);u.GetWindowTextW(h,t,512);found.append((h,c.value,t.value))
  return True
 u.EnumWindows(visit,0);return found
for flag in ['--editor-selftest','--editor-snapshot']:
 p=subprocess.run([str(exe),flag,str(out)],env=env,timeout=10)
 assert p.returncode==2,(flag,p.returncode)
snapshot=out/'player.bmp'
p=subprocess.Popen([str(exe),'--snapshot',str(fixture),str(snapshot)],env=env,creationflags=subprocess.CREATE_NO_WINDOW)
try:
 main=None
 for _ in range(150):
  own=windows(p.pid)
  matches=[x for x in own if x[1]=='NintendoMusicStudioWindow']
  if matches:main=matches[0];break
  time.sleep(.01)
 assert main,'Player window missing'
 assert 'Nintendo Music Studio' in main[2] and json.loads((Path(__file__).resolve().parents[1]/'RELEASE.json').read_text())['version'] in main[2],main
 # Former composer/import shortcuts must not open another window or stop playback.
 u.PostMessageW(main[0],0x100,0x71,0);u.PostMessageW(main[0],0x100,0x72,0)
 time.sleep(.25)
 # Studio is the public product name; reject actual editor windows/actions,
 # rather than treating the legitimate player caption as an editor.
 own=windows(p.pid);assert not any('Editor' in c or '편집' in t for _,c,t in own),own
 assert p.wait(timeout=20)==0
 assert snapshot.exists() and snapshot.stat().st_size>100000
finally:
 if p.poll() is None:
  # Only this exact Popen child is owned by this test.
  p.terminate();p.wait(timeout=5)
result={'pass':True,'name_version':True,'legacy_editor_CLI_rejected':True,'F2_F3_no_editor_windows':True,'offscreen_player_snapshot':str(snapshot),'audio':'dummy','user_app_operated':False}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
