#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <string>
#include <cstdint>

// No longer a service, just a utility class with static methods.
class GameManager {
public:
    static uint16_t getHighScore(const std::string& gameName);
    static void updateHighScore(const std::string& gameName, uint16_t newScore);

private:
    static constexpr const char* SCORE_FILE_PATH = "/data/games/scores.json";
};

#endif // GAME_MANAGER_H