#include "VectorLayer.h"
#include <vector>

// 描画処理
void VectorLayer::draw(HDC hdc) const
{

    // すべてのストロークデータを処理
    for (const auto &stroke : strokes_)
    {
        // 線の点が2つ未満の場合は線を描画できないので何もしない
        if (stroke.size() < 2)
        {
            continue;
        }
        // ストロークの線を結んでいく
        for (size_t i = 0; i < stroke.size() - 1; i++)
        {
            const auto &p1 = stroke[i];
            const auto &p2 = stroke[i + 1];

            // 筆圧に応じて線の太さを計算 (1-16pxの範囲)
            int penWidth = (p1.pressure * 15 / 1024) + 1;

            // このセグメント用のペンを作成
            HPEN hPen = CreatePen(PS_SOLID, penWidth, RGB(0, 0, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            // p1からp2へ直線を引く (線形補間)
            MoveToEx(hdc, p1.point.x, p1.point.y, nullptr);
            LineTo(hdc, p2.point.x, p2.point.y);

            // ペンの後片付け
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }
    }
}

void VectorLayer::addPoint(const PenPoint &p, DrawMode mode, int width)
{
    // ストロークが無ければ何もしない
    if (strokes_.empty())
    {
        return;
    }

    auto &current_stroke = strokes_.back();

    // 直前と同じでなければ追加
    if (current_stroke.empty() || !arePointsEqual(current_stroke.back(), p))
    {
        current_stroke.push_back(p);
    }
}

void VectorLayer::clear()
{
    strokes_.clear();
}

void VectorLayer::startNewStroke()
{
    strokes_.push_back({}); // 新しいストロークを開始するために空のベクターを追加
}

const std::vector<std::vector<PenPoint>> &VectorLayer::getStrokes() const
{
    return strokes_;
}
