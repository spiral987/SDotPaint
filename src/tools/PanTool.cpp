#include "PanTool.h"
#include "view/ViewManager.h"

PanTool::PanTool(ViewManager &viewManager)
    : m_viewManager(viewManager)
{
}

// マウスが押された時の処理
void PanTool::OnPointerDown(const PointerEvent &event)
{

    m_viewManager.PanStart(event.screenPos);
    SetCapture(event.hwnd); // マウス入力をこのウィンドウで受け取り続けるようにする
}

// マウスが動いた時の処理
void PanTool::OnPointerUpdate(const PointerEvent &event)
{
    m_viewManager.PanUpdate(event.screenPos);
    InvalidateRect(event.hwnd, NULL, FALSE); // 画面の再描画を要求
}

// マウスが離された時の処理
void PanTool::OnPointerUp(const PointerEvent &event)
{
    ReleaseCapture(); // マウス入力のキャプチャを解放
    InvalidateRect(event.hwnd, NULL, FALSE);
}

// カーソルを設定する処理
void PanTool::SetCursor()
{
    ::SetCursor(LoadCursor(nullptr, IDC_HAND)); // グローバル名前空間のSetCursorをよんでいる。カーソルを「手のひら」の形に変更
}