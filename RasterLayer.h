#pragma once

#include "ILayer.h"

#include <windows.h>
#include <vector>

class RasterLayer : public ILayer
{
private:
    HBITMAP hBitmap_ = nullptr; // ビットマップを作成
    HDC hMemoryDC_ = nullptr;   // ビットマップに描画するためのコンテキスト
    int width_ = 0;             // 幅
    int height_ = 0;            // 高さ

    POINT lastPoint_ = {-1, -1};

public:
    // コンストラクタ、デストラクタ
    RasterLayer(int width, int height, HDC hdc);
    ~RasterLayer();

    void draw(HDC hdc) const override;
    void addPoint(const PenPoint &p, DrawMode mode, int width) override;
    void clear() override;
    void startNewStroke() override;

    // ダミー
    const std::vector<std::vector<PenPoint>> &getStrokes() const override;
};