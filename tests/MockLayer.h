#pragma once
#include "../ILayer.h"
#include <vector>

// ILayerのフリをする、テスト用の偽物レイヤー
class MockLayer : public ILayer
{
public:
    // どのメソッドが呼ばれたかを記録するためのフラグ
    bool draw_was_called = false;
    bool addPoint_was_called = false;
    bool clear_was_called = false;
    bool startNewStroke_was_called = false;

    // 呼び出された時の引数を記録する変数
    DrawMode mode_passed;

    // --- ILayerのインターフェースを実装 ---
    void draw(HDC hdc) const override
    {
        // constメソッドなので、mutableキーワードでフラグの変更を許可する
        // (少し高度なテクニックです)
        const_cast<MockLayer *>(this)->draw_was_called = true;
    }

    void addPoint(const PenPoint &p, DrawMode mode) override
    {
        addPoint_was_called = true;
        mode_passed = mode; // 渡されたモードを記録
    }

    void clear() override
    {
        clear_was_called = true;
    }

    void startNewStroke() override
    {
        startNewStroke_was_called = true;
    }

    // 使わないメソッドは空実装
    void createNewRasterLayer(int, int, HDC) {}
    const std::vector<std::vector<PenPoint>> &getStrokes() const override
    {
        static std::vector<std::vector<PenPoint>> dummy;
        return dummy;
    }
};