#pragma once
#include "ITool.h"

class ViewManager; // 前方宣言しておくことで相互インクルードを防ぐ

class RotateTool : public ITool
{
private:
    ViewManager &m_viewManager; // インスタンスを参照で扱う
    POINT m_startScreenPoint;   // 回転開始時のスクリーン座標を記憶する

public:
    RotateTool(ViewManager &viewManager);

    // IToolの仮想関数をoverrideして具体的に実装
    void OnPointerDown(const PointerEvent &event) override;
    void OnPointerUpdate(const PointerEvent &event) override;
    void OnPointerUp(const PointerEvent &event) override;
    void SetCursor() override;
};