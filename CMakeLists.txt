# WinHTTPを使用した軽量版CMakeLists.txt（cpr不要）
cmake_minimum_required(VERSION 3.14)
project(SDotPaint LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ビルド最適化設定
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")

# CPUコア数に基づく並列ビルド設定
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

include(FetchContent)

# nlohmann/jsonのみ使用（cprは不要）
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG "v3.11.3"
)
FetchContent_MakeAvailable(nlohmann_json)

#--- 実行ファイルのターゲット定義 ---
add_executable(SDotPaint WIN32
    main.cpp
    LayerManager.cpp
    RasterLayer.cpp
    GeminiService-winhttp.cpp  # WinHTTP版を使用
)

target_compile_definitions(SDotPaint PRIVATE
    UNICODE
    _UNICODE
)

#--- ライブラリのリンク ---
target_link_libraries(SDotPaint PRIVATE
    gdi32
    msimg32   # TransparentBlt等のGDI拡張機能
    nlohmann_json::nlohmann_json
    winhttp    # Windows標準のHTTPライブラリ
    ws2_32
)

# 単体テスト (GoogleTest) の設定
enable_testing()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0 
)

set(gtest_force_shared_crt ON CACHE BOOL "Force shared CRT on gtest")
FetchContent_MakeAvailable(googletest)

# テストは一旦コメントアウト
# add_executable(
#   SdotPaint_tests
#   tests/LayerManager.test.cpp
#   LayerManager.cpp
# )
# 
# target_link_libraries(
#   SdotPaint_tests 
#   PRIVATE gtest_main
#   nlohmann_json::nlohmann_json
# )
# 
# include(GoogleTest)
# gtest_discover_tests(SdotPaint_tests)
