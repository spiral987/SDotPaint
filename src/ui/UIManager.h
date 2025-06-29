#pragma once
#include <windows.h>
#include <CommCtrl.h>

// UIコントロールのIDを定義
#define ID_ADD_LAYER_BUTTON 1001
#define ID_DELETE_LAYER_BUTTON 1002
#define ID_LAYER_LISTBOX 1003

class LayerManager;
// ボタンやリストのUIを設定する
class UIManager
{
public:
    // コンストラクタでメインウィンドウのハンドルを受け取る。
    UIManager(HWND hParent, LayerManager &layerManager);
    ~UIManager();

    // UI要素をすべて作成します
    void CreateControls();

    // レイヤーリストをサブクラス化
    void SetupLayerListSubclass();

    // レイヤーリストを更新する
    void UpdateLayerList();

    // スライダーの値を取得する
    int GetSliderValue() const;

    // スライダーの値を更新する
    void SetSliderValue(int value);

    // スライダーの値表示用ボックスを更新する
    void UpdateStaticValue(int value);

    // WM_COMMANDメッセージを処理します
    void HandleCommand(WPARAM wParam);

    // ウィンドウサイズが変更されたときにUIを再配置します
    void ResizeControls(int parentWidth, int parentHeight);

    // getter
    int GetEditingIndex() const { return _nEditingIndex; }
    // 各コントロールのハンドルをここに保持します
    HWND GetLayerListHandle() const { return m_hLayerList; }
    HWND GetSliderHandle() const { return m_hSlider; }
    HWND GetStaticValueHandle() const { return m_hStaticValue; }
    HWND GetAddButtonHandle() const { return m_hAddButton; }
    HWND GetDelButtonHandle() const { return m_hDelButton; }

private:
    HWND m_hParent;                // 親ウィンドウのハンドル
    HFONT m_hButtonFont = nullptr; // ボタン用のフォントハンドル
    LayerManager &m_layerManager;
    int _nEditingIndex = -1; // 編集中のレイヤーのインデックス

    // UIコントロールのハンドルを追加
    HWND m_hSlider = nullptr;      // スライダーのハンドルを保持
    HWND m_hStaticValue = nullptr; // 数値表示用
    HWND m_hLayerList = nullptr;   // レイヤーリストボックス
    HWND m_hAddButton = nullptr;   // 追加ボタン
    HWND m_hDelButton = nullptr;   // 削除ボタン
};