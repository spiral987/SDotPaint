// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>
#include <debugapi.h> // OutputDebugStringW を使う
#include <CommCtrl.h>
#include <string>
#include <memory>
#include <cmath> // 数学関数(atan2f)のために必要

#include "core/LayerManager.h"
#include "core/PenData.h"
#include "ui/UIManager.h"

// GDI+のため
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// UIManagerのインスタンスを保持するユニークポインタ
std::unique_ptr<UIManager> g_uiManager;

ULONG_PTR gdiplusToken;

// ダブルバッファリング用のグローバル変数
Bitmap *g_pBackBuffer = nullptr;
int g_nClientWidth = 0;
int g_nClientHeight = 0;

// 視点移動用のグローバル変数
static bool g_isPanMode = false;      // 視点移動モードかどうかのフラグ
static POINT g_panLastPoint = {0, 0}; // 視点移動時の最後のマウス位置
static PointF g_viewCenter = {0, 0};  // ワールド座標系でのビュー中心
// static POINT g_viewOffset = {0, 0};   // 視点のオフセット量

static bool g_isRotateMode = false;  // 回転モードかどうかのフラグ
static float g_rotationAngle = 0.0f; // 現在の総回転角度
static float g_startAngle = 0.0f;    // 回転開始時の角度

static bool g_isZoomMode = false;               // ズームモードかどうかのフラグ
static float g_zoomFactor = 1.0f;               // 現在のズーム率
static float g_baseZoomFactor = 1.0f;           // ズーム開始時のズーム率
static POINT g_zoomStartPoint = {0, 0};         // ズーム開始時のスクリーン座標
static PointF g_zoomCenterWorld = {0.0f, 0.0f}; // ズーム基点のワールド座標

static bool g_isPenContact = false; // ペンの接触状態を自前で管理するフラグ

// マウスリーブイベントをトラックするためのフラグ
static bool g_bTrackingMouse = false;

// グローバル変数に「前回のスクリーン座標」を保持する変数を追加
static POINT g_lastScreenPoint = {-1, -1};
static UINT32 g_lastPressure = 0;

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 描画ポイント追加関数のプロトタイプ宣言
void AddDrawingPoint(HWND hwnd, WPARAM wParam, LPARAM lParam, LayerManager &layer_manager);

// エディットコントロールのサブクラスプロシージャ
LRESULT CALLBACK EditControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// リストボックスのサブクラスプロシージャ
LRESULT CALLBACK LayerListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// 背景色に応じてテキスト色を決定する関数
COLORREF GetContrastingTextColor(COLORREF bgColor);

// モード管理を統合する関数
void UpdateToolMode(HWND hwnd);

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

