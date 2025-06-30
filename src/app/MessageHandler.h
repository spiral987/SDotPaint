#pragma once
#include <windows.h>

#include "view/ViewManager.h"

class MessageHandler
{

private:
    HWND m_hwnd;
    ViewManager m_viewManager; // <<< ViewManagerのインスタンスを追加

    // 操作中の一時的な状態
    POINT m_operationStartPoint; // パン、ズーム、回転の開始点を記録
    bool m_isTransforming;       // 何らかの視点操作中かどうかのフラグ

    POINT m_lastScreenPoint; // 前回の点の座標
    UINT32 m_lastPressure;   // 前回の点の筆圧

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

public:
    MessageHandler(HWND hwnd);
    LRESULT ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};