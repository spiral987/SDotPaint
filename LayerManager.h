// LayerManager.h

#pragma once

#include "ILayer.h"
#include "DrawMode.h"

#include <vector>
#include <memory> //unique_ptr = スマートなポインタ

// アプリケーションのデータ(レイヤー管理)
class LayerManager
{
private:
    std::vector<std::unique_ptr<ILayer>> layers_; // レイヤーを保持
    int activeLayerIndex_ = -1;                   // 現在アクティブなレイヤーを保持
    DrawMode currentMode_ = DrawMode::Pen;        // モードを保持

public:
    LayerManager(); // コンストラクタ
    explicit LayerManager(std::unique_ptr<ILayer> testLayer);

    void createNewRasterLayer(int width, int height, HDC hdc); // ラスターレイヤーを作成する

    void setDrawMode(DrawMode newMode);

    // アクティブなレイヤーに処理を渡す関数たち
    void draw(HDC hdc) const;
    void addPoint(const PenPoint &p);
    void clear();
    void startNewStroke();

    // 現在アクティブなレイヤーを取得
    ILayer *getActiveLayer() const;
};