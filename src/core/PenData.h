#pragma once
#include <windows.h>
#include <cstdint>

// ペン入力のデータ構造 いろいろなファイルから再利用する予定
struct PenPoint
{
    POINT point;     // 座標
    UINT32 pressure; // 筆圧
};

// 2つの点が同じか比較するヘルパー関数
// 頻繁に呼ばれるためinlineにしておく
inline bool arePointsEqual(const PenPoint &p1, const PenPoint &p2)
{
    return (p1.point.x == p2.point.x &&
            p1.point.y == p2.point.y &&
            p1.pressure == p2.pressure);
}