#pragma once

#include <windows.h>
#include "ITool.h"

// 前方宣言
class LayerManager;
class ViewManager;

class EraserTool : public ITool
{
private:
    LayerManager &m_layerManager;
    ViewManager &m_viewManager;
    HWND m_hwnd;
    POINT m_lastScreenPoint; // プレビュー描画のために直前の座標を保持
    UINT32 m_lastPressure;   // プレビュー描画のために直前の筆圧を保持

public:
    EraserTool(HWND hwnd, LayerManager &layerManager, ViewManager &viewManager);
    void OnPointerDown(const PointerEvent &event) override;
    void OnPointerUpdate(const PointerEvent &event) override;
    void OnPointerUp(const PointerEvent &event) override;
    void SetCursor() override;
};