# CMakeの最低バージョンを指定
cmake_minimum_required(VERSION 3.20)

# プロジェクトの名前と使用言語を定義
project(SDotPaint VERSION 1.0.1 LANGUAGES CXX)

# C++の言語標準を指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# プロジェクトをUnicode対応にする（ウインドウタイトル用）
add_compile_definitions(UNICODE)
add_compile_definitions(_UNICODE)


# 実行ファイル名を設定
set(EXECUTABLE_NAME "SDotPaint")


# ソースファイルの収集

# srcディレクトリ以下のすべての.cppファイルを変数SOURCESに格納
file(GLOB_RECURSE APP_SOURCES "src/*.cpp")

# --- デバッグ用ログの追加 ---
message(STATUS "APP_SOURCES before REMOVE_ITEM: ${APP_SOURCES}")

# テストファイルをメインアプリケーションのビルドから除外
list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/test/RasterLayerTest.cpp")

# --- デバッグ用ログの追加 ---
message(STATUS "APP_SOURCES after REMOVE_ITEM: ${APP_SOURCES}")

# 実行ファイルの作成
# 収集したソースファイルを使って実行ファイルを作成
add_executable(${EXECUTABLE_NAME} WIN32 ${APP_SOURCES})

# add_executableの後にインクルードディレクトリを指定
target_include_directories(${EXECUTABLE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# ソースファイルの文字コードをUTF-8として扱う設定 (MSVCコンパイラ用)
if(MSVC)
  target_compile_options(${EXECUTABLE_NAME} PRIVATE "/utf-8")
endif()


# リンクするライブラリの設定
# 必要なWindowsのライブラリをリンク
target_link_libraries(${EXECUTABLE_NAME}
    gdi32
    user32
    d2d1
    dwrite
    windowscodecs
    comdlg32
    comctl32
)

# Google Testを追加 (テストが有効な場合のみビルド)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)


# 新しいテスト実行ファイルを作成 (TestRunner)
add_executable(SDotPaint_TestRunner
    src/test/RasterLayerTest.cpp
    src/layers/RasterLayer.cpp    
    )

# テスト実行ファイルをGoogle Testライブラリにリンク
target_link_libraries(SDotPaint_TestRunner
    GTest::gtest_main
    GTest::gtest
    gdiplus
)

# テスト実行ファイルにもインクルードディレクトリを追加
target_include_directories(SDotPaint_TestRunner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${googletest_SOURCE_DIR}/googletest/include
)

# テストを有効にする
enable_testing()

# テストを追加
add_test(NAME RasterLayerTest COMMAND SDotPaint_TestRunner)