#include "ui/UIManager.h"
#include "core/LayerManager.h"

#include <string>
#include <windows.h>
#include <CommCtrl.h>

// レイヤーリストボックスのサブクラスプロシージャ（前方宣言）
LRESULT CALLBACK LayerListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

LRESULT CALLBACK EditControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

UIManager::UIManager(HWND hParent, LayerManager &layerManager)
    : m_hParent(hParent), m_layerManager(layerManager)
{
    // 安全のため初期化
    m_hSlider = nullptr;      // スライダーのハンドルを保持
    m_hStaticValue = nullptr; // 数値表示用
    m_hLayerList = nullptr;   // レイヤーリストボックス
    m_hAddButton = nullptr;   // 追加ボタン
    m_hDelButton = nullptr;   // 削除ボタン
}

UIManager::~UIManager()
{
    // フォントリソースを解放
    if (m_hButtonFont)
    {
        DeleteObject(m_hButtonFont);
    }
}

void UIManager::CreateControls()
{
    // 親ウインドウのクライアント領域とインスタンスハンドルを取得
    RECT clientRect;
    GetClientRect(m_hParent, &clientRect);
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(m_hParent, GWLP_HINSTANCE);

    // ボタン用のフォント
    m_hButtonFont = CreateFontW(
        -14,                      // フォントの高さ (負の値は文字の高さを指定)
        0,                        // 文字の幅 (0で自動)
        0, 0,                     // エスケープメントと傾き (通常は0)
        FW_NORMAL,                // フォントの太さ (FW_NORMAL, FW_BOLDなど)
        FALSE, FALSE, FALSE,      // イタリック、下線、打ち消し線
        SHIFTJIS_CHARSET,         // 文字セット (日本語なのでSHIFTJIS)
        OUT_DEFAULT_PRECIS,       // 出力精度
        CLIP_DEFAULT_PRECIS,      // クリッピング精度
        DEFAULT_QUALITY,          // 出力品質
        DEFAULT_PITCH | FF_SWISS, // ピッチとファミリー
        L"Yu Gothic UI"           // フォント名 (MS UI Gothicなども可)
    );

    // スライダーコントロールの作成
    m_hSlider = CreateWindowExW(
        0,
        TRACKBAR_CLASSW, // スライダークラス
        L"Pen Size",
        WS_CHILD | WS_VISIBLE | TBS_VERT, // 子ウィンドウ・可視・縦方向
        10, 50,                           // X, Y座標
        30, 500,                          // 幅, 高さ
        m_hParent,                        // 親ウィンドウ
        (HMENU)1,                         // コントロールID
        hInstance,
        nullptr);

    if (m_hSlider)
    {
        SendMessage(m_hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(1, 100)); // スライダーの範囲を設定 (最小値1, 最大値100）
        SendMessage(m_hSlider, TBM_SETPOS, TRUE, 5);                    // 初期位置を5に設定
    }

    // スライダーの値表示用
    m_hStaticValue = CreateWindowExW(
        0,
        L"STATIC", // コントロールのクラス名
        L"5",      // 初期テキスト（ペンの初期値に合わせる）
        WS_CHILD | WS_VISIBLE,
        45, 50,    // X, Y座標（スライダーの右隣あたりに配置）
        50, 20,    // 幅, 高さ
        m_hParent, // 親ウィンドウ
        (HMENU)2,  // コントロールID（他と被らないように）
        hInstance,
        nullptr);

    m_hLayerList = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED,
        clientRect.right - 200, 10, 190, 200, // 右端から200px幅で配置
        m_hParent, (HMENU)ID_LAYER_LISTBOX, hInstance, nullptr);

    // 追加ボタン
    m_hAddButton = CreateWindowExW(
        0, L"BUTTON", L"レイヤー追加",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 10, 100, 30, m_hParent, (HMENU)ID_ADD_LAYER_BUTTON, hInstance, nullptr);

    // 削除ボタン
    m_hDelButton = CreateWindowExW(
        0, L"BUTTON", L"レイヤー削除",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        120, 10, 100, 30, m_hParent, (HMENU)ID_DELETE_LAYER_BUTTON, hInstance, nullptr);

    // 作成したフォントを各ボタンに設定
    if (m_hButtonFont)
    {
        SendMessage(m_hAddButton, WM_SETFONT, (WPARAM)m_hButtonFont, TRUE);
        SendMessage(m_hDelButton, WM_SETFONT, (WPARAM)m_hButtonFont, TRUE);
    }
}

