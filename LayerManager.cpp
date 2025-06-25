#include "LayerManager.h"
#include "VectorLayer.h"

// コンストラクタ デフォルトでベクタレイヤーを一つ作成
LayerManager::LayerManager()
{
    layers_.push_back(std::make_unique<VectorLayer>());
    activeLayerIndex_ = 0;
}

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
        layer->addPoint(p);
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