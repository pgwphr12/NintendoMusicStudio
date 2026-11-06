#include "formats/Midi.hpp"
#include "formats/MidiDevice.hpp"
#include <fstream>
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <iostream>
#include <chrono>
#include <cmath>
using namespace ngmv;
int wmain(int argc,wchar_t**argv){const char*stage="read";try{if(argc<2)return 2;auto start=std::chrono::steady_clock::now();auto path=std::filesystem::path(argv[1]);auto bytes=readMusicFile(path);stage="parse";auto song=parseMidi(bytes);std::cout<<"parse PASS events="<<song.events.size()<<" duration_ms="<<song.endFrame*1000/InternalRate<<" channels_mask="<<song.used<<"\n";stage="bank/render";midiDeviceOutput(false);bool full=argc>2&&!wcscmp(argv[2],L"--full");bool imported=argc>2&&!wcscmp(argv[2],L"--import");auto bank=imported?virtualMidiFontsToImport().at(0):argc>3&&full?std::filesystem::path(argv[3]):argc>2&&!full?std::filesystem::path(argv[2]):automaticMidiBank();DWORD id=UINT_MAX,flags=0;auto q=midiOutMessage(reinterpret_cast<HMIDIOUT>(UINT_PTR(MIDI_MAPPER)),DRVM_MAPPER_PREFERRED_GET,reinterpret_cast<DWORD_PTR>(&id),reinterpret_cast<DWORD_PTR>(&flags));MIDIOUTCAPSW cap{};auto cr=midiOutGetDevCapsW(id,&cap,sizeof(cap));std::cout<<"mapper query="<<q<<" id="<<id<<" caps="<<cr<<" virtual="<<(wcsstr(cap.szPname,L"VirtualMIDISynth")!=nullptr)<<"\n";
 std::cout<<"imported="<<imported<<" selected_bank="<<bank.filename().u8string()<<"\n";auto backend=makeMidiBackend(bytes,path,bank);AudioBlock out;double energy=0;for(uint64_t i=0;i<(full?(song.endFrame+4799)/4800:100);i++){backend->render(4800,out);for(auto v:out.mix)energy+=v.l*v.l+v.r*v.r;}backend->seek(5000);backend->render(4096,out);double seek=0;for(auto v:out.mix)seek+=v.l*v.l+v.r*v.r;if(!std::isfinite(energy)||!std::isfinite(seek))throw std::runtime_error("Nonfinite samples");std::cout<<"render energy="<<energy<<" seek5s energy="<<seek<<" bank_bytes="<<std::filesystem::file_size(bank)<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" PASS\n";return 0;}catch(const std::exception&e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}}
