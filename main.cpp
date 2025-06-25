// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>
#include "LayerManager.h"
#include "PenData.h"

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理します
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 描画ポイント追加関数のプロトタイプ宣言
void AddDrawingPoint(HWND hwnd, WPARAM wParam, LPARAM lParam, LayerManager &layer_manager);

// main関数の代わりに使用されるWinMain関数
int WINAPI WinMain(
    HINSTANCE hInstance,     // プログラムを識別するためのインスタンスハンドル
    HINSTANCE hPrevInstance, // 常にNULL（古いWindowsの名残）
    LPSTR lpszCmdLine,       // コマンドライン引数
    int nShowCmd)            // ウィンドウの表示状態
{
    // 1. ウィンドウクラスの設計と登録
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
    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"ウィンドウクラスの登録に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // 2. ウィンドウの生成
    // ----------------------------------------------------------------

    HWND hwnd = CreateWindowExW(
        0,                       // 拡張ウィンドウスタイル
        CLASS_NAME,              // 登録したウィンドウクラス名
        L"My Paint Application", // ウィンドウのタイトル
        WS_OVERLAPPEDWINDOW,     // 最も一般的なウィンドウスタイル

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

    // 3. メッセージループ（ウインドウがメッセージの発生を監視するためのループ）
    // ----------------------------------------------------------------

    MSG msg = {};
    // アプリケーションが終了するまでメッセージを取得し続ける
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg); // キーボード入力を文字メッセージに変換
        DispatchMessage(&msg);  // メッセージを適切なウィンドウプロシージャに送る
    }

    return (int)msg.wParam; // メッセージループが終了したときの戻り値
}

// 共通の描画ポイント追加処理
void AddDrawingPoint(HWND hwnd, WPARAM wParam, LPARAM lParam, LayerManager &layer_manager)
{

    // ポインターIDを取得
    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);

    // ポインターの基本情報を取得
    POINTER_INFO pointerInfo;
    if (!GetPointerInfo(pointerId, &pointerInfo))
    {
        // ポインター情報の取得に失敗
        return;
    }

    POINT screenPoint = pointerInfo.ptPixelLocation; //* ポインターの画面上の位置を取得
    ScreenToClient(hwnd, &screenPoint);              //* 画面座標をクライアント座標に変換

    UINT32 pressure = 512; // デフォルト筆圧（中間値）

    if (pointerInfo.pointerType == PT_PEN)
    {
        // ペンの詳細情報を取得
        POINTER_PEN_INFO penInfo;
        if (GetPointerPenInfo(pointerId, &penInfo))
        {
            POINTER_PEN_INFO penInfo;

            // 筆圧値を取得（0-1024の範囲）
            if (GetPointerPenInfo(pointerId, &penInfo) && penInfo.pressure <= 1024)
            {
                pressure = penInfo.pressure;
            }
        }
    }

    layer_manager.addPoint({screenPoint, pressure}); // ペイントモデルにポイントを追加
}

// ウィンドウプロシージャ（メッセージが発せられたときに呼び出される関数）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // マウスの位置を保持するための変数。static変数を使用して、ウィンドウプロシージャが呼び出されるたびに初期化されないようにする
    static LayerManager layer_manager; // LayerManagerのstaticなインスタンス    // メッセージの種類に応じて処理を分岐
    switch (uMsg)
    { // ペンでタッチダウンしたときのメッセージ
    case WM_POINTERDOWN:
    {
        // 右ボタンがクリックされた場合はクリア処理
        if (IS_POINTER_SECONDBUTTON_WPARAM(wParam))
        {
            layer_manager.clear();
            InvalidateRect(hwnd, nullptr, TRUE); // 背景を白でクリア
            return 0;
        }

        layer_manager.startNewStroke();
        // fallthrough - 左ボタンの場合は描画処理へ
    }
    case WM_POINTERUPDATE:
    {
        // ペンが実際に画面に接触しているときのみ
        if (IS_POINTER_INCONTACT_WPARAM(wParam))
        {
            // ポインターが有効な範囲内かつ接触している場合のみ、描画ポイントを追加
            AddDrawingPoint(hwnd, wParam, lParam, layer_manager);
            InvalidateRect(hwnd, nullptr, FALSE); // ウィンドウを再描画するように要求
        }
        return 0; // メッセージを処理したことを示す
    }

    // ウィンドウが破棄されるときのメッセージ
    case WM_DESTROY:
        PostQuitMessage(0); // メッセージループを終了させる
        return 0;           // ウィンドウを描画する必要があるときのメッセージ

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps); // 画面用描画コンテキストを取得

        layer_manager.draw(hdc);

        EndPaint(hwnd, &ps);
    }
        return 0;
    default:
        // 自分で処理しないメッセージは、デフォルトの処理に任せる（非常に重要）
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}