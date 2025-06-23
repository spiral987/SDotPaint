# CMakeの最低バージョンを指定します。この行は必須のおまじないです。
cmake_minimum_required(VERSION 3.12)

# プロジェクトの名前と使用言語を定義します。
# ここで指定した名前が、Visual Studioで開いた際のソリューション名などになります。
project(SDotPaint LANGUAGES CXX)

# C++の言語標準を指定します。モダンなC++を使うために設定を推奨します。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#--- ここからがビルド設定の核心部分です ---

# 実行ファイルを生成するターゲットを定義します。
# add_executable(<ターゲット名> [WIN32] [ソースファイル...])
#
# <ターゲット名>: SDotPaint という名前の実行ファイルを作ります。
# WIN32: これを指定すると、WindowsのGUIアプリケーションとして扱われます。
#        コンソール(黒い画面)が表示されなくなり、g++の-mwindowsオプションと同じ役割を果たします。
add_executable(SDotPaint WIN32 main.cpp)


# "SDotPaint"ターゲットに対して、コンパイル時の定義を追加します。
# これは g++ の -DUNICODE -D_UNICODE オプションに相当します。
target_compile_definitions(SDotPaint PRIVATE
    UNICODE
    _UNICODE
)

# "SDotPaint"ターゲットに対して、リンクするライブラリを指定します。
# これは g++ の -lgdi32 オプションに相当します。
target_link_libraries(SDotPaint PRIVATE
    gdi32
)