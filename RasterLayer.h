#pragma once

#include "ILayer.h"

#include <windows.h>
#include <vector>
#include <string>
#include <memory>

namespace Gdiplus
{
    class Bitmap;
    class Graphics;
    class Image;
}

// ストローク情報を格納する構造体
struct Stroke
{
    std::vector<PenPoint> points;
    int penWidth;
    COLORREF color;
    DrawMode mode;
};

class RasterLayer : public ILayer
{
private:
    std::unique_ptr<Gdiplus::Bitmap> hBitmap_; // ビットマップを作成
    std::vector<Stroke> m_strokes;             // ストロークデータを保存

    int width_ = 0;  // 幅
    int height_ = 0; // 高さ
    std::wstring name_;
    Stroke currentStroke_; // 現在描画中のストローク

    PenPoint lastPoint_ = {{-1, -1}, 0};

public:
    // コンストラクタ、デストラクタ
    RasterLayer(int width, int height, std::wstring name);
    ~RasterLayer();

    void draw(Gdiplus::Graphics *g, float opacity = 1.0f, Gdiplus::Image *pTexture = nullptr) const override;
    void addPoint(const PenPoint &p, DrawMode mode, int width, COLORREF color) override;
    void clear() override;
    void startNewStroke() override;

    const std::wstring &getName() const override;
    void setName(const std::wstring &newName) override;

    COLORREF getAverageColor() const override;                             // 平均色を返す
    const std::vector<std::vector<PenPoint>> &getStrokes() const override; // ダミー
};