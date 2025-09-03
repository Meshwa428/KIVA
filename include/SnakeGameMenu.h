#ifndef SNAKE_GAME_MENU_H
#define SNAKE_GAME_MENU_H

#include "IMenu.h"

class SnakeGameMenu : public IMenu {
public:
    SnakeGameMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;
    
    // This function will override the default status bar, giving us the full screen.
    bool drawCustomStatusBar(App* app, U8G2& display) override;

    const char* getTitle() const override { return "Snake"; }
    MenuType getMenuType() const override;

private:
    enum class GameState { TITLE_SCREEN, PLAYING, PAUSED, GAME_OVER };
    enum class Direction { UP, LEFT, DOWN, RIGHT };

    // Game Logic Functions
    void resetGame(App* app);
    void updateGame(App* app);
    void createFood();
    bool isFoodOnSnake(int8_t x, int8_t y);
    void checkCollisions(App* app);

    // Drawing Functions
    void drawTitleScreen(App* app, U8G2& display);
    void drawGamePlay(App* app, U8G2& display);
    void drawPausedScreen(App* app, U8G2& display);
    void drawGameOverScreen(App* app, U8G2& display);
    void playHaptic(App* app, int duration_ms);

    // Game Constants
    static constexpr int MAX_SNAKE_LENGTH = 128;
    static constexpr int GRID_SIZE = 4;
    static constexpr int ARENA_WIDTH_UNITS = 128 / GRID_SIZE;
    static constexpr int ARENA_HEIGHT_UNITS = 64 / GRID_SIZE;
    static constexpr int SNAKE_SPEED_MS = 150;

    // Game State Variables
    GameState gameState_;
    Direction direction_;
    Direction lastMovedDirection_;
    
    int8_t snakeLength_;
    int8_t snakeBody_[MAX_SNAKE_LENGTH][2]; // [0] = x, [1] = y in grid units
    
    int8_t foodX_; // in grid units
    int8_t foodY_; // in grid units
    
    uint16_t score_;
    uint16_t highScore_; // Simple high score for the session
    unsigned long lastMoveTime_;
    bool flashToggle_;
};

#endif // SNAKE_GAME_MENU_H