#include "RasterLayer.h"

#include <stdexcept> //ランタイムエラーメッセージのため
#include <algorithm>
#include <gdiplus.h>
#include <numeric>

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

void RasterLayer::draw(Graphics *g, float opacity, Image *pTexture) const
{
    if (!g)
        return;

    // 描画品質を設定
    g->SetSmoothingMode(SmoothingModeAntiAlias);

    // 全てのストロークを順番に描画していく
    for (const auto &stroke : m_strokes)
    {
        if (stroke.points.size() < 1)
            continue; // 点がなければスキップ

        // 1. ストロークの軌跡から、塗りつぶすべき領域の形（パス）を作成する
        GraphicsPath path;
        // ストロークの各点を円で繋いで、滑らかな線の形を作る
        for (size_t i = 0; i < stroke.points.size(); ++i)
        {
            const auto &point = stroke.points[i];
            float pressure = point.pressure / 1023.0f;
            float width = stroke.penWidth * pressure;
            if (width < 1.0f)
                width = 1.0f;

            // 点の位置に円を追加する
            path.AddEllipse(
                (float)point.point.x - width / 2.0f,
                (float)point.point.y - width / 2.0f,
                width,
                width);

            // 点が2つ以上ある場合、円と円の間を繋いで隙間をなくす
            if (i > 0)
            {
                const auto &prevPoint = stroke.points[i - 1];
                float prevPressure = prevPoint.pressure / 1023.0f;
                float prevWidth = stroke.penWidth * prevPressure;
                if (prevWidth < 1.0f)
                    prevWidth = 1.0f;

                PointF points[] = {
                    PointF((float)prevPoint.point.x, (float)prevPoint.point.y),
                    PointF((float)point.point.x, (float)point.point.y)};
                Pen connectionPen(Color::Black, prevWidth > width ? prevWidth : width); // 太い方に合わせる
                path.AddPolygon(points, 2);                                             // この実装は完全ではないが、隙間を埋める簡単な方法
            }
        }

        // 2. ブラシを作成する
        // テクスチャが指定されていればテクスチャブラシ、なければ単色ブラシ
        std::unique_ptr<Brush> brush;
        if (pTexture && stroke.mode == DrawMode::Pen)
        {
            brush = std::make_unique<TextureBrush>(pTexture);
        }
        else if (stroke.mode == DrawMode::Pen)
        {
            Color penColor;
            penColor.SetFromCOLORREF(stroke.color);
            brush = std::make_unique<SolidBrush>(penColor);
        }
        else // Eraserモード
        {
            // 消しゴムは未実装。実装する場合は、
            // 別途ビットマップに描画し、それをマスクとして使うなどの工夫が必要になります。
            // ここでは一旦何もしないでおきます。
            continue;
        }

        // 3. 出来上がった線の形（パス）をブラシで塗りつぶす
        g->FillPath(brush.get(), &path);
    }
}

void RasterLayer::addPoint(const PenPoint &p, DrawMode mode, int width, COLORREF color)
{

    // ストロークがなければ何もしない（startNewStrokeが呼ばれていない）
    if (m_strokes.empty())
    {
        return;
    }

    // 現在のストローク（＝最後に追加されたストローク）に点を追加
    m_strokes.back().points.push_back(p);
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
    // 新しいStrokeオブジェクトを作成し、m_strokesベクターの末尾に追加
    m_strokes.emplace_back(Stroke{
        {},    // points (空のベクター)
        width, // penWidth
        color, // color
        mode   // mode
    });
}

// ILayerの仮想関数をオーバーライドするためのダミー実装
void RasterLayer::startNewStroke()
{
    // 何もしない。代わりに引数付きのstartNewStrokeが呼ばれることを期待する。
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
