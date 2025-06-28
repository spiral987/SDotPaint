// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>
#include <debugapi.h> // OutputDebugStringW を使うため
#include <CommCtrl.h>
#include <string>
#include "LayerManager.h"
#include "PenData.h"

// GDI+のためのインクルード
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// UIコントロールのIDを定義
#define ID_ADD_LAYER_BUTTON 1001
#define ID_DELETE_LAYER_BUTTON 1002
#define ID_LAYER_LISTBOX 1003

// UIコントロールのハンドルを追加
HWND hSlider = nullptr;      // スライダーのハンドルを保持
HWND hStaticValue = nullptr; // 数値表示用
HWND hLayerList = nullptr;   // レイヤーリストボックス
HWND hAddButton = nullptr;   // 追加ボタン
HWND hDelButton = nullptr;   // 削除ボタン

ULONG_PTR gdiplusToken;

// レイヤー名編集用のグローバル変数
HWND hEdit = nullptr;          // 編集用エディットコントロールのハンドル
WNDPROC oldEditProc = nullptr; // 元のエディットコントロールのプロシージャ
int g_nEditingIndex = -1;      // 編集中のレイヤーのインデックス

// ダブルバッファリング用のグローバル変数
Bitmap *g_pBackBuffer = nullptr;
int g_nClientWidth = 0;
int g_nClientHeight = 0;

// マウスリーブイベントをトラックするためのフラグ
static bool g_bTrackingMouse = false;

// 描画ポイント追加関数のプロトタイプ宣言
void AddDrawingPoint(HWND hwnd, WPARAM wParam, LPARAM lParam, LayerManager &layer_manager);

// レイヤーリストを更新するヘルパー関数のプロトタイプ宣言
void UpdateLayerList(HWND hwnd, LayerManager &layerManager);

