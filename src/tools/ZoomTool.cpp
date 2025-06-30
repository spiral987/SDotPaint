#include "ZoomTool.h"
#include "view/ViewManager.h"

ZoomTool::ZoomTool(ViewManager &viewManager)
    : m_viewManager(viewManager)
{
}

// マウスが押された時の処理
void ZoomTool::OnPointerDown(const PointerEvent &event)
{
    m_startScreenPoint = event.screenPos;
    m_viewManager.ZoomStart();
    SetCapture(event.hwnd); // マウス入力をこのウィンドウで受け取り続けるようにする
}

// マウスが動いた時の処理
void ZoomTool::OnPointerUpdate(const PointerEvent &event)
{
    m_viewManager.ZoomUpdate(event.screenPos, m_startScreenPoint);
    InvalidateRect(event.hwnd, NULL, FALSE); // 画面の再描画を要求
}

// マウスが離された時の処理
void ZoomTool::OnPointerUp(const PointerEvent &event)
{
    ReleaseCapture(); // マウス入力のキャプチャを解放
    InvalidateRect(event.hwnd, NULL, FALSE);
}

// カーソルを設定する処理
void ZoomTool::SetCursor()
{
    ::SetCursor(LoadCursor(nullptr, IDC_SIZEALL)); // グローバル名前空間のSetCursorをんでいる。カーソルの形を変更
}