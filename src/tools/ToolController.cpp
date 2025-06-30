// src/tools/ToolController.cpp (新規作成)

#include "ToolController.h"

// これからインスタンスを作成する、すべての具象ツールクラスのヘッダーをインクルードする
#include "PenTool.h"
#include "EraserTool.h"
#include "PanTool.h"
#include "ZoomTool.h"
#include "RotateTool.h"

// コンストラクタの実装
ToolController::ToolController(HWND hwnd, ViewManager &viewManager, LayerManager &layerManager)
    : m_currentTool(nullptr) // 最初はどのツールも選択されていないのでnullptrで初期化
{
    // ここで、アプリケーションで使う全てのツールをインスタンス化する
    // std::make_uniqueを使って安全にメモリを確保し、m_toolsマップに格納する
    m_tools[ToolType::Pen] = std::make_unique<PenTool>(hwnd, layerManager, viewManager);
    m_tools[ToolType::Eraser] = std::make_unique<EraserTool>(hwnd, layerManager, viewManager);
    m_tools[ToolType::Pan] = std::make_unique<PanTool>(viewManager);
    m_tools[ToolType::Zoom] = std::make_unique<ZoomTool>(viewManager);
    m_tools[ToolType::Rotate] = std::make_unique<RotateTool>(viewManager);

    // アプリケーション起動時のデフォルトツールをペンに設定する
    this->SetTool(ToolType::Pen);
}

// ツールを切り替えるメソッド
void ToolController::SetTool(ToolType type)
{
    // マップから指定されたツールを探す
    auto it = m_tools.find(type);
    if (it != m_tools.end()) // ツールが見つかった場合
    {
        // 現在のツールへのポインタを更新する
        m_currentTool = it->second.get();
        // 新しいツールのカーソル形状を設定する
        m_currentTool->SetCursor();
    }
}

// 以降のメソッドは、受け取ったイベントを現在のツールにそのまま渡すだけのシンプルな役割

void ToolController::OnPointerDown(const PointerEvent &event)
{
    if (m_currentTool) // 現在のツールが設定されていれば
    {
        m_currentTool->OnPointerDown(event);
    }
}

void ToolController::OnPointerUpdate(const PointerEvent &event)
{
    if (m_currentTool)
    {
        m_currentTool->OnPointerUpdate(event);
    }
}

void ToolController::OnPointerUp(const PointerEvent &event)
{
    if (m_currentTool)
    {
        m_currentTool->OnPointerUp(event);
    }
}