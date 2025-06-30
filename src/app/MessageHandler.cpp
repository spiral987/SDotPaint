#include <windows.h>

#include "globals.h"
#include "MessageHandler.h"
#include "core/LayerManager.h"
#include "ui/UIManager.h"

MessageHandler::MessageHandler(HWND hwnd)
    : m_hwnd(hwnd),
      m_viewManager(0, 0),
      m_isTransforming(false),
      m_lastScreenPoint({-1, -1}),
      m_lastPressure(0)
{
    this->HandleCreate();
}

void MessageHandler::HandleCreate()
{
    // ウィンドウのクライアント領域のサイズを取得
    RECT rect;
    GetClientRect(m_hwnd, &rect);

    g_nClientWidth = rect.right - rect.left;
    g_nClientHeight = rect.bottom - rect.top;

    m_viewManager.UpdateClientSize(g_nClientWidth, g_nClientHeight);

    // ダブルバッファリング用のビットマップを作成
    g_pBackBuffer = new Bitmap(g_nClientWidth, g_nClientHeight, PixelFormat32bppARGB);

    // 最初のレイヤーを追加し、リストを更新
    layer_manager.createNewRasterLayer(g_nClientWidth, g_nClientHeight, L"レイヤー1");

    // UIManagerを作成してUI処理をする
    g_pUIManager = std::make_unique<UIManager>(m_hwnd, layer_manager);
    g_pUIManager->CreateControls();
    g_pUIManager->SetupLayerListSubclass();
    g_pUIManager->UpdateLayerList();
    OutputDebugStringW(L"UI controls created successfully.\n"); // ★デバッグ出力追加★
}

BOOL MessageHandler::HandleDrawItem(WPARAM wParam, LPARAM lParam)
{
    return g_pUIManager->HandleDrawItem(wParam, lParam);
}

void MessageHandler::HandleKeyUp(WPARAM wParam, LPARAM lParam)
{
    // キーが離されたら、現在のキー状態に基づいてモードを更新する
    UpdateToolMode();
}

void MessageHandler::HandleKeyDown(WPARAM wParam, LPARAM lParam)
{
    // キーが押されたら、まずモードを更新する
    if (wParam == VK_CONTROL || wParam == VK_SPACE || wParam == 'R')
    {
        UpdateToolMode();
        return; // モード変更キーの場合は、他の処理をしない
    }

    switch (wParam)
    {
    case 'E': // 消しゴム
    {
        SetFocus(m_hwnd);
        layer_manager.setDrawMode(DrawMode::Eraser);
        { // 変数スコープを明確にするための括弧
            int width = layer_manager.getCurrentToolWidth();
            g_pUIManager->SetSliderValue(width);
            g_pUIManager->UpdateStaticValue(width);
        }
        break;
    }
    case 'Q': // ペン
    {
        SetFocus(m_hwnd);
        layer_manager.setDrawMode(DrawMode::Pen);
        {
            int width = layer_manager.getCurrentToolWidth();
            g_pUIManager->SetSliderValue(width);
            g_pUIManager->UpdateStaticValue(width);
        }
        break;
    }
    case 'C': // 色選択(Color)
    {
        SetFocus(m_hwnd);
        // 1. ダイアログ設定用の構造体を準備
        CHOOSECOLOR cc;
        static COLORREF customColors[16]; // カスタムカラーを保存する配列

        ZeroMemory(&cc, sizeof(cc)); // 構造体をゼロで初期化
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = m_hwnd;                      // 親ウィンドウのハンドル
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
        SetFocus(m_hwnd);
        break;
    }
    }
}

void MessageHandler::HandleMouseMove(WPARAM wParam, LPARAM lParam)
{
    // UIManager経由でレイヤーリストのハンドルを取得
    HWND hLayerList = nullptr;
    if (g_pUIManager)
    {
        hLayerList = g_pUIManager->GetLayerListHandle();
    }

    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    HWND hChildUnderCursor = ChildWindowFromPoint(m_hwnd, pt);

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
}

void MessageHandler::HandleCommand(WPARAM wParam, LPARAM lParam)
{
    if (g_pUIManager)
    {
        g_pUIManager->HandleCommand(wParam);
    }
}

