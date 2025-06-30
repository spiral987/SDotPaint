// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>
#include <debugapi.h> // OutputDebugStringW を使う
#include <CommCtrl.h>
#include <string>
#include <memory>
#include <cmath> // 数学関数(atan2f)のために必要

#include "globals.h"
#include "MessageHandler.h"
#include "core/LayerManager.h"
#include "core/PenData.h"
#include "ui/UIManager.h"
#include "ui/UIHandlers.h"

// GDI+のため
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// インスタンスを保持するユニークポインタ
std::unique_ptr<UIManager> g_pUIManager;
std::unique_ptr<MessageHandler> g_pMessageHandler;
LayerManager layer_manager; // LayerManagerのインスタンス

ULONG_PTR gdiplusToken;

// ダブルバッファリング用の変数
Bitmap *g_pBackBuffer = nullptr;
int g_nClientWidth = 0;
int g_nClientHeight = 0;

// 視点移動用のグローバル変数
bool g_isPanMode = false;           // 視点移動モードかどうかのフラグ
POINT g_panLastPoint = {0, 0};      // 視点移動時の最後のマウス位置
PointF g_viewCenter = {0.0f, 0.0f}; // ワールド座標系でのビュー中心
//  POINT g_viewOffset = {0, 0};   // 視点のオフセット量

bool g_isRotateMode = false;  // 回転モードかどうかのフラグ
float g_rotationAngle = 0.0f; // 現在の総回転角度
float g_startAngle = 0.0f;    // 回転開始時の角度

bool g_isZoomMode = false;               // ズームモードかどうかのフラグ
float g_zoomFactor = 1.0f;               // 現在のズーム率
float g_baseZoomFactor = 1.0f;           // ズーム開始時のズーム率
POINT g_zoomStartPoint = {0, 0};         // ズーム開始時のスクリーン座標
PointF g_zoomCenterWorld = {0.0f, 0.0f}; // ズーム基点のワールド座標

bool g_isPenContact = false; // ペンの接触状態を自前で管理するフラグ

bool g_isTransforming = false; // 視点操作中かどうかのフラグ

// マウスリーブイベントをトラックするためのフラグ
bool g_bTrackingMouse = false;

// グローバル変数に「前回のスクリーン座標」を保持する変数を追加
POINT g_lastScreenPoint = {-1, -1};
UINT32 g_lastPressure = 0;

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// main関数の代わりに使用されるWinMain関数
int WINAPI WinMain(
    HINSTANCE hInstance,     // プログラムを識別するためのインスタンスハンドル
    HINSTANCE hPrevInstance, // 常にNULL（古いWindowsの名残）
    LPSTR lpszCmdLine,       // コマンドライン引数
    int nShowCmd)            // ウィンドウの表示状態
{
    OutputDebugStringW(L"--- WinMain STARTED---\n");

    // GDI+の初期化
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // ウィンドウクラスの設計と登録
    // ----------------------------------------------------------------

    const wchar_t CLASS_NAME[] = L"Sample Window Class"; // ウィンドウクラス名

    WNDCLASSEXW wc = {}; //! WNDCLASSEX構造体をゼロで初期化　これはウインドウの設計図になる

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW; // ウィンドウサイズ変更時に再描画
    wc.lpfnWndProc = WindowProc;        //! メッセージを処理する関数（ウィンドウプロシージャ）を指定
    wc.hInstance = hInstance;           // このプログラムのインスタンスハンドル
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);   // デフォルトの矢印カーソル
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // ウィンドウの背景色
    wc.lpszClassName = L"SDotPaintWindow";         // ★クラス名を設定★

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"ウィンドウクラスの登録に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // ウィンドウの生成
    // ----------------------------------------------------------------

    HWND hwnd = CreateWindowExW(
        0,                                     // 拡張ウィンドウスタイル
        L"SDotPaintWindow",                    // ウィンドウのタイトル
        L"DotPaint - お絵描きアプリ",          // ★ウィンドウタイトル★
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, // 最も一般的なウィンドウスタイル
        // 位置とサイズ (CW_USEDEFAULTでOSに任せる)
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr,   // 親ウィンドウハンドル
        nullptr,   // メニューハンドル
        hInstance, // このプログラムのインスタンスハンドル
        nullptr    // 追加のアプリケーションデータ
    );
    if (hwnd == nullptr)
    {
        MessageBoxW(nullptr, L"ウィンドウの生成に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // タッチ対応ウィンドウとして登録する
    EnableMouseInPointer(TRUE);

    // 作成したウィンドウを表示
    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    // メッセージループ（ウインドウがメッセージの発生を監視するためのループ）
    // ----------------------------------------------------------------

    MSG msg = {};
    // アプリケーションが終了するまでメッセージを取得し続ける
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg); // キーボード入力を文字メッセージに変換
        DispatchMessage(&msg);  // メッセージを適切なウィンドウプロシージャに送る
    }

    GdiplusShutdown(gdiplusToken); // GDI+のシャットダウン
    return (int)msg.wParam;        // メッセージループが終了したときの戻り値
}

// ウィンドウプロシージャ（メッセージが発せられたときに呼び出される関数）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    if (uMsg == WM_CREATE)
    {
        g_pMessageHandler = std::make_unique<MessageHandler>(hwnd);
        return 0;
    }

    if (g_pMessageHandler)
    {
        // MessageHandlerが処理し、その結果を返す
        return g_pMessageHandler->ProcessMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
