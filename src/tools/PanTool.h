#pragma once
#include "ITool.h"

class PanTool : public ITool
{
private:
    ViewManager &m_viewManager; // 参照で受け取る
    POINT m_lastPoint;

public:
    PanTool(ViewManager &viewManager);
    void OnPointerDown(const PointerEvent &event) override;
    void OnPointerUpdate(const PointerEvent &event) override;
    void OnPointerUp(const PointerEvent &event) override;
    void SetCursor() override;
};