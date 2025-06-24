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
TEST(PaintModelTest, AddPointIncreasesCountAndStoresCorrectData)
{
    // 1. Arrange
    PaintModel model;
    PenPoint p1 = {{10, 20}, 512};   // 座標(10, 20), 筆圧512
    PenPoint p2 = {{-5, 100}, 1024}; // 座標(-5, 100), 筆圧1024

    // 2. Act - 1回目の追加
    model.addPoint(p1);

    // 3. Assert - 1回目
    ASSERT_EQ(model.getPoints().size(), 1);
    EXPECT_EQ(model.getPoints()[0].point.x, 10);
    EXPECT_EQ(model.getPoints()[0].point.y, 20);
    EXPECT_EQ(model.getPoints()[0].pressure, 512); // 筆圧もテストする

    // 2. Act - 2回目の追加
    model.addPoint(p2);

    // 3. Assert - 2回目
    ASSERT_EQ(model.getPoints().size(), 2);
    EXPECT_EQ(model.getPoints()[1].point.x, -5);
    EXPECT_EQ(model.getPoints()[1].point.y, 100);
    EXPECT_EQ(model.getPoints()[1].pressure, 1024); // 筆圧もテストする
}

// 点を消去するテストケース
TEST(PaintModelTest, ClearPointsRemovesAllPoints)
{
    // 1. Arrange (準備)
    PaintModel model;
    model.addPoint({{1, 1}, 100});
    model.addPoint({{2, 2}, 200});
    ASSERT_EQ(model.getPoints().size(), 2); // 準備が正しいことを念のため確認

    // 2. Act (実行)
    model.clearPoints();

    // 3. Assert (検証)
    // clear後は点の数が0に戻っていることを期待する
    EXPECT_EQ(model.getPoints().size(), 0);
}

// 連続した重複ポイントは追加しないことをテストする
TEST(PaintModelTest, DoesNotAddDuplicateConsecutivePoints)
{
    // 1. Arrange (準備)
    PaintModel model;
    PenPoint p1 = {{10, 20}, 512};
    model.addPoint(p1);
    // この時点で点の数は1のはず
    ASSERT_EQ(model.getPoints().size(), 1);

    // 2. Act (実行)
    // 全く同じ点をもう一度追加しようとする
    model.addPoint(p1);

    // 3. Assert (検証)
    // 重複は追加されないはずなので、点の数は1のままであることを期待する
    EXPECT_EQ(model.getPoints().size(), 1);
}
