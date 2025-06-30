#pragma once
#include <windows.h>
#include "view/ViewManager.h"  // ViewManagerが必要
#include "core/LayerManager.h" // LayerManagerが必要

// イベント情報をまとめた構造体
struct PointerEvent
{
    HWND hwnd;
    POINT screenPos;
    UINT32 pressure;
    // ...その他必要な情報...
};

// 全てのツールの基底となるインターフェース
class ITool
{
public:
    virtual ~ITool() {}
    virtual void OnPointerDown(const PointerEvent &event) = 0;
    virtual void OnPointerUpdate(const PointerEvent &event) = 0;
    virtual void OnPointerUp(const PointerEvent &event) = 0;
    virtual void SetCursor() = 0; // ツールに応じたカーソルを設定する
};