#define _USE_MATH_DEFINES
#include <windows.h>
#include <cmath>
#include "view/ViewManager.h"

ViewManager::ViewManager(int clientWidth, int clientHeight)
    : m_clientWidth(clientWidth),
      m_clientHeight(clientHeight),
      m_zoomFactor(1.0f),
      m_rotationAngle(0.0f)

{
    // ビューの中心をクライアント領域の中心に初期化
    m_viewCenter.X = static_cast<float>(clientWidth) / 2.0f;
    m_viewCenter.Y = static_cast<float>(clientHeight) / 2.0f;
}

void ViewManager::PanStart(POINT screenPoint)
{
    m_panLastPoint = screenPoint;
}

void ViewManager::PanUpdate(POINT screenPoint)
{
    // スクリーン座標での移動量
    float dx = static_cast<float>(screenPoint.x - m_panLastPoint.x);
    float dy = static_cast<float>(screenPoint.y - m_panLastPoint.y);

    // 現在の回転角度を打ち消す
    float angleRad = -m_rotationAngle * (M_PI / 180.0f);
    float rotatedDx = dx * cosf(angleRad) - dy * sinf(angleRad);
    float rotatedDy = dx * sinf(angleRad) + dy * cosf(angleRad);

    // ズーム率を考慮して移動量をスケーリング
    if (m_zoomFactor > 0.0f)
    {
        rotatedDx /= m_zoomFactor;
        rotatedDy /= m_zoomFactor;
    }

    // ビューの中心を更新
    m_viewCenter.X -= rotatedDx;
    m_viewCenter.Y -= rotatedDy;

    // 現在の点を次の計算のために保存
    m_panLastPoint = screenPoint;
}

void ViewManager::RotateStart()
{
    m_startRotationAngle = m_rotationAngle;
}

void ViewManager::RotateUpdate(POINT currentScreenPoint, POINT startScreenPoint)
{
    PointF center = {static_cast<float>(m_clientWidth) / 2.0f, static_cast<float>(m_clientHeight) / 2.0f};

    // 開始点と中心との角度
    float startDx = static_cast<float>(startScreenPoint.x) - center.X;
    float startDy = static_cast<float>(startScreenPoint.y) - center.Y;
    float startAngle = atan2f(startDy, startDx);

    // 現在地点と中心との角度
    float currentDx = static_cast<float>(currentScreenPoint.x) - center.X;
    float currentDy = static_cast<float>(currentScreenPoint.y) - center.Y;
    float currentAngle = atan2f(currentDy, currentDx);

    // 角度の差分を計算し、総回転角度に加える（ラジアンから度に変換）
    float deltaAngle = currentAngle - startAngle;
    m_rotationAngle = m_startRotationAngle + (deltaAngle * (180.0f / M_PI));
}

void ViewManager::ZoomStart()
{
    m_startZoomFactor = m_zoomFactor;
    m_startViewCenter = m_viewCenter;
}

void ViewManager::ZoomUpdate(POINT currentScreenPoint, POINT startScreenPoint)
{
    // 開始点からのX軸方向の移動量でズーム率を計算
    float totalDeltaX = static_cast<float>(currentScreenPoint.x - startScreenPoint.x);

    // 指数関数的にズーム率を変化させる
    // 0.005は操作の感度調整用の係数
    float newZoomFactor = m_startZoomFactor * expf(totalDeltaX * 0.005f);

    // ズーム率に上限と下限を設ける
    if (newZoomFactor < 0.1f)
        newZoomFactor = 0.1f;
    if (newZoomFactor > 10.0f)
        newZoomFactor = 10.0f;

    // ズームの中心点（操作開始点）のワールド座標を計算
    PointF zoomCenterWorld = ScreenToWorld(startScreenPoint);

    // ズーム基点がズレないようにビューの中心を補正
    if (newZoomFactor > 0.0f)
    {
        m_viewCenter.X = zoomCenterWorld.X + (m_viewCenter.X - zoomCenterWorld.X) * (m_zoomFactor / newZoomFactor);
        m_viewCenter.Y = zoomCenterWorld.Y + (m_viewCenter.Y - zoomCenterWorld.Y) * (m_zoomFactor / newZoomFactor);
    }

    m_zoomFactor = newZoomFactor;
}

void ViewManager::ResetView()
{
    m_viewCenter.X = static_cast<float>(m_clientWidth) / 2.0f;
    m_viewCenter.Y = static_cast<float>(m_clientHeight) / 2.0f;
    m_zoomFactor = 1.0f;
    m_rotationAngle = 0.0f;
}

// ワールド座標 → [パン] → [ズーム] → [回転] → [画面配置] → スクリーン座標
void ViewManager::GetTransformMatrix(Matrix *pMatrix)
{
    if (!pMatrix)
    {
        return;
    } // 安全のためのNULLチェック

    pMatrix->Reset(); // まず単位行列にリセット

    float centerX = static_cast<float>(m_clientWidth) / 2.0f;
    float centerY = static_cast<float>(m_clientHeight) / 2.0f;

    // GDI+の変換は、最後に適用したいものから順に呼び出すため、実際は下のTranslateからScaleと順番に実行
    pMatrix->Translate(centerX, centerY);                 // 画面中央に移動
    pMatrix->Rotate(m_rotationAngle);                     // 原点を中心に回転
    pMatrix->Scale(m_zoomFactor, m_zoomFactor);           // 原点を中心にスケーリング
    pMatrix->Translate(-m_viewCenter.X, -m_viewCenter.Y); // ビュー中心を原点に移動
}

// スクリーン座標 → [画面配置逆] → [回転逆] → [ズーム逆] → [パン逆] → ワールド座標
PointF ViewManager::ScreenToWorld(POINT screenPoint)
{
    Matrix transformMatrix;
    this->GetTransformMatrix(&transformMatrix);

    transformMatrix.Invert(); // スクリーン→ワールドは逆行列を使う

    PointF worldPoint = {(float)screenPoint.x, (float)screenPoint.y};
    transformMatrix.TransformPoints(&worldPoint, 1);

    return worldPoint;
}

void ViewManager::UpdateClientSize(int width, int height)
{
    m_clientWidth = width;
    m_clientHeight = height;
}
