// Windows APIを使用するために必要なヘッダファイル
#include <windows.h>

#include <string>

// ウィンドウプロシージャのプロトタイプ宣言
// この関数がウィンドウへの様々なメッセージ（イベント）を処理します
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// main関数の代わりに使用されるWinMain関数
int WINAPI WinMain(
    HINSTANCE hInstance,     // プログラムを識別するためのインスタンスハンドル
    HINSTANCE hPrevInstance, // 常にNULL（古いWindowsの名残）
    LPSTR lpCmdLine,         // コマンドライン引数
    int nCmdShow)            // ウィンドウの表示状態
{
    // 1. ウィンドウクラスの設計と登録
    // ----------------------------------------------------------------

    const wchar_t CLASS_NAME[] = L"Sample Window Class"; // ウィンドウクラス名

    WNDCLASSEXW wc = {}; //! WNDCLASSEX構造体をゼロで初期化　これはウインドウの設計図になる

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW; // ウィンドウサイズ変更時に再描画
    wc.lpfnWndProc = WindowProc;        //! メッセージを処理する関数（ウィンドウプロシージャ）を指定
    wc.hInstance = hInstance;           // このプログラムのインスタンスハンドル
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);      // デフォルトの矢印カーソル
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // ウィンドウの背景色

    // 設計したウィンドウクラスをOSに登録
    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"ウィンドウクラスの登録に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // 2. ウィンドウの生成
    // ----------------------------------------------------------------

    HWND hwnd = CreateWindowExW(
        0,                       // 拡張ウィンドウスタイル
        CLASS_NAME,              // 登録したウィンドウクラス名
        L"My Paint Application", // ウィンドウのタイトル
        WS_OVERLAPPEDWINDOW,     // 最も一般的なウィンドウスタイル

        // 位置とサイズ (CW_USEDEFAULTでOSに任せる)
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,

        NULL,      // 親ウィンドウハンドル
        NULL,      // メニューハンドル
        hInstance, // このプログラムのインスタンスハンドル
        NULL       // 追加のアプリケーションデータ
    );

    if (hwnd == NULL)
    {
        MessageBoxW(NULL, L"ウィンドウの生成に失敗しました！", L"エラー", MB_ICONERROR);
        return 0;
    }

    // 作成したウィンドウを表示
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 3. メッセージループ（ウインドウがメッセージの発生を監視するためのループ）
    // ----------------------------------------------------------------

    MSG msg = {};
    // アプリケーションが終了するまでメッセージを取得し続ける
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg); // キーボード入力を文字メッセージに変換
        DispatchMessage(&msg);  // メッセージを適切なウィンドウプロシージャに送る
    }

    return (int)msg.wParam; // メッセージループが終了したときの戻り値
}

// ウィンドウプロシージャ（メッセージが発せられたときに呼び出される関数）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // マウスの位置を保持するための変数。static変数を使用して、ウィンドウプロシージャが呼び出されるたびに初期化されないようにする
    static POINT ptMouse = {-1, -1}; // マウスの位置を無効な座標で初期化(一度もクリックされていない状態を示す)

    switch (uMsg)
    {
    // 　マウスの左クリックが押されたときのメッセージ
    case WM_LBUTTONDOWN:
    {
        // lParamからマウスの位置を取得
        ptMouse.x = LOWORD(lParam); // lParamの下位ワードからX座標を取得
        ptMouse.y = HIWORD(lParam); // lParamの上位ワードからY座標を取得

        // ウインドウ全体を無効化し、再描画する
        InvalidateRect(hwnd, NULL, TRUE); // ウィンドウ全体を再描画するように要求

        return 0; // メッセージを処理したことを示す
    }

    // ウィンドウが破棄されるときのメッセージ
    case WM_DESTROY:
        PostQuitMessage(0); // メッセージループを終了させる
        return 0;

    // ウィンドウを描画する必要があるときのメッセージ
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 保存された座標が有効な場合（マウスがクリックされた場合）
        if (ptMouse.x >= 0 && ptMouse.y >= 0)
        {
            // その座標に点を描画する
            SetPixel(hdc, ptMouse.x, ptMouse.y, RGB(0, 0, 0)); // 黒色の点を描画
        }

        EndPaint(hwnd, &ps);
    }
        return 0;
    }

    // 自分で処理しないメッセージは、デフォルトの処理に任せる（非常に重要）
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}