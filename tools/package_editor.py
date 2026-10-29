"""Portable editor + viewer and full corresponding source, with CRC/SHA."""
if __name__ == '__main__':
    import runpy
    from pathlib import Path as _Path
    runpy.run_path(str(_Path(__file__).with_name('package_studio.py')),run_name='__main__')
    raise SystemExit(0)
from pathlib import Path
import hashlib,json,shutil,subprocess,os,zipfile
HERE=Path(__file__).resolve().parent;SRC=HERE.parent;WS=SRC.parents[2]
BUILD=WS/'work/settings-build/ngmv';OUT=WS/'outputs';RUNTIME=OUT/'NintendoMusicEditor-0.1-Windows-x64'
RUNTIME.mkdir(exist_ok=True)
for name in ['NintendoMusicEditor.exe','NintendoGameMusicViewer.exe','gme.dll']:
    shutil.copy2(BUILD/'bin/Release'/name,RUNTIME/name)
for name in ['LICENSE','THIRD_PARTY_NOTICES.md','SOURCE.md']:
    shutil.copy2(SRC/name,RUNTIME/name)
shutil.copytree(SRC/'licenses',RUNTIME/'licenses',dirs_exist_ok=True)
shutil.copy2(SRC/'docs/MUSIC-EDITOR.md',RUNTIME/'README.md')
shutil.copy2(SRC/'README.md',RUNTIME/'VIEWER-README.md')
(RUNTIME/'examples').mkdir(exist_ok=True)
for n in ['First light.ngms','demo-pal.nsf','demo-ntsc.nsf']:
    shutil.copy2(BUILD/'editor-test-output'/n,RUNTIME/'examples'/n)
env=os.environ.copy();env['SDL_AUDIODRIVER']='dummy'
p=subprocess.run([str(RUNTIME/'NintendoMusicEditor.exe'),'--selftest',str(RUNTIME/'validation')],env=env,timeout=20)
assert p.returncode==0
assert json.loads((RUNTIME/'validation/ui-test-result.json').read_text())['pass']
subprocess.run([str(RUNTIME/'NintendoMusicEditor.exe'),'--snapshot',str(RUNTIME/'editor.bmp')],check=True,timeout=15)
subprocess.run([str(RUNTIME/'NintendoGameMusicViewer.exe'),'--snapshot',str(RUNTIME/'examples/demo-ntsc.nsf'),str(RUNTIME/'validation/viewer.bmp')],check=True,timeout=20,env=env)
from PIL import Image
Image.open(RUNTIME/'editor.bmp').save(OUT/'NintendoMusicEditor-0.1.png')
dirs={'src','tests','third_party','docs','licenses','samples','.github','tools'}
files={'CMakeLists.txt','.clang-format','.gitignore','LICENSE','README.md','README.en.md','SOURCE.md','THIRD_PARTY_NOTICES.md','VALIDATION.md'}
results=[]
for root,name in [(RUNTIME,'NintendoMusicEditor-0.1-Windows-x64.zip'),(SRC,'NintendoMusicEditor-0.1-Source.zip')]:
    path=OUT/name
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for p in sorted(root.rglob('*')):
            rel=p.relative_to(root)
            if not p.is_file() or '__pycache__' in rel.parts:continue
            if root==SRC and rel.parts[0] not in dirs|files:continue
            if root==RUNTIME and (rel.parts[0]=='validation' or p.suffix=='.bmp'):continue
            z.write(p,Path(root.name if root==RUNTIME else 'NintendoMusicEditor-0.1-Source')/rel)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
        if root==SRC:
            assert any(n.endswith('src/editor/Editor.cpp') for n in z.namelist())
            assert any(n.endswith('third_party/game-music-emu-0.6.5.zip') for n in z.namelist())
        else:assert not any(n.endswith(('.cpp','.h','.hpp','.pdb')) for n in z.namelist())
    digest=hashlib.sha256(path.read_bytes()).hexdigest();path.with_suffix('.zip.sha256').write_text(digest+'  '+name+'\n')
    results.append({'file':name,'bytes':path.stat().st_size,'sha256':digest,'CRC_pass':True})
(OUT/'NintendoMusicEditor-0.1-validation.json').write_text(json.dumps({'packages':results,'project_roundtrip_and_NSF_tests_pass':True,'hidden_frontend_selftest_pass':True,'actual_NSF_preview_with_dummy_audio':True,'real_audio_device_or_hardware_NES_tested':False,'NES_four_channel_only':True},indent=2))
print(json.dumps(results,indent=2))
