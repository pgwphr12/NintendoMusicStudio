"""Modeless About regression; only exact Popen children, offscreen, dummy audio.

Read-only observations verify that native mouse hit testing actually changes
playback/mute/volume while About is open, rather than merely delivering messages.
"""
import ctypes
from ctypes import wintypes as W
import json
import os
from pathlib import Path
import subprocess
import sys
import time

exe, fixture, out = map(lambda p: Path(p).resolve(), sys.argv[1:4])
out.mkdir(parents=True, exist_ok=True)
u = ctypes.WinDLL('user32', use_last_error=True)
CB = ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
u.EnumWindows.argtypes = [CB, W.LPARAM]
u.GetWindowThreadProcessId.argtypes = [W.HWND, ctypes.POINTER(W.DWORD)]
u.GetClassNameW.argtypes = [W.HWND, W.LPWSTR, ctypes.c_int]
u.GetWindowTextW.argtypes = [W.HWND, W.LPWSTR, ctypes.c_int]
u.IsWindowEnabled.argtypes = [W.HWND]
u.IsWindow.argtypes = [W.HWND]
u.GetDlgItem.argtypes = [W.HWND, ctypes.c_int]
u.GetDlgItem.restype = W.HWND
u.GetClientRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]
u.GetWindowRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]
u.PostMessageW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM]
u.SendMessageTimeoutW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM,
                                W.UINT, W.UINT, ctypes.POINTER(ctypes.c_size_t)]


def windows(pid, cls):
    found = []
    @CB
    def visit(h, _):
        p = W.DWORD()
        u.GetWindowThreadProcessId(h, ctypes.byref(p))
        if p.value == pid:
            c = ctypes.create_unicode_buffer(256)
            u.GetClassNameW(h, c, len(c))
            if c.value == cls:
                found.append(h)
        return True
    u.EnumWindows(visit, 0)
    return found


def wait_for(fn, text, seconds=4):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = fn()
        if value:
            return value
        time.sleep(.015)
    raise AssertionError(text)


def send(h, msg, a=0, b=0):
    value = ctypes.c_size_t()
    ok = u.SendMessageTimeoutW(h, msg, a, b, 2, 1500, ctypes.byref(value))
    assert ok, ('Unresponsive owned window', msg, ctypes.get_last_error())
    return value.value


results = []
for entry in ['F1', 'button', 'recording-F1']:
    target = out / (entry + '.bmp')
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', NGMV_TEST_SESSION='about1415-' + entry,
               NGMV_TEST_SNAPSHOT_MS='12000')
    args = [str(exe), '--snapshot', str(fixture), str(target)]
    if entry == 'recording-F1':
        args.append('--recording')
    p = subprocess.Popen(args, env=env, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        main = wait_for(lambda: windows(p.pid, 'NintendoMusicStudioWindow'), 'No main window')[0]
        state = lambda selector: send(main, 0x8000 + 74, selector)
        wait_for(lambda: state(0) and state(1) and state(2) > 50, 'No active playback')
        title = ctypes.create_unicode_buffer(512)
        u.GetWindowTextW(main, title, len(title))
        assert json.loads((Path(__file__).resolve().parents[1] / 'RELEASE.json').read_text())['version'] in title.value, title.value
        rect = W.RECT()
        u.GetWindowRect(main, ctypes.byref(rect))
        assert rect.left < -10000, 'Test must stay offscreen'

        def click(x, y):
            r = W.RECT()
            u.GetClientRect(main, ctypes.byref(r))
            scale = min(r.right / 1920, r.bottom / 1080)
            px = int((r.right - 1920 * scale) / 2 + x * scale)
            py = int((r.bottom - 1080 * scale) / 2 + y * scale)
            send(main, 0x201, 1, px | (py << 16))
            send(main, 0x202, 0, px | (py << 16))

        def open_info():
            if entry == 'button':
                click(1650, 45)
            else:
                send(main, 0x100, 0x70)
            info = wait_for(lambda: windows(p.pid, 'NintendoMusicStudioInfo'), 'About missing')
            assert len(info) == 1
            assert u.IsWindowEnabled(main), 'About disabled the player'
            assert not windows(p.pid, '#32770'), 'Unexpected modal dialog'
            return info[0]

        about = open_info()
        before = state(2)
        time.sleep(.30)
        assert state(2) > before + 120, 'Playback stopped with About open'
        for _ in range(8):
            assert open_info() == about, 'Duplicate About windows'

        if entry != 'recording-F1':
            click(580, 160)
            assert state(1) == 0, 'Pause click ignored'
            before = state(2)
            time.sleep(.18)
            assert abs(state(2) - before) < 30, 'Pause did not stop audio clock'
            click(580, 160)
            assert state(1) == 1, 'Resume click ignored'
            click(1565, 178)
            assert state(3) == 95, 'Master-volume click ignored'
            # First label's top strip also fits compact multi-voice layouts.
            click(100, 284)
            assert state(4) == 1, 'Channel-mute click ignored'
            click(100, 284)
            assert state(4) == 0, 'Channel-unmute click ignored'
            click(730, 160)
            assert state(1) == 0 and state(2) < 50, 'Stop click ignored'
            click(580, 160)
            assert state(1) == 1, 'Play after stop ignored'
        else:
            send(main, 0x100, 0x20)
            assert state(1) == 0
            send(main, 0x100, 0x20)
            assert state(1) == 1

        # Escape from the child control is handled through the modeless loop.
        body = u.GetDlgItem(about, 1)
        notices = ctypes.create_unicode_buffer(524288)
        text_result = ctypes.c_size_t()
        assert u.SendMessageTimeoutW(body, 0x000D, len(notices), ctypes.addressof(notices), 2, 2000, ctypes.byref(text_result)), "About text query timed out"
        assert 'Embedded component notices' in notices.value and 'GNU GENERAL PUBLIC LICENSE' in notices.value, 'Standalone EXE notices missing'

        u.PostMessageW(body, 0x100, 0x1b, 0)
        wait_for(lambda: not u.IsWindow(about), 'Escape did not close About')
        about = open_info()
        send(about, 0x111, 2)  # Close button / IDCANCEL
        assert not u.IsWindow(about), 'Close button did not close About'
        about = open_info()
        send(about, 0x10)  # title-bar close
        assert not u.IsWindow(about), 'Title-bar close failed'
        about = open_info()
        if entry == 'F1':
            # Existing bounded capture also destroys the owner with About open.
            send(main, 0x113, 2)
            assert p.wait(timeout=10) == 0 and target.stat().st_size > 100000
        else:
            send(main, 0x10)
            assert p.wait(timeout=10) == 0
        assert not windows(p.pid, 'NintendoMusicStudioInfo'), 'Orphaned About'
        results.append({'entry': entry, 'pass': True, 'duplicate_windows': 0,
                        'playback_continued': True, 'owner_clicks_worked': True,
                        'escape_close_reopen_owner_cleanup': True})
    finally:
        if p.poll() is None:
            p.terminate()  # exact owned child only
            p.wait(timeout=5)

result = {'pass': True, 'audio': 'dummy', 'offscreen': True,
          'user_app_operated': False, 'cases': results}
(out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
print(json.dumps(result))