void MessageHandler::HandleVScroll(WPARAM wParam, LPARAM lParam)
{
    // スライダーからのメッセージか確認
    if (g_pUIManager && (HWND)lParam == g_pUIManager->GetSliderHandle())
    {
        int newWidth = g_pUIManager->GetSliderValue();

        // 現在のモードに応じて、対応するツールの太さを更新
        if (layer_manager.getCurrentMode() == DrawMode::Pen)
        {
            layer_manager.setPenWidth(newWidth);
        }
        else
        {
            layer_manager.setEraserWidth(newWidth);
        }

        g_pUIManager->UpdateStaticValue(newWidth);

        // フォーカスをメインウィンドウに戻す(これが無いとスライダーから抜け出せなくなる)
        SetFocus(m_hwnd);
    }
}

void MessageHandler::HandlePointerDown(WPARAM wParam, LPARAM lParam)
{
    g_isPenContact = true;
    SetFocus(m_hwnd);

    POINTER_INFO pointerInfo;
    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    if (!GetPointerInfo(pointerId, &pointerInfo))
    {
        return;
    }

    POINT currentPoint = pointerInfo.ptPixelLocation;
    ScreenToClient(m_hwnd, &currentPoint);

    // 視点操作モード（パン、ズーム、回転）の場合
    if (g_isPanMode || g_isRotateMode || g_isZoomMode)
    {
        m_isTransforming = true;
        m_operationStartPoint = currentPoint;
        if (g_isPanMode)
        {
            m_viewManager.PanStart(currentPoint);
        }
        if (g_isRotateMode)
        {
            m_viewManager.RotateStart();
        }
        if (g_isZoomMode)
        {
            m_viewManager.ZoomStart();
        }
        // RotateStart, ZoomStartは空なので呼ばなくてもOK
        SetCapture(m_hwnd);
        return;
    }

    else
    {
        // 右ボタンがクリックされた場合はクリア処理
        if (IS_POINTER_SECONDBUTTON_WPARAM(wParam))
        {
            layer_manager.clear();
            InvalidateRect(m_hwnd, nullptr, TRUE); // 背景を白でクリア
            return;
        }

        layer_manager.startNewStroke();

        POINTER_PEN_INFO penInfo;
        UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
        if (GetPointerPenInfo(pointerId, &penInfo) && penInfo.pointerInfo.pointerType == PT_PEN)
        {
            POINT p = penInfo.pointerInfo.ptPixelLocation;
            ScreenToClient(m_hwnd, &p);
            UINT32 pressure = penInfo.pressure;

            // ワールド座標への変換をViewManagerに任せる
            PointF worldPoint = m_viewManager.ScreenToWorld(p);
            layer_manager.addPoint({(LONG)worldPoint.X, (LONG)worldPoint.Y, pressure});

            // 最初の点の筆圧を保存
            m_lastScreenPoint = p;
            g_lastPressure = pressure;
        }
        return;
    }
}

