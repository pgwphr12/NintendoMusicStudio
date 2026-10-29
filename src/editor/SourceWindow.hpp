#pragma once
#include <windows.h>
#include <filesystem>
namespace ngmv::editor {
void runSourceEditor(HWND owner,const std::filesystem::path&initial={});
void sourceEditorSelftest(const std::filesystem::path&directory);
}
