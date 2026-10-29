#pragma once
#include "editor/NoteImport.hpp"
static void checkSharedNotes(const std::filesystem::path&out){
 using namespace ngmv;using namespace ngmv::editor;auto need=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};
 auto write=[&](const std::filesystem::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());};
 Song source;source.bars=1;source.resolution=120;source.instruments[0].volume=15;source.put({0,17,33,64,12,1});source.put({0,97,44,72,10,2});
 auto path=out/L"fine-source.nsf";write(path,compileNsf(source));std::atomic<bool>cancel{false};std::atomic<int>progress{0};auto song=importNesNotes(path,0,1,120,cancel,progress);need(progress==100&&song.resolution==120&&song.notes.size()==2,"Shared note import count/resolution");
 need(song.notes[0].pitch==64&&song.notes[1].pitch==72&&song.notes[0].duty==1&&song.notes[1].duty==2,"Imported pitch/duty");
 // The compiled driver schedules commands on play frames; the first NSF play
 // call occurs one period after init. Compare to audible command times,
 // rather than assuming the compiler's logical tick zero is already playing.
 double frameTicks=16667e-6*120*120/60.;double expectedStart=(std::ceil(17/frameTicks)+1)*frameTicks,expectedEnd=(std::ceil(50/frameTicks)+1)*frameTicks;
 need(std::abs(song.notes[0].step-expectedStart)<=2&&std::abs(song.notes[0].step+song.notes[0].length-expectedEnd)<=2,"Fine inspection does not follow NSF play-frame timing");need(song.notes[0].step%30!=0,"Imported timing was forced onto a sixteenth-note grid");
 auto before=compileNsf(song);song.notes[0].pitch+=12;song.notes[0].length+=10;song.notes[0].velocity=6;song.notes[0].duty=3;song.notes[1].step+=15;song.validate();auto restored=Song::parse(song.serialize());need(restored.serialize()==song.serialize(),"Shared imported-note project roundtrip");auto changed=compileNsf(restored);need(changed!=before,"Imported musical edits not compiled");
 auto wav=out/L"shared-notes-edited.wav";exportSongWav(restored,wav,cancel,progress);need(std::filesystem::file_size(wav)==56+uint64_t(std::llround(song.seconds()*48000))*8,"Shared WAV duration");
 std::ifstream f(wav,std::ios::binary);f.seekg(56);std::vector<Stereo>pcm(4096);f.read(reinterpret_cast<char*>(pcm.data()),pcm.size()*sizeof(Stereo));auto core=makeBackend(changed);core->continuousEnd(true);AudioBlock block;core->render(4096,block);for(int i=0;i<4096;i++)need(pcm[i].l==block.mix[i].l&&pcm[i].r==block.mix[i].r,"WAV does not render the edited common Song");
 // Version1 projects from the earlier viewer remain readable.
 auto legacy=Song::parse("NGMS 1\n120 1 0 \"legacy\"\n2 10 0\n2 10 0\n2 10 0\n2 10 0\n1\n0 0 4 60 15\n");need(legacy.resolution==4&&legacy.notes[0].duty==-1,"Legacy project compatibility");
 bool rejected=false;cancel=true;try{importNesNotes(path,0,1,120,cancel,progress);}catch(...){rejected=true;}need(rejected,"Import cancellation");cancel=false;
 auto expansion=compileNsf(source);expansion[123]=1;write(out/L"unsupported-expansion.nsf",expansion);rejected=false;try{importNesNotes(out/L"unsupported-expansion.nsf",0,.1,120,cancel,progress);}catch(const std::exception&e){rejected=std::string(e.what()).find("Expansion-chip")!=std::string::npos;}need(rejected,"Expansion silently lost during shared import");
 auto dpcm=compileNsf(source);bool enabled=false,loop=false;for(size_t i=128;i+4<128+4096;i++)if(dpcm[i]==0xa9&&dpcm[i+2]==0x8d&&dpcm[i+4]==0x40){if(dpcm[i+3]==0x11){dpcm[i+3]=0x15;dpcm[i+1]=31;enabled=true;}if(dpcm[i+3]==0x10){dpcm[i+1]=0x4f;loop=true;}}need(enabled&&loop,"Own DPCM fixture initialization");write(out/L"unsupported-dpcm.nsf",dpcm);rejected=false;try{importNesNotes(out/L"unsupported-dpcm.nsf",0,.2,120,cancel,progress);}catch(const std::exception&e){rejected=std::string(e.what()).find("DPCM sample")!=std::string::npos;}need(rejected,"DPCM silently lost during shared import");
 std::cout<<"PASS shared Song import/fine timing/pitch-duty-note edits/save/WAV/legacy/unsupported/cancel\n";
}