void MessageHandler::HandlePointerUpdate(WPARAM wParam, LPARAM lParam)
{
    // ペンがキャンバスに接触していない場合は何もしない
    if (!g_isPenContact)
        return;

    // 視点移動処理
    if (m_isTransforming)
    {
        POINTER_INFO pointerInfo;
        UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
        if (!GetPointerInfo(pointerId, &pointerInfo))
            return;

        POINT currentPoint = pointerInfo.ptPixelLocation;
        ScreenToClient(m_hwnd, &currentPoint);

        // 各モードに応じてViewManagerのメソッドを呼び出すだけ！
        if (g_isPanMode)
        {
            m_viewManager.PanUpdate(currentPoint);
        }
        else if (g_isRotateMode)
        {
            // 回転は開始点からの差分で計算するので、開始点も渡す
            m_viewManager.RotateUpdate(currentPoint, m_operationStartPoint);
        }
        else if (g_isZoomMode)
        {
            // ズームも開始点からの差分で計算する
            m_viewManager.ZoomUpdate(currentPoint, m_operationStartPoint);
        }

        // 変更を画面に反映させる
        InvalidateRect(m_hwnd, NULL, FALSE);
        return; // 視点操作中は描画処理を行わない
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
                ScreenToClient(m_hwnd, &p);
                UINT32 pressure = penInfo.pressure;

                // 1. ワールド座標に変換し、レイヤーのデータに記録（永続化のため）
                // ワールド座標への変換をViewManagerに任せる
                PointF worldPoint = m_viewManager.ScreenToWorld(p);
                layer_manager.addPoint({(LONG)worldPoint.X, (LONG)worldPoint.Y, pressure});

                // 2. 画面に直接、"アンチエイリアスのかかった"軽量な線を描画する
                HDC hdc = GetDC(m_hwnd);
                if (hdc)
                {
                    Graphics screenGraphics(hdc);
                    screenGraphics.SetSmoothingMode(SmoothingModeAntiAlias);

                    // プレビュー描画にも、完全な変換行列を適用する
                    Matrix transformMatrix;
                    m_viewManager.GetTransformMatrix(&transformMatrix);
                    screenGraphics.SetTransform(&transformMatrix);

                    int maxToolWidth = layer_manager.getCurrentToolWidth();
                    COLORREF toolColorRef = layer_manager.getPenColor();
                    if (layer_manager.getCurrentMode() == DrawMode::Eraser)
                    {
                        toolColorRef = RGB(255, 255, 255);
                    }

                    // RasterLayer同様、平均筆圧で太さを計算
                    float currentPressureFactor = (float)pressure / 1024.0f;
                    float lastPressureFactor = (float)g_lastPressure / 1024.0f;
                    float averagePressureFactor = (currentPressureFactor + lastPressureFactor) / 2.0f;
                    float pressureWidth = maxToolWidth * averagePressureFactor;

                    // 変換行列で既にズームが適用されているので、プレビュー幅でズームを掛ける必要はなくなる
                    float previewWidth = pressureWidth;
                    if (previewWidth < 1.0f / m_viewManager.GetZoomFactor())
                    {
                        // どんなに細くても最低1ピクセルは表示されるように調整
                        previewWidth = 1.0f / m_viewManager.GetZoomFactor();
                    }

                    Color penColor(GetRValue(toolColorRef), GetGValue(toolColorRef), GetBValue(toolColorRef));
                    Pen gdiplusPen(penColor, previewWidth);

                    // RasterLayerの設定と完全に一致させる
                    gdiplusPen.SetStartCap(LineCapRound);
                    gdiplusPen.SetEndCap(LineCapRound);
                    gdiplusPen.SetLineJoin(LineJoinRound); // 角を滑らかにする設定を追加

                    if (m_lastScreenPoint.x != -1)
                    {
                        // 描画する座標も、ワールド座標に変換したものを使う
                        PointF lastWorldPoint = m_viewManager.ScreenToWorld(m_lastScreenPoint);
                        screenGraphics.DrawLine(&gdiplusPen, lastWorldPoint, worldPoint);
                    }

                    ReleaseDC(m_hwnd, hdc);
                }

                // 現在の情報を「直前の情報」として更新
                m_lastScreenPoint = p;
                m_lastPressure = pressure;
            }
        }
    }
}

void MessageHandler::HandlePointerUp(WPARAM wParam, LPARAM lParam)
{
    if (m_isTransforming)
    {
        m_isTransforming = false;            // ★操作終了
        InvalidateRect(m_hwnd, NULL, FALSE); // ★最後に最高品質で描き直すよう要求
    }

    g_isPenContact = false;
    m_lastScreenPoint = {-1, -1};
    m_lastPressure = 0;

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
        InvalidateRect(m_hwnd, NULL, FALSE);

        if (g_pUIManager)
        {
            InvalidateRect(g_pUIManager->GetLayerListHandle(), NULL, FALSE);
        }
    }
}

