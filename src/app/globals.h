// src/app/globals.h
#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <memory>

using namespace Gdiplus;

// UIコントロールID定数
constexpr int ID_LAYER_LISTBOX = 1001;
constexpr int ID_ADD_LAYER_BUTTON = 1002;
constexpr int ID_DELETE_LAYER_BUTTON = 1003;
constexpr int ID_SLIDER = 1004;
constexpr int ID_STATIC_VALUE = 1005;

// 前方宣言 (ヘッダー同士の循環参照を防ぐため)
class UIManager;
class LayerManager;
class MessageHandler;

// --- グローバル変数の宣言 ---

// マネージャーを保持するユニークポインタ
extern std::unique_ptr<UIManager> g_pUIManager;
extern std::unique_ptr<MessageHandler> g_pMessageHandler;
extern LayerManager layer_manager; // もしlayer_managerもグローバルなら

extern ULONG_PTR gdiplusToken;

// ダブルバッファリング用の変数
extern Bitmap *g_pBackBuffer;
extern int g_nClientWidth;
extern int g_nClientHeight;

extern POINT g_panLastPoint; // 視点移動時の最後のマウス位置

extern float g_rotationAngle; // 現在の総回転角度

// マウスリーブイベントをトラックするためのフラグ
extern bool g_bTrackingMouse;

// 最後の点の筆圧をほぞんする変数を追加
extern UINT32 g_lastPressure;

extern bool g_isPenContact;