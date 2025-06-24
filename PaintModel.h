// PaintModel.h

#ifndef PAINT_MODEL_H
#define PAINT_MODEL_H

#include <vector>
#include <windows.h> // POINT構造体を使用するために必要
#include <cstdint>

struct PenPoint
{
    POINT point;
    UINT32 pressure; // ペンの圧力
};

// アプリケーションのデータとロジックを管理するクラス
class PaintModel
{
private:
    // 点のリストを保持するメンバー変数
    std::vector<PenPoint> points_;

public:
    // 新しい点をリストに追加する
    void addPoint(PenPoint p);

    // 現在の点のリスト（読み取り専用=const）を取得する
    // 参照渡しにすると高速・省メモリ
    // 末尾のconstはメンバ変数を書き換えないことを保証するもの
    const std::vector<PenPoint> &getPoints() const;

    // すべての点をクリアする
    void clearPoints();
};

#endif // PAINT_MODEL_H