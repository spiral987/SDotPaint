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

    HBITMAP hCustomBrush_ = nullptr;
    int brushWidth_ = 64;
    int brushHeight_ = 64;

public:
    // コンストラクタ、デストラクタ
    RasterLayer(int width, int height, HDC hdc);
    ~RasterLayer();

    void setCustomBrush(const std::vector<POINT> &points) override;

    void draw(HDC hdc) const override;
    void addPoint(const PenPoint &p, DrawMode mode) override;
    void clear() override;
    void startNewStroke() override;

    // ダミー
    const std::vector<std::vector<PenPoint>> &getStrokes() const override;
};