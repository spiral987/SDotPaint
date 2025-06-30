#pragma once
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

// 視点の処理
class ViewManager
{
private:
    // ビューの状態
    PointF m_viewCenter;   // ワールド中心座標
    float m_rotationAngle; // 回転角度
    float m_zoomFactor;    // ズーム倍率

    // 操作開始時(ペンでタッチした瞬間)を記憶
    PointF m_startViewCenter;
    float m_startRotationAngle;
    float m_startZoomFactor;

    // パン操作の一時的なスクリーン座標
    POINT m_panLastPoint;

    // ウインドウサイズ
    int m_clientHeight;
    int m_clientWidth;

public:
    // コンストラクタ
    ViewManager(int clientWidth, int clientHeight);

    // --- 視点操作のインターフェース ---
    void PanStart(POINT screenPoint);  // 移動開始点を記録しておく
    void PanUpdate(POINT screenPoint); // ペンの移動に応じて視点を移動

    void RotateStart();
    void RotateUpdate(POINT currentScreenPoint, POINT startScreenPoint);

    void ZoomStart();
    void ZoomUpdate(POINT currentScreenPoint, POINT startScreenPoint);

    void ResetView(); // 視点をリセット

    // 座標変換などユーティリティ
    void GetTransformMatrix(Matrix *pMatrix); // キャンバスの座標（ワールド座標）からウインドウの座標（スクリーン座標）への変換行列を生成する
    PointF ScreenToWorld(POINT screenPoint);  // スクリーン座標をワールド座標に変換する
    void UpdateClientSize(int width, int height);

    // getter
    float GetZoomFactor() const { return m_zoomFactor; }
    float GetRotationAngle() const { return m_rotationAngle; }
};