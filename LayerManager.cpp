#include "LayerManager.h"
#include "VectorLayer.h"
#include "RasterLayer.h"

// コンストラクタ デフォルトでベクタレイヤーを一つ作成
LayerManager::LayerManager()
{
}

void LayerManager::createNewRasterLayer(int width, int height, HDC hdc)
{
    // 既存のレイヤーをクリア
    layers_.clear();
    // 新しいRasterLayerを作成し、リストに追加
    layers_.push_back(std::make_unique<RasterLayer>(width, height, hdc));
    activeLayerIndex_ = 0;
}

void LayerManager::setDrawMode(DrawMode newMode)
{
    currentMode_ = newMode;
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
        layer->addPoint(p, currentMode_);
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

ILayer *LayerManager::getActiveLayer() const
{
    if (activeLayerIndex_ >= 0 && activeLayerIndex_ < layers_.size())
    {
        return layers_[activeLayerIndex_].get();
    }
    return nullptr;
}