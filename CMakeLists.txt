# CMakeの最低バージョンを指定
cmake_minimum_required(VERSION 3.14)

# プロジェクトの名前と使用言語を定義
project(SDotPaint LANGUAGES CXX)

# C++の言語標準を指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#--- 実行ファイルのターゲット定義 ---
add_executable(SDotPaint WIN32
    main.cpp 
    LayerManager.cpp
    VectorLayer.cpp
    RasterLayer.cpp
)

target_compile_definitions(SDotPaint PRIVATE
    UNICODE
    _UNICODE
)

target_link_libraries(SDotPaint PRIVATE
    gdi32
)


# ===================================================================
# 単体テスト (GoogleTest) の設定
# ===================================================================

# CTest（CMakeのテスト機能）を有効化
enable_testing()

# FetchContentモジュールをインクルード
include(FetchContent)

# GoogleTestの情報を宣言 (ダウンロード元を指定)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG    v1.14.0 
)

# GoogleTestに、我々のプロジェクトと同じランタイムライブラリを使ってもらうための設定
#set(gtest_force_shared_crt ON CACHE BOOL "Force shared CRT on gtest")

# GoogleTestを実際にダウンロードしてプロジェクトで利用可能にする
#FetchContent_MakeAvailable(googletest)


# テスト用の新しい実行ファイルを定義
#add_executable(
  #SdotPaint_tests
 # tests/LayerManager.test.cpp
 # LayerManager.cpp
#  RasterLayer.cpp
#)

# テスト用実行ファイルにGoogleTestをリンクする
#target_link_libraries(
#  SdotPaint_tests 
#  PRIVATE gtest_main
#)

# GoogleTestのインクルードディレクトリをテストターゲットに追加
#target_include_directories(
#    SdotPaint_tests 
#    PRIVATE ${googletest_SOURCE_DIR}/googletest/include
#)

# CTestにテストを追加する
#include(GoogleTest)
#gtest_discover_tests(SdotPaint_tests)