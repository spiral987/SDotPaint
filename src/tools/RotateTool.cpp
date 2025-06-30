#include "RotateTool.h"
#include "view/ViewManager.h"

RotateTool::RotateTool(ViewManager &viewManager)
    : m_viewManager(viewManager)
{
}

// マウスが押された時の処理
void RotateTool::OnPointerDown(const PointerEvent &event)
{
    m_startScreenPoint = event.screenPos;
    m_viewManager.RotateStart();
    SetCapture(event.hwnd); // マウス入力をこのウィンドウで受け取り続けるようにする
}

// マウスが動いた時の処理
void RotateTool::OnPointerUpdate(const PointerEvent &event)
{
    m_viewManager.RotateUpdate(event.screenPos, m_startScreenPoint);
    InvalidateRect(event.hwnd, NULL, FALSE); // 画面の再描画を要求
}

// マウスが離された時の処理
void RotateTool::OnPointerUp(const PointerEvent &event)
{
    ReleaseCapture(); // マウス入力のキャプチャを解放
    InvalidateRect(event.hwnd, NULL, FALSE);
}

// カーソルを設定する処理
void RotateTool::SetCursor()
{
    ::SetCursor(LoadCursor(nullptr, IDC_ARROW)); // グローバル名前空間のSetCursorをんでいる。カーソルを「やじるし」の形に変更
}