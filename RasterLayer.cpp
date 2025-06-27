#include "RasterLayer.h"
#include <stdexcept> //ランタイムエラーメッセージのため
#include <random>    // 乱数生成のために追加

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
    // テクスチャブラシが設定されている場合
    if (hTextureBrush_)
    {
        if (mode == DrawMode::Eraser)
        {
            // 消しゴムモードの場合は従来の処理を流用 (またはテクスチャ消しゴムを別途実装)
            // ... (従来の消しゴム処理)
            return;
        }

        HDC hBrushDC = CreateCompatibleDC(hMemoryDC_);
        SelectObject(hBrushDC, hTextureBrush_);

        int destX = p.point.x - brushWidth_ / 2;
        int destY = p.point.y - brushHeight_ / 2;

        BLENDFUNCTION blendFunc = {};
        blendFunc.BlendOp = AC_SRC_OVER;
        blendFunc.SourceConstantAlpha = 255;  // ビットマップのアルファ値を使用
        blendFunc.AlphaFormat = AC_SRC_ALPHA; // ソースがアルファ値を持つことを示す

        // アルファブレンディングで描画
        AlphaBlend(hMemoryDC_, destX, destY, brushWidth_, brushHeight_,
                   hBrushDC, 0, 0, brushWidth_, brushHeight_, blendFunc);

        DeleteDC(hBrushDC);
    }
    else if (hCustomBrush_)
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

void RasterLayer::setTextureBrush(const TextureBrushParams &params)
{
    textureParams_ = params;

    // 既存のブラシを解放
    if (hTextureBrush_)
    {
        DeleteObject(hTextureBrush_);
        hTextureBrush_ = nullptr;
    }

    // 32bppのアルファチャンネル付きビットマップを作成
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = brushWidth_;
    bmi.bmiHeader.biHeight = -brushHeight_; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *pPixels = nullptr;
    HDC hdc = GetDC(nullptr);
    hTextureBrush_ = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pPixels, NULL, 0);
    ReleaseDC(nullptr, hdc);

    if (!hTextureBrush_)
        return;

    // 乱数生成の準備
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> pos_dist(0, brushWidth_ - 1);
    std::uniform_int_distribution<> size_dist(params.dot_size_min, params.dot_size_max);
    std::uniform_int_distribution<> alpha_dist(params.alpha_min, params.alpha_max);

    // 点の総数を計算
    int total_dots = (brushWidth_ * brushHeight_ * params.density) / 100;

    // ピクセルデータへのアクセス
    DWORD *pixels = (DWORD *)pPixels;

    // ランダムな点を描画していく
    for (int i = 0; i < total_dots; ++i)
    {
        int x = pos_dist(gen);
        int y = pos_dist(gen);
        int size = size_dist(gen);
        BYTE alpha = (BYTE)alpha_dist(gen);

        // 円形のブラシエリア内に点を描画
        int dx_from_center = x - brushWidth_ / 2;
        int dy_from_center = y - brushHeight_ / 2;
        if ((dx_from_center * dx_from_center + dy_from_center * dy_from_center) < (brushWidth_ * brushWidth_ / 4))
        {
            // 簡単のため、ここでは1x1の点を描画 (sizeに応じた円を描画するとより高品質になる)
            if (x >= 0 && x < brushWidth_ && y >= 0 && y < brushHeight_)
            {
                // BGRA形式でピクセルを設定 (黒い点)
                pixels[y * brushWidth_ + x] = (alpha << 24) | (0x000000);
            }
        }
    }
}
