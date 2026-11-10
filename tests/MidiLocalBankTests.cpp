#include "formats/Midi.hpp"
#include "formats/MidiDevice.hpp"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cwctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace ngmv;
namespace fs=std::filesystem;
void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}
double energy(const AudioBlock&b){double e=0;for(auto v:b.mix){need(std::isfinite(v.l)&&std::isfinite(v.r),"Nonfinite synthesis");e+=v.l*v.l+v.r*v.r;}return e;}
int wmain(int argc,wchar_t**argv){try{
 auto root=fs::absolute(argv[1]),cases=root/"local-cases";fs::create_directories(cases);
 SetEnvironmentVariableW(L"NGMV_TEST_SESSION",L"local-midi-owned");SetEnvironmentVariableW(L"SDL_AUDIODRIVER",L"dummy");
 auto copy=[&](const fs::path&a,const fs::path&b){fs::copy_file(a,b,fs::copy_options::overwrite_existing);};
 auto bad=[&](const fs::path&p){std::ofstream(p,std::ios::binary)<<"broken own bank";};
 auto source=root/"channels.mid",file=cases/L"\uac19\uc740 \uc774\ub984.MID",dls=file,sf2=file;dls.replace_extension(".dls");sf2.replace_extension(".sf2");copy(source,file);copy(root/"own2.dls",dls);copy(root/"own.sf2",sf2);
 auto bytes=readMusicFile(file);AudioBlock block;unsigned checks=0;
 auto expect=[&](const fs::path&expected,bool local){auto b=makeMidiBackend(bytes,file);need(b->usesLocalMidiBank()==local,"Wrong bank provenance");need(b->tracks()[0].game==expected.filename().u8string(),"Wrong bank selected");need(!b->hasDeviceOutput(),"Sibling bank sent to MIDI device");b->render(8192,block);need(energy(block)>0,"Selected bank silent");++checks;return b;};
 SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",L"1");midiSoundFont(cases/"stale-missing.sf2");midiDeviceOutput(true);
 expect(dls,true);bad(dls);expect(sf2,true);copy(root/"own2.dls",dls);bad(sf2);expect(dls,true);
 fs::remove(dls);copy(root/"own.sf2",sf2);expect(sf2,true);
 // Both invalid: try the configured VMS bank, regardless of a stale saved font.
 auto config=cases/"VirtualMIDISynth.conf",vms=cases/"vms.sf2";copy(root/"own.sf2",vms);
 {std::ofstream f(config);f<<"[SoundFonts]\nsf1=vms.sf2\nsf1.enabled=1\n";}
 bad(dls);bad(sf2);SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",nullptr);SetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",config.c_str());expect(vms,false);
 // No VMS installed: invalid siblings fall back to existing Windows default.
 SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",L"1");expect(windowsMidiBank(),false);
 // A merely similar basename must not match.
 fs::remove(dls);fs::remove(sf2);auto unrelated=cases/L"\uac19\uc740 \uc774\ub984-extra.dls";copy(root/"own2.dls",unrelated);need(localMidiBankCandidates(file).empty(),"Prefix match bank selected");
 // Explicit one-off font choice still overrides automatic local selection.
 copy(root/"own2.dls",dls);auto explicitBank=makeMidiBackend(bytes,file,root/"own.sf2");need(!explicitBank->usesLocalMidiBank()&&explicitBank->tracks()[0].game=="own.sf2","Explicit choice ignored");++checks;
 auto silent=makeMidiBackend(readMusicFile(root/"empty.mid"),root/"empty.mid",root/"own2.dls");silent->render(8192,block);need(silent->channels().empty()&&energy(block)==0,"Empty MIDI fabricated notes");++checks;
 midiSoundFont({});midiDeviceOutput(false);SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",nullptr);SetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",nullptr);
 std::cout<<"PASS "<<checks<<" sibling priority/fallback/DLS2/Unicode/no-VMS/explicit cases\n";
 if(argc>3){
  SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",L"1");std::vector<fs::path>files;
  for(auto&entry:fs::directory_iterator(argv[2])){auto e=entry.path().extension().wstring();std::transform(e.begin(),e.end(),e.begin(),::towlower);if(entry.is_regular_file()&&(e==L".mid"||e==L".midi"))files.push_back(entry.path());}
  std::sort(files.begin(),files.end());need(!files.empty(),"No real MIDI files");std::ofstream report{fs::path(argv[3])};report<<"index,channels,frames,energy,bank,seconds,status\n";
  unsigned passed=0;for(size_t i=0;i<files.size();++i){auto started=std::chrono::steady_clock::now();try{
   auto data=readMusicFile(files[i]);auto song=parseMidi(data);auto autoBank=makeMidiBackend(data,files[i]);auto d=files[i];d.replace_extension(".dls");
   need(autoBank->usesLocalMidiBank()&&autoBank->tracks()[0].game==d.filename().u8string(),"Real local DLS not selected");
   double sum=0;for(uint64_t left=song.endFrame;left;){size_t n=size_t(std::min<uint64_t>(left,4096));autoBank->render(n,block);sum+=energy(block);left-=n;}need(song.used?sum>0:sum==0,"Real whole-song MIDI energy contradicts note presence");
   autoBank->seek(int(song.endFrame/96));autoBank->render(4096,block);energy(block);autoBank->selectTrack(0);autoBank->render(4096,block);energy(block);
   auto sf=files[i];sf.replace_extension(".sf2");auto alternative=makeMidiBackend(data,files[i],sf);for(unsigned left=48000;left;){unsigned n=std::min(left,4096u);alternative->render(n,block);energy(block);left-=n;}
   need(data==readMusicFile(files[i]),"Original MIDI changed");++passed;
   report<<i+1<<','<<autoBank->channels().size()<<','<<song.endFrame<<','<<sum<<",DLS,"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<','<<(song.used?"PASS":"SILENT_EMPTY")<<std::endl;
  }catch(const std::exception&e){report<<i+1<<",0,0,0,ERROR,0,"<<e.what()<<std::endl;}}
  std::cout<<"Actual folder full-song synthesis+middle seek/restart+SF2 render: "<<passed<<'/'<<files.size()<<'\n';need(passed==files.size(),"Some supplied MIDI failed; see private matrix");
 }
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