void MessageHandler::HandleSize(WPARAM wParam, LPARAM lParam)
{
    // ウィンドウサイズを更新
    g_nClientWidth = LOWORD(lParam);
    g_nClientHeight = HIWORD(lParam);

    // 古いバックバッファを削除
    delete g_pBackBuffer;

    // 新しいサイズのバックバッファを作成（GDI+オブジェクトなのでPixelFormatを指定する）
    g_pBackBuffer = new Bitmap(g_nClientWidth, g_nClientHeight, PixelFormat32bppARGB);

    // UIManagerで再配置
    if (g_pUIManager)
    {
        g_pUIManager->ResizeControls(g_nClientWidth, g_nClientHeight);
    }

    // ここで再描画をかけておくと、リサイズ時に描画が追従する
    InvalidateRect(m_hwnd, NULL, FALSE);
}
void MessageHandler::HandleDestroy(WPARAM wParam, LPARAM lParam)
{
    // バックバッファを解放
    delete g_pBackBuffer;
    GdiplusShutdown(gdiplusToken);
    PostQuitMessage(0); // メッセージループを終了させる
}
void MessageHandler::HandlePaint(WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    // バックバッファがまだ作成されていない場合は何もしない
    if (g_pBackBuffer)
    {
        // 1. バックバッファのGraphicsオブジェクトを取得
        Graphics backBufferGraphics(g_pBackBuffer);

        // 2. 描画を始める前に、バックバッファ全体を白でクリアする
        backBufferGraphics.Clear(Color(255, 255, 255, 255));

        // 視点操作中なら、速度優先の最も軽い補間モードに設定
        if (m_isTransforming)
        {
            backBufferGraphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
        }
        // 通常時なら、品質優先の補間モードに設定
        else
        {
            backBufferGraphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        }

        // ワールド座標からスクリーン座標への変換をViewManagerに任せる
        Matrix transformMatrix;
        m_viewManager.GetTransformMatrix(&transformMatrix); // ViewManagerから変換行列を取得
        backBufferGraphics.SetTransform(&transformMatrix);  // バックバッファをスクリーン座標にする

        layer_manager.draw(&backBufferGraphics);

        // 3. 【最適化の鍵】完成したバックバッファから、「無効化された領域(ps.rcPaint)だけ」を画面にコピー
        Graphics screenGraphics(hdc);
        screenGraphics.DrawImage(g_pBackBuffer, ps.rcPaint.left, ps.rcPaint.top,
                                 ps.rcPaint.left, ps.rcPaint.top,
                                 ps.rcPaint.right - ps.rcPaint.left,
                                 ps.rcPaint.bottom - ps.rcPaint.top,
                                 UnitPixel);
    }

    EndPaint(m_hwnd, &ps);
}

// モード管理をする関数
void MessageHandler::UpdateToolMode()
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

LRESULT MessageHandler::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {

    // メニューをアプリケーション側で描画してくださいというメッセージが来たら(オーナードロー)
    case WM_DRAWITEM:
    {
        return this->HandleDrawItem(wParam, lParam);
        break;
    }

    // キーを離したときの処理
    case WM_KEYUP:
    {
        this->HandleKeyUp(wParam, lParam);
        break;
    }

    // ボタンやリスト
    case WM_COMMAND:
    {
        this->HandleCommand(wParam, lParam);
        break;
    }

    case WM_KEYDOWN:
    {
        g_pMessageHandler->HandleKeyDown(wParam, lParam);
        break;
    }

    case WM_VSCROLL:
    {
        this->HandleVScroll(wParam, lParam);
        break;
    }
    case WM_POINTERDOWN:
    {
        this->HandlePointerDown(wParam, lParam);
        break;
    }
    case WM_POINTERUPDATE:
    {
        this->HandlePointerUpdate(wParam, lParam);

        break;
    }

    case WM_POINTERUP:
    {
        this->HandlePointerUp(wParam, lParam);

        break;
    }

    case WM_SIZE:
    {

        this->HandleSize(wParam, lParam);

        break;
    }

    // ウィンドウが破棄されるときのメッセージ
    case WM_DESTROY:
    {
        this->HandleDestroy(wParam, lParam);

        break; // ウィンドウを描画する必要があるときのメッセージ
    }

    // ウインドウが再表示されたタイミング
    case WM_PAINT:
    {

        this->HandlePaint(wParam, lParam);

        break;
    }

    default:
        // 自分で処理しないメッセージは、デフォルトの処理に任せる（非常に重要）
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return 0;
}