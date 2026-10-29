"""One-program Studio runtime + complete corresponding source; no private music."""
from pathlib import Path
import hashlib,json,shutil,subprocess,os,zipfile
from PIL import Image
SRC=Path(__file__).resolve().parent.parent;WS=SRC.parents[2];BUILD=WS/'work/settings-build/ngmv';OUT=WS/'outputs'
RUNTIME=OUT/'NintendoMusicStudio-1.4.9-Windows-x64';RUNTIME.mkdir(exist_ok=True)
for name in ['NintendoGameMusicViewer.exe','gme.dll']:shutil.copy2(BUILD/'bin/Release'/name,RUNTIME/name)
assert len(list(RUNTIME.glob('*.exe')))==1
for n in ['LICENSE','THIRD_PARTY_NOTICES.md','SOURCE.md']:shutil.copy2(SRC/n,RUNTIME/n)
shutil.copytree(SRC/'licenses',RUNTIME/'licenses',dirs_exist_ok=True)
shutil.copy2(SRC/'docs/MUSIC-EDITOR.md',RUNTIME/'README.md');shutil.copy2(SRC/'docs/GBA-MIXER.md',RUNTIME/'GBA-MIXER.md')
(RUNTIME/'examples').mkdir(exist_ok=True)
for name in ['First light.ngms','demo-pal.nsf','demo-ntsc.nsf']:shutil.copy2(BUILD/'editor-test-output'/name,RUNTIME/'examples'/name)
env=os.environ.copy();env.pop('NGMV_TEST_GBA_SOURCE',None);env['SDL_AUDIODRIVER']='dummy';env['NGMV_TEST_SESSION']='studio-140-package'
validation=OUT/'NintendoMusicStudio-1.4.9-validation';validation.mkdir(exist_ok=True)
exe=RUNTIME/'NintendoGameMusicViewer.exe'
subprocess.run([str(exe),'--editor-selftest',str(validation/'ui')],check=True,timeout=60,env=env)
assert json.loads((validation/'ui/ui-test-result.json').read_text())['pass']
subprocess.run([str(exe),'--editor-snapshot',str(validation/'composer.bmp')],check=True,timeout=15,env=env)
subprocess.run([str(exe),'--snapshot',str(RUNTIME/'examples/demo-ntsc.nsf'),str(validation/'viewer.bmp')],check=True,timeout=20,env=env)
Image.open(validation/'ui/shared-editor.bmp').save(OUT/'NintendoMusicStudio-1.4.9-shared-editor.png')
Image.open(validation/'ui/sampled-editor.bmp').save(OUT/'NintendoMusicStudio-1.4.9-sampled-editor.png')
for n in ['composer','viewer']:Image.open(validation/(n+'.bmp')).save(OUT/('NintendoMusicStudio-1.4.9-'+n+'.png'))
report='# Nintendo Music Studio 1.4.9 — shared GBA piano roll and preview checkpoint8\n\nRecognized standard-m4a forward ROM PCM8/fixed8 GSF/MINIGSF now imports through the existing cancellable background note-import dialog into the SAME persistent Song/Note editor used for NES import and composition. No standalone app or waveform-analysis substitute. The piano roll supports1–64 sampled musical tracks with paged selection, overlapping notes and MIDI0–127; selected note/track sample selection, note movement/length/pitch/velocity, mute/volume, undo/redo, project3/4 save/reopen and common edited WAV are connected. New NES song and old project1/2 remain supported.\n\nPreview uses a prepared backend around the EXACT SampleSongRenderer used by WAV, with one-pass per-track waveform voices, phase-continuous controls and seek. It never compiles samples as fake NES NSF. Edited sampled music can return to the existing main waveform screen through an in-memory Song snapshot while the persistent editor retains its project. Original playback and edited preview stay distinct. The legacy32-bit mask cannot wrap onto tracks32–63; higher-track mute uses actual per-track voices. A sample change explicitly resets an initial offset if the new sample is shorter.\n\nPaint/dirty checks compare immutable bank/control identities and model fields instead of serializing megabytes on every mouse movement. Bounded project load supports64MiB with validation before replacing the current Song. Hidden own-data tests cover8track editing/sample choice/undo/reopen/preview/WAV/waveform Player handoff, malformed load retention, new-NES channel reset,1.2MiB project and1000 dirty checks. A separate64track test checks exact sampler/preview samples, per-track sum, seek, end, mask no-wrap, real dummy callback/pause/high-track mute/history. Actual private Overworld six-second import uses the production dialog worker and cancellation, edits five musical properties in the same editor, save/reopen/preview/WAV and a private screenshot. These are dummy audio tests, not listening/hardware validation.\n\nKnown subset only: GBA PSG notes/activity (including muted sequencer START), custom drivers, RAM/compressed/reverse samples, timer1 routing and irregular captures are rejected. Imported dry mixer-lock timing is not calibrated exact original DAC delay, filtering or reverb. Capture boundaries may truncate tails; import a suitable range. All game formats are not universally editable, and saving a modified GSF/ROM is not implemented. Large project parsing/save and heavy model edits still have synchronous work despite the dirty-check improvement. Main waveform layouts with many tracks may be compact.\n\n1.4.1 and all previous releases are preserved. One-EXE runtime and separate full corresponding source include modified LGPL/GPL core/licenses and own tests, CRC/SHA and exact-byte own-GSF whitelist. Private game music/banks/projects/WAV/screenshots and ROMs remain outside packages.\n'
(validation/'REPORT.md').write_text(report,encoding='utf-8')
dirs={'src','tests','third_party','docs','licenses','samples','.github','tools'}
files={'CMakeLists.txt','.clang-format','.gitignore','LICENSE','README.md','README.en.md','SOURCE.md','THIRD_PARTY_NOTICES.md','VALIDATION.md'}
results=[]
for root,name in [(RUNTIME,'NintendoMusicStudio-1.4.9-Windows-x64.zip'),(SRC,'NintendoMusicStudio-1.4.9-Source.zip')]:
 p=OUT/name
 with zipfile.ZipFile(p,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in sorted(root.rglob('*')):
   rel=f.relative_to(root)
   if not f.is_file() or '__pycache__' in rel.parts:continue
   if root==SRC and rel.parts[0] not in dirs|files:continue
   z.write(f,Path(root.name if root==RUNTIME else 'NintendoMusicStudio-1.4.9-Source')/rel)
 with zipfile.ZipFile(p) as z:
  assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
  if root==RUNTIME:assert len([n for n in z.namelist() if n.endswith('.exe')])==1
  else:
   for key in ['src/editor/SampleBackend.hpp','src/editor/SampleBackend.cpp','tests/SampleBackendTests.cpp','src/editor/GbaImport.hpp','src/editor/GbaImport.cpp','tests/GbaImportTests.cpp','tests/AutomationTests.cpp','src/formats/GbaMixCapture.hpp','tests/GbaMixTests.cpp','src/editor/GbaRecording.hpp','src/editor/GbaRecording.cpp','tests/GbaRecordingTests.cpp','src/editor/SampleSong.hpp','src/editor/SampleSong.cpp','tests/SampleSongTests.cpp','src/core/GbaInspection.hpp','src/formats/GbaRomSample.hpp','tests/GbaSampleTests.cpp','src/formats/GbaVoiceCapture.hpp','tests/GbaInspectionTests.cpp','src/editor/NoteImport.cpp','src/editor/NoteImportWindow.cpp','src/editor/SourceEdit.cpp','src/editor/SourceWindow.cpp','src/editor/GameRip.cpp','src/editor/Editor.cpp','src/formats/GbaMixerTap.hpp','third_party/apply_gme_inspect.cmake','third_party/viogsf/vbam/gba/GBAinline.h','third_party/game-music-emu-0.6.5.zip']:assert any(n.endswith(key) for n in z.namelist()),key
  assert not any('gba-mixer-private' in n or 'studio-02-validation' in n for n in z.namelist())
  # Only the two independently generated ARM test GSFs may appear as GSF
  # assets. Compare their exact bytes with the own fixture generator output.
  own_gsf={'samples/Original-GBA-Test.gsf':'channels.gsf','samples/Original-GBA-PCM-Test.gsf':'pcm.gsf'}
  for n in z.namelist():
   lower=n.lower()
   assert not lower.endswith(('.minigsf','.gsflib','.gba','.nds','.miniusf','.usflib','.sf2')),n
   if lower.endswith('.gsf'):
    rel='/'.join(n.split('/')[1:]);assert root==SRC and rel in own_gsf,n
    assert z.read(n)==(BUILD/'fixtures'/own_gsf[rel]).read_bytes(),n
 digest=hashlib.sha256(p.read_bytes()).hexdigest();p.with_suffix('.zip.sha256').write_text(digest+'  '+name+'\n')
 results.append({'file':name,'bytes':p.stat().st_size,'SHA256':digest,'CRC_pass':True})
(OUT/'NintendoMusicStudio-1.4.9-packages.json').write_text(json.dumps({'one_program':True,'packages':results,'report':str(validation/'REPORT.md')},indent=2))
print(json.dumps(results,indent=2))