void UIManager::SetupLayerListSubclass()
{
    if (m_hLayerList && &m_layerManager)
    {
        // リストボックスのサブクラス化
        SetWindowSubclass(m_hLayerList, LayerListProc, 0, (DWORD_PTR)&m_layerManager);
    }
}

void UIManager::UpdateLayerList()
{
    // リストボックスをクリア
    SendMessage(m_hLayerList, LB_RESETCONTENT, 0, 0);

    // LayerManagerからレイヤーのリストを取得してリストボックスに追加
    const auto &layers = m_layerManager.getLayers();
    for (const auto &layer : layers)
    {
        SendMessage(m_hLayerList, LB_ADDSTRING, 0, (LPARAM)layer->getName().c_str());
    }

    // 現在アクティブなレイヤーを選択状態にする
    int activeIndex = m_layerManager.getActiveLayerIndex();
    SendMessage(m_hLayerList, LB_SETCURSEL, activeIndex, 0);

    // ウィンドウ全体を再描画
    InvalidateRect(m_hParent, nullptr, TRUE);
}

int UIManager::GetSliderValue() const
{
    if (m_hSlider)
    {
        return (int)SendMessage(m_hSlider, TBM_GETPOS, 0, 0);
    }
    return 5; // デフォルト値
}

void UIManager::SetSliderValue(int value)
{
    if (m_hSlider)
    {
        SendMessage(m_hSlider, TBM_SETPOS, TRUE, value);
        UpdateStaticValue(value);
    }
}

void UIManager::UpdateStaticValue(int value)
{
    if (m_hStaticValue)
    {
        std::wstring valueStr = std::to_wstring(value);
        SetWindowTextW(m_hStaticValue, valueStr.c_str());
    }
}

void UIManager::HandleCommand(WPARAM wParam)
{

    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    switch (wmId)
    {
        // 追加ボタン
    case ID_ADD_LAYER_BUTTON:
    {
        RECT rect;
        GetClientRect(m_hParent, &rect);
        m_layerManager.addNewRasterLayer(rect.right - rect.left, rect.bottom - rect.top);
        UpdateLayerList(); // リストを更新
        SetFocus(m_hParent);
        break;
    }
    case ID_DELETE_LAYER_BUTTON:
    {
        m_layerManager.deleteActiveLayer();
        UpdateLayerList(); // リストを更新
        SetFocus(m_hParent);
        break;
    }

    case ID_LAYER_LISTBOX:
    {

        // 名称変更のため ダブルクリックされた場合
        if (wmEvent == LBN_DBLCLK)
        {
            // 選択されている項目のインデックスを取得
            int selectedIndex = SendMessage(m_hLayerList, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR)
            {
                _nEditingIndex = selectedIndex; // 編集中のインデックスを保存

                // 選択項目の矩形（位置とサイズ）を取得
                RECT itemRect;
                SendMessage(m_hLayerList, LB_GETITEMRECT, selectedIndex, (LPARAM)&itemRect);

                // 編集用エディットコントロールを作成
                HWND hEdit = CreateWindowExW(
                    0, L"EDIT", L"",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    itemRect.left, itemRect.top, itemRect.right - itemRect.left, itemRect.bottom - itemRect.top,
                    m_hLayerList, // 親をリストボックスにする
                    (HMENU)999,   // 独自のID
                    (HINSTANCE)GetWindowLongPtr(m_hParent, GWLP_HINSTANCE),
                    NULL);

                // 現在のレイヤー名を設定
                const auto &layers = m_layerManager.getLayers();
                SetWindowTextW(hEdit, layers[selectedIndex]->getName().c_str());

                // エディットコントロールのフォントをリストボックスに合わせる
                SendMessage(hEdit, WM_SETFONT, SendMessage(m_hLayerList, WM_GETFONT, 0, 0), TRUE);

                // エディットコントロールのプロシージャをサブクラス化
                SetWindowSubclass(hEdit, EditControlProc, 0, (DWORD_PTR)&m_layerManager);

                // エディットコントロールにフォーカスを合わせ、テキストを全選択
                SetFocus(hEdit);
                SendMessage(hEdit, EM_SETSEL, 0, -1);
            }
        }
        // 選択が変更された場合
        else if (wmEvent == LBN_SELCHANGE)
        {
            int selectedIndex = SendMessage(m_hLayerList, LB_GETCURSEL, 0, 0);
            if (selectedIndex != LB_ERR)
            {
                m_layerManager.setActiveLayer(selectedIndex);
                InvalidateRect(m_hParent, NULL, FALSE); // 再描画
            }
        }
        break;
    }
    }
}

