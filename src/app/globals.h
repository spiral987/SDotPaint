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

// 視点移動用のグローバル変数
extern bool g_isPanMode;     // 視点移動モードかどうかのフラグ
extern POINT g_panLastPoint; // 視点移動時の最後のマウス位置
extern PointF g_viewCenter;  // ワールド座標系でのビュー中心

extern bool g_isRotateMode;   // 回転モードかどうかのフラグ
extern float g_rotationAngle; // 現在の総回転角度
extern float g_startAngle;    // 回転開始時の角度

extern bool g_isZoomMode;        // ズームモードかどうかのフラグ
extern float g_zoomFactor;       // 現在のズーム率
extern float g_baseZoomFactor;   // ズーム開始時のズーム率
extern POINT g_zoomStartPoint;   // ズーム開始時のスクリーン座標
extern PointF g_zoomCenterWorld; // ズーム基点のワールド座標

extern bool g_isPenContact; // ペンの接触状態を自前で管理するフラグ

extern bool g_isTransforming; // 視点操作中かどうかのフラグ

// マウスリーブイベントをトラックするためのフラグ
extern bool g_bTrackingMouse;

// グローバル変数に「前回のスクリーン座標」を保持する変数を追加
extern POINT g_lastScreenPoint;
extern UINT32 g_lastPressure;