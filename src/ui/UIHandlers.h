#pragma once
#include <windows.h>
#include <CommCtrl.h>

class UIHandlers
{
public:
    // Windows API関数はthisポインタを受け付けないため、staticにする
    static LRESULT CALLBACK LayerListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    static LRESULT CALLBACK EditControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};