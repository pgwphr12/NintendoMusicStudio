#pragma once
#include "Song.hpp"
#include <windows.h>
#include <filesystem>
#include <optional>
namespace ngmv::editor {std::optional<Song> runNoteImportDialog(HWND,const std::filesystem::path&initial,int bpm);
// Explicit hidden UI test entry; uses the production dialog worker and controls.
std::optional<Song> testNoteImportDialog(const std::filesystem::path&,int bpm,double seconds,bool cancelNow);
}
