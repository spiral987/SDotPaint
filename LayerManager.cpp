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

// ★変更: レイヤー作成時に名前を渡す
void LayerManager::createNewRasterLayer(int width, int height, HDC hdc, std::wstring name)
{
    layers_.push_back(std::make_unique<RasterLayer>(width, height, hdc, name));
    activeLayerIndex_ = (int)layers_.size() - 1;
}

// ★追加: 新しいラスターレイヤーを追加する
void LayerManager::addNewRasterLayer(int width, int height, HDC hdc)
{
    // 「レイヤーN」形式で名前を生成
    std::wstring layerName = L"レイヤー" + std::to_wstring(layers_.size() + 1);
    createNewRasterLayer(width, height, hdc, layerName);
}

// ★追加: アクティブなレイヤーを削除する
void LayerManager::deleteActiveLayer()
{
    // レイヤーが1つしかない場合は削除しない
    if (layers_.size() <= 1 || activeLayerIndex_ < 0)
    {
        return;
    }

    layers_.erase(layers_.begin() + activeLayerIndex_);

    // アクティブなインデックスを調整
    if (activeLayerIndex_ >= layers_.size())
    {
        activeLayerIndex_ = (int)layers_.size() - 1;
    }
}

// レイヤーに処理を依頼する関数たち
void LayerManager::draw(HDC hdc) const
{
    // ★変更: すべてのレイヤーを描画する
    for (const auto &layer : layers_)
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

void LayerManager::setActiveLayer(int index)
{
    if (index >= 0 && index < layers_.size())
    {
        activeLayerIndex_ = index;
    }
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

const std::vector<std::unique_ptr<ILayer>> &LayerManager::getLayers() const
{
    return layers_;
}

int LayerManager::getActiveLayerIndex() const
{
    return activeLayerIndex_;
}
