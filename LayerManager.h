// LayerManager.h

#pragma once

#include "ILayer.h"
#include <vector>
#include <memory> //unique_ptr = スマートなポインタ

// アプリケーションのデータ(レイヤー管理)
class LayerManager
{
private:
    std::vector<std::unique_ptr<ILayer>> layers_;
    int activeLayerIndex_ = -1;

public:
    LayerManager(); // コンストラクタ

    // アクティブなレイヤーに処理を渡す関数たち
    void draw(HDC hdc) const;
    void addPoint(const PenPoint &p);
    void clear();
    void startNewStroke();

    // 現在アクティブなレイヤーを取得
    ILayer *getActiveLayer() const;
};