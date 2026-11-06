#include "Midi.hpp"
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <tlhelp32.h>
#include <algorithm>
#include <fstream>
#include <cwctype>
#include <map>
#include <stdexcept>
namespace ngmv {
namespace {
std::wstring decode(const std::vector<char>&b){
 if(b.size()>=2&&uint8_t(b[0])==255&&uint8_t(b[1])==254){if(b.size()%2)throw std::runtime_error("Invalid VirtualMIDISynth UTF16 config.");std::wstring out;for(size_t p=2;p<b.size();p+=2)out.push_back(wchar_t(uint8_t(b[p])|(uint16_t(uint8_t(b[p+1]))<<8)));return out;}
 size_t start=b.size()>=3&&uint8_t(b[0])==239&&uint8_t(b[1])==187&&uint8_t(b[2])==191?3:0;auto cp=CP_UTF8;int n=MultiByteToWideChar(cp,MB_ERR_INVALID_CHARS,b.data()+start,int(b.size()-start),nullptr,0);if(!n){cp=CP_ACP;n=MultiByteToWideChar(cp,0,b.data()+start,int(b.size()-start),nullptr,0);}if(!n)throw std::runtime_error("Cannot decode VirtualMIDISynth config.");std::wstring out(n,0);MultiByteToWideChar(cp,cp==CP_UTF8?MB_ERR_INVALID_CHARS:0,b.data()+start,int(b.size()-start),out.data(),n);return out;
}
std::wstring trim(std::wstring v){auto first=v.find_first_not_of(L" \t\r\n");if(first==v.npos)return {};return v.substr(first,v.find_last_not_of(L" \t\r\n")-first+1);}
std::filesystem::path configPath(){
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot!=INVALID_HANDLE_VALUE){PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);if(Process32FirstW(snapshot,&entry))do{if(_wcsicmp(entry.szExeFile,L"VirtualMIDISynth.exe"))continue;HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);if(!process)continue;wchar_t path[32768]{};DWORD n=32768;bool ok=QueryFullProcessImageNameW(process,0,path,&n)!=FALSE;CloseHandle(process);if(ok){auto config=std::filesystem::path(path).parent_path()/L"VirtualMIDISynth.conf";if(std::filesystem::is_regular_file(config)){CloseHandle(snapshot);return config;}}}while(Process32NextW(snapshot,&entry));CloseHandle(snapshot);}
 for(auto name:{L"ProgramW6432",L"ProgramFiles"}){wchar_t root[32768]{};DWORD n=GetEnvironmentVariableW(name,root,32768);if(n&&n<32768){auto path=std::filesystem::path(root)/L"VirtualMIDISynth"/L"VirtualMIDISynth.conf";if(std::filesystem::is_regular_file(path))return path;}}
 return {};
}
}
std::vector<std::filesystem::path> virtualMidiFonts(const std::filesystem::path&config){
 std::ifstream file(config,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("Cannot read VirtualMIDISynth config. Choose SF2 with F8.");auto n=file.tellg();if(n<=0||n>1024*1024)throw std::runtime_error("Invalid VirtualMIDISynth config size.");std::vector<char>bytes(static_cast<size_t>(n));file.seekg(0);if(!file.read(bytes.data(),n))throw std::runtime_error("Incomplete VirtualMIDISynth config read.");auto text=decode(bytes);if(text.find(L'\0')!=text.npos)throw std::runtime_error("Invalid VirtualMIDISynth config text.");
 std::map<std::wstring,std::wstring>values;bool section=false;size_t p=0;
 while(p<text.size()){size_t end=text.find(L'\n',p);if(end==text.npos)end=text.size();auto line=trim(text.substr(p,end-p));p=end+1;if(line.empty()||line[0]==L';'||line[0]==L'#')continue;if(line[0]==L'['){section=_wcsicmp(line.c_str(),L"[SoundFonts]")==0;continue;}if(!section)continue;auto equal=line.find(L'=');if(equal==line.npos)continue;auto key=trim(line.substr(0,equal));std::transform(key.begin(),key.end(),key.begin(),::towlower);values[key]=trim(line.substr(equal+1));}
 std::vector<std::filesystem::path>out;
 for(const auto&item:values){const auto&key=item.first;if(key.size()<3||key.substr(0,2)!=L"sf"||key.find_first_not_of(L"0123456789",2)!=key.npos)continue;auto enabled=values.find(key+L".enabled");if(enabled==values.end()||enabled->second!=L"1")continue;
  auto value=item.second;if(value.size()>=2&&value.front()==L'"'&&value.back()==L'"')value=value.substr(1,value.size()-2);if(value.empty())throw std::runtime_error("Active VirtualMIDISynth soundfont has no path.");std::filesystem::path path(value);if(path.is_relative())path=config.parent_path()/path;out.push_back(path);if(out.size()>256)throw std::runtime_error("Too many VirtualMIDISynth soundfonts.");
 }
 return out;
}
std::vector<std::filesystem::path> virtualMidiFontsToImport(){
 std::filesystem::path config;wchar_t test[32768]{},dummy[32]{};
 bool isolated=GetEnvironmentVariableW(L"NGMV_TEST_SESSION",nullptr,0)&&GetEnvironmentVariableW(L"SDL_AUDIODRIVER",dummy,32)&&!wcscmp(dummy,L"dummy");
 DWORD n=isolated?GetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",test,32768):0;
 if(n>=32768)throw std::runtime_error("Test MIDI config path too long.");config=n?std::filesystem::path(test):configPath();
 if(config.empty())throw std::runtime_error("VirtualMIDISynth configuration was not found. No Windows DLS substitution was made.");
 auto fonts=virtualMidiFonts(config);if(fonts.empty())throw std::runtime_error("VirtualMIDISynth has no enabled soundfont. Enable its SF2 and try importing again.");
 for(auto&p:fonts){std::error_code ec;if(!std::filesystem::is_regular_file(p,ec)||ec)throw std::runtime_error("An enabled VirtualMIDISynth soundfont is missing. Fix its path before importing.");}
 return fonts;
}
std::filesystem::path automaticMidiBank(){
 // Isolated dummy tests use system bank, unless given an owned test config.
 wchar_t dummy[32]{},test[32768]{};bool isolated=GetEnvironmentVariableW(L"NGMV_TEST_SESSION",nullptr,0)&&GetEnvironmentVariableW(L"SDL_AUDIODRIVER",dummy,32)&&!wcscmp(dummy,L"dummy");
 std::filesystem::path config;
 if(isolated){DWORD n=GetEnvironmentVariableW(L"NGMV_TEST_VMS_CONFIG",test,32768);if(!n)return windowsMidiBank();if(n>=32768)throw std::runtime_error("Test MIDI config path too long.");config=test;}
 else {
  // Windows-default waveform mode synthesizes internally. A configured VMS
  // font takes priority even when a mapper's preferred-ID query is unavailable
  // or reports GS. Never silently replace an existing VMS configuration with DLS.
  config=configPath();if(config.empty())return windowsMidiBank();
 }
 auto fonts=virtualMidiFonts(config);if(fonts.size()!=1)throw std::runtime_error("Automatic VirtualMIDISynth import needs one enabled SF2. Select a soundfont with F8 (multiple-bank layering is not supported).");std::error_code ec;if(!std::filesystem::is_regular_file(fonts[0],ec)||ec)throw std::runtime_error("VirtualMIDISynth active SF2 is missing. Choose its file with F8.");return fonts[0];
}
}
