#pragma once

#include "ITool.h" // IToolインターフェースとPointerEvent構造体のため
#include <memory>  // std::unique_ptrのため
#include <map>     // std::mapのため

// 前方宣言
class ViewManager;
class LayerManager;

// アプリケーション内のツールを識別するためのenum（列挙型）
// これを使うことで、文字列やマジックナンバーを使わずにツールを安全に指定できる
enum class ToolType
{
    Pen,
    Eraser,
    Pan,
    Zoom,
    Rotate
};

class ToolController
{
private:
    // 全てのツールオブジェクトを所有・管理するマップ
    // ToolTypeをキーとして、対応するツールの実体をstd::unique_ptrで保持する
    std::map<ToolType, std::unique_ptr<ITool>> m_tools;

    // 現在アクティブなツールへのポインタ
    // m_toolsが指すオブジェクトのいずれかを指す
    ITool *m_currentTool;

public:
    // コンストラクタ：ツールを作成するために必要な全ての依存オブジェクトを受け取る
    ToolController(HWND hwnd, ViewManager &viewManager, LayerManager &layerManager);

    // 現在のツールを切り替える
    void SetTool(ToolType type);

    // イベントを現在のツールに転送（デリゲート）する
    void OnPointerDown(const PointerEvent &event);
    void OnPointerUpdate(const PointerEvent &event);
    void OnPointerUp(const PointerEvent &event);
};