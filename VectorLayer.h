#pragma once
#include "ILayer.h"
#include <vector>

#include <string>
#include <memory>

namespace Gdiplus
{
    class Bitmap;
    class Graphics;
}

class VectorLayer : public ILayer
{
private:
    std::vector<std::vector<PenPoint>> strokes_;

public:
    void draw(Gdiplus::Graphics *g) const override;
    void addPoint(const PenPoint &p, DrawMode mode, int width, COLORREF color) override;
    void clear() override;

    void startNewStroke() override;
    const std::vector<std::vector<PenPoint>> &getStrokes() const override;
};