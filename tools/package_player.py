"""Ordinary-audio1.5.7 runtime and corresponding source; no supplied music or external upload."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,zipfile
SRC=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description='Package Nintendo Music Studio1.5.7 runtime and corresponding source.')
parser.add_argument('--build',type=Path,default=SRC/'build')
parser.add_argument('--output',type=Path,default=SRC/'dist')
args=parser.parse_args();BUILD=args.build.resolve();OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=True)
NAME='NintendoMusicStudio-1.5.7-Windows-x64';runtime=OUT/NAME;runtime.mkdir(exist_ok=True)
for n in ['NintendoMusicStudio.exe','gme.dll']:
 shutil.copy2(BUILD/'bin/Release'/n,runtime/n)
for n in ['README.md','README.en.md','LICENSE','THIRD_PARTY_NOTICES.md','SOURCE.md','VALIDATION.md','RELEASE.json','CHANGELOG.md','LICENSE-REVIEW.md','LICENSE-INVENTORY.json']:
 shutil.copy2(SRC/n,runtime/n)
shutil.copytree(SRC/'licenses',runtime/'licenses',dirs_exist_ok=True)
validation=OUT/'NintendoMusicStudio-1.5.7-validation';validation.mkdir(exist_ok=True)
subprocess.run([os.sys.executable,str(SRC/'tests/player_only_ui.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'ui')],check=True,timeout=40)
assert json.loads((validation/'ui/result.json').read_text())['pass']
subprocess.run([os.sys.executable,str(SRC/'tests/ui_about.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'about-packaged')],check=True,timeout=60)
assert json.loads((validation/'about-packaged/result.json').read_text())['pass']
subprocess.run([os.sys.executable,str(SRC/'tests/ui_controls.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'controls-packaged')],check=True,timeout=60)
assert json.loads((validation/'controls-packaged/result.json').read_text())['pass']
release=json.loads((SRC/'RELEASE.json').read_text())
assert release['product']=='Nintendo Music Studio' and release['version']=='1.5.7'
assert release['entrypoint']=='NintendoMusicStudio.exe'
assert 'NintendoMusicStudio.exe' in (SRC/'README.md').read_text(encoding='utf-8')
assert 'NintendoMusicStudio.exe' in (SRC/'README.en.md').read_text(encoding='utf-8')
assert not(SRC/'src/editor').exists()
cmake=(SRC/'CMakeLists.txt').read_text()
for name in ['music_composition','music_editor_ui','music_game_rip','src/editor']:
 assert name not in cmake
assert not any((BUILD/'bin/Release/NintendoMusicStudio.map').read_text(errors='replace').find(n)>=0 for n in ['runMusicEditor','SampleSongRenderer','SourceWindow','?selftest@Editor'])
dirs={'src','tests','third_party','docs','licenses','samples','.github','tools'}
files={'CMakeLists.txt','.clang-format','.gitignore','LICENSE','README.md','README.en.md','SOURCE.md','THIRD_PARTY_NOTICES.md','VALIDATION.md','RELEASE.json','CHANGELOG.md','LICENSE-REVIEW.md','LICENSE-INVENTORY.json'}
original_hashes={'03bbf089a707366253f647c8ff6c29bfe7f7d5967058f72739826babadd4b9e2','b8cb756ab0c23bfc8827d43f82de03fe7c83e49334a17137c715217d3c3bd6a6'}
results=[]
for root,stem in [(runtime,NAME),(SRC,'NintendoMusicStudio-1.5.7-Source')]:
 target=OUT/(stem+'.zip')
 with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in sorted(root.rglob('*')):
   rel=f.relative_to(root)
   if not f.is_file() or any(p in rel.parts for p in ['.git','__pycache__']):continue
   if root==SRC and rel.parts[0] not in dirs|files:continue
   assert 'editor' not in rel.parts
   assert f.name not in ['뷁.vgm','1793424975458.vgm','끄투엔 로비mp3.mp3']
   assert f.suffix.lower() not in ['.mp3','.minigsf','.gsflib','.gba','.nds','.miniusf','.usflib','.sf2']
   assert hashlib.sha256(f.read_bytes()).hexdigest() not in original_hashes
   z.write(f,Path(stem)/rel)
 with zipfile.ZipFile(target) as z:
  assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
  assert not any('audiodecoder.usf-' in n for n in z.namelist())
  assert not any('/third_party/libvgm/' in n or '/es5506-legacy/' in n for n in z.namelist())
  if root==runtime:assert len([n for n in z.namelist() if n.endswith('.exe')])==1
  else:
   for suffix in ['docs/RECORDING-157.json','tools/audit_licenses.py','docs/VGM-SOUND-1418.md','docs/VGM-TIMBRE-151.md','docs/VGM-TIMBRE-151.json','docs/VGM-LEVEL-152.json','docs/VGM-INTERPOLATION-153.json','docs/ORDINARY-AUDIO-154.json','docs/PLAYER-FIXES-155.json','docs/FULLSCREEN-156.json','tests/ui_fullscreen_repeat.py','tests/ui_fullscreen_autoplay.py','tests/ui_audio_repeat.py','tests/ui_late_repeat_matrix.py','tests/ui_recording_wave.py','src/formats/AudioFileBackend.cpp','tests/AudioFileTests.cpp','tests/audio_fixture_data.json','tests/ui_ordinary_audio.py','docs/ORDINARY-AUDIO-154.json','docs/PLAYER-FIXES-155.json','docs/FULLSCREEN-156.json','tests/ui_fullscreen_repeat.py','tests/ui_fullscreen_autoplay.py','tests/ui_audio_repeat.py','tests/ui_late_repeat_matrix.py','tests/ui_recording_wave.py','third_party/miniaudio/miniaudio.h','third_party/miniaudio/stb_vorbis.c','licenses/miniaudio-MIT.txt','licenses/stb-vorbis-MIT.txt','docs/VGM-SOUND-1418.json','third_party/lazyusf/LICENSE-EVIDENCE.json','third_party/lazyusf/usf/usf.c','third_party/lazyusf/Cog-Acknowledgements.json','licenses/lazyusf2-Cog-GPL-2.txt','licenses/lazyusf2-CIC-BSD-2-Clause.txt','licenses/selected-chips-BSD-3-Clause.txt','licenses/vgsound-ES5506-zlib.txt','licenses/Nuked-OPN2-LGPL-2.1.txt','src/ui/Window.cpp','tests/player_only_ui.py','src/formats/ExtendedVgmBackend.cpp','third_party/vgsound-es5506/src/es550x/es5506.cpp','third_party/licensed-chips/emu/cores/ym3438.c','src/formats/LicensedChips.cpp','LICENSE-REVIEW.md','LICENSE-INVENTORY.json']:
    assert any(n.endswith('/'+suffix) for n in z.namelist())
 sha=hashlib.sha256(target.read_bytes()).hexdigest();target.with_suffix('.zip.sha256').write_text(sha+'  '+target.name+'\n')
 results.append({'file':target.name,'bytes':target.stat().st_size,'SHA256':sha,'CRC_pass':True})
(OUT/'NintendoMusicStudio-1.5.7-packages.json').write_text(json.dumps({'version':'1.5.7','player_only':True,'release_type':'recording-cell-alignment','external_upload':False,'legal_certification':False,'license_work':'Prior licensed replacements retained; miniaudio0.11.23 and stb_vorbis MIT options/notices/full sources included','sound_baseline':'1.4.15','public_identity':'Nintendo Music Studio 1.5.7','digitally_signed':False,'README_and_license_notices_in_both_packages':True,'packages':results},indent=2))
print(json.dumps(results,indent=2))
