#pragma once

#include "PenData.h"
#include "DrawMode.h"

#include <windows.h>
#include <vector>

// すべてのレイヤーの基底となるインターフェースクラス
class ILayer
{
public:
    // 仮想デストラクタは必須です
    virtual ~ILayer() = default;

    // 純粋仮想関数（このクラスを継承するクラスは必ず実装しなければならない）
    virtual void draw(HDC hdc) const = 0;                        // 描画関数
    virtual void addPoint(const PenPoint &p, DrawMode mode) = 0; // 点を追加する関数
    virtual void clear() = 0;                                    // レイヤーをクリアする関数
    virtual void startNewStroke() = 0;                           // 新しい線が始まる命令

    virtual const std::vector<std::vector<PenPoint>> &getStrokes() const = 0; // 点のリストを取得する関数(テスト用)
};