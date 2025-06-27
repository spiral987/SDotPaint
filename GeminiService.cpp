#include "GeminiService.h"
#include <cpr/cpr.h>         // cprライブラリを使用
#include <nlohmann/json.hpp> // nlohmann/jsonライブラリを使用
#include <iostream>
#include <cstdlib>
#include <string>

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
        // Gemini 1.5 Flash APIのエンドポイントURL
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey_;

        // Geminiに送信するリクエストボディをJSONで作成
        json requestBody = {
            {"contents", {{"parts", {// プロンプトを書く
                                     {"text", "You are a creative shape designer. Based on the user's request \"" + prompt + "\", please generate a list of polygon vertex coordinates that fit within a 64x64 size in JSON format. The format should be {\"points\": [{\"x\":-1, \"y\":-1}, ...]} and each coordinate should be an integer value from 0 to 63."}}}}}};

        // cprを使ってHTTP POSTリクエストを送信
        cpr::Response r = cpr::Post(cpr::Url{url},
                                    cpr::Body{requestBody.dump()}, // JSONを文字列に変換してボディに設定
                                    cpr::Header{{"Content-Type", "application/json"}});

        if (r.status_code == 200)
        {
            // 成功した場合、レスポンスのJSONをパース
            json responseJson = json::parse(r.text);

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
                return "Error: Unexpected response format";
            }
        }
        else
        {
            // エラー処理
            return "Error: HTTP " + std::to_string(r.status_code) + " - " + r.text;
        }
    }
    catch (const json::exception &e)
    {
        return "Error: JSON parsing failed - " + std::string(e.what());
    }
    catch (const std::exception &e)
    {
        return "Error: " + std::string(e.what());
    }
}