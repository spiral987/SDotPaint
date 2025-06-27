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
    if (hBitmap_)
        DeleteObject(hBitmap_);
    if (hMemoryDC_)
        DeleteDC(hMemoryDC_);
}

void RasterLayer::draw(HDC hdc) const
{
    BitBlt(hdc, 0, 0, width_, height_, hMemoryDC_, 0, 0, SRCCOPY);
}

void RasterLayer::addPoint(const PenPoint &p, DrawMode mode, int width)
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

    int finalWidth = max(1, static_cast<int>(width * (p.pressure / 1023.0f)));

    // 筆圧に応じたペンを作成
    HPEN hPen = CreatePen(PS_SOLID, finalWidth, color);
    HPEN hOldPen = (HPEN)SelectObject(hMemoryDC_, hPen);

    // 直前の点から現在の点まで、メモリDC上のビットマップに線を描く
    MoveToEx(hMemoryDC_, lastPoint_.x, lastPoint_.y, nullptr);
    LineTo(hMemoryDC_, p.point.x, p.point.y);

    SelectObject(hMemoryDC_, hOldPen);
    DeleteObject(hPen);

    // 次の描画のために現在の点の位置を保存
    lastPoint_ = p.point;
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
