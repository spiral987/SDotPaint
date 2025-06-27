#pragma once
#include <string>

class GeminiService
{
private:
    std::string apiKey_;

public:
    // コンストラクタでAPIキーを受け取る
    explicit GeminiService(std::string apiKey);

    // 環境変数からAPIキーを取得する静的メソッド
    static std::string getApiKeyFromEnv();

    // プロンプトを送信して、形状データをJSON形式の文字列で受け取る
    std::string generateShapeData(const std::string &prompt);
};