#include "GameManager.h"
#include "SdCardManager.h"
#include "Logger.h"
#include <ArduinoJson.h>

constexpr const char* GameManager::SCORE_FILE_PATH;

uint16_t GameManager::getHighScore(const std::string& gameName) {
    // Scores are read from the SD card every time they are requested.
    if (!SdCardManager::getInstance().exists(SCORE_FILE_PATH)) {
        return 0; // If the file doesn't exist, the high score is 0.
    }

    String jsonContent = SdCardManager::getInstance().readFile(SCORE_FILE_PATH);
    JsonDocument scoresDoc; // JsonDocument is created on the stack and freed on exit.
    deserializeJson(scoresDoc, jsonContent);

    return scoresDoc[gameName.c_str()]["high_score"] | 0;
}

void GameManager::updateHighScore(const std::string& gameName, uint16_t newScore) {
    // To update a score, we must first read the entire file.
    JsonDocument scoresDoc;
    if (SdCardManager::getInstance().exists(SCORE_FILE_PATH)) {
        String jsonContent = SdCardManager::getInstance().readFile(SCORE_FILE_PATH);
        deserializeJson(scoresDoc, jsonContent);
    } else {
        // If the file doesn't exist, start with an empty object.
        scoresDoc.to<JsonObject>();
    }

    uint16_t currentHighScore = scoresDoc[gameName.c_str()]["high_score"] | 0;

    if (newScore > currentHighScore) {
        LOG(LogLevel::INFO, "GAME_MGR", "New high score for %s: %u. Saving to SD card.", gameName.c_str(), newScore);
        scoresDoc[gameName.c_str()]["high_score"] = newScore;

        String updatedJson;
        serializeJson(scoresDoc, updatedJson);
        SdCardManager::getInstance().writeFile(SCORE_FILE_PATH, updatedJson.c_str());
    }
}