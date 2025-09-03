#include "SnakeGameMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <cstdlib> // For random()

SnakeGameMenu::SnakeGameMenu() : 
    gameState_(GameState::TITLE_SCREEN),
    direction_(Direction::RIGHT),
    lastMovedDirection_(Direction::RIGHT),
    snakeLength_(0),
    foodX_(0),
    foodY_(0),
    score_(0),
    highScore_(0),
    lastMoveTime_(0),
    flashToggle_(false)
{}

// This must return true to hide the default status bar
bool SnakeGameMenu::drawCustomStatusBar(App* app, U8G2& display) {
    return true; 
}

MenuType SnakeGameMenu::getMenuType() const {
    // NOTE: This was added to Config.h in the previous step
    return MenuType::SNAKE_GAME;
}

void SnakeGameMenu::onEnter(App* app, bool isForwardNav) {
    // Every time we enter the game "app", we start at the title screen.
    gameState_ = GameState::TITLE_SCREEN;
    lastMoveTime_ = millis(); // For title screen flashing
}

void SnakeGameMenu::onExit(App* app) {
    // No cleanup needed
}

void SnakeGameMenu::playHaptic(App* app, int duration_ms) {
    app->getHardwareManager().setVibration(true);
    delay(duration_ms);
    app->getHardwareManager().setVibration(false);
}

void SnakeGameMenu::resetGame(App* app) {
    snakeLength_ = 3;
    // Initial snake position (horizontal, middle of the screen)
    snakeBody_[0][0] = 5; snakeBody_[0][1] = ARENA_HEIGHT_UNITS / 2;
    snakeBody_[1][0] = 6; snakeBody_[1][1] = ARENA_HEIGHT_UNITS / 2;
    snakeBody_[2][0] = 7; snakeBody_[2][1] = ARENA_HEIGHT_UNITS / 2;

    direction_ = Direction::RIGHT;
    lastMovedDirection_ = Direction::RIGHT;

    createFood();
    score_ = 0;
    lastMoveTime_ = millis();
    gameState_ = GameState::PLAYING;
    playHaptic(app, 300);
}

void SnakeGameMenu::onUpdate(App* app) {
    unsigned long currentTime = millis();
    
    if (gameState_ == GameState::TITLE_SCREEN) {
        if (currentTime - lastMoveTime_ > 400) {
            flashToggle_ = !flashToggle_;
            lastMoveTime_ = currentTime;
        }
    }
    
    if (gameState_ == GameState::PLAYING) {
        if (currentTime - lastMoveTime_ > SNAKE_SPEED_MS) {
            updateGame(app);
            lastMoveTime_ = currentTime;
        }
    }
}

void SnakeGameMenu::updateGame(App* app) {
    // Move the body
    for (int i = 0; i < snakeLength_ - 1; i++) {
        snakeBody_[i][0] = snakeBody_[i+1][0];
        snakeBody_[i][1] = snakeBody_[i+1][1];
    }

    // Move the head
    int8_t* head = snakeBody_[snakeLength_ - 1];
    switch (direction_) {
        case Direction::UP:    head[1]--; break;
        case Direction::DOWN:  head[1]++; break;
        case Direction::LEFT:  head[0]--; break;
        case Direction::RIGHT: head[0]++; break;
    }
    lastMovedDirection_ = direction_;

    // Check for collisions
    checkCollisions(app);
    if (gameState_ != GameState::PLAYING) return; // checkCollisions might change the state

    // Check for food
    if (head[0] == foodX_ && head[1] == foodY_) {
        playHaptic(app, 100);
        score_ += 10;
        if (snakeLength_ < MAX_SNAKE_LENGTH) {
            snakeLength_++;
            // The new head is already at the food position.
            // We just need to copy the new last element from the previous one
            // to make it "grow" correctly on the next frame.
            snakeBody_[snakeLength_ - 1][0] = snakeBody_[snakeLength_ - 2][0];
            snakeBody_[snakeLength_ - 1][1] = snakeBody_[snakeLength_ - 2][1];
        }
        createFood();
    }
}

void SnakeGameMenu::checkCollisions(App* app) {
    const int8_t* head = snakeBody_[snakeLength_ - 1];

    // Wall collision
    if (head[0] < 0 || head[0] >= ARENA_WIDTH_UNITS ||
        head[1] < 0 || head[1] >= ARENA_HEIGHT_UNITS) {
        gameState_ = GameState::GAME_OVER;
        playHaptic(app, 300);
        if (score_ > highScore_) {
            highScore_ = score_;
        }
        return;
    }

    // Self collision
    for (int i = 0; i < snakeLength_ - 1; i++) {
        if (head[0] == snakeBody_[i][0] && head[1] == snakeBody_[i][1]) {
            gameState_ = GameState::GAME_OVER;
            playHaptic(app, 300);
            if (score_ > highScore_) {
                highScore_ = score_;
            }
            return;
        }
    }
}

void SnakeGameMenu::createFood() {
    do {
        foodX_ = random(0, ARENA_WIDTH_UNITS);
        foodY_ = random(0, ARENA_HEIGHT_UNITS);
    } while (isFoodOnSnake(foodX_, foodY_));
}

bool SnakeGameMenu::isFoodOnSnake(int8_t x, int8_t y) {
    for (int i = 0; i < snakeLength_; i++) {
        if (snakeBody_[i][0] == x && snakeBody_[i][1] == y) {
            return true;
        }
    }
    return false;
}

