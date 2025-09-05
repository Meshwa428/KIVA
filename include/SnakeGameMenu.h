#ifndef SNAKE_GAME_MENU_H
#define SNAKE_GAME_MENU_H

#include "IMenu.h"

// --- Data Structures based on arduboy-misssnake ---

// A simple (x,y) point structure
struct SnakePoint {
    int8_t x;
    int8_t y;
    SnakePoint(int8_t x_ = 0, int8_t y_ = 0) : x(x_), y(y_) {}

    SnakePoint& operator+=(const SnakePoint& p) {
        x += p.x;
        y += p.y;
        return *this;
    }
    bool operator==(const SnakePoint& p) const {
        return (x == p.x) && (y == p.y);
    }
};

// A circular buffer queue for the snake's body
class SnakeQueue {
public:
    SnakeQueue();
    void reset();
    bool isFull() const;
    bool contains(const SnakePoint& p) const;
    void insert(const SnakePoint& p);
    void remove();
    const SnakePoint& operator[](uint8_t index) const;
    uint8_t size() const;
private:
    static constexpr int MAX_QUEUE_SIZE = 255;
    SnakePoint items_[MAX_QUEUE_SIZE];
    uint8_t first_;
    uint8_t size_;
};

// Main snake object
struct Snake {
    SnakePoint head;
    SnakeQueue body;
    SnakePoint direction;
    uint8_t growCounter;
    void init();
    void move();
    bool checkFood(const SnakePoint& food) const;
    bool checkBody() const;
    bool checkBorder() const;
    bool checkOverlap(const SnakePoint& p) const;
};


class SnakeGameMenu : public IMenu {
public:
    SnakeGameMenu();

    // Game Constants
    static constexpr int SPRITE_SIZE = 4;
    static constexpr int ARENA_WIDTH_UNITS = 128 / SPRITE_SIZE;
    static constexpr int ARENA_HEIGHT_UNITS = 64 / SPRITE_SIZE;
    static constexpr int SNAKE_SPEED_START_MS = 166; // approx 6 FPS
    static constexpr int SNAKE_SPEED_END_MS = 83;    // approx 12 FPS
    static constexpr int SNAKE_SPEED_INCREASE_INTERVAL = 30000; // Speed up every 30s

    // IMenu Interface Overrides
    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;
    bool drawCustomStatusBar(App* app, U8G2& display) override;
    const char* getTitle() const override { return "Snake"; }
    MenuType getMenuType() const override;

private:
    enum class GameState { TITLE_SCREEN, PLAYING, GAME_OVER };
    
    // --- Game Logic Functions (Cleaned Up) ---
    void stateLogicTitle(App* app, InputEvent event);
    void stateLogicPlaying(App* app, InputEvent event);
    void stateLogicGameOver(App* app, InputEvent event);
    void initNewGame();
    void updateGame(App* app);
    void createFood();
    bool isFoodOnSnake(int8_t x, int8_t y);
    void checkCollisions(App* app);
    
    // --- Audio Functions ---
    void playPressTune(App* app);
    void playEatTune(App* app);
    void playDeadTune(App* app);
    void playWinTune(App* app);

    // --- Drawing Functions ---
    void drawTitleScreen(App* app, U8G2& display);
    void drawGamePlay(App* app, U8G2& display);
    void drawGameOverScreen(App* app, U8G2& display);
    void printScore(U8G2& display, int x, int y, uint16_t val);

    // --- Game Objects & State Variables ---
    GameState gameState_;
    Snake snake_;
    SnakePoint food_;
    uint16_t score_;
    uint16_t highScore_;
    unsigned long lastUpdateTime_;
    unsigned long speedIncreaseTimer_;
    int currentSpeedMs_;
    uint8_t animCounter_;
    bool newHighScoreFlag_;
};

#endif // SNAKE_GAME_MENU_H