COLORREF GetContrastingTextColor(COLORREF bgColor)
{
    // 背景色の明るさを計算 (簡易的な方法)
    int brightness = (GetRValue(bgColor) * 299 + GetGValue(bgColor) * 587 + GetBValue(bgColor) * 114) / 1000;
    return (brightness > 128) ? RGB(0, 0, 0) : RGB(255, 255, 255);
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

        g_nClientWidth = rect.right - rect.left;
        g_nClientHeight = rect.bottom - rect.top;

        // ビューの中心をウィンドウの中心に初期化
        g_viewCenter.X = g_nClientWidth / 2.0f;
        g_viewCenter.Y = g_nClientHeight / 2.0f;

        // ダブルバッファリング用のビットマップを作成
        g_pBackBuffer = new Bitmap(g_nClientWidth, g_nClientHeight, PixelFormat32bppARGB);

        // 最初のレイヤーを追加し、リストを更新
        layer_manager.createNewRasterLayer(g_nClientWidth, g_nClientHeight, L"レイヤー1");

        // UIManagerを作成してUI処理をする
        g_uiManager = std::make_unique<UIManager>(hwnd, layer_manager);
        g_uiManager->CreateControls();
        g_uiManager->SetupLayerListSubclass();
        g_uiManager->UpdateLayerList();

        return 0;
    }

    // メニューをアプリケーション側で描画してくださいというメッセージが来たら(オーナードロー)
    case WM_DRAWITEM:
    {
        // このメッセージがレイヤーリストボックスからのものか確認
        if ((UINT)wParam == ID_LAYER_LISTBOX)
        {
            DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT *)lParam;

            // 描画対象の項目がない場合（リストが空など）は何もしない
            if (pdis->itemID == -1)
            {
                return TRUE;
            }

            // 1. 描画対象のレイヤーを取得
            const auto &layers = layer_manager.getLayers();
            const auto &layer = layers[pdis->itemID];

            // 2. レイヤーの平均色を取得
            COLORREF bgColor = layer->getAverageColor();

            // 3. 背景色から、見やすいテキスト色を決定
            COLORREF textColor = GetContrastingTextColor(bgColor);

            // 4. 背景を描画
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(pdis->hDC, &pdis->rcItem, hBrush);
            DeleteObject(hBrush);

            // 5. テキスト（レイヤー名）を描画
            SetTextColor(pdis->hDC, textColor);
            SetBkMode(pdis->hDC, TRANSPARENT); // テキストの背景を透明にする
            DrawTextW(pdis->hDC, layer->getName().c_str(), -1, &pdis->rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

            // 6. 項目が選択されている場合は、フォーカス用の点線の四角形を描画
            if (pdis->itemState & ODS_SELECTED)
            {
                DrawFocusRect(pdis->hDC, &pdis->rcItem);
            }

            return TRUE; // メッセージを処理したことを示す
        }
        break;
    }

    // Altキーを離したときの処理
    case WM_KEYUP:
    {
        // キーが離されたら、現在のキー状態に基づいてモードを更新する
        UpdateToolMode(hwnd);
        return 0;
    }

    case WM_MOUSEMOVE:
    {

        // UIManager経由でレイヤーリストのハンドルを取得
        HWND hLayerList = nullptr;
        if (g_uiManager)
        {
            hLayerList = g_uiManager->GetLayerListHandle();
        }

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

        return 0; // WM_MOUSEMOVEはここで処理を終える
    }

    // ボタンやリスト
    case WM_COMMAND:
    {

        if (g_uiManager)
        {
            g_uiManager->HandleCommand(wParam);
        }
        return 0;
    }

    case WM_KEYDOWN:
    {
        // キーが押されたら、まずモードを更新する
        if (wParam == VK_CONTROL || wParam == VK_SPACE || wParam == 'R')
        {
            UpdateToolMode(hwnd);
            return 0; // モード変更キーの場合は、他の処理をしない
        }

        switch (wParam)
        {
        case 'E': // 消しゴム
        {
            SetFocus(hwnd);
            layer_manager.setDrawMode(DrawMode::Eraser);
            { // 変数スコープを明確にするための括弧
                int width = layer_manager.getCurrentToolWidth();
                g_uiManager->SetSliderValue(width);
                g_uiManager->UpdateStaticValue(width);
            }
            break;
        }
        case 'Q': // ペン
        {
            SetFocus(hwnd);
            layer_manager.setDrawMode(DrawMode::Pen);
            {
                int width = layer_manager.getCurrentToolWidth();
                g_uiManager->SetSliderValue(width);
                g_uiManager->UpdateStaticValue(width);
            }
            break;
        }
        case 'C': // 色選択(Color)
        {
            SetFocus(hwnd);
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
        if (g_uiManager && (HWND)lParam == g_uiManager->GetSliderHandle())
        {
            int newWidth = g_uiManager->GetSliderValue();

            // 現在のモードに応じて、対応するツールの太さを更新
            if (layer_manager.getCurrentMode() == DrawMode::Pen)
            {
                layer_manager.setPenWidth(newWidth);
            }
            else
            {
                layer_manager.setEraserWidth(newWidth);
            }

            g_uiManager->UpdateStaticValue(newWidth);

            // フォーカスをメインウィンドウに戻す(これが無いとスライダーから抜け出せなくなる)
            SetFocus(hwnd);
        }
        return 0;
    }

    // ペンでタッチダウンしたときのメッセージ
    case WM_POINTERDOWN:
    {
        g_isPenContact = true;
        SetFocus(hwnd);

        // 視点移動モード中の処理
        if (g_isPanMode)
        {
            // ペンの現在位置を取得して、移動開始点として保存
            POINTER_INFO pointerInfo;
            UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
            if (GetPointerInfo(pointerId, &pointerInfo))
            {
                g_panLastPoint = pointerInfo.ptPixelLocation;
                ScreenToClient(hwnd, &g_panLastPoint);
                SetCapture(hwnd); // ウィンドウ外にマウスが出てもメッセージを補足する
            }
            return 0; // 描画処理は行わない
        }

        // 回転モード中の処理
        else if (g_isRotateMode)
        {
            POINTER_INFO pointerInfo;
            UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
            if (GetPointerInfo(pointerId, &pointerInfo))
            {
                POINT currentPoint = pointerInfo.ptPixelLocation;
                ScreenToClient(hwnd, &currentPoint);

                // ウィンドウ中心と現在地の角度を計算して保存
                float dx = (float)currentPoint.x - (g_nClientWidth / 2.0f);
                float dy = (float)currentPoint.y - (g_nClientHeight / 2.0f);
                g_startAngle = atan2f(dy, dx);
            }
            return 0;
        }

        // ズームモード中の処理
        else if (g_isZoomMode)
        {
            POINTER_INFO pointerInfo;
            UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
            if (GetPointerInfo(pointerId, &pointerInfo))
            {
                POINT p = pointerInfo.ptPixelLocation;
                ScreenToClient(hwnd, &p);

                // ズーム操作の開始情報を記録
                g_zoomStartPoint = p;
                g_baseZoomFactor = g_zoomFactor;

                // ズームの基点となるワールド座標を計算して保存
                Matrix transformMatrix;
                float centerX = g_nClientWidth / 2.0f;
                float centerY = g_nClientHeight / 2.0f;
                transformMatrix.Translate(centerX, centerY);
                transformMatrix.Rotate(g_rotationAngle);
                transformMatrix.Scale(g_zoomFactor, g_zoomFactor);
                transformMatrix.Translate(-g_viewCenter.X, -g_viewCenter.Y);
                transformMatrix.Invert();
                PointF pointF = {(float)p.x, (float)p.y};
                transformMatrix.TransformPoints(&pointF, 1);
                g_zoomCenterWorld = pointF;
            }
            return 0;
        }
        else
        {
            // 右ボタンがクリックされた場合はクリア処理
            if (IS_POINTER_SECONDBUTTON_WPARAM(wParam))
            {
                layer_manager.clear();
                InvalidateRect(hwnd, nullptr, TRUE); // 背景を白でクリア
                return 0;
            }

            layer_manager.startNewStroke();

            POINTER_PEN_INFO penInfo;
            UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
            if (GetPointerPenInfo(pointerId, &penInfo) && penInfo.pointerInfo.pointerType == PT_PEN)
            {
                POINT p = penInfo.pointerInfo.ptPixelLocation;
                ScreenToClient(hwnd, &p);
                UINT32 pressure = penInfo.pressure;

                // ワールド座標に変換して、レイヤーのデータに記録（これは永続化のため）
                Matrix transformMatrix;
                float centerX = g_nClientWidth / 2.0f;
                float centerY = g_nClientHeight / 2.0f;
                transformMatrix.Translate(centerX, centerY);
                transformMatrix.Rotate(g_rotationAngle);
                transformMatrix.Scale(g_zoomFactor, g_zoomFactor);
                transformMatrix.Translate(-g_viewCenter.X, -g_viewCenter.Y);
                transformMatrix.Invert();
                PointF pointF = {(float)p.x, (float)p.y};
                transformMatrix.TransformPoints(&pointF, 1);
                layer_manager.addPoint({(LONG)pointF.X, (LONG)pointF.Y, pressure});

                // 最初の点のスクリーン座標と筆圧を保存
                g_lastScreenPoint = p;
                g_lastPressure = pressure;
            }
            return 0;
        }
    }
    case WM_POINTERUPDATE:
    {
        // 視点移動処理
        if (g_isPenContact)
        {
            // 視点移動モード中の処理
            if (g_isPanMode)
            {
                OutputDebugStringW(L"    -> Pan processing branch ENTERED.\n");

                POINTER_INFO pointerInfo;
                UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
                if (GetPointerInfo(pointerId, &pointerInfo))
                {
                    POINT currentPoint = pointerInfo.ptPixelLocation;
                    ScreenToClient(hwnd, &currentPoint);

                    // スクリーン座標での移動量
                    float dx = (float)currentPoint.x - (float)g_panLastPoint.x;
                    float dy = (float)currentPoint.y - (float)g_panLastPoint.y;

                    // 移動量を現在の回転角度の逆方向に回転させてから、ビュー中心に適用
                    float angleRad = -g_rotationAngle * (3.14159265f / 180.0f);
                    float rotatedDx = dx * cosf(angleRad) - dy * sinf(angleRad);
                    float rotatedDy = dx * sinf(angleRad) + dy * cosf(angleRad);

                    if (g_zoomFactor > 0.0f)
                    {
                        rotatedDx /= g_zoomFactor;
                        rotatedDy /= g_zoomFactor;
                    }

                    // 移動量を計算してオフセットに加算
                    g_viewCenter.X -= rotatedDx;
                    g_viewCenter.Y -= rotatedDy;

                    // 現在位置を次の計算のために保存
                    g_panLastPoint = currentPoint;

                    // 画面を再描画
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0; // 描画処理は行わない
            }

            // 回転モード中の処理
            else if (g_isRotateMode)
            {
                POINTER_INFO pointerInfo;
                UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
                if (GetPointerInfo(pointerId, &pointerInfo))
                {
                    POINT currentPoint = pointerInfo.ptPixelLocation;
                    ScreenToClient(hwnd, &currentPoint);

                    // ウィンドウ中心と現在地の角度を計算
                    float dx = (float)currentPoint.x - (g_nClientWidth / 2.0f);
                    float dy = (float)currentPoint.y - (g_nClientHeight / 2.0f);
                    float currentAngle = atan2f(dy, dx);

                    // 開始角度からの差分を計算し、総回転角度に加える
                    float deltaAngle = currentAngle - g_startAngle;
                    g_rotationAngle += deltaAngle * (180.0f / 3.14159265f); // ラジアンから度に変換

                    // 次のUPDATEのために、開始角度を現在の角度に更新
                    g_startAngle = currentAngle;

                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0;
            }

            // ズームモード中の処理
            else if (g_isZoomMode)
            {
                POINTER_INFO pointerInfo;
                UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
                if (GetPointerInfo(pointerId, &pointerInfo))
                {
                    POINT currentPoint = pointerInfo.ptPixelLocation;
                    ScreenToClient(hwnd, &currentPoint);

                    float totalDeltaX = (float)currentPoint.x - (float)g_zoomStartPoint.x;
                    float oldZoom = g_zoomFactor;

                    // 指数関数的にズーム率を変化させると、より自然な操作感になる
                    g_zoomFactor = g_baseZoomFactor * expf(totalDeltaX * 0.005f);

                    // ズーム率に下限と上限を設ける
                    if (g_zoomFactor < 0.1f)
                        g_zoomFactor = 0.1f;
                    if (g_zoomFactor > 10.0f)
                        g_zoomFactor = 10.0f;

                    // ズーム基点がズレないようにビューの中心を補正
                    if (g_zoomFactor > 0.0f)
                    {
                        g_viewCenter.X = g_zoomCenterWorld.X + (g_viewCenter.X - g_zoomCenterWorld.X) * (oldZoom / g_zoomFactor);
                        g_viewCenter.Y = g_zoomCenterWorld.Y + (g_viewCenter.Y - g_zoomCenterWorld.Y) * (oldZoom / g_zoomFactor);
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0;
            }
            else
            {
                // 描画処理

                if (IS_POINTER_INCONTACT_WPARAM(wParam))
                {
                    POINTER_PEN_INFO penInfo;
                    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
                    if (GetPointerPenInfo(pointerId, &penInfo) && penInfo.pointerInfo.pointerType == PT_PEN)
                    {
                        POINT p = penInfo.pointerInfo.ptPixelLocation;
                        ScreenToClient(hwnd, &p);
                        UINT32 pressure = penInfo.pressure;

                        // 1. ワールド座標に変換し、レイヤーのデータに記録（永続化のため）
                        Matrix transformMatrix;
                        float centerX = g_nClientWidth / 2.0f;
                        float centerY = g_nClientHeight / 2.0f;
                        transformMatrix.Translate(centerX, centerY);
                        transformMatrix.Rotate(g_rotationAngle);
                        transformMatrix.Scale(g_zoomFactor, g_zoomFactor);
                        transformMatrix.Translate(-g_viewCenter.X, -g_viewCenter.Y);
                        transformMatrix.Invert();
                        PointF pointF = {(float)p.x, (float)p.y};
                        transformMatrix.TransformPoints(&pointF, 1);
                        layer_manager.addPoint({(LONG)pointF.X, (LONG)pointF.Y, pressure});

                        // ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼ ここからが新しい超高速描画 ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼

                        // 2. 画面に直接、"アンチエイリアスのかかった"軽量な線を描画する
                        HDC hdc = GetDC(hwnd);
                        if (hdc)
                        {
                            Graphics screenGraphics(hdc);
                            screenGraphics.SetSmoothingMode(SmoothingModeAntiAlias);

                            int maxToolWidth = layer_manager.getCurrentToolWidth();
                            COLORREF toolColorRef = layer_manager.getPenColor();
                            if (layer_manager.getCurrentMode() == DrawMode::Eraser)
                            {
                                toolColorRef = RGB(255, 255, 255);
                            }

                            // 【修正点1】RasterLayer同様、平均筆圧で太さを計算
                            float currentPressureFactor = (float)pressure / 1024.0f;
                            float lastPressureFactor = (float)g_lastPressure / 1024.0f;
                            float averagePressureFactor = (currentPressureFactor + lastPressureFactor) / 2.0f;
                            float pressureWidth = maxToolWidth * averagePressureFactor;

                            float previewWidth = pressureWidth * g_zoomFactor;
                            if (previewWidth < 1.0f)
                            {
                                previewWidth = 1.0f;
                            }

                            Color penColor(GetRValue(toolColorRef), GetGValue(toolColorRef), GetBValue(toolColorRef));
                            Pen gdiplusPen(penColor, previewWidth);

                            // 【修正点2】RasterLayerの設定と完全に一致させる
                            gdiplusPen.SetStartCap(LineCapRound);
                            gdiplusPen.SetEndCap(LineCapRound);
                            gdiplusPen.SetLineJoin(LineJoinRound); // 角を滑らかにする設定を追加

                            if (g_lastScreenPoint.x != -1)
                            {
                                screenGraphics.DrawLine(&gdiplusPen, g_lastScreenPoint.x, g_lastScreenPoint.y, p.x, p.y);
                            }

                            ReleaseDC(hwnd, hdc);
                        }

                        // 現在の情報を「直前の情報」として更新
                        g_lastScreenPoint = p;
                        g_lastPressure = pressure;
                    }
                }
            }
        }
        // ホバー時の処理は LayerListProc に移譲
        return 0;
    }

    case WM_POINTERUP:
    {
        g_isPenContact = false;
        // 前回のスクリーン座標をリセット
        g_lastScreenPoint = {-1, -1};
        g_lastPressure = 0;

        // OutputDebugStringW(L"--- WM_POINTERUP ---\n");

        if (g_isRotateMode)
        {
            // 何もしない
        }
        //  視点移動モード中の処理
        else if (g_isPanMode)
        {
            ReleaseCapture(); // マウスキャプチャを解放
        }
        else if (g_isZoomMode)
        {
            // 何もしない
        }
        else
        {
            // ペンが離れたら、現在のストロークを終了する
            layer_manager.startNewStroke();
            // 【重要】ペンを離したときに、最終的な正しい絵を再描画する
            InvalidateRect(hwnd, NULL, FALSE);

            if (g_uiManager)
            {
                InvalidateRect(g_uiManager->GetLayerListHandle(), NULL, FALSE);
            }
        }

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

        // UIManagerで再配置
        if (g_uiManager)
        {
            g_uiManager->ResizeControls(g_nClientWidth, g_nClientHeight);
        }

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

    // ウインドウが再表示されたタイミング
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // バックバッファがまだ作成されていない場合は何もしない
        if (g_pBackBuffer)
        {
            // 1. バックバッファのGraphicsオブジェクトを取得
            Graphics backBufferGraphics(g_pBackBuffer);

            // 2. バックバッファ全体をクリアし、全レイヤーを描画（全体を再描画）
            backBufferGraphics.Clear(Color(255, 255, 255, 255));
            backBufferGraphics.SetSmoothingMode(SmoothingModeAntiAlias);

            Matrix transformMatrix;
            float centerX = g_nClientWidth / 2.0f;
            float centerY = g_nClientHeight / 2.0f;
            transformMatrix.Translate(centerX, centerY);
            transformMatrix.Rotate(g_rotationAngle);
            transformMatrix.Scale(g_zoomFactor, g_zoomFactor);
            transformMatrix.Translate(-g_viewCenter.X, -g_viewCenter.Y);
            backBufferGraphics.SetTransform(&transformMatrix);

            layer_manager.draw(&backBufferGraphics);

            // 3. 【最適化の鍵】完成したバックバッファから、「無効化された領域(ps.rcPaint)だけ」を画面にコピー
            Graphics screenGraphics(hdc);
            screenGraphics.DrawImage(g_pBackBuffer, ps.rcPaint.left, ps.rcPaint.top,
                                     ps.rcPaint.left, ps.rcPaint.top,
                                     ps.rcPaint.right - ps.rcPaint.left,
                                     ps.rcPaint.bottom - ps.rcPaint.top,
                                     UnitPixel);
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

        // ペンホバーの処理
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

    // ペンが領域から離れたときの処理
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
        layer_manager->renameLayer(g_uiManager->GetEditingIndex(), buffer);

        // リストボックスを更新
        g_uiManager->UpdateLayerList();

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

// モード管理を統合するヘルパー関数
void UpdateToolMode(HWND hwnd)
{
    // 現在の各キーの押下状態を取得
    bool isCtrlDown = GetKeyState(VK_CONTROL) < 0;
    bool isSpaceDown = GetKeyState(VK_SPACE) < 0;
    bool isRDown = GetKeyState('R') < 0;

    // 1. ズームモード (Ctrl + Space) を最優先で判定
    if (isCtrlDown && isSpaceDown)
    {
        if (!g_isZoomMode)
        {
            g_isZoomMode = true;
            g_isPanMode = false;
            g_isRotateMode = false;
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        }
    }
    // 2. パンモード (Space単体) を次に判定
    else if (isSpaceDown)
    {
        if (!g_isPanMode)
        {
            g_isPanMode = true;
            g_isZoomMode = false;
            g_isRotateMode = false;
            SetCursor(LoadCursor(nullptr, IDC_HAND));
        }
    }
    // 3. 回転モード (R単体) を次に判定
    else if (isRDown)
    {
        if (!g_isRotateMode)
        {
            g_isRotateMode = true;
            g_isPanMode = false;
            g_isZoomMode = false;
            SetCursor(LoadCursor(nullptr, IDC_CROSS));
        }
    }
    // 4. 上記のいずれでもなければ、全ての視点操作モードを解除
    else
    {
        if (g_isZoomMode || g_isPanMode || g_isRotateMode)
        {
            g_isZoomMode = false;
            g_isPanMode = false;
            g_isRotateMode = false;
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }
    }
}