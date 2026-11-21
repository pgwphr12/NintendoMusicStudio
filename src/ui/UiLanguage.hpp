#pragma once
#include <windows.h>
#include <string>
#include <string_view>
namespace ngmv {
bool uiEnglish();
void setUiLanguage(bool english);
std::wstring translateUi(std::wstring_view text);
BOOL setUiText(HWND,const wchar_t*);
BOOL setUiDlgText(HWND,int,const wchar_t*);
void rememberUiTree(HWND);
}
