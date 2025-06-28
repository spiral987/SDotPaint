#pragma once

#include "PenData.h"
#include "DrawMode.h"

#include <windows.h>
#include <vector>
#include <string>

// GDI+ の前方宣言
namespace Gdiplus
{
    class Graphics;
}

// すべてのレイヤーの基底となるインターフェースクラス
// LayerManagerでそれぞれのレイヤーを呼び出す際に、Layerクラスで実装しておくべき関数を定義する
class ILayer
{
public:
    // 仮想デストラクタは必須
    virtual ~ILayer() = default;

    // 純粋仮想関数（このクラスを継承するクラスは必ず実装しなければならない）
    virtual const std::wstring &getName() const = 0;                                        // レイヤー名を取得する関数
    virtual void draw(Gdiplus::Graphics *g) const = 0;                                      // 描画関数
    virtual void addPoint(const PenPoint &p, DrawMode mode, int width, COLORREF color) = 0; // 点を追加する関数
    virtual void clear() = 0;                                                               // レイヤーをクリアする関数
    virtual void startNewStroke() = 0;                                                      // 新しい線が始まる命令

    virtual const std::vector<std::vector<PenPoint>> &getStrokes() const = 0; // 点のリストを取得する関数(テスト用)
};