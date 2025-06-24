#include "PaintModel.h"
#include <iostream>

bool arePointsEqual(const PenPoint &p1, const PenPoint &p2)
{
    return (p1.point.x == p2.point.x && p1.point.y == p2.point.y && p1.pressure == p2.pressure);
}

void PaintModel::addPoint(PenPoint p)
{
    // 直前のポイントと同じ点を追加しようとしている場合は追加しない
    if (!points_.empty() && arePointsEqual(points_.back(), p))
    {
        return; // 何もしない
    }
    points_.push_back(p);
}

const std::vector<PenPoint> &PaintModel::getPoints() const
{
    return points_;
}

void PaintModel::clearPoints()
{
    // デバッグメッセージを表示するためのバッファ
    wchar_t debugMsg[256];
    swprintf_s(debugMsg, L"Clearing all points.\n"); // クリア前の点の数をデバッグ出力
    OutputDebugStringW(debugMsg);                    // デバッグ出力
    points_.clear();
}