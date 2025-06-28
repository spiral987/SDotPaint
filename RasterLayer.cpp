#include "RasterLayer.h"
#include <stdexcept> //ランタイムエラーメッセージのため
#include <algorithm>
#include <gdiplus.h>

using namespace Gdiplus;

// コンストラクタ ここで画用紙(ビットマップ)を作成する
RasterLayer::RasterLayer(int width, int height, std::wstring name)
    : width_(width), height_(height), name_(name)
{
    // 32ビットARGB形式でビットマップを作成
    // 作成時、全ピクセルは自動的に透明な黒でクリア
    hBitmap_ = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
    if (!hBitmap_)
    {
        throw std::runtime_error("Failed to create GDI+ Bitmap.");
    }
}

// デストラクタ
RasterLayer::~RasterLayer()
{
    // unique_ptrが自動的にhBitmap_を解放する
}

void RasterLayer::draw(Graphics *g) const
{
    if (g && hBitmap_)
    {
        // 自身のビットマップを、指定されたGraphicsオブジェクト（＝画面）に描画
        g->DrawImage(hBitmap_.get(), 0, 0);
    }
}

void RasterLayer::addPoint(const PenPoint &p, DrawMode mode, int width, COLORREF color)
{

    // Bitmapからこのレイヤー専用のGraphicsオブジェクトを作成
    Graphics layerGraphics(hBitmap_.get());
    // アンチエイリアシングを有効にして線を滑らかに
    layerGraphics.SetSmoothingMode(SmoothingModeAntiAlias);

    if (lastPoint_.point.x != -1) // 最初の点ではない場合
    {
        // GDI+の色オブジェクトの作成
        Color penColor;

        // 線の太さを計算
        float currentPressure = (float)p.pressure / 1023.0f;             // 現在の筆圧を0.0f-1.0fに正規化
        float lastPressure = (float)lastPoint_.pressure / 1023.0f;       // 直前の筆圧を正規化
        float averagePressure = (currentPressure + lastPressure) / 2.0f; // 平均の筆圧を計算

        // 最大幅を乗算して、実際のペンの太さを決定
        float penWidth = averagePressure * width;

        if (penWidth < 1.0f)
        {
            penWidth = 1.0f; // 最小でも1pxは保証する
        }

        if (mode == DrawMode::Pen)
        {
            // ペンモード：不透明で描画
            penColor.SetFromCOLORREF(color);
        }
        else
        { // Eraserモード
            // 消しゴムモード：透明色で描画することでピクセルを消す
            penColor = Color(0, 0, 0, 0);
        }

        Pen pen(penColor, penWidth);
        // 線の先端を丸くする
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);

        if (mode == DrawMode::Eraser)
        {
            // 消しゴムの場合は、合成モードを「Copy」に設定
            // これにより、描画先のピクセル値を完全に上書きする（透明で上書き＝消す）
            layerGraphics.SetCompositingMode(CompositingModeSourceCopy);
        }
        else
        {
            // ペンモードの場合は通常通りに合成
            layerGraphics.SetCompositingMode(CompositingModeSourceOver);
        }

        layerGraphics.DrawLine(&pen, lastPoint_.point.x, lastPoint_.point.y, p.point.x, p.point.y);
    }
    lastPoint_ = {p.point.x, p.point.y, p.pressure};
}

// clear: ビットマップ全体を白で塗りつぶす
void RasterLayer::clear()
{
    // レイヤーのGraphicsを取得
    Graphics layerGraphics(hBitmap_.get());
    // 透明色でレイヤー全体をクリア
    layerGraphics.Clear(Color(0, 0, 0, 0));
}

// startNewStroke: ペンを一度離した時の処理
void RasterLayer::startNewStroke()
{
    // 次のaddPointが呼ばれた時に、そこが新しい線の始点となるようにリセット
    lastPoint_ = {-1, -1};
}

const std::wstring &RasterLayer::getName() const
{
    return name_;
}

void RasterLayer::setName(const std::wstring &newName)
{
    name_ = newName;
}

// getStrokes: RasterLayerでは使わないので、空のリストを返すダミー実装
const std::vector<std::vector<PenPoint>> &RasterLayer::getStrokes() const
{
    // このメソッドが呼ばれないようにするのが理想だが、インターフェースにあるので実装は必要
    static const std::vector<std::vector<PenPoint>> empty_strokes;
    return empty_strokes;
}
