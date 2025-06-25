#pragma once
#include "ILayer.h"
#include <vector>

class VectorLayer : public ILayer
{
private:
    std::vector<std::vector<PenPoint>> strokes_;

public:
    void draw(HDC hdc) const override;
    void addPoint(const PenPoint &p) override;
    void clear() override;

    void startNewStroke() override;
    const std::vector<std::vector<PenPoint>> &getStrokes() const override;
};