#include <windows.h>
#include <gdiplus.h>

#include "app/globals.h"
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

    m_viewManager.ResetView(); // ビューをリセットして中心に

    // ダブルバッファリング用のビットマップを作成
    g_pBackBuffer = new Bitmap(g_nClientWidth, g_nClientHeight, PixelFormat32bppARGB);

    // 最初のレイヤーを追加し、リストを更新
    layer_manager.createNewRasterLayer(g_nClientWidth, g_nClientHeight, L"レイヤー1");

    // UIManagerを作成してUI処理をする
    m_toolController = std::make_unique<ToolController>(m_hwnd, m_viewManager, layer_manager); // TODO グローバルでも動く？
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
    UpdateToolMode();

    switch (wParam)
    {
    case 'E': // 消しゴム
    {
        // 消しゴムキーが押されたら、LayerManagerのモードを変更し、ツールを更新する
        layer_manager.setCurrentMode(DrawMode::Eraser);
        UpdateToolMode();
        break;
    }
    case 'Q': // ペン
    {
        layer_manager.setCurrentMode(DrawMode::Pen);
        UpdateToolMode();
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

    POINTER_PEN_INFO penInfo;
    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    if (!GetPointerPenInfo(pointerId, &penInfo))
    {
        return;
    }

    // イベントを設定
    PointerEvent event;
    event.hwnd = m_hwnd;
    event.screenPos = penInfo.pointerInfo.ptPixelLocation;
    ScreenToClient(m_hwnd, &event.screenPos);
    event.pressure = penInfo.pressure;

    m_toolController->OnPointerDown(event);
}

void MessageHandler::HandlePointerUpdate(WPARAM wParam, LPARAM lParam)
{
    // ペンがキャンバスに接触していない場合は何もしない
    if (!g_isPenContact)
    {
        return;
    }

    // イベント情報の構築 (この部分は変更なし)
    POINTER_PEN_INFO penInfo;
    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    if (!GetPointerPenInfo(pointerId, &penInfo))
    {
        return;
    }

    PointerEvent event;
    event.hwnd = m_hwnd;
    event.screenPos = penInfo.pointerInfo.ptPixelLocation;
    ScreenToClient(m_hwnd, &event.screenPos);
    event.pressure = penInfo.pressure;

    // ツールコントローラーにイベントを転送
    m_toolController->OnPointerUpdate(event);
}

void MessageHandler::HandlePointerUp(WPARAM wParam, LPARAM lParam)
{

    g_isPenContact = false;

    // イベント情報の構築 (この部分は変更なし)
    POINTER_PEN_INFO penInfo;
    UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    if (!GetPointerPenInfo(pointerId, &penInfo))
    {
        return;
    }

    PointerEvent event;
    event.hwnd = m_hwnd;
    event.screenPos = penInfo.pointerInfo.ptPixelLocation;
    ScreenToClient(m_hwnd, &event.screenPos);
    event.pressure = penInfo.pressure;

    m_toolController->OnPointerUp(event);

    // レイヤーウインドウを再描画して背景色を適用
    if (g_pUIManager)
    {
        g_pUIManager->UpdateLayerList();
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

        Color grayColor(255, 192, 192, 192); // 灰色でクリアしておく

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

        // 描画可能領域を白くする
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        RectF canvasRect(
            0.0f,
            0.0f,
            static_cast<float>(layer_manager.getCanvasWidth()),
            static_cast<float>(layer_manager.getCanvasHeight()));
        backBufferGraphics.FillRectangle(&whiteBrush, canvasRect);

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
    // キーの状態を取得 (変更なし)
    bool isSpaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool isCtrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isRDown = (GetAsyncKeyState('R') & 0x8000) != 0;

    // Zg_isPanModeなどのフラグを操作する代わりに、ToolControllerにツール設定を依頼する
    if (isCtrlDown && isSpaceDown)
    {
        m_toolController->SetTool(ToolType::Zoom);
    }
    else if (isSpaceDown)
    {
        m_toolController->SetTool(ToolType::Pan);
    }
    else if (isRDown)
    {
        m_toolController->SetTool(ToolType::Rotate);
    }
    else
    {
        // デフォルトツール（ペンか消しゴム）に戻す
        // LayerManagerが知っている現在の描画モードに基づいてツールを設定
        if (layer_manager.getCurrentMode() == DrawMode::Pen)
        {
            m_toolController->SetTool(ToolType::Pen);
        }
        else
        {
            m_toolController->SetTool(ToolType::Eraser);
        }
    }

    if (g_pUIManager) // UIManagerが有効な場合のみ
    {
        int currentWidth = layer_manager.getCurrentToolWidth();
        g_pUIManager->SetSliderValue(currentWidth);
        g_pUIManager->UpdateStaticValue(currentWidth);
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