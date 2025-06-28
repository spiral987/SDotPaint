// LayerManager.h

#pragma once

#include "ILayer.h"
#include "DrawMode.h"

#include <vector>
#include <memory> //unique_ptr = スマートなポインタ
#include <string>

namespace Gdiplus
{
    class Graphics;
}

// アプリケーションのデータ(レイヤー管理)
class LayerManager
{
private:
    std::vector<std::unique_ptr<ILayer>> layers_; // レイヤーを保持
    int activeLayerIndex_ = -1;                   // 現在アクティブなレイヤーを保持
    DrawMode currentMode_ = DrawMode::Pen;        // モードを保持
    int penWidth_ = 5;                            // ペンの太さ
    int eraserWidth_ = 20;                        // 消しゴムの太さ
    COLORREF penColor_ = RGB(0, 0, 0);            // ペンの色

public:
    LayerManager(); // コンストラクタ
    explicit LayerManager(std::unique_ptr<ILayer> testLayer);

    // レイヤーの追加や削除
    void createNewRasterLayer(int width, int height, std::wstring name); // ラスターレイヤーを作成する
    void addNewRasterLayer(int width, int height);                       // ラスターレイヤーの作成
    void deleteActiveLayer();                                            // レイヤーの削除

    // アクティブなレイヤーに処理を渡す関数たち
    void draw(Gdiplus::Graphics *g) const;
    void addPoint(const PenPoint &p);
    void clear();
    void startNewStroke();

    // setter
    void setDrawMode(DrawMode newMode);
    void setPenWidth(int width);
    void setEraserWidth(int width);
    void setPenColor(COLORREF color);
    void setActiveLayer(int index);

    // getter
    ILayer *getActiveLayer() const; //  現在アクティブなレイヤーを取得
    DrawMode getCurrentMode() const;
    COLORREF getPenColor() const;
    // 現在のペンの太さを返す
    int getCurrentToolWidth() const;
    const std::vector<std::unique_ptr<ILayer>> &getLayers() const; // レイヤー配列を返す
    int getActiveLayerIndex() const;
};