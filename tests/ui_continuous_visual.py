from pathlib import Path
import ctypes as c
from ctypes import wintypes as w
import os,subprocess,time,json,hashlib,sys
exe=Path(sys.argv[1]).resolve();music=Path(sys.argv[2]).resolve();out=Path(sys.argv[3]).resolve();out.mkdir(parents=True,exist_ok=True)
u=c.WinDLL('user32',use_last_error=True)
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)]
callback=c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
def find(pid,cls):
    result=[]
    @callback
    def visit(h,_):
        p=w.DWORD();u.GetWindowThreadProcessId(h,c.byref(p));name=c.create_unicode_buffer(160);u.GetClassNameW(h,name,160)
        if p.value==pid and name.value==cls:result.append(h)
        return True
    u.EnumWindows(visit,0)
    return result[0]if result else None
def wait(fn,seconds=8):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        value=fn()
        if value:return value
        time.sleep(.02)
    raise TimeoutError('Native visual condition not met')
env=os.environ.copy();env.update(SDL_AUDIODRIVER='dummy',NGMV_TEST_SESSION='flowb8',NGMV_TEST_SNAPSHOT_MS='20000',NGMV_SETTINGS_TEST_FILE=str(out/'settings.ini'))
(out/'settings.ini').write_text('spectrumView=0\nspectrumStyle=3\nmaster=100\nallChannelVolume=100\n')
settings=Path(os.environ['LOCALAPPDATA'])/'GameMusicPlayer/settings.ini'
settings_before=hashlib.sha256(settings.read_bytes()).hexdigest()
sha=hashlib.sha256(music.read_bytes()).hexdigest()
with(out/'app.log').open('wb')as log:
    p=subprocess.Popen([str(exe),'--snapshot',str(music),str(out/'snapshot.bmp'),'--size=1280x720'],env=env,stdout=log,stderr=log)
    try:
        h=wait(lambda:find(p.pid,'NintendoMusicStudioWindow'))
        q=lambda index:int(u.SendMessageW(h,0x8000+74,index,0))
        key=lambda code:u.PostMessageW(h,0x100,code,0)
        wait(lambda:q(0)and q(1));time.sleep(.3)
        assert q(30)==0
        t0=q(36);history=q(35);time.sleep(.3)
        assert q(36)>t0+150 and q(35)>history,'History did not collect in waveform mode'
        key(32);wait(lambda:not q(1));frozen=q(2);time.sleep(.25)
        assert q(2)==frozen and q(44)==0 and not q(43),'Paused waveform did not drain to silence'
        paused_time=q(36);before=q(35);key(0x75);wait(lambda:q(30)==1);time.sleep(.2)
        assert q(36)>paused_time+100 and q(35)>=before,'Mode toggle erased history'
        # Select every spectrum style through its normal native choice window.
        choices=[]
        for choice in [2600,2601,2602]:
            key(0x76);popup=wait(lambda:find(p.pid,'NintendoMusicStudioSpectrumChoices'))
            old_time=q(36);old_size=q(35);u.SendMessageW(popup,0x111,choice,0);time.sleep(.15)
            assert q(31)==(3 if choice==2602 else choice-2600)
            assert q(36)>old_time and q(35)>=old_size
            choices.append(q(31));u.SendMessageW(popup,0x10,0,0)
        key(0x78);wait(lambda:q(15));key(0x7a);wait(lambda:q(14));old=q(36);time.sleep(.2);assert q(36)>old
        key(0x7a);wait(lambda:not q(14));key(0x78);wait(lambda:not q(15))
        key(0x75);wait(lambda:q(30)==0);key(32);wait(lambda:q(1));time.sleep(.2);assert q(44)>0
        old=q(36);size=q(35)
        # Native stop button at logical 730,160 within the1920x1080 view.
        rect=w.RECT();u.GetClientRect(h,c.byref(rect));x=int(730*rect.right/1920);y=int(160*rect.bottom/1080)
        u.SendMessageW(h,0x201,1,(y<<16)|x);wait(lambda:not q(1));time.sleep(.25)
        assert q(2)==0 and q(44)==0 and q(36)>old and q(35)>=size,'Stop froze/reset visual timeline'
        result={'pass':True,'history_collected_in_waveform_mode':True,'pause_drains_waveform_to_zero':True,'audio_clock_frozen_while_paused':True,'mode_styles':choices,'recording_and_fullscreen_clock_continues':True,'stop_resets_audio_only_and_preserves_history':True,'history_columns_at_end':q(35),'visual_time_ms':q(36),'source_and_user_settings_unchanged':True,'audio':'dummy; owned offscreen process only'}
        u.PostMessageW(h,0x10,0,0);p.wait(timeout=8)
        assert hashlib.sha256(settings.read_bytes()).hexdigest()==settings_before
        assert hashlib.sha256(music.read_bytes()).hexdigest()==sha
        (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
    finally:
        if p.poll()is None:
            if h:u.PostMessageW(h,0x10,0,0)
            try:p.wait(timeout=3)
            except subprocess.TimeoutExpired:p.terminate();p.wait(timeout=5)
