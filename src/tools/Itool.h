#pragma once
#include <windows.h>

// イベント情報をまとめた構造体
struct PointerEvent
{
    HWND hwnd;
    POINT screenPos;
    UINT32 pressure;
};

// 全てのツールの基底となるインターフェース
class ITool
{
public:
    virtual ~ITool() = default; // これによって派生クラスのリソースが必ず解放される
    virtual void OnPointerDown(const PointerEvent &event) = 0;
    virtual void OnPointerUpdate(const PointerEvent &event) = 0;
    virtual void OnPointerUp(const PointerEvent &event) = 0;
    virtual void SetCursor() = 0; // ツールに応じたカーソルを設定する
};