void UIManager::ResizeControls(int parentWidth, int parentHeight)
{
    // レイアウトの定数を定義しておくと、後で調整しやすくなります
    const int PADDING = 10;            // コントロール間の余白
    const int RIGHT_PANEL_WIDTH = 200; // 右側パネルの幅
    const int BUTTON_HEIGHT = 30;      // ボタンの高さ
    const int SLIDER_WIDTH = 30;       // スライダーの幅
    const int STATIC_TEXT_WIDTH = 50;  // テキスト表示の幅

    // 1. 右側パネル（レイヤーリスト、追加・削除ボタン）の再配置
    if (m_hLayerList)
    {
        int listX = parentWidth - RIGHT_PANEL_WIDTH + PADDING;
        int listY = PADDING;
        int listWidth = RIGHT_PANEL_WIDTH - (PADDING * 2);
        int listHeight = parentHeight - (BUTTON_HEIGHT + PADDING * 3); // ボタンの高さを引く

        MoveWindow(m_hLayerList, listX, listY, listWidth, listHeight, TRUE);
    }
    if (m_hAddButton)
    {
        int btnWidth = (RIGHT_PANEL_WIDTH - (PADDING * 3)) / 2; // ボタンを2つ並べる
        int btnY = parentHeight - BUTTON_HEIGHT - PADDING;
        int btn1X = parentWidth - RIGHT_PANEL_WIDTH + PADDING;

        MoveWindow(m_hAddButton, btn1X, btnY, btnWidth, BUTTON_HEIGHT, TRUE);
    }
    if (m_hDelButton)
    {
        int btnWidth = (RIGHT_PANEL_WIDTH - (PADDING * 3)) / 2;
        int btnY = parentHeight - BUTTON_HEIGHT - PADDING;
        int btn2X = parentWidth - RIGHT_PANEL_WIDTH + PADDING * 2 + btnWidth;

        MoveWindow(m_hDelButton, btn2X, btnY, btnWidth, BUTTON_HEIGHT, TRUE);
    }

    // 2. 左側パネル（スライダー、数値表示）の再配置
    if (m_hSlider)
    {
        int sliderY = PADDING * 2 + BUTTON_HEIGHT; // 上のボタンとかぶらないように
        int sliderHeight = parentHeight - sliderY - PADDING;

        MoveWindow(m_hSlider, PADDING, sliderY, SLIDER_WIDTH, sliderHeight, TRUE);
    }
    if (m_hStaticValue)
    {
        int staticX = PADDING + SLIDER_WIDTH;
        int staticY = PADDING * 2 + BUTTON_HEIGHT;

        MoveWindow(m_hStaticValue, staticX, staticY, STATIC_TEXT_WIDTH, 20, TRUE);
    }
}
