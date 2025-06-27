#include "GeminiService.h"
#include <nlohmann/json.hpp> // nlohmann/jsonライブラリを使用
#include <iostream>
#include <cstdlib>
#include <string>

// Windows HTTP API
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// nlohmann/jsonを使いやすくするためにエイリアスを設定
using json = nlohmann::json;

GeminiService::GeminiService(std::string apiKey) : apiKey_(std::move(apiKey)) {}

std::string GeminiService::getApiKeyFromEnv()
{
    const char *apiKey = std::getenv("GEMINI_API_KEY");
    if (apiKey != nullptr)
    {
        return std::string(apiKey);
    }
    return "";
}

std::string GeminiService::generateShapeData(const std::string &prompt)
{
    try
    {
        // Geminiに送信するリクエストボディをJSONで作成
        json requestBody = {
            {"contents", {{"parts", {// プロンプトを書く
                                     {"text", "You are a creative shape designer. Based on the user's request \"" + prompt + "\", please generate a list of polygon vertex coordinates that fit within a 64x64 size in JSON format. The format should be {\"points\": [{\"x\":-1, \"y\":-1}, ...]} and each coordinate should be an integer value from 0 to 63."}}}}}};

        std::string requestBodyStr = requestBody.dump();
        std::string pathWithKey = "/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey_;

        // APIキーを含むパスをワイド文字列に変換
        std::wstring wPathWithKey(pathWithKey.begin(), pathWithKey.end());

        // WinHTTPを使ってHTTPリクエストを送信
        HINTERNET hSession = WinHttpOpen(L"GeminiClient/1.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession)
        {
            return "Error: Failed to initialize WinHTTP session";
        }

        HINTERNET hConnect = WinHttpConnect(hSession, L"generativelanguage.googleapis.com",
                                            INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return "Error: Failed to connect to server";
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                                wPathWithKey.c_str(),
                                                NULL, WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                WINHTTP_FLAG_SECURE);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "Error: Failed to create request";
        }

        // ヘッダーを設定
        std::wstring headers = L"Content-Type: application/json\r\n";
        WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

        // リクエストを送信
        BOOL bResults = WinHttpSendRequest(hRequest,
                                           WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                           (LPVOID)requestBodyStr.c_str(),
                                           requestBodyStr.length(),
                                           requestBodyStr.length(), 0);

        if (!bResults)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "Error: Failed to send request";
        }

        // レスポンスを受信
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        if (!bResults)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "Error: Failed to receive response";
        }

        // HTTPステータスコードを確認
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            NULL, &dwStatusCode, &dwSize, NULL);

        if (dwStatusCode != 200)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "Error: HTTP " + std::to_string(dwStatusCode);
        }

        // レスポンスデータを読み取り
        std::string responseData;
        dwSize = 0; // 既存の変数を再利用
        do
        {
            dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (dwSize > 0)
            {
                std::vector<char> buffer(dwSize + 1);
                DWORD dwDownloaded = 0;
                WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
                buffer[dwDownloaded] = '\0';
                responseData += buffer.data();
            }
        } while (dwSize > 0);

        // クリーンアップ
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        // デバッグ: レスポンスの内容を確認
        if (responseData.empty())
        {
            return "Error: Empty response received";
        }

        // デバッグ出力: レスポンスの内容を表示
        std::cout << "Response received (length: " << responseData.length() << "):" << std::endl;
        std::cout << responseData.substr(0, 500) << std::endl; // 最初の500文字を表示
        std::cout << "--- End of response preview ---" << std::endl;

        // レスポンスの最初の部分をチェック
        if (responseData.length() > 100)
        {
            // レスポンスが長い場合は最初の100文字を確認
            std::string preview = responseData.substr(0, 100);
            if (preview.find("<!DOCTYPE") != std::string::npos ||
                preview.find("<html") != std::string::npos)
            {
                return "Error: Received HTML instead of JSON. Check API key and endpoint.";
            }
        }

        try
        {
            // レスポンスのJSONをパース
            std::cout << "Attempting to parse JSON..." << std::endl;
            json responseJson = json::parse(responseData);
            std::cout << "JSON parsing successful" << std::endl;

            // Geminiが生成したテキスト部分を抽出して返す
            if (responseJson.contains("candidates") &&
                !responseJson["candidates"].empty() &&
                responseJson["candidates"][0].contains("content") &&
                responseJson["candidates"][0]["content"].contains("parts") &&
                !responseJson["candidates"][0]["content"]["parts"].empty())
            {
                return responseJson["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
            }
            else
            {
                std::cout << "Response JSON structure: " << responseJson.dump(2) << std::endl;
                return "Error: Unexpected response format. Response: " + responseData.substr(0, 200);
            }
        }
        catch (const json::parse_error &e)
        {
            std::cout << "JSON parse error at byte " << e.byte << ": " << e.what() << std::endl;
            return "Error: JSON parsing failed at byte " + std::to_string(e.byte) + " - " + std::string(e.what()) + ". Response preview: " + responseData.substr(0, 300);
        }
        catch (const std::exception &e)
        {
            return "Error: " + std::string(e.what());
        }
    }
    catch (const std::exception &e)
    {
        return "Error: " + std::string(e.what());
    }
}
