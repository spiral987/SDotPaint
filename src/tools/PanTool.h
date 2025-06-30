#pragma once
#include "ITool.h"

// 前方宣言: "ViewManagerというクラスがどこかにあるよ"とコンパイラに教える
// これにより、ヘッダーファイル同士の相互インクルードを防ぎ、コンパイル時間を短縮できる
class ViewManager;

class PanTool : public ITool
{
private:
    ViewManager &m_viewManager; // ViewManagerへの参照を保持するメンバ変数

public:
    // コンストラクタ: ViewManagerを受け取る
    PanTool(ViewManager &viewManager);

    // IToolの仮想関数をオーバーライド（上書き）して、PanToolの具体的な処理を実装する
    void OnPointerDown(const PointerEvent &event) override;
    void OnPointerUpdate(const PointerEvent &event) override;
    void OnPointerUp(const PointerEvent &event) override;
    void SetCursor() override;
};