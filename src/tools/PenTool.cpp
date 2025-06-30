#include <windows.h>
#include <gdiplus.h>

#include "PenTool.h"
#include "core/LayerManager.h"
#include "core/DrawMode.h"
#include "view/ViewManager.h"

using namespace Gdiplus;

PenTool::PenTool(HWND hwnd, LayerManager &layerManager, ViewManager &viewManager)
    : m_hwnd(hwnd),
      m_layerManager(layerManager),
      m_viewManager(viewManager),
      m_lastScreenPoint({-1, -1}), // 未定義の値で初期化
      m_lastPressure(0)
{
}

// マウスが押された時の処理
void PenTool::OnPointerDown(const PointerEvent &event)
{

    m_layerManager.setCurrentMode(DrawMode::Pen);

    m_layerManager.startNewStroke();

    // ワールド座標への変換をViewManagerに任せる
    PointF worldPoint = m_viewManager.ScreenToWorld(event.screenPos);
    m_layerManager.addPoint({(LONG)worldPoint.X, (LONG)worldPoint.Y, event.pressure});

    // 最初の点の筆圧を保存
    m_lastScreenPoint = event.screenPos;
    m_lastPressure = event.pressure;
}

// マウスが動いた時の処理
void PenTool::OnPointerUpdate(const PointerEvent &event)
{
    PointF worldPoint = m_viewManager.ScreenToWorld(event.screenPos);
    m_layerManager.addPoint({(LONG)worldPoint.X, (LONG)worldPoint.Y, event.pressure});

    // 2. 画面に直接、"アンチエイリアスのかかった"軽量な線を描画する
    HDC hdc = GetDC(m_hwnd);
    if (hdc)
    {
        Graphics screenGraphics(hdc);
        screenGraphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // プレビュー描画にも、完全な変換行列を適用する
        Matrix transformMatrix;
        m_viewManager.GetTransformMatrix(&transformMatrix);
        screenGraphics.SetTransform(&transformMatrix);

        int maxToolWidth = m_layerManager.getCurrentToolWidth();
        COLORREF toolColorRef = m_layerManager.getPenColor();
        Color penColor(GetRValue(toolColorRef), GetGValue(toolColorRef), GetBValue(toolColorRef));

        if (m_layerManager.getCurrentMode() == DrawMode::Eraser)
        {
            toolColorRef = RGB(255, 255, 255);
        }

        // RasterLayer同様、平均筆圧で太さを計算
        float currentPressureFactor = (float)event.pressure / 1024.0f;
        float lastPressureFactor = (float)m_lastPressure / 1024.0f;
        float averagePressureFactor = (currentPressureFactor + lastPressureFactor) / 2.0f;
        float pressureWidth = maxToolWidth * averagePressureFactor;

        // 変換行列で既にズームが適用されているので、プレビュー幅でズームを掛ける必要はなくなる
        float previewWidth = pressureWidth;
        if (previewWidth < 1.0f / m_viewManager.GetZoomFactor())
        {
            // どんなに細くても最低1ピクセルは表示されるように調整
            previewWidth = 1.0f / m_viewManager.GetZoomFactor();
        }

        Pen gdiplusPen(penColor, previewWidth);

        // RasterLayerの設定と完全に一致させる
        gdiplusPen.SetStartCap(LineCapRound);
        gdiplusPen.SetEndCap(LineCapRound);
        gdiplusPen.SetLineJoin(LineJoinRound); // 角を滑らかにする設定を追加

        if (m_lastScreenPoint.x != -1)
        {
            // 描画する座標も、ワールド座標に変換したものを使う
            PointF lastWorldPoint = m_viewManager.ScreenToWorld(m_lastScreenPoint);
            screenGraphics.DrawLine(&gdiplusPen, lastWorldPoint, worldPoint);
        }

        ReleaseDC(m_hwnd, hdc);
    }

    // 現在の情報を「直前の情報」として更新
    m_lastScreenPoint = event.screenPos;
    m_lastPressure = event.pressure;
}

// マウスが離された時の処理
void PenTool::OnPointerUp(const PointerEvent &event)
{
    // ★ MessageHandlerから移植
    // 1. ストロークを確定させる
    m_layerManager.startNewStroke();

    // 2. 最終的な結果をきれいに再描画するようウィンドウに依頼
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// カーソルを設定する処理
void PenTool::SetCursor()
{
    ::SetCursor(LoadCursor(nullptr, IDC_CROSS)); // 十字カーソル
}