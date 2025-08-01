#include "gtest/gtest.h"        // Google Testのヘッダー
#include "layers/RasterLayer.h" // テスト対象のRasterLayerクラスのヘッダー

// RasterLayerのコンストラクタと基本的なgetterのテスト
TEST(RasterLayerTest, ConstructorAndGetters)
{
    int width = 100;
    int height = 200;
    std::wstring name = L"TestLayer";

    // RasterLayerオブジェクトを作成
    RasterLayer layer(width, height, name);

    // 幅と高さが正しく設定されているか確認
    ASSERT_EQ(layer.getWidth(), width);
    ASSERT_EQ(layer.getHeight(), height);

    // 名前が正しく設定されているか確認
    ASSERT_EQ(layer.getName(), name);
}

// clearメソッドのテスト (初期状態では全て透明なので、clearしても透明のまま)
TEST(RasterLayerTest, ClearMethod)
{
    int width = 50;
    int height = 50;
    std::wstring name = L"ClearTestLayer";
    RasterLayer layer(width, height, name);

    // clearを呼び出す
    layer.clear();

    // 平均色が透明（黒）であることを期待（GDI+のBitmap初期化とClear動作に基づく）
    // RGB(0,0,0)は透明な黒のデフォルトとして扱われる可能性がある
    // しかし、透明度情報がCOLORREFには含まれないため、ここでは完全に透明であることを直接確認することは難しい。
    // そのため、ここではclearがエラーなく実行されることと、将来的に可視ピクセルがないこと（白）を期待する
    // RasterLayer::getAverageColor()は、透明なピクセルを無視して平均色を計算するため、
    // 全ピクセルが透明な場合は白 (RGB(255, 255, 255)) を返します。
    ASSERT_EQ(layer.getAverageColor(), RGB(255, 255, 255));
}