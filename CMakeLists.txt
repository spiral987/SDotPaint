# CMakeの最低バージョンを指定
cmake_minimum_required(VERSION 3.20)

# プロジェクトの名前と使用言語を定義
project(SDotPaint VERSION 1.0.0 LANGUAGES CXX)

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
file(GLOB_RECURSE SOURCES "src/*.cpp")


# 実行ファイルの作成
# 収集したソースファイルを使って実行ファイルを作成
add_executable(${EXECUTABLE_NAME} WIN32 ${SOURCES})

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


