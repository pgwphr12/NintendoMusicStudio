#pragma once
#include <windows.h>
#include <filesystem>
#include <functional>
namespace ngmv::editor {
struct Song;
int runMusicEditor(HINSTANCE,HWND owner={},const std::filesystem::path&image={},bool snapshot=false,
                   std::function<void(const std::filesystem::path&)> viewer={},bool analyze=false,const std::filesystem::path&initialMusic={},std::function<void(const Song&)> sampledViewer={});
bool confirmMusicEditorExit(HWND owner);
}
