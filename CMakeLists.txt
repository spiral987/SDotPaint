# CMakeの最低バージョンを指定
cmake_minimum_required(VERSION 3.14)

# プロジェクトの名前と使用言語を定義
project(SDotPaint LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++の言語標準を指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#--- 静的解析ツールの設定 ---
find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    message(STATUS "Clang-Tidy found at: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=-*,modernize-*,readability-*,performance-*,bugprone-*,cppcoreguidelines-*")
else()
    message(WARNING "Clang-Tidy not found. Static analysis will be skipped.")
endif()


#--- メインの実行ファイル定義 ---
add_executable(SDotPaint WIN32 main.cpp PaintModel.cpp)

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
enable_testing()
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG    v1.14.0 
)

# ★★★ この一行を復活させます！ ★★★
set(gtest_force_shared_crt ON CACHE BOOL "Force shared CRT on gtest")

FetchContent_MakeAvailable(googletest)

add_executable(
  SdotPaint_tests 
  tests/PaintModel.test.cpp
  PaintModel.cpp
)

target_link_libraries(
  SdotPaint_tests 
  PRIVATE gtest_main
)

target_include_directories(
    SdotPaint_tests 
    PRIVATE ${googletest_SOURCE_DIR}/googletest/include
)

include(GoogleTest)
gtest_discover_tests(SdotPaint_tests)