void SnakeGameMenu::handleInput(App* app, InputEvent event) {
    switch (gameState_) {
        case GameState::TITLE_SCREEN:
            if (event != InputEvent::NONE) {
                resetGame(app);
            }
            break;

        case GameState::PLAYING:
            switch(event) {
                case InputEvent::BTN_UP_PRESS:
                    if (lastMovedDirection_ != Direction::DOWN) direction_ = Direction::UP;
                    break;
                case InputEvent::BTN_DOWN_PRESS:
                    if (lastMovedDirection_ != Direction::UP) direction_ = Direction::DOWN;
                    break;
                case InputEvent::BTN_LEFT_PRESS:
                    if (lastMovedDirection_ != Direction::RIGHT) direction_ = Direction::LEFT;
                    break;
                case InputEvent::BTN_RIGHT_PRESS:
                    if (lastMovedDirection_ != Direction::LEFT) direction_ = Direction::RIGHT;
                    break;
                case InputEvent::BTN_OK_PRESS:
                case InputEvent::BTN_ENCODER_PRESS:
                    gameState_ = GameState::PAUSED;
                    break;
                case InputEvent::BTN_BACK_PRESS:
                    app->changeMenu(MenuType::BACK);
                    break;
                default: break;
            }
            break;

        case GameState::PAUSED:
            if (event == InputEvent::BTN_OK_PRESS || event == InputEvent::BTN_ENCODER_PRESS) {
                gameState_ = GameState::PLAYING;
                lastMoveTime_ = millis(); // Prevent jump after unpausing
            } else if (event == InputEvent::BTN_BACK_PRESS) {
                app->changeMenu(MenuType::BACK);
            }
            break;
            
        case GameState::GAME_OVER:
            if (event != InputEvent::NONE) {
                gameState_ = GameState::TITLE_SCREEN;
            }
            break;
    }
}

void SnakeGameMenu::draw(App* app, U8G2& display) {
    switch (gameState_) {
        case GameState::TITLE_SCREEN:
            drawTitleScreen(app, display);
            break;
        case GameState::PLAYING:
            drawGamePlay(app, display);
            break;
        case GameState::PAUSED:
            drawGamePlay(app, display); // Draw the game state behind the pause message
            drawPausedScreen(app, display);
            break;
        case GameState::GAME_OVER:
            drawGamePlay(app, display); // Draw final state
            drawGameOverScreen(app, display);
            break;
    }
}

void SnakeGameMenu::drawTitleScreen(App* app, U8G2& display) {
    display.setFont(u8g2_font_9x15B_tr);
    const char* title = "SNAKE";
    display.drawStr((128 - display.getStrWidth(title)) / 2, 24, title);

    display.setFont(u8g2_font_6x10_tf);
    if(flashToggle_) {
        const char* prompt = "Press any key";
        display.drawStr((128 - display.getStrWidth(prompt)) / 2, 45, prompt);
    }

    char scoreStr[20];
    snprintf(scoreStr, sizeof(scoreStr), "HI: %u", highScore_);
    display.drawStr((128 - display.getStrWidth(scoreStr)) / 2, 60, scoreStr);
}

void SnakeGameMenu::drawGamePlay(App* app, U8G2& display) {
    // Draw snake
    for (int i = 0; i < snakeLength_; i++) {
        // --- MODIFICATION: Draw the box 1 pixel smaller to create a border effect ---
        display.drawBox(snakeBody_[i][0] * GRID_SIZE, snakeBody_[i][1] * GRID_SIZE, GRID_SIZE - 1, GRID_SIZE - 1);
    }

    // Draw food
    // --- MODIFICATION: Draw the food 1 pixel smaller as well ---
    display.drawBox(foodX_ * GRID_SIZE, foodY_ * GRID_SIZE, GRID_SIZE - 1, GRID_SIZE - 1);

    // Draw score
    char scoreStr[10];
    snprintf(scoreStr, sizeof(scoreStr), "%u", score_);
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(2, display.getAscent(), scoreStr);
}

void SnakeGameMenu::drawPausedScreen(App* app, U8G2& display) {
    display.setDrawColor(2); // XOR mode
    display.drawBox(0, 0, 128, 64);
    display.setDrawColor(1);

    display.setFont(u8g2_font_9x15B_tr);
    const char* text = "PAUSED";
    display.drawStr((128 - display.getStrWidth(text)) / 2, 36, text);
}

void SnakeGameMenu::drawGameOverScreen(App* app, U8G2& display) {
    // --- MODIFICATION: Invert screen and draw black text ---
    // 1. Draw a white box over the whole screen
    display.setDrawColor(1);
    display.drawBox(0, 0, 128, 64);

    // 2. Set the draw color to black (0) for the text
    display.setDrawColor(0);

    // 3. Draw the "GAME OVER" text
    display.setFont(u8g2_font_9x15B_tr);
    const char* text = "GAME OVER";
    display.drawStr((128 - display.getStrWidth(text)) / 2, 28, text);

    // 4. Draw the score text
    char scoreStr[20];
    snprintf(scoreStr, sizeof(scoreStr), "Score: %u", score_);
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr((128 - display.getStrWidth(scoreStr)) / 2, 45, scoreStr);
    
    // 5. Reset draw color back to white for the next frame
    display.setDrawColor(1);
}