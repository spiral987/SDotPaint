#include "gtest/gtest.h"
#include "core/LayerManager.h"
#include "MockLayer.h"
#include <memory>

// テストフィクスチャクラスを定義
class LayerManagerTest : public ::testing::Test
{
protected:
    // 各テストの前に自動で呼ばれる準備関数
    void SetUp() override
    {
        layerManager.init(100, 100);
    }

    // メンバー変数としてlayerManagerを持つ
    // コンストラクタでMockLayerを渡す
    LayerManager layerManager{std::make_unique<MockLayer>()};
};

// LayerManagerが正しくレイヤーに命令を伝達するかをテストする
TEST(LayerManagerTest, ForwardsCallsToActiveLayer)
{
    // 1. Arrange (準備)
    // テスト用のMockLayerを作成し、LayerManagerに注入する
    auto mock_layer = std::make_unique<MockLayer>();
    // MockLayerへのポインタを、後で確認するために保持しておく
    MockLayer *mock_layer_ptr = mock_layer.get();
    // テスト用コンストラクタでLayerManagerを生成
    LayerManager manager(std::move(mock_layer));

    // 2. Act (実行) & 3. Assert (検証)

    // manager.clear()を呼んだら、mock_layer.clear()が呼ばれるはず
    manager.clear();
    EXPECT_TRUE(mock_layer_ptr->clear_was_called);

    // manager.startNewStroke()を呼んだら、mock_layer.startNewStroke()が呼ばれるはず
    manager.startNewStroke();
    EXPECT_TRUE(mock_layer_ptr->startNewStroke_was_called);

    // manager.addPoint()を呼んだら、mock_layer.addPoint()が呼ばれるはず
    manager.addPoint({}); // ダミーのデータを渡す
    EXPECT_TRUE(mock_layer_ptr->addPoint_was_called);
}

// モード設定のテスト
TEST(LayerManagerTest, CorrectlyPassesDrawMode)
{
    // 1. Arrange
    auto mock_layer = std::make_unique<MockLayer>();
    MockLayer *mock_layer_ptr = mock_layer.get();
    LayerManager manager(std::move(mock_layer));

    // 2. Act
    // ペンモードに設定して、addPointを呼ぶ
    manager.setDrawMode(DrawMode::Pen);
    manager.addPoint({});
    // 3. Assert
    // レイヤーにはペンモードが渡されているはず
    EXPECT_EQ(mock_layer_ptr->mode_passed, DrawMode::Pen);

    // 2. Act
    // 消しゴムモードに設定して、addPointを呼ぶ
    manager.setDrawMode(DrawMode::Eraser);
    manager.addPoint({});
    // 3. Assert
    // レイヤーには消しゴムモードが渡されているはず
    EXPECT_EQ(mock_layer_ptr->mode_passed, DrawMode::Eraser);
}