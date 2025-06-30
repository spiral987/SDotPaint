#pragma once
#include <windows.h>

class MessageHandler
{
public:
    MessageHandler(HWND hwnd);
    LRESULT ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd;

    // ハンドラ
    void HandleCreate();
    BOOL HandleDrawItem(WPARAM wParam, LPARAM lParam);
    void HandleKeyUp(WPARAM wParam, LPARAM lParam);
    void HandleKeyDown(WPARAM wParam, LPARAM lParam);
    void HandleMouseMove(WPARAM wParam, LPARAM lParam);
    void HandleCommand(WPARAM wParam, LPARAM lParam);
    void HandleVScroll(WPARAM wParam, LPARAM lParam);
    void HandlePointerDown(WPARAM wParam, LPARAM lParam);
    void HandlePointerUpdate(WPARAM wParam, LPARAM lParam);
    void HandlePointerUp(WPARAM wParam, LPARAM lParam);
    void HandleSize(WPARAM wParam, LPARAM lParam);
    void HandleDestroy(WPARAM wParam, LPARAM lParam);
    void HandlePaint(WPARAM wParam, LPARAM lParam);

    void UpdateToolMode();
};