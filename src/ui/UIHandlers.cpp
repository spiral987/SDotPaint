#include <windows.h>
#include <CommCtrl.h>

#include "app/globals.h"
#include "ui/UIHandlers.h"
#include "ui/UIManager.h"
#include "core/LayerManager.h"

// レイヤーリストボックスのサブクラスプロシージャ
LRESULT CALLBACK UIHandlers::LayerListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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
LRESULT CALLBACK UIHandlers::EditControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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
        layer_manager->renameLayer(g_pUIManager->GetEditingIndex(), buffer);

        // リストボックスを更新
        g_pUIManager->UpdateLayerList();

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
