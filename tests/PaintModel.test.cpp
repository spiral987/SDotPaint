#include "gtest/gtest.h"
#include "../PaintModel.h"

// TEST(テストフィクスチャ名, テストケース名)
TEST(PaintModelTest, IsInitiallyEmpty)
{
    // 1. Arrange (準備)
    PaintModel model;

    // 2. Act (実行)
    // - 初期状態をテストするので、実行するアクションはなし

    // 3. Assert (検証)
    // model.getPoints() で取得したベクターのサイズが 0 であることを期待する
    EXPECT_EQ(model.getPoints().size(), 0);
}

// 点を追加するテストケース
TEST(PaintModelTest, AddPointIncreasesCountAndStoresCorrectCoordinates)
{
    // 1. Arrange (準備)
    PaintModel model;

    // 2. Act (実行) - 1回目の追加
    model.addPoint({10, 20});

    // 3. Assert (検証) - 1回目
    EXPECT_EQ(model.getPoints().size(), 1); // サイズが1になっているか
    EXPECT_EQ(model.getPoints()[0].x, 10);  // 最初の点のX座標が正しいか
    EXPECT_EQ(model.getPoints()[0].y, 20);  // 最初の点のY座標が正しいか

    // 2. Act (実行) - 2回目の追加
    model.addPoint({-5, 100});

    // 3. Assert (検証) - 2回目
    EXPECT_EQ(model.getPoints().size(), 2); // サイズが2になっているか
    EXPECT_EQ(model.getPoints()[1].x, -5);  // 2番目の点のX座標が正しいか
    EXPECT_EQ(model.getPoints()[1].y, 100); // 2番目の点のY座標が正しいか
}

// 点を消去するテストケース
TEST(PaintModelTest, ClearPointsRemovesAllPoints)
{
    // 1. Arrange (準備)
    PaintModel model;
    model.addPoint({1, 1});
    model.addPoint({2, 2});
    model.addPoint({3, 3});
    ASSERT_EQ(model.getPoints().size(), 3); // 準備が正しいことを念のため確認

    // 2. Act (実行)
    model.clearPoints();

    // 3. Assert (検証)
    // clear後は点の数が0に戻っていることを期待する
    EXPECT_EQ(model.getPoints().size(), 0);
}