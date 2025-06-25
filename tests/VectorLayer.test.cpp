#include "gtest/gtest.h"
#include "../VectorLayer.h" // テスト対象をVectorLayerに変更

// テストフィクスチャ名を VectorLayerTest に変更
TEST(VectorLayerTest, IsInitiallyEmpty)
{
    VectorLayer layer;
    EXPECT_EQ(layer.getStrokes().size(), 0);
}

// 点を追加するテストケース
TEST(VectorLayerTest, AddPointIncreasesCountAndStoresCorrectData)
{
    VectorLayer layer;
    PenPoint p1 = {{10, 20}, 512};
    PenPoint p2 = {{-5, 100}, 1024};

    // 最初のストロークを開始
    layer.startNewStroke();
    layer.addPoint(p1);

    // ストローク数とポイント数を確認
    ASSERT_EQ(layer.getStrokes().size(), 1);
    ASSERT_EQ(layer.getStrokes()[0].size(), 1);
    EXPECT_EQ(layer.getStrokes()[0][0].point.x, 10);
    EXPECT_EQ(layer.getStrokes()[0][0].point.y, 20);
    EXPECT_EQ(layer.getStrokes()[0][0].pressure, 512);

    // 同じストロークに2つ目の点を追加
    layer.addPoint(p2);
    ASSERT_EQ(layer.getStrokes().size(), 1);
    ASSERT_EQ(layer.getStrokes()[0].size(), 2);
    EXPECT_EQ(layer.getStrokes()[0][1].point.x, -5);
    EXPECT_EQ(layer.getStrokes()[0][1].point.y, 100);
    EXPECT_EQ(layer.getStrokes()[0][1].pressure, 1024);
}

// 点を消去するテストケース
TEST(VectorLayerTest, ClearPointsRemovesAllPoints)
{
    VectorLayer layer;
    layer.startNewStroke();
    layer.addPoint({{1, 1}, 100});
    layer.addPoint({{2, 2}, 200});
    ASSERT_EQ(layer.getStrokes().size(), 1);
    ASSERT_EQ(layer.getStrokes()[0].size(), 2);

    layer.clear();
    EXPECT_EQ(layer.getStrokes().size(), 0);
}

// 複数のストロークを管理できることを確認するテストケース
TEST(VectorLayerTest, ManagesMultipleStrokes)
{
    VectorLayer layer;

    // 最初のストローク
    layer.startNewStroke();
    layer.addPoint({{10, 10}, 512});
    layer.addPoint({{20, 20}, 600});

    // 2つ目のストローク
    layer.startNewStroke();
    layer.addPoint({{30, 30}, 700});

    // 2つのストロークがあることを確認
    ASSERT_EQ(layer.getStrokes().size(), 2);
    EXPECT_EQ(layer.getStrokes()[0].size(), 2);
    EXPECT_EQ(layer.getStrokes()[1].size(), 1);

    // 各ストロークの内容を確認
    EXPECT_EQ(layer.getStrokes()[0][0].point.x, 10);
    EXPECT_EQ(layer.getStrokes()[0][1].point.x, 20);
    EXPECT_EQ(layer.getStrokes()[1][0].point.x, 30);
}