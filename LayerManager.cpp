#include "LayerManager.h"
#include "VectorLayer.h"
#include "RasterLayer.h"

// コンストラクタ デフォルトでベクタレイヤーを一つ作成
LayerManager::LayerManager()
{
}

LayerManager::LayerManager(std::unique_ptr<ILayer> testLayer)

{
    layers_.push_back(std::move(testLayer));
    activeLayerIndex_ = 0;
}

void LayerManager::createNewRasterLayer(int width, int height, HDC hdc)
{
    // 既存のレイヤーをクリア
    layers_.clear();
    // 新しいRasterLayerを作成し、リストに追加
    layers_.push_back(std::make_unique<RasterLayer>(width, height, hdc));
    activeLayerIndex_ = 0;
}

// レイヤーに処理を依頼する関数たち
void LayerManager::draw(HDC hdc) const
{
    if (auto *layer = getActiveLayer())
    {
        layer->draw(hdc);
    }
}

void LayerManager::addPoint(const PenPoint &p)
{
    if (auto *layer = getActiveLayer())
    {
        COLORREF drawColor = RGB(255, 255, 255);
        if (currentMode_ == DrawMode::Pen) // ペンモードならプライベートフィールドの色を使用する
        {
            drawColor = getPenColor();
        }
        layer->addPoint(p, currentMode_, getCurrentToolWidth(), drawColor);
    }
}

void LayerManager::clear()
{
    if (auto *layer = getActiveLayer())
    {
        layer->clear();
    }
}

void LayerManager::startNewStroke()
{
    if (auto *layer = getActiveLayer())
    {
        layer->startNewStroke();
    }
}

// setter

void LayerManager::setDrawMode(DrawMode newMode)
{
    currentMode_ = newMode;
}

void LayerManager::setPenWidth(int width)
{
    penWidth_ = width;
}

void LayerManager::setEraserWidth(int width)
{
    eraserWidth_ = width;
}

void LayerManager::setPenColor(COLORREF color)
{
    penColor_ = color;
}

// getter

ILayer *LayerManager::getActiveLayer() const
{
    if (activeLayerIndex_ >= 0 && activeLayerIndex_ < layers_.size())
    {
        return layers_[activeLayerIndex_].get();
    }
    return nullptr;
}

DrawMode LayerManager::getCurrentMode() const
{
    return currentMode_;
}

COLORREF LayerManager::getPenColor() const
{
    return penColor_;
}

int LayerManager::getCurrentToolWidth() const
{
    if (currentMode_ == DrawMode::Pen)
    {
        return penWidth_;
    }
    else
    {
        return eraserWidth_;
    }
}