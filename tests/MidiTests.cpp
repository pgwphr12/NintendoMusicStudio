#include "formats/Midi.hpp"
#include "formats/MidiDevice.hpp"
#include "player/Player.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <stdexcept>
using namespace ngmv;
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
static std::vector<uint8_t> read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static double energy(const std::vector<Stereo>&v){double e=0;for(auto s:v){check(std::isfinite(s.l)&&std::isfinite(s.r),"Nonfinite MIDI");e+=s.l*s.l+s.r*s.r;}return e;}
int wmain(int argc,wchar_t**argv){try{
    midiSelectOutput(4);check(midiDeviceOutput(),"Explicit external route lost");midiSelectOutput(3);check(!midiDeviceOutput(),"Windows default incorrectly routes to waveform-less device");SetEnvironmentVariableW(L"NGMV_TEST_SESSION",L"owned-midi-unit");SetEnvironmentVariableW(L"SDL_AUDIODRIVER",L"dummy");
    auto root=std::filesystem::path(argv[1]);SetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",(root/"virtual-midi.conf").c_str());auto b=read(root/"channels.mid");auto song=parseMidi(b);
    check(song.used==3,"MIDI channels lost");check(song.endFrame==132000,"Tempo map duration wrong");
    check(song.events[2].frame==24000,"Running-status/tempo event position wrong");
    for(auto n:{"format2.mid","bad-running.mid","bad-vlq.mid","truncated.mid"}){bool failed=false;try{parseMidi(read(root/n));}catch(const std::exception&){failed=true;}check(failed,"Invalid MIDI accepted");}
    for(auto name:{"no-eot.mid","wrapped.mid","running-meta.mid","bank-fallback.mid","sysex.mid"}){auto bytes=read(root/name);check(identify(bytes,root/name)==Format::Midi,"MIDI wrapper identification lost");auto core=makeMidiBackend(bytes,root/name,root/"own.sf2");AudioBlock out;core->render(8192,out);check(energy(out.mix)>0,"Compatible MIDI stayed silent");}
    check(parseMidi(read(root/"sysex.mid")).sysex.size()==1,"Native vendor SysEx lost");
    bool limited=false;try{parseMidi(b,2);}catch(const std::exception&){limited=true;}check(limited,"Configurable bounded event limit ignored");
    auto active=virtualMidiFonts(root/"virtual-midi.conf");check(active.size()==1&&active[0]==root/"own.sf2","Disabled/active VirtualMIDISynth font selection wrong");
    check(virtualMidiFonts(root/"virtual-midi-utf16.conf")==active,"Unicode config differs");
    check(virtualMidiFonts(root/"virtual-midi-multiple.conf").size()==2,"Multiple active fonts silently lost");
    SetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",(root/"virtual-midi.conf").c_str());check(automaticMidiBank()==root/"own.sf2","Windows default ignored VMS SF2");check(virtualMidiFontsToImport()==active,"Explicit VMS import did not return active SF2");
    SetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",(root/"virtual-midi-multiple.conf").c_str());check(virtualMidiFontsToImport().size()==2,"Explicit VMS import lost enabled font choices");bool ambiguous=false;try{automaticMidiBank();}catch(const std::exception&){ambiguous=true;}check(ambiguous,"Ambiguous VMS configuration silently fell back to DLS");SetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",(root/"virtual-midi.conf").c_str());

    auto dense=parseMidi(read(root/"million.mid"));check(dense.events.size()>1000000,"Dense MIDI old event cap retained");
    auto smpte=read(root/"single.mid");smpte[12]=uint8_t(-25);smpte[13]=40;check(parseMidi(smpte).endFrame==119040,"SMPTE timing wrong");
    for(auto n:{"own.sf2","own.dls","empty-loop.sf2"}){
        auto bank=root/n;validateMidiBank(bank);auto backend=makeMidiBackend(b,root/"channels.mid",bank);check(backend->channels().size()==2,"Channels merged");AudioBlock a;backend->render(8192,a);check(energy(a.mix)>1,"Silent MIDI bank");auto first=a.mix;backend->selectTrack(0);backend->render(8192,a);check(a.mix.size()==first.size(),"Replay size");for(size_t i=0;i<first.size();i++)check(a.mix[i].l==first[i].l&&a.mix[i].r==first[i].r,"Nondeterministic MIDI replay");
        backend->seek(100);backend->render(512,a);auto seek=a.mix;backend->selectTrack(0);size_t skip=4800;while(skip){auto n=std::min<size_t>(512,skip);backend->render(n,a);skip-=n;}backend->render(512,a);double diff=0;for(size_t i=0;i<seek.size();i++)diff=std::max(diff,double(std::abs(seek[i].l-a.mix[i].l)));check(diff<.002,"MIDI seek not equivalent to synthesis replay");
        backend->selectTrack(0);backend->muteMask(3);backend->render(512,a);check(energy(a.mix)==0&&energy(a.voices[0])>0,"MIDI mute/stem isolation failed");uint64_t end=0;check(backend->decodedEndFrame(end)&&end==song.endFrame,"MIDI exact EOF missing");backend->seek(2750);backend->render(512,a);check(energy(a.mix)==0,"MIDI EOF not silent");
        std::cout<<n<<" synthesis/replay/seek/stems/mute/EOF PASS\n";
    }
    for(auto n:{"bad-bank.sf2","bad-sample.sf2","bad-index.sf2"}){bool fail=false;try{validateMidiBank(root/n);}catch(const std::exception&){fail=true;}check(fail,"Invalid SF2 accepted");}
    auto f=makeMidiBackend(read(root/"drums.mid"),root/"drums.mid",root/"own.sf2");AudioBlock a;f->render(4096,a);check(energy(a.mix)>0&&f->channels()[0].name=="Channel 10","MIDI channel10 drums missing");
    auto sixteen=makeMidiBackend(read(root/"sixteen.mid"),root/"sixteen.mid",root/"own.sf2");for(unsigned c=0;c<16;c++)check(sixteen->channels()[c].name=="Channel "+std::to_string(c+1),"MIDI channel label not simplified");sixteen->render(512,a);check(a.voices.size()==16,"Sixteen MIDI channels merged");for(const auto&v:a.voices)check(energy(v)>0,"Missing MIDI channel audio");
    for(size_t i=0;i<a.mix.size();i++){float l=0,r=0;for(const auto&v:a.voices){l+=v[i].l;r+=v[i].r;}check(l==a.mix[i].l&&r==a.mix[i].r,"MIDI stems do not reconstruct mix");}
    auto base=makeMidiBackend(read(root/"single.mid"),root/"single.mid",root/"own.sf2");auto program=makeMidiBackend(read(root/"program.mid"),root/"program.mid",root/"own.sf2");AudioBlock p;base->render(8192,a);program->render(8192,p);double difference=0;for(size_t i=0;i<a.mix.size();i++)difference+=std::abs(a.mix[i].l-p.mix[i].l);check(difference>1,"MIDI program change ignored");
    midiSoundFont(root/"own.sf2");check(midiSoundFont()==root/"own.sf2","Soundfont selection lost");auto global=makeBackend(b,root/"channels.mid");global->render(512,a);check(energy(a.mix)>0,"Selected font not used");midiSoundFont({});
    {auto prepared=makeMidiBackend(b,root/"channels.mid",root/"own.sf2");prepared->seek(500);Player restored;std::vector<ChannelState>states(2);states[0].mute=true;states[1].volume=.55f;restored.openPrepared(std::move(prepared),Format::Midi,24000,states);check(restored.seconds()==.5,"Prepared bank position reset");check(restored.states()[0].mute&&restored.states()[1].volume==.55f,"Prepared bank lost initial controls");}
    {SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",L"1");check(!virtualMidiInstalled(),"No-VMS test presence ignored");midiSoundFont(root/"missing.sf2");midiDeviceOutput(true);auto basic=makeBackend(b,root/"channels.mid");check(!basic->hasDeviceOutput(),"No VMS retained stale external output");check(basic->tracks()[0].game==windowsMidiBank().filename().u8string(),"No VMS retained stale custom bank");basic->render(512,a);check(energy(a.mix)>0,"Default-only synthesis is silent");SetEnvironmentVariableW(L"NGMV_TEST_NO_VMS",nullptr);midiSoundFont({});midiDeviceOutput(false);}
    validateMidiBank({});auto windows=makeMidiBackend(b,root/"channels.mid");windows->render(8192,a);check(energy(a.mix)>0,"Actual Windows default bank is silent");check(windows->tracks()[0].comment.find("Automatic MIDI bank:")!=std::string::npos,"Bank provenance missing");
    if(argc>2){auto file=std::filesystem::path(argv[2]);auto real=makeMidiBackend(read(file),file);double total=0;for(int i=0;i<600;i++){real->render(800,a);total+=energy(a.mix);}check(total>0,"Provided MIDI silent over10seconds");real->seek(5000);real->render(4096,a);check(energy(a.mix)>0,"Provided MIDI seek silent");std::cout<<"Provided MIDI: channels="<<real->channels().size()<<" length_ms="<<real->tracks()[0].lengthMs<<" first10s_energy="<<total<<" seek5s=PASS\n";}
    std::cout<<"Windows default bank, PCM SF2/DLS, MIDI format0/1, tempo/SMPTE, program/drums, malformed data: PASS\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
