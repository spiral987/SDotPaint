// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include "LayerManager.h"
#include "PenData.h"

HWND hSlider = nullptr;      // スライダーのハンドルを保持
HWND hStaticValue = nullptr; // 数値表示用

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理
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
    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"ウィンドウクラスの登録に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // ウィンドウの生成
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

    // スライダーコントロールの作成
    hSlider = CreateWindowExW(
        0,
        TRACKBAR_CLASSW, // スライダークラス
        L"Pen Size",
        WS_CHILD | WS_VISIBLE | TBS_VERT, // 子ウィンドウ・可視・縦方向
        10, 10,                           // X, Y座標
        30, 500,                          // 幅, 高さ
        hwnd,                             // 親ウィンドウ
        (HMENU)1,                         // コントロールID
        hInstance,
        nullptr);

    SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(1, 100)); // スライダーの範囲を設定 (最小値1, 最大値100）
    SendMessage(hSlider, TBM_SETPOS, TRUE, 5);                    // 初期位置を5に設定

    hStaticValue = CreateWindowExW(
        0,
        L"STATIC", // コントロールのクラス名
        L"5",      // ★初期テキスト（ペンの初期値に合わせる）
        WS_CHILD | WS_VISIBLE,
        45, 10,   // ★X, Y座標（スライダーの右隣あたりに配置）
        50, 20,   // 幅, 高さ
        hwnd,     // 親ウィンドウ
        (HMENU)2, // コントロールID（他と被らないように）
        hInstance,
        nullptr);

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
    {
    case WM_CREATE:
    {
        // ウィンドウのクライアント領域のサイズを取得
        RECT rect;
        GetClientRect(hwnd, &rect);

        // レイヤーを初期化(今回はラスターレイヤー)
        HDC hdc = GetDC(hwnd);
        layer_manager.createNewRasterLayer(rect.right - rect.left, rect.bottom - rect.top, hdc);
        ReleaseDC(hwnd, hdc);
        return 0; // 処理したので0を返す
    }

    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case 'E':
        {
            layer_manager.setDrawMode(DrawMode::Eraser);
            { // 変数スコープを明確にするための括弧
                int width = layer_manager.getCurrentToolWidth();
                SendMessage(hSlider, TBM_SETPOS, TRUE, width); // ペンのふとさをスライダーに適用
                // テキストも更新
                SetWindowTextW(hStaticValue, std::to_wstring(width).c_str());
            }
            break;
        }
        case 'Q':
        {
            layer_manager.setDrawMode(DrawMode::Pen);
            {
                int width = layer_manager.getCurrentToolWidth();
                SendMessage(hSlider, TBM_SETPOS, TRUE, width);
                SetWindowTextW(hStaticValue, std::to_wstring(width).c_str());
            }
            break;
        }
        }
        return 0;
    }

    case WM_VSCROLL:
    {
        // スライダーからのメッセージか確認
        if ((HWND)lParam == hSlider)
        {
            // スライダーの現在の位置を取得
            int newWidth = SendMessage(hSlider, TBM_GETPOS, 0, 0);

            // 現在のモードに応じて、対応するツールの太さを更新
            if (layer_manager.getCurrentMode() == DrawMode::Pen)
            {
                layer_manager.setPenWidth(newWidth);
            }
            else
            {
                layer_manager.setEraserWidth(newWidth);
            }

            // 静的テキストの表示を更新
            SetWindowTextW(hStaticValue, std::to_wstring(newWidth).c_str());

            // フォーカスをメインウィンドウに戻す(これが無いとスライダーから抜け出せなくなる)
            SetFocus(hwnd);
        }
        return 0;
    }

        // ペンでタッチダウンしたときのメッセージ
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