// エディットコントロールのサブクラスプロシージャ
LRESULT CALLBACK EditControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// リストボックスのサブクラスプロシージャ
LRESULT CALLBACK LayerListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

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
    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"ウィンドウクラスの登録に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // ウィンドウの生成
    // ----------------------------------------------------------------

    HWND hwnd = CreateWindowExW(
        0,                                     // 拡張ウィンドウスタイル
        CLASS_NAME,                            // 登録したウィンドウクラス名
        L"My Paint Application",               // ウィンドウのタイトル
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

// レイヤーリストボックスを更新する関数
void UpdateLayerList(HWND hwnd, LayerManager &layerManager)
{
    // リストボックスをクリア
    SendMessage(hLayerList, LB_RESETCONTENT, 0, 0);

    // LayerManagerからレイヤーのリストを取得してリストボックスに追加
    const auto &layers = layerManager.getLayers();
    for (const auto &layer : layers)
    {
        SendMessage(hLayerList, LB_ADDSTRING, 0, (LPARAM)layer->getName().c_str());
    }

    // 現在アクティブなレイヤーを選択状態にする
    int activeIndex = layerManager.getActiveLayerIndex();
    SendMessage(hLayerList, LB_SETCURSEL, activeIndex, 0);

    // ウィンドウ全体を再描画
    InvalidateRect(hwnd, NULL, TRUE);
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

        // インスタンスハンドルを取得
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

        // 追加ボタン
        hAddButton = CreateWindowExW(
            0, L"BUTTON", L"レイヤー追加",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 10, 100, 30, hwnd, (HMENU)ID_ADD_LAYER_BUTTON, hInstance, NULL);

        // 削除ボタン
        hDelButton = CreateWindowExW(
            0, L"BUTTON", L"レイヤー削除",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            120, 10, 100, 30, hwnd, (HMENU)ID_DELETE_LAYER_BUTTON, hInstance, NULL);

        // レイヤーリストボックス (右側に配置)
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        hLayerList = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            clientRect.right - 200, 10, 190, 200, // 右端から200px幅で配置
            hwnd, (HMENU)ID_LAYER_LISTBOX, hInstance, NULL);

        // リストボックスのサブクラス化
        SetWindowSubclass(hLayerList, LayerListProc, 0, (DWORD_PTR)&layer_manager);

        // スライダーコントロールの作成
        hSlider = CreateWindowExW(
            0,
            TRACKBAR_CLASSW, // スライダークラス
            L"Pen Size",
            WS_CHILD | WS_VISIBLE | TBS_VERT, // 子ウィンドウ・可視・縦方向
            10, 50,                           // X, Y座標
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
            L"5",      // 初期テキスト（ペンの初期値に合わせる）
            WS_CHILD | WS_VISIBLE,
            45, 50,   // X, Y座標（スライダーの右隣あたりに配置）
            50, 20,   // 幅, 高さ
            hwnd,     // 親ウィンドウ
            (HMENU)2, // コントロールID（他と被らないように）
            hInstance,
            nullptr);

        // ウィンドウのクライアント領域のサイズを取得
        RECT rect;
        GetClientRect(hwnd, &rect);

        g_nClientWidth = rect.right - rect.left;
        g_nClientHeight = rect.bottom - rect.top;

        // 最初のレイヤーを追加し、リストを更新
        layer_manager.createNewRasterLayer(g_nClientWidth, g_nClientHeight, L"レイヤー1");

        // リストボックスを初期更新
        UpdateLayerList(hwnd, layer_manager);

        return 0;
    }

    // Altキーを離したときの処理
    case WM_KEYUP:
    {
        if (wParam == VK_MENU)
        {
            // Altが離されたら、必ずホバー状態を解除
            if (layer_manager.getHoveredLayerIndex() != -1)
            {
                layer_manager.setHoveredLayer(-1);
                InvalidateRect(hwnd, NULL, FALSE); // 再描画を要求
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {

        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        HWND hChildUnderCursor = ChildWindowFromPoint(hwnd, pt);

        // マウスカーソルがリストボックスから出たことを検知するために、イベントを監視する
        if (!g_bTrackingMouse)
        {
            TRACKMOUSEEVENT tme = {sizeof(tme)};
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hLayerList; // リストボックスを監視対象にする
            if (TrackMouseEvent(&tme))
            {
                g_bTrackingMouse = true;
            }
        }

        // Altキーが押されているか？
        if (GetKeyState(VK_MENU) < 0)
        {

            // カーソルはリストボックスの上にあるか？
            if (hChildUnderCursor == hLayerList)
            {

                // リストボックスのクライアント座標に変換
                ScreenToClient(hLayerList, &pt);

                // カーソル位置のアイテムインデックスを取得
                DWORD itemIndexResult = SendMessage(hLayerList, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
                int newHoveredIndex = -1;

                // HIWORDが0なら、アイテムの「上」にカーソルがある
                if (HIWORD(itemIndexResult) == 0)
                {
                    newHoveredIndex = LOWORD(itemIndexResult);
                }
                // elseの場合はアイテムの外側（余白など）なので -1 のまま

                // ホバー状態が変化した場合のみ、更新と再描画を行う
                if (newHoveredIndex != layer_manager.getHoveredLayerIndex())
                {
                    wchar_t debugStr[256];

                    OutputDebugStringW(debugStr);

                    layer_manager.setHoveredLayer(newHoveredIndex);
                    InvalidateRect(hwnd, NULL, FALSE); // 再描画
                }
            }
            else // Altキーは押されているが、カーソルはリストボックスの外
            {
                if (layer_manager.getHoveredLayerIndex() != -1)
                {
                    OutputDebugStringW(L"[DEBUG] Mouse left listbox with Alt key.\n");
                    layer_manager.setHoveredLayer(-1);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
        }
        else // Altキーが押されていない
        {
            // ホバー状態であれば解除する
            if (layer_manager.getHoveredLayerIndex() != -1)
            {
                OutputDebugStringW(L"[DEBUG] Alt key released, resetting hover.\n");
                layer_manager.setHoveredLayer(-1);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0; // WM_MOUSEMOVEはここで処理を終える
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        switch (wmId)
        {
        case ID_ADD_LAYER_BUTTON:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            layer_manager.addNewRasterLayer(rect.right - rect.left, rect.bottom - rect.top);
            UpdateLayerList(hwnd, layer_manager); // リストを更新
            SetFocus(hwnd);
            break;
        }
        case ID_DELETE_LAYER_BUTTON:
        {
            layer_manager.deleteActiveLayer();
            UpdateLayerList(hwnd, layer_manager); // リストを更新
            SetFocus(hwnd);
            break;
        }

        case ID_LAYER_LISTBOX:
        {

            // ダブルクリックされた場合
            if (wmEvent == LBN_DBLCLK)
            {
                // 選択されている項目のインデックスを取得
                int selectedIndex = SendMessage(hLayerList, LB_GETCURSEL, 0, 0);
                if (selectedIndex != LB_ERR)
                {
                    g_nEditingIndex = selectedIndex; // 編集中のインデックスを保存

                    // 選択項目の矩形（位置とサイズ）を取得
                    RECT itemRect;
                    SendMessage(hLayerList, LB_GETITEMRECT, selectedIndex, (LPARAM)&itemRect);

                    // 編集用エディットコントロールを作成
                    hEdit = CreateWindowExW(
                        0, L"EDIT", L"",
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        itemRect.left, itemRect.top, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top,
                        hLayerList, // 親をリストボックスにする
                        (HMENU)999, // 独自のID
                        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                        NULL);

                    // 現在のレイヤー名を設定
                    const auto &layers = layer_manager.getLayers();
                    SetWindowTextW(hEdit, layers[selectedIndex]->getName().c_str());

                    // エディットコントロールのフォントをリストボックスに合わせる
                    SendMessage(hEdit, WM_SETFONT, SendMessage(hLayerList, WM_GETFONT, 0, 0), TRUE);

                    // エディットコントロールのプロシージャをサブクラス化
                    SetWindowSubclass(hEdit, EditControlProc, 0, (DWORD_PTR)&layer_manager);

                    // エディットコントロールにフォーカスを合わせ、テキストを全選択
                    SetFocus(hEdit);
                    SendMessage(hEdit, EM_SETSEL, 0, -1);
                }
            }
            // 選択が変更された場合
            else if (wmEvent == LBN_SELCHANGE)
            {
                int selectedIndex = SendMessage(hLayerList, LB_GETCURSEL, 0, 0);
                if (selectedIndex != LB_ERR)
                {
                    layer_manager.setActiveLayer(selectedIndex);
                    InvalidateRect(hwnd, NULL, FALSE); // 再描画
                }
            }
            break;
        }
        }
        return 0;
    }

    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case 'E': // 消しゴム
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
        case 'Q': // ペン
        {
            layer_manager.setDrawMode(DrawMode::Pen);
            {
                int width = layer_manager.getCurrentToolWidth();
                SendMessage(hSlider, TBM_SETPOS, TRUE, width);
                SetWindowTextW(hStaticValue, std::to_wstring(width).c_str());
            }
            break;
        }
        case 'C': // 色選択(Color)
        {
            // 1. ダイアログ設定用の構造体を準備
            CHOOSECOLOR cc;
            static COLORREF customColors[16]; // カスタムカラーを保存する配列

            ZeroMemory(&cc, sizeof(cc)); // 構造体をゼロで初期化
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;                        // 親ウィンドウのハンドル
            cc.lpCustColors = (LPDWORD)customColors;    // カスタムカラー配列へのポインタ
            cc.rgbResult = layer_manager.getPenColor(); // 初期色を現在のペンの色に設定
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;        // ダイアログのスタイル

            // 2. 「色の設定」ダイアログを表示
            if (ChooseColor(&cc) == TRUE)
            {
                // 3. ユーザーがOKを押したら、選択された色で更新
                layer_manager.setPenColor(cc.rgbResult);
            }
            // フォーカスを戻しておく
            SetFocus(hwnd);
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
        SetFocus(hwnd);
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
        OutputDebugStringW(L"[1] WM_MOUSEMOVE received.\n");

        POINTER_PEN_INFO penInfo;
        UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
        if (!GetPointerPenInfo(pointerId, &penInfo))
            break;
        if (penInfo.pointerInfo.pointerType != PT_PEN)
            break;

        // --- ペンが画面に触れている時（描画処理）---
        if (IS_POINTER_INCONTACT_WPARAM(wParam))
        {
            POINT p = penInfo.pointerInfo.ptPixelLocation;
            ScreenToClient(hwnd, &p);
            UINT32 pressure = penInfo.pressure;

            layer_manager.addPoint({p.x, p.y, pressure});
            InvalidateRect(hwnd, NULL, FALSE);
        }
        // --- ペンがホバー状態の時（Alt+ホバー処理）---
        else
        {
            // Altキーが押されているか？
            if (GetKeyState(VK_MENU) < 0)
            {
                OutputDebugStringW(L"  [2] Alt key is pressed.\n");

                POINT pt = penInfo.pointerInfo.ptPixelLocation; // スクリーン座標
                ScreenToClient(hwnd, &pt);                      // メインウィンドウのクライアント座標に変換
                HWND hChildUnderCursor = ChildWindowFromPoint(hwnd, pt);

                // カーソルはリストボックスの上にあるか？
                if (hChildUnderCursor == hLayerList)
                {

                    // 3. カーソルがリストボックス上にあることを確認
                    OutputDebugStringW(L"    [3] Cursor is over the ListBox.\n");

                    ScreenToClient(hLayerList, &penInfo.pointerInfo.ptPixelLocation); // リストボックスのクライアント座標に変換
                    POINT listbox_pt = penInfo.pointerInfo.ptPixelLocation;

                    DWORD itemIndexResult = SendMessage(hLayerList, LB_ITEMFROMPOINT, 0, MAKELPARAM(listbox_pt.x, listbox_pt.y));
                    int newHoveredIndex = (HIWORD(itemIndexResult) == 0) ? LOWORD(itemIndexResult) : -1;

                    if (newHoveredIndex != layer_manager.getHoveredLayerIndex())
                    {
                        wchar_t debugStr[256];
                        layer_manager.setHoveredLayer(newHoveredIndex);
                        swprintf_s(debugStr, L"      [4] Hover Index Changed to: %d\n", newHoveredIndex);
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
                else // カーソルはリストボックスの外
                {
                    if (layer_manager.getHoveredLayerIndex() != -1)
                    {
                        layer_manager.setHoveredLayer(-1);
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
            }
            else // Altキーが押されていない
            {
                if (layer_manager.getHoveredLayerIndex() != -1)
                {
                    layer_manager.setHoveredLayer(-1);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
        }
        return 0;
    }

    case WM_POINTERUP:
    {
        // ペンが離れたら、現在のストロークを終了する
        layer_manager.startNewStroke();
        return 0;
    }

    case WM_SIZE:
    {
        // ウィンドウサイズを更新
        g_nClientWidth = LOWORD(lParam);
        g_nClientHeight = HIWORD(lParam);

        // 古いバックバッファを削除
        delete g_pBackBuffer;

        // 新しいサイズのバックバッファを作成（GDI+オブジェクトなのでPixelFormatを指定する）
        g_pBackBuffer = new Bitmap(g_nClientWidth, g_nClientHeight, PixelFormat32bppARGB);

        // ここで再描画をかけておくと、リサイズ時に描画が追従する
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    // ウィンドウが破棄されるときのメッセージ
    case WM_DESTROY:
    {
        // バックバッファを解放
        delete g_pBackBuffer;
        GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0); // メッセージループを終了させる
        return 0;           // ウィンドウを描画する必要があるときのメッセージ
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // バックバッファがまだ作成されていない場合は何もしない
        if (g_pBackBuffer)
        {
            // バックバッファからグラフィックスオブジェクトを作成
            Graphics backBufferGraphics(g_pBackBuffer);

            // 背景を白でクリア
            backBufferGraphics.Clear(Color(255, 255, 255, 255));

            // LayerManagerに描画を依頼（ホバー状態に応じた描画が行われる）
            layer_manager.draw(&backBufferGraphics);

            // 画面にバックバッファの内容を一度に転送
            Graphics screenGraphics(hdc);
            screenGraphics.DrawImage(g_pBackBuffer, 0, 0);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        // 自分で処理しないメッセージは、デフォルトの処理に任せる（非常に重要）
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
// レイヤーリストボックスのサブクラスプロシージャ
LRESULT CALLBACK LayerListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LayerManager *layer_manager = (LayerManager *)dwRefData;

    switch (uMsg)
    {

        // ★★★ ペンホバーの処理をここに追加 ★★★
    case WM_POINTERUPDATE:
    {
        // ペンが触れていない（ホバー）状態か？
        if (!IS_POINTER_INCONTACT_WPARAM(wParam))
        {
            // Altキーが押されているか？
            if (GetKeyState(VK_MENU) < 0)
            {
                POINTER_INFO pointerInfo;
                UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
                if (GetPointerInfo(pointerId, &pointerInfo))
                {
                    POINT pt = pointerInfo.ptPixelLocation; // スクリーン座標を取得
                    ScreenToClient(hwnd, &pt);              // リストボックスのクライアント座標に変換

                    DWORD itemIndexResult = SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
                    int newHoveredIndex = (HIWORD(itemIndexResult) == 0) ? LOWORD(itemIndexResult) : -1;

                    if (newHoveredIndex != layer_manager->getHoveredLayerIndex())
                    {
                        layer_manager->setHoveredLayer(newHoveredIndex);
                        InvalidateRect(GetParent(hwnd), NULL, FALSE); // 親ウィンドウを再描画
                    }
                }
            }
            else // Altキーが押されていない
            {
                if (layer_manager->getHoveredLayerIndex() != -1)
                {
                    layer_manager->setHoveredLayer(-1);
                    InvalidateRect(GetParent(hwnd), NULL, FALSE);
                }
            }
        }
        return 0; // メッセージを処理した
    }

    // ★★★ ペンが領域から離れたときの処理を追加 ★★★
    case WM_POINTERLEAVE:
    {
        if (layer_manager->getHoveredLayerIndex() != -1)
        {
            layer_manager->setHoveredLayer(-1);
            InvalidateRect(GetParent(hwnd), NULL, FALSE);
        }
        g_bTrackingMouse = false; // マウス用のフラグもリセットしておく
        return 0;
    }

    // マウス用のリーブ処理も残しておく
    case WM_MOUSELEAVE:
    {
        if (layer_manager->getHoveredLayerIndex() != -1)
        {
            layer_manager->setHoveredLayer(-1);
            InvalidateRect(GetParent(hwnd), NULL, FALSE);
        }
        g_bTrackingMouse = false;
        return 0;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, LayerListProc, 0);
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// レイヤー名編集用エディットコントロールのサブクラスプロシージャ
LRESULT CALLBACK EditControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LayerManager *layer_manager = (LayerManager *)dwRefData;

    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) // Enterキーが押された
        {
            // フォーカスを失わせることで、WM_KILLFOCUSを発生させる
            SetFocus(GetParent(hwnd));
            return 0;
        }
        else if (wParam == VK_ESCAPE) // Escapeキーが押された
        {
            // 何もせずエディットボックスを破棄
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_KILLFOCUS: // エディットコントロールがフォーカスを失った
    {
        wchar_t buffer[256];
        GetWindowTextW(hwnd, buffer, 256);

        // レイヤー名を更新
        layer_manager->renameLayer(g_nEditingIndex, buffer);

        // リストボックスを更新するためにメインウィンドウに通知
        UpdateLayerList(GetParent(GetParent(hwnd)), *layer_manager);

        // エディットボックスを破棄
        DestroyWindow(hwnd);
    }
        return 0;

    case WM_NCDESTROY: // エディットコントロールが破棄されるとき
        // サブクラス化を解除
        RemoveWindowSubclass(hwnd, EditControlProc, 0);
        break;
    }

    // デフォルトのプロシージャを呼び出す
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}