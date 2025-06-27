#include "RasterLayer.h"
#include <stdexcept> //ランタイムエラーメッセージのため

// コンストラクタ ここで画用紙(ビットマップ)を作成する
RasterLayer::RasterLayer(int width, int height, HDC hdc) : width_(width), height_(height)
{
    // 1. メモリデバイスコンテキスト(DC)を作成
    hMemoryDC_ = CreateCompatibleDC(hdc);
    if (!hMemoryDC_)
    {
        throw std::runtime_error("Failed to create memory DC.");
    }

    // 2. 描画ターゲットとなるビットマップを作成
    hBitmap_ = CreateCompatibleBitmap(hdc, width_, height_);
    if (!hBitmap_)
    {
        DeleteDC(hMemoryDC_); // 失敗したら後片付け
        throw std::runtime_error("Failed to create compatible bitmap.");
    }

    // 3. 作成したビットマップをメモリDCに選択（これでメモリDCが画用紙になる）
    SelectObject(hMemoryDC_, hBitmap_);

    // 4. 初期状態として、キャンバスを白で塗りつぶす
    clear();
}

// デストラクタ
RasterLayer::~RasterLayer()
{
    if (hCustomBrush_)
        DeleteObject(hCustomBrush_);
    if (hBitmap_)
        DeleteObject(hBitmap_);
    if (hMemoryDC_)
        DeleteDC(hMemoryDC_);
}

void RasterLayer::draw(HDC hdc) const
{
    BitBlt(hdc, 0, 0, width_, height_, hMemoryDC_, 0, 0, SRCCOPY);
}

void RasterLayer::setCustomBrush(const std::vector<POINT> &points)
{
    // 既存のカスタムブラシがあれば解放
    if (hCustomBrush_)
    {
        DeleteObject(hCustomBrush_);
        hCustomBrush_ = nullptr;
    }

    // 点がなければ何もしない
    if (points.empty())
        return;

    // 1. ブラシ用のビットマップとメモリDCを作成
    HDC hdc = GetDC(nullptr); // 画面のDCを取得
    hCustomBrush_ = CreateCompatibleBitmap(hMemoryDC_, brushWidth_, brushHeight_);
    HDC hBrushDC = CreateCompatibleDC(hMemoryDC_);
    SelectObject(hBrushDC, hCustomBrush_);

    // 2. ブラシの背景を透明色（ここではピンク）で塗りつぶす
    HBRUSH hTransparentBrush = CreateSolidBrush(RGB(255, 0, 255));
    RECT brushRect = {0, 0, brushWidth_, brushHeight_};
    FillRect(hBrushDC, &brushRect, hTransparentBrush);
    DeleteObject(hTransparentBrush);

    // 3. ブラシを黒で塗りつぶす
    HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
    SelectObject(hBrushDC, hBlackBrush);

    // 4. 受け取った座標リストを使って多角形を描画
    Polygon(hBrushDC, points.data(), (int)points.size());

    // 5. 後片付け
    DeleteObject(hBlackBrush);
    DeleteDC(hBrushDC);
    ReleaseDC(nullptr, hdc);
}

void RasterLayer::addPoint(const PenPoint &p, DrawMode mode)
{

    if (hCustomBrush_)
    {
        // ブラシ用のデバイスコンテキストを作成
        HDC hBrushDC = CreateCompatibleDC(hMemoryDC_);
        HBITMAP hOldBrushBitmap = (HBITMAP)SelectObject(hBrushDC, hCustomBrush_);

        // GDIのTransparentBlt関数で、ブラシの背景色(ピンク)を透明にしてスタンプする
        TransparentBlt(hMemoryDC_,                   // 描画先 (メインキャンバス)
                       p.point.x - brushWidth_ / 2,  // 描画先のx座標 (カーソル中心に)
                       p.point.y - brushHeight_ / 2, // 描画先のy座標 (カーソル中心に)
                       brushWidth_, brushHeight_,    // 描画するサイズ
                       hBrushDC,                     // 描画元 (ブラシ)
                       0, 0,                         // 描画元のx, y座標
                       brushWidth_, brushHeight_,    // 描画元のサイズ
                       RGB(255, 0, 255));            // 透明にする色

        // クリーンアップ
        SelectObject(hBrushDC, hOldBrushBitmap);
        DeleteDC(hBrushDC);
    }
    else
    {
        // 新しいストロークの最初の点の場合
        if (lastPoint_.x == -1)
        {
            lastPoint_ = p.point;
        }

        // モードに応じて色を変更する
        COLORREF color;
        if (mode == DrawMode::Eraser)
        {
            color = RGB(255, 255, 255);
        }
        else
        {
            color = RGB(0, 0, 0);
        }

        // 筆圧に応じたペンを作成
        int penWidth = (p.pressure * 15 / 1024) + 1;
        HPEN hPen = CreatePen(PS_SOLID, penWidth, color);
        HPEN hOldPen = (HPEN)SelectObject(hMemoryDC_, hPen);

        // 直前の点から現在の点まで、メモリDC上のビットマップに線を描く
        MoveToEx(hMemoryDC_, lastPoint_.x, lastPoint_.y, nullptr);
        LineTo(hMemoryDC_, p.point.x, p.point.y);

        SelectObject(hMemoryDC_, hOldPen);
        DeleteObject(hPen);

        // 次の描画のために現在の点の位置を保存
        lastPoint_ = p.point;
    }
}

// clear: ビットマップ全体を白で塗りつぶす
void RasterLayer::clear()
{
    RECT rect = {0, 0, width_, height_};
    FillRect(hMemoryDC_, &rect, (HBRUSH)(COLOR_WINDOW + 1));
}

// startNewStroke: ペンを一度離した時の処理
void RasterLayer::startNewStroke()
{
    // 次のaddPointが呼ばれた時に、そこが新しい線の始点となるようにリセット
    lastPoint_ = {-1, -1};
}

// getStrokes: RasterLayerでは使わないので、空のリストを返すダミー実装
const std::vector<std::vector<PenPoint>> &RasterLayer::getStrokes() const
{
    // このメソッドが呼ばれないようにするのが理想だが、インターフェースにあるので実装は必要
    static const std::vector<std::vector<PenPoint>> empty_strokes;
    return empty_strokes;
}
