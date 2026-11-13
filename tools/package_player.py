"""Stable/test player runtime and corresponding source; no supplied music or external upload."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,zipfile,re
SRC=Path(__file__).resolve().parent.parent
release=json.loads((SRC/'RELEASE.json').read_text(encoding='utf-8'))
version=release['version'];test=release.get('channel')=='test'
if test:assert re.fullmatch(r'\d+\.\d+\.\d+-beta [1-9]\d*',version),'Test display version must use X.Y.Z-beta N'
PACKAGE='NintendoMusicStudio-'+version.replace(' ','-')
parser=argparse.ArgumentParser(description='Package Nintendo Music Studio1.6.2 runtime and corresponding source.')
parser.add_argument('--build',type=Path,default=SRC/'build')
parser.add_argument('--exe',type=Path,help='Use this already-built player executable instead of BUILD/bin/Release/NintendoMusicStudio.exe')
parser.add_argument('--output',type=Path,default=SRC/'dist')
parser.add_argument('--skip-ui',action='store_true',help='Test iteration: retain prior/user-confirmed UI baseline; report no fresh UI checks')
args=parser.parse_args();BUILD=args.build.resolve();EXE=(args.exe.resolve() if args.exe else BUILD/'bin/Release/NintendoMusicStudio.exe');OUT=args.output.resolve();OUT.mkdir(parents=True,exist_ok=True)
NAME=PACKAGE+'-Windows-x64';runtime=OUT/NAME;runtime.mkdir(exist_ok=True)
shutil.copy2(EXE,runtime/'NintendoMusicStudio.exe')
for n in ['README.md','README.en.md','LICENSE','THIRD_PARTY_NOTICES.md','SOURCE.md','VALIDATION.md','RELEASE.json','CHANGELOG.md','LICENSE-REVIEW.md','LICENSE-INVENTORY.json','RELEASE-NOTES-1.6.md','RELEASE-NOTES-1.6.1h.md','RELEASE-NOTES-1.6.2.md','RELEASE-NOTES-1.6.3-1-beta.md','RELEASE-NOTES-1.6.3-beta-2.md','RELEASE-NOTES-1.6.3-beta-10.md','RELEASE-NOTES-1.6.3-beta-9.md','RELEASE-NOTES-1.6.3-beta-8.md','RELEASE-NOTES-1.6.3-beta-7.md','RELEASE-NOTES-1.6.3-beta-6.md','RELEASE-NOTES-1.6.3-beta-5.md','RELEASE-NOTES-1.6.3-beta-4.md','RELEASE-NOTES-1.6.3-beta-3.md','RELEASE-NOTES-1.6.2-2.md','RELEASE-NOTES-1.6.2-3.md','RELEASE-NOTES-1.6.2-4.md','RELEASE-NOTES-1.6.2-5.md','RELEASE-NOTES-1.6.2-6.md','RELEASE-NOTES-1.6.2-7.md','RELEASE-NOTES-1.6.2-8.md','RELEASE-NOTES-1.6.2-9.md','RELEASE-NOTES-1.6.2-10.md','RELEASE-NOTES-1.6.2-11.md','RELEASE-NOTES-1.6.2-12.md']:
 shutil.copy2(SRC/n,runtime/n)
shutil.copytree(SRC/'licenses',runtime/'licenses',dirs_exist_ok=True)
shutil.copy2(SRC/'docs/ROADMAP-BETA-9-WII.md',runtime/'ROADMAP-BETA-9-WII.md')
validation=OUT/(PACKAGE+'-validation');validation.mkdir(exist_ok=True)
subprocess.run([os.sys.executable,str(SRC/'tools/check_standalone.py'),str(runtime/'NintendoMusicStudio.exe')],check=True)
standalone=validation/'exe-only';standalone.mkdir(exist_ok=True)
assert not any(standalone.glob('*.dll'))
shutil.copy2(runtime/'NintendoMusicStudio.exe',standalone/'NintendoMusicStudio.exe')
assert b'GNU GENERAL PUBLIC LICENSE' in (standalone/'NintendoMusicStudio.exe').read_bytes()
if not args.skip_ui:
 subprocess.run([os.sys.executable,str(SRC/'tests/player_only_ui.py'),str(standalone/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'standalone-ui')],check=True,timeout=40)

 subprocess.run([os.sys.executable,str(SRC/'tests/player_only_ui.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'ui')],check=True,timeout=40)
 assert json.loads((validation/'ui/result.json').read_text())['pass']
 subprocess.run([os.sys.executable,str(SRC/'tests/ui_about.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'about-packaged')],check=True,timeout=60)
 assert json.loads((validation/'about-packaged/result.json').read_text())['pass']
 subprocess.run([os.sys.executable,str(SRC/'tests/ui_controls.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/fds.nsf'),str(validation/'controls-packaged')],check=True,timeout=60)
 assert json.loads((validation/'controls-packaged/result.json').read_text())['pass']
 subprocess.run([os.sys.executable,str(SRC/'tests/ui_visibility.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures'),str(validation/'visibility-packaged')],check=True,timeout=60)
 assert json.loads((validation/'visibility-packaged/result.json').read_text())['pass']
 subprocess.run([os.sys.executable,str(SRC/'tests/ui_midi.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/midi'),str(validation/'midi-packaged')],check=True,timeout=60)
 assert json.loads((validation/'midi-packaged/result.json').read_text())['pass']
 subprocess.run([os.sys.executable,str(SRC/'tests/ui_midi_live.py'),str(runtime/'NintendoMusicStudio.exe'),str(BUILD/'fixtures/midi'),str(validation/'live-bank-packaged')],check=True,timeout=45)
 assert json.loads((validation/'live-bank-packaged/result.json').read_text())['pass']
release=json.loads((SRC/'RELEASE.json').read_text())
assert release['product']=='Nintendo Music Studio' and release['version']==version
assert release['entrypoint']=='NintendoMusicStudio.exe'
assert 'NintendoMusicStudio.exe' in (SRC/'README.md').read_text(encoding='utf-8')
assert 'NintendoMusicStudio.exe' in (SRC/'README.en.md').read_text(encoding='utf-8')
assert not(SRC/'src/editor').exists()
cmake=(SRC/'CMakeLists.txt').read_text()
for name in ['music_composition','music_editor_ui','music_game_rip','src/editor']:
 assert name not in cmake
assert not any(EXE.with_suffix('.map').read_text(errors='replace').find(n)>=0 for n in ['runMusicEditor','SampleSongRenderer','SourceWindow','?selftest@Editor'])
dirs={'src','tests','third_party','docs','licenses','samples','.github','tools'}
files={'CMakeLists.txt','.clang-format','.gitignore','LICENSE','README.md','README.en.md','SOURCE.md','THIRD_PARTY_NOTICES.md','VALIDATION.md','RELEASE.json','CHANGELOG.md','LICENSE-REVIEW.md','LICENSE-INVENTORY.json','RELEASE-NOTES-1.6.md','RELEASE-NOTES-1.6.1h.md','RELEASE-NOTES-1.6.2.md','RELEASE-NOTES-1.6.3-1-beta.md','RELEASE-NOTES-1.6.3-beta-2.md','RELEASE-NOTES-1.6.3-beta-10.md','RELEASE-NOTES-1.6.3-beta-9.md','RELEASE-NOTES-1.6.3-beta-8.md','RELEASE-NOTES-1.6.3-beta-7.md','RELEASE-NOTES-1.6.3-beta-6.md','RELEASE-NOTES-1.6.3-beta-5.md','RELEASE-NOTES-1.6.3-beta-4.md','RELEASE-NOTES-1.6.3-beta-3.md','RELEASE-NOTES-1.6.2-2.md','RELEASE-NOTES-1.6.2-3.md','RELEASE-NOTES-1.6.2-4.md','RELEASE-NOTES-1.6.2-5.md','RELEASE-NOTES-1.6.2-6.md','RELEASE-NOTES-1.6.2-7.md','RELEASE-NOTES-1.6.2-8.md','RELEASE-NOTES-1.6.2-9.md','RELEASE-NOTES-1.6.2-10.md','RELEASE-NOTES-1.6.2-11.md','RELEASE-NOTES-1.6.2-12.md'}
original_hashes={'3c5359904516ea87a22391aded6520057d3796c875f14fcafc2781f06d81071c','45557026edc32018b5125c4e62c3cb45af71f5fd99db8692c7175f44b01df9c1','cdb68939395b828d90fc58abcc57b2b98555f78fba20a6fb218f7a85292fb805','03bbf089a707366253f647c8ff6c29bfe7f7d5967058f72739826babadd4b9e2','b8cb756ab0c23bfc8827d43f82de03fe7c83e49334a17137c715217d3c3bd6a6'}
files.add("RELEASE-NOTES-1.6.3-beta-8.md")
results=[]
for root,stem in [(runtime,NAME),(SRC,PACKAGE+'-Source')]:
 target=OUT/(stem+'.zip')
 with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for f in sorted(root.rglob('*')):
   rel=f.relative_to(root)
   if not f.is_file() or any(p in rel.parts for p in ['.git','__pycache__']):continue
   if root==SRC and rel.parts[0] not in dirs|files:continue
   assert 'editor' not in rel.parts
   assert f.name not in ['slogic.vgm','1794124986077.vgm','뷁.vgm','1793424975458.vgm','ay8930.vgm','끄투엔 로비mp3.mp3']
   assert f.suffix.lower() not in ['.nes','.mp3','.minigsf','.gsflib','.gba','.nds','.miniusf','.usflib','.sf2','.dls','.mid','.midi']
   assert hashlib.sha256(f.read_bytes()).hexdigest() not in original_hashes
   z.write(f,Path(stem)/rel)
 with zipfile.ZipFile(target) as z:
  assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
  assert not any('audiodecoder.usf-' in n for n in z.namelist())
  assert not any('/third_party/libvgm/' in n or '/es5506-legacy/' in n for n in z.namelist())
  if root==runtime:
   assert len([n for n in z.namelist() if n.endswith('.exe')])==1
   assert not any(n.lower().endswith('.dll') for n in z.namelist())
  else:
   for suffix in ['docs/GAMECUBE-WII-1.6.3-beta-9.json','RELEASE-NOTES-1.6.3-beta-10.md','RELEASE-NOTES-1.6.3-beta-9.md','docs/VISUAL-OPL-1.6.3-beta-8.json','tests/ui_continuous_visual.py','src/visualizer/WaveformTail.hpp','RELEASE-NOTES-1.6.3-beta-8.md','docs/MIDI-LOCAL-1.6.3-beta-7.json','tests/MidiLocalBankTests.cpp','tests/ui_midi_local.py','docs/WAVE-SMOOTH-1.6.3-beta-6.json','tests/ui_wave_performance.py','docs/WAVE-GBA-1.6.3-beta-6-revision.json','src/formats/GbaSongUsage.hpp','tests/GbaSongUsageTests.cpp','tests/WaveTrackingTests.cpp','src/formats/WholeTrackUsage.cpp','src/formats/WholeTrackUsage.hpp','tests/YmUsageTests.cpp','tests/WholeTrackUsageTests.cpp','tests/ui_show_all_channels.py','docs/YM-USAGE-1.6.3-beta-7.json','tests/OplUsageTests.cpp','docs/WAVE-FREE-1.6.3-beta-6.json','tests/WaveStabilityTests.cpp','tests/LegacyEnvelope.cpp','src/visualizer/Envelope.cpp','src/visualizer/Envelope.hpp','docs/WAVE-STABILITY-1.6.3-beta-5.json','docs/NO-VMS-1.6.3-beta-4.json','tests/FurnaceChipTests.cpp','third_party/licensed-chips/emu/cores/sn76496.c','third_party/licensed-chips/emu/cores/sn764intf.h','third_party/licensed-chips/emu/cores/sn76496.h','third_party/licensed-chips/UPSTREAM.json','src/formats/FurnaceEsRate.hpp','tests/EsPitchTests.cpp','docs/ES-PITCH-1.6.3-beta-3.json','tests/ui_remote_independent.py','docs/REMOTE-1.6.3-beta-2.json','src/ui/HiddenChannels.cpp','tests/ui_hidden_channels.py','docs/HIDDEN-CHANNELS-1.6.3-1-beta.json','docs/VERSIONING.md','docs/RELEASE-1.6.2.json','tests/ui_spectrum_choices.py','docs/SPECTRUM-CHOICES-1.6.2-12.json','src/visualizer/SpectrogramHistory.hpp','docs/SCROLLING-SPECTROGRAM-1.6.2-11.json','src/visualizer/Spectrum.hpp','src/visualizer/AudioTap.hpp','src/ui/SpectrumView.cpp','src/visualizer/SpectrumCurve.hpp','tests/SpectrumTests.cpp','tests/ui_spectrum.py','docs/SPECTRUM-STYLES-1.6.2-10.json','docs/SPECTRUM-1.6.2-9.json','src/audio/GraphicEqualizer.cpp','third_party/mesence-eq/orfanidis_eq.h','third_party/mesence-eq/Equalizer.cpp','third_party/mesence-eq/Equalizer.h','third_party/mesence-eq/UPSTREAM.json','third_party/mesence-eq/LICENSE','licenses/MesenCE-GPL-3.0.txt','tests/ui_mesence_equalizer.py','tests/ui_footer_buttons.py','src/audio/GraphicEqualizer.hpp','src/ui/ExtraWindows.cpp','tests/EqualizerTests.cpp','tests/ui_remote_equalizer.py','tests/CommonVolumeTests.cpp','tests/ui_common_volume.py','tests/RomStabilityTests.cpp','tests/ui_rom_stability.py','tests/GbaRomVoiceTests.cpp','tests/GbsAgbTests.cpp','third_party/gme-agb-patch/Gb_Apu.cpp','third_party/gme-agb-patch/Gb_Apu.h','third_party/gme-agb-patch/Gb_Oscs.cpp','third_party/gme-agb-patch/Gb_Oscs.h','third_party/gme-agb-patch/Gbs_Emu.cpp','third_party/gme-agb-patch/Gbs_Emu.h','third_party/gme-agb-patch/UPSTREAM.json','docs/MESENCE-EQ-1.6.2-8.json','docs/FOOTER-1.6.2-7.json','docs/COMMON-VOLUME-1.6.2-6.json','docs/ROM-STABILITY-1.6.2-5.json','docs/GBA-GB-1.6.2-4.json','docs/NES-ROM-1.6.2-3.json','RELEASE-NOTES-1.6.2-3.md','RELEASE-NOTES-1.6.2-4.md','RELEASE-NOTES-1.6.2-5.md','RELEASE-NOTES-1.6.2-6.md','RELEASE-NOTES-1.6.2-7.md','RELEASE-NOTES-1.6.2-8.md','RELEASE-NOTES-1.6.2-9.md','RELEASE-NOTES-1.6.2-10.md','RELEASE-NOTES-1.6.2-11.md','RELEASE-NOTES-1.6.2-12.md','src/formats/NesRom.cpp','src/formats/NesRom.hpp','src/formats/NesRomProfiles.hpp','tests/NesRomTests.cpp','tests/ui_nes_rom.py','docs/ROM-MUSIC-1.6.2-2.json','RELEASE-NOTES-1.6.2-2.md','docs/ROM-MUSIC-1.6.2.json','RELEASE-NOTES-1.6.2.md','RELEASE-NOTES-1.6.3-1-beta.md','RELEASE-NOTES-1.6.3-beta-2.md','RELEASE-NOTES-1.6.3-beta-10.md','RELEASE-NOTES-1.6.3-beta-9.md','RELEASE-NOTES-1.6.3-beta-8.md','RELEASE-NOTES-1.6.3-beta-7.md','RELEASE-NOTES-1.6.3-beta-6.md','RELEASE-NOTES-1.6.3-beta-5.md','RELEASE-NOTES-1.6.3-beta-4.md','RELEASE-NOTES-1.6.3-beta-3.md','src/formats/GbaRom.cpp','src/formats/GbaRom.hpp','tests/GbaRomTests.cpp','tests/ui_rom.py','docs/NO-VMS-1.6.1h.json','RELEASE-NOTES-1.6.1h.md','docs/RELEASE-1.6.json','RELEASE-NOTES-1.6.md','docs/VMS-LIVE-1512h4.json','tests/ui_midi_live.py','docs/VMS-IMPORT-1512h3.json','docs/MIDI-DEFAULT-1512h2.json','docs/MIDI-HOTFIX-1512h.json','src/formats/MidiAutoBank.cpp','tools/midi_compatibility_probe.cpp','docs/MIDI-STATIC-1512.json','src/formats/MidiDevice.cpp','src/formats/MidiDevice.hpp','src/app/embedded_licenses.rc.in','tests/MidiDeviceTests.cpp','docs/VISIBILITY-1511.json','tests/ChannelVisibilityTests.cpp','tests/ui_visibility.py','docs/CHIP-VOICES-1510.json','third_party/furnace-ay8930/ay8910.cpp','third_party/furnace-ay8930/ay8910.h','third_party/furnace-ay8930/UPSTREAM.json','licenses/AY8930-BSD-3-Clause.txt','tests/ChipVoiceTests.cpp','tests/make_chip_fixtures.py','tests/ui_chip_voices.py','docs/MIDI-159.json','src/formats/MidiBackend.cpp','src/formats/MidiBank.inl','src/formats/MidiParser.cpp','src/formats/Midi.hpp','third_party/tinysoundfont/tsf.h','third_party/tinysoundfont/UPSTREAM.json','licenses/TinySoundFont-MIT.txt','tests/MidiTests.cpp','tests/make_midi_fixtures.py','tests/ui_midi.py','docs/AUDIO-OPEN-158.json','tests/ui_audio_open_play.py','docs/RECORDING-157.json','tools/audit_licenses.py','docs/VGM-SOUND-1418.md','docs/VGM-TIMBRE-151.md','docs/VGM-TIMBRE-151.json','docs/VGM-LEVEL-152.json','docs/VGM-INTERPOLATION-153.json','docs/ORDINARY-AUDIO-154.json','docs/PLAYER-FIXES-155.json','docs/FULLSCREEN-156.json','tests/ui_fullscreen_repeat.py','tests/ui_fullscreen_autoplay.py','tests/ui_audio_repeat.py','tests/ui_late_repeat_matrix.py','tests/ui_recording_wave.py','src/formats/AudioFileBackend.cpp','tests/AudioFileTests.cpp','tests/audio_fixture_data.json','tests/ui_ordinary_audio.py','docs/ORDINARY-AUDIO-154.json','docs/PLAYER-FIXES-155.json','docs/FULLSCREEN-156.json','tests/ui_fullscreen_repeat.py','tests/ui_fullscreen_autoplay.py','tests/ui_audio_repeat.py','tests/ui_late_repeat_matrix.py','tests/ui_recording_wave.py','third_party/miniaudio/miniaudio.h','third_party/miniaudio/stb_vorbis.c','licenses/miniaudio-MIT.txt','licenses/stb-vorbis-MIT.txt','docs/VGM-SOUND-1418.json','third_party/lazyusf/LICENSE-EVIDENCE.json','third_party/lazyusf/usf/usf.c','third_party/lazyusf/Cog-Acknowledgements.json','licenses/lazyusf2-Cog-GPL-2.txt','licenses/lazyusf2-CIC-BSD-2-Clause.txt','licenses/selected-chips-BSD-3-Clause.txt','licenses/vgsound-ES5506-zlib.txt','licenses/Nuked-OPN2-LGPL-2.1.txt','src/ui/Window.cpp','tests/player_only_ui.py','src/formats/ExtendedVgmBackend.cpp','third_party/vgsound-es5506/src/es550x/es5506.cpp','third_party/licensed-chips/emu/cores/ym3438.c','src/formats/LicensedChips.cpp','LICENSE-REVIEW.md','LICENSE-INVENTORY.json']:
    assert any(n.endswith('/'+suffix) for n in z.namelist())
 sha=hashlib.sha256(target.read_bytes()).hexdigest();target.with_suffix('.zip.sha256').write_text(sha+'  '+target.name+'\n')
 results.append({'file':target.name,'bytes':target.stat().st_size,'SHA256':sha,'CRC_pass':True})
(OUT/(PACKAGE+'-packages.json')).write_text(json.dumps({'version':version,'player_only':True,'ui_tests_run':not args.skip_ui,'release_type':release['release_type'],'external_upload':False,'legal_certification':False,'license_work':'Prior licensed replacements retained; miniaudio0.11.23, stb_vorbis and TinySoundFont MIT and AY8930 BSD3 notices/full sources included; Windows and user banks excluded','sound_baseline':'1.4.15','public_identity':'Nintendo Music Studio '+version,'digitally_signed':False,'README_and_license_notices_in_both_packages':True,'packages':results},indent=2))
print(json.dumps(results,indent=2))
