#include "LayerManager.h"
#include "layers/RasterLayer.h"

// コンストラクタ デフォルトでベクタレイヤーを一つ作成
LayerManager::LayerManager()
{
}

LayerManager::LayerManager(std::unique_ptr<ILayer> testLayer)

{
    m_layers.push_back(std::move(testLayer));
    activeLayerIndex_ = 0;
}

// レイヤー作成時に名前を渡す
void LayerManager::createNewRasterLayer(int width, int height, std::wstring name)
{
    m_layers.push_back(std::make_unique<RasterLayer>(width, height, name));
    activeLayerIndex_ = (int)m_layers.size() - 1;
}

// 新しいラスターレイヤーを追加する
void LayerManager::addNewRasterLayer(int width, int height)
{
    // 「レイヤーN」形式で名前を生成
    std::wstring layerName = L"レイヤー" + std::to_wstring(m_layers.size() + 1);
    createNewRasterLayer(width, height, layerName);
}

// アクティブなレイヤーを削除する
void LayerManager::deleteActiveLayer()
{
    // レイヤーが1つしかない場合は削除しない
    if (m_layers.size() <= 1 || activeLayerIndex_ < 0)
    {
        return;
    }

    m_layers.erase(m_layers.begin() + activeLayerIndex_);

    // アクティブなインデックスを調整
    if (activeLayerIndex_ >= m_layers.size())
    {
        activeLayerIndex_ = (int)m_layers.size() - 1;
    }
}

void LayerManager::renameLayer(int index, const std::wstring &newName)
{
    if (index >= 0 && index < m_layers.size())
    {
        m_layers[index]->setName(newName);
    }
}

// レイヤーに処理を依頼する関数たち
void LayerManager::draw(Gdiplus::Graphics *g) const
{
    // すべてのレイヤーを描画する
    // ホバー状態に応じて不透明度を変えて描画
    for (int i = 0; i < m_layers.size(); ++i)
    {
        if (m_layers[i])
        {
            float opacity = 1.0f;

            // Altキーが押されていて、他のレイヤーがホバーされている場合
            if (hoveredLayerIndex_ != -1 && hoveredLayerIndex_ != static_cast<int>(i))
            {
                opacity = 0.05f; // 5%の不透明度
            }

            m_layers[i]->draw(g, opacity);
        }
    }
}

RECT LayerManager::addPoint(const PenPoint &p)
{
    if (auto *layer = getActiveLayer())
    {
        COLORREF drawColor = RGB(255, 255, 255);
        if (currentMode_ == DrawMode::Pen) // ペンモードならプライベートフィールドの色を使用する
        {
            drawColor = getPenColor();
        }
        return layer->addPoint(p, currentMode_, getCurrentToolWidth(), drawColor); // 呼び出し&RECTを返す
    }
    return {0, 0, 0, 0};
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
    if (index >= 0 && index < m_layers.size())
    {
        activeLayerIndex_ = index;
    }
}

void LayerManager::setHoveredLayer(int index)
{
    if (hoveredLayerIndex_ != index)
    {
        hoveredLayerIndex_ = index;
        std::string debug = "Hovered layer set to: " + std::to_string(index) + "\n";
        OutputDebugStringA(debug.c_str());
    }
}

void LayerManager::setCurrentMode(DrawMode mode)
{
    currentMode_ = mode;
}

// getter

ILayer *LayerManager::getActiveLayer() const
{
    if (activeLayerIndex_ >= 0 && activeLayerIndex_ < m_layers.size())
    {
        return m_layers[activeLayerIndex_].get();
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
    return m_layers;
}

int LayerManager::getActiveLayerIndex() const
{
    return activeLayerIndex_;
}

int LayerManager::getHoveredLayerIndex() const
{
    return hoveredLayerIndex_;
}

int LayerManager::getCanvasWidth() const
{
    if (m_layers.empty())
    {
        return 0;
    }
    // 最初のレイヤーの幅をキャンバスの幅とする
    return m_layers[0]->getWidth();
}

int LayerManager::getCanvasHeight() const
{
    if (m_layers.empty())
    {
        return 0;
    }
    // 最初のレイヤーの高さをキャンバスの高さとする
    return m_layers[0]->getHeight();
}
