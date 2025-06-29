#include "RasterLayer.h"

#include <stdexcept> //ランタイムエラーメッセージのため
#include <algorithm>
#include <gdiplus.h>
#include <numeric>
#include <cmath>

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

void RasterLayer::draw(Graphics *g, float opacity) const
{
    if (g && hBitmap_)
    {
        if (opacity >= 1.0f)
        {
            // 不透明度が100%ならそのまま描画
            g->DrawImage(hBitmap_.get(), 0, 0);
        }
        else
        {
            // 半透明描画
            ImageAttributes imageAttr;
            ColorMatrix colorMatrix = {
                1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, opacity, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

            imageAttr.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

            g->DrawImage(hBitmap_.get(), Rect(0, 0, width_, height_), 0, 0, width_, height_, UnitPixel, &imageAttr);
        }
    }
}

RECT RasterLayer::addPoint(const PenPoint &p, DrawMode mode, int width, COLORREF color)
{

    // Bitmapからこのレイヤー専用のGraphicsオブジェクトを作成
    Graphics layerGraphics(hBitmap_.get());
    // アンチエイリアシングを有効にして線を滑らかに
    // layerGraphics.SetSmoothingMode(SmoothingModeAntiAlias);

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

        // 更新された領域を計算する
        RECT dirtyRect;
        dirtyRect.left = min(lastPoint_.point.x, p.point.x);
        dirtyRect.top = min(lastPoint_.point.y, p.point.y);
        dirtyRect.right = max(lastPoint_.point.x, p.point.x);
        dirtyRect.bottom = max(lastPoint_.point.y, p.point.y);

        // ペンの太さ分だけ矩形を広げる（安全マージン）
        int margin = (int)ceil(penWidth / 2.0f) + 2;
        InflateRect(&dirtyRect, margin, margin);

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
    return {p.point.x, p.point.y, p.point.x, p.point.y}; // 差分更新のためにRECTを返す。（最初の点の場合はその点自身を返す）
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

COLORREF RasterLayer::getAverageColor() const
{
    // ビットマップが存在しない場合はデフォルト色（白）を返す
    if (!hBitmap_)
    {
        return RGB(255, 255, 255);
    }

    long long totalR = 0;
    long long totalG = 0;
    long long totalB = 0;
    int nonTransparentPixels = 0;

    // ビットマップの領域をロックして、ピクセルデータに直接アクセスする
    BitmapData bitmapData;
    Rect rect(0, 0, width_, height_);
    hBitmap_->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    // ピクセルデータの先頭ポインタを取得
    BYTE *pixels = (BYTE *)bitmapData.Scan0;

    for (int y = 0; y < height_; y++)
    {
        // y行目の先頭のピクセルへのポインタ
        ARGB *line = (ARGB *)(pixels + y * bitmapData.Stride);

        for (int x = 0; x < width_; x++)
        {
            // (x, y) のピクセル色 (ARGB形式)
            ARGB color = line[x];

            // アルファ値（透明度）を取得
            BYTE alpha = (color >> 24) & 0xff;

            // 完全に透明ではないピクセルのみを計算対象にする
            if (alpha > 0)
            {
                totalB += (color >> 0) & 0xff;
                totalG += (color >> 8) & 0xff;
                totalR += (color >> 16) & 0xff;
                nonTransparentPixels++;
            }
        }
    }

    // ビットマップのロックを解除（非常に重要！）
    hBitmap_->UnlockBits(&bitmapData);

    // 色が描画されているピクセルが存在する場合
    if (nonTransparentPixels > 0)
    {
        BYTE avgR = (BYTE)(totalR / nonTransparentPixels);
        BYTE avgG = (BYTE)(totalG / nonTransparentPixels);
        BYTE avgB = (BYTE)(totalB / nonTransparentPixels);
        return RGB(avgR, avgG, avgB);
    }
    else
    {
        // 何も描画されていない場合は、デフォルト色（白）を返す
        return RGB(255, 255, 255);
    }
}

// getStrokes: RasterLayerでは使わないので、空のリストを返すダミー実装
const std::vector<std::vector<PenPoint>> &RasterLayer::getStrokes() const
{
    // このメソッドが呼ばれないようにするのが理想だが、インターフェースにあるので実装は必要
    static const std::vector<std::vector<PenPoint>> empty_strokes;
    return empty_strokes;
}
