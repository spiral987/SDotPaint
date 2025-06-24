// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>
#include <string>
#include <vector>

#include "PaintModel.h"

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理します
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 描画ポイント追加関数のプロトタイプ宣言
void AddDrawingPoint(HWND hwnd, WPARAM wParam, LPARAM lParam, PaintModel &paint_model);

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
void AddDrawingPoint(HWND hwnd, WPARAM wParam, LPARAM lParam, PaintModel &paint_model)
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
    ScreenToClient(hwnd, &screenPoint);              //* 画面座標をクライアント座標に変換    // ペンの場合のみ筆圧情報を取得
    UINT32 pressure = 512;                           // デフォルト筆圧（中間値）
    if (pointerInfo.pointerType == PT_PEN)
    {
        // ペンの詳細情報を取得
        POINTER_PEN_INFO penInfo;
        if (GetPointerPenInfo(pointerId, &penInfo))
        {
            pressure = penInfo.pressure;

            // デバッグ用：筆圧情報を出力
            wchar_t debugMsg[256];
            swprintf_s(debugMsg, L"Pen pressure: %d\n", pressure);
            OutputDebugStringW(debugMsg);
        }
    }

    // 筆圧値の範囲チェック（0-1024が正常範囲）
    if (pressure > 1024)
    {
        pressure = 1024; // 異常値の場合は最大値に設定
    }

    // PenPointを作成してモデルに追加
    PenPoint p;
    //* 変換後のクライアント座標を使用
    p.point = screenPoint; // クライアント座標を設定
    p.pressure = pressure; // 筆圧値を設定
    paint_model.addPoint(p);
}

// ウィンドウプロシージャ（メッセージが発せられたときに呼び出される関数）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // マウスの位置を保持するための変数。static変数を使用して、ウィンドウプロシージャが呼び出されるたびに初期化されないようにする
    static PaintModel paint_model; // PaintModelのstaticなインスタンス    // メッセージの種類に応じて処理を分岐
    switch (uMsg)
    { // ペンでタッチダウンしたときのメッセージ
    case WM_POINTERDOWN:
    {
        // 右ボタンがクリックされた場合はクリア処理
        if (IS_POINTER_SECONDBUTTON_WPARAM(wParam))
        {
            paint_model.clearPoints();
            InvalidateRect(hwnd, nullptr, TRUE); // 背景を白でクリア
            return 0;
        }
        // fallthrough - 左ボタンの場合は描画処理へ
    }
    case WM_POINTERUPDATE:
    {
        // ペンが実際に画面に接触しているときのみ
        if (IS_POINTER_INCONTACT_WPARAM(wParam))
        {
            // ポインターが有効な範囲内かつ接触している場合のみ、描画ポイントを追加
            AddDrawingPoint(hwnd, wParam, lParam, paint_model);
            InvalidateRect(hwnd, nullptr, FALSE); // ウィンドウを再描画するように要求
        }
        return 0; // メッセージを処理したことを示す
    }

    case WM_RBUTTONDOWN:
    {
        // 右クリックで描画をクリア
        paint_model.clearPoints();
        InvalidateRect(hwnd, nullptr, TRUE); // 背景を白でクリア
        return 0;                            // メッセージを処理したことを示す
    }

    // ウィンドウが破棄されるときのメッセージ
    case WM_DESTROY:
        PostQuitMessage(0); // メッセージループを終了させる
        return 0;           // ウィンドウを描画する必要があるときのメッセージ
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps); // 画面用描画コンテキストを取得

        // 黒いブラシを作成（円の塗りつぶし用）
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, blackBrush); // SelectObjectは新しいブラシを選択し、古いブラシを返す

        // ペンも黒に設定（円の輪郭用）
        HPEN blackPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(hdc, blackPen);        //! 保存されているすべてのマウスの点を描画する==============================================
        // PaintModelから点のリストを取得
        const auto& points = paint_model.getPoints();
        
        if (!points.empty())
        {
            // 最初の点は円として描画
            const auto& firstPoint = points[0];
            int x = firstPoint.point.x;
            int y = firstPoint.point.y;
            int radius = (firstPoint.pressure / 128) + 1;
            Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
            
            // 2点目以降は前の点と線で繋ぐ
            for (size_t i = 1; i < points.size(); ++i)
            {
                const auto& prevPoint = points[i - 1];
                const auto& currPoint = points[i];
                
                // 線の太さを筆圧に応じて調整
                int penWidth = (currPoint.pressure / 128) + 1;
                HPEN thickPen = CreatePen(PS_SOLID, penWidth, RGB(0, 0, 0));
                HPEN oldThickPen = (HPEN)SelectObject(hdc, thickPen);
                
                // 前の点から現在の点まで線を描画
                MoveToEx(hdc, prevPoint.point.x, prevPoint.point.y, nullptr);
                LineTo(hdc, currPoint.point.x, currPoint.point.y);
                
                // 現在の点に円も描画（線の端を丸くするため）
                int radius = (currPoint.pressure / 128) + 1;
                Ellipse(hdc, currPoint.point.x - radius, currPoint.point.y - radius, 
                        currPoint.point.x + radius, currPoint.point.y + radius);
                
                // ペンを復元・削除
                SelectObject(hdc, oldThickPen);
                DeleteObject(thickPen);
            }
        }
        //! ================================================================================

        // 作成したブラシを消す前に元のブラシとペンを復元
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);

        // 作成したブラシとペンを削除（メモリリーク防止）
        DeleteObject(blackBrush);
        DeleteObject(blackPen);
        EndPaint(hwnd, &ps); // 描画を終了
    }
        return 0;
    default:
        // 自分で処理しないメッセージは、デフォルトの処理に任せる（非常に重要）
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}