#include "Event.h"
#include "EventDispatcher.h"
#include "SnakeGameMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "SnakeGameSprites.h"
#include <cstdlib>

// --- Point Constants ---
static const SnakePoint DIR_UP    = SnakePoint( 0, -1);
static const SnakePoint DIR_DOWN  = SnakePoint( 0,  1);
static const SnakePoint DIR_RIGHT = SnakePoint( 1,  0);
static const SnakePoint DIR_LEFT  = SnakePoint(-1,  0);
static const SnakePoint SNAKE_START = SnakePoint(18, 8);

// --- SnakeQueue Implementation ---
SnakeQueue::SnakeQueue() : first_(0), size_(0) {}
void SnakeQueue::reset() { first_ = 0; size_ = 0; }
bool SnakeQueue::isFull() const { return (size_ == MAX_QUEUE_SIZE -1); }
uint8_t SnakeQueue::size() const { return size_; }
const SnakePoint& SnakeQueue::operator[](uint8_t index) const { return items_[(first_ + index) % MAX_QUEUE_SIZE]; }
bool SnakeQueue::contains(const SnakePoint& p) const {
    for (uint8_t i = 0; i < size_; i++) {
        if (items_[(first_ + i) % MAX_QUEUE_SIZE] == p) return true;
    }
    return false;
}
void SnakeQueue::insert(const SnakePoint& p) {
    if (!isFull()) {
        items_[(first_ + size_) % MAX_QUEUE_SIZE] = p;
        size_++;
    }
}
void SnakeQueue::remove() {
    if (size_ > 0) {
        first_ = (first_ + 1) % MAX_QUEUE_SIZE;
        size_--;
    }
}

// --- Snake Implementation ---
void Snake::init() {
    direction = DIR_RIGHT;
    growCounter = 3;
    head = SNAKE_START;
    body.reset();
}
void Snake::move() {
    body.insert(head);
    head += direction;
    if (growCounter > 0) {
        growCounter--;
    } else {
        body.remove();
    }
}
bool Snake::checkFood(const SnakePoint& food) const { return (head == food); }
bool Snake::checkBody() const { return body.contains(head); }
bool Snake::checkBorder() const {
    return (head.x < 1) || (head.x > SnakeGameMenu::ARENA_WIDTH_UNITS - 2) || (head.y < 1) || (head.y > SnakeGameMenu::ARENA_HEIGHT_UNITS - 2);
}
bool Snake::checkOverlap(const SnakePoint& p) const { return body.contains(p) || (head == p); }


// --- SnakeGameMenu Implementation ---
SnakeGameMenu::SnakeGameMenu() : 
    gameState_(GameState::TITLE_SCREEN),
    score_(0), highScore_(0), lastUpdateTime_(0), speedIncreaseTimer_(0),
    currentSpeedMs_(SNAKE_SPEED_START_MS), animCounter_(0), newHighScoreFlag_(false)
{}

bool SnakeGameMenu::drawCustomStatusBar(App* app, U8G2& display) { return true; }
MenuType SnakeGameMenu::getMenuType() const { return MenuType::SNAKE_GAME; }

void SnakeGameMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    gameState_ = GameState::TITLE_SCREEN;
    lastUpdateTime_ = millis();
    animCounter_ = 0;
}
void SnakeGameMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    // Make sure the amplifier is off when we leave the game
    app->getGameAudio().noTone();
}

void SnakeGameMenu::playPressTune(App* app) {
    app->getGameAudio().tone(425, 20);
}

void SnakeGameMenu::playEatTune(App* app) {
    app->getGameAudio().tone(512, 30);
}

void SnakeGameMenu::playDeadTune(App* app) {
    app->getGameAudio().tone(1100, 80);
    delay(100);
    app->getGameAudio().tone(1000, 80);
    delay(100);
    app->getGameAudio().tone(500, 500);
}

void SnakeGameMenu::playWinTune(App* app) {
    app->getGameAudio().tone(500, 50);
    delay(80);
    app->getGameAudio().tone(800, 50);
    delay(80);
    app->getGameAudio().tone(1100, 100);
}

void SnakeGameMenu::initNewGame() {
    snake_.init();
    do {
        food_.x = random(1, ARENA_WIDTH_UNITS - 1);
        food_.y = random(1, ARENA_HEIGHT_UNITS - 1);
    } while (snake_.checkOverlap(food_));

    score_ = 0;
    currentSpeedMs_ = SNAKE_SPEED_START_MS;
    speedIncreaseTimer_ = millis();
    lastUpdateTime_ = millis();
    gameState_ = GameState::PLAYING;
}

void SnakeGameMenu::onUpdate(App* app) {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime_ < 33) return; // ~30 FPS lock
    
    animCounter_++;

    if (gameState_ == GameState::PLAYING) {
        if (currentTime > speedIncreaseTimer_ + SNAKE_SPEED_INCREASE_INTERVAL) {
            if (currentSpeedMs_ > SNAKE_SPEED_END_MS) {
                currentSpeedMs_ -= 10;
            }
            speedIncreaseTimer_ = currentTime;
        }

        if (currentTime - lastUpdateTime_ > currentSpeedMs_) {
            snake_.move();
            lastUpdateTime_ = currentTime;
            if (snake_.checkFood(food_)) {
                snake_.growCounter++;
                score_++;
                do {
                    food_.x = random(1, ARENA_WIDTH_UNITS - 1);
                    food_.y = random(1, ARENA_HEIGHT_UNITS - 1);
                } while (snake_.checkOverlap(food_));
                playEatTune(app);
            }
            if (snake_.checkBody() || snake_.checkBorder()) {
                gameState_ = GameState::GAME_OVER;
                playDeadTune(app);
                if (score_ > highScore_) {
                    highScore_ = score_;
                    newHighScoreFlag_ = true;
                    playWinTune(app);
                } else {
                    newHighScoreFlag_ = false;
                }
            }
        }
    }
}

void SnakeGameMenu::handleInput(InputEvent event, App* app) {
    switch(gameState_) {
        case GameState::TITLE_SCREEN: stateLogicTitle(app, event); break;
        case GameState::PLAYING:      stateLogicPlaying(app, event); break;
        case GameState::GAME_OVER:    stateLogicGameOver(app, event); break;
    }
}

void SnakeGameMenu::stateLogicTitle(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS) {
        EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
    } else {
        playPressTune(app);
        initNewGame();
    }
}

void SnakeGameMenu::stateLogicPlaying(App* app, InputEvent event) {
    if (event == InputEvent::BTN_UP_PRESS && snake_.direction != DIR_DOWN) snake_.direction = DIR_UP;
    else if (event == InputEvent::BTN_DOWN_PRESS && snake_.direction != DIR_UP) snake_.direction = DIR_DOWN;
    else if (event == InputEvent::BTN_LEFT_PRESS && snake_.direction != DIR_RIGHT) snake_.direction = DIR_LEFT;
    else if (event == InputEvent::BTN_RIGHT_PRESS && snake_.direction != DIR_LEFT) snake_.direction = DIR_RIGHT;
    else if (event == InputEvent::BTN_BACK_PRESS) EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
}

void SnakeGameMenu::stateLogicGameOver(App* app, InputEvent event) {
    if (event != InputEvent::NONE) {
        gameState_ = GameState::TITLE_SCREEN;
    }
}

void SnakeGameMenu::draw(App* app, U8G2& display) {
    switch (gameState_) {
        case GameState::TITLE_SCREEN: drawTitleScreen(app, display); break;
        case GameState::PLAYING:      drawGamePlay(app, display); break;
        case GameState::GAME_OVER:    drawGameOverScreen(app, display); break;
    }
}

void SnakeGameMenu::printScore(U8G2& display, int x, int y, uint16_t val) {
    char text[4];
    snprintf(text, sizeof(text), "%03u", val);
    display.drawStr(x, y, text);
}

void SnakeGameMenu::drawTitleScreen(App* app, U8G2& display) {
    display.drawXBM(52, 0, SnakeSpriteSize::TITLE_W, SnakeSpriteSize::TITLE_H, snake_title_bits);
    if ((animCounter_ / 15) % 2 == 0) {
        display.drawXBM(0, 0, SnakeSpriteSize::TITLE_ANIM_W, SnakeSpriteSize::TITLE_ANIM_H, snake_title_anim1_bits);
    } else {
        display.drawXBM(0, 0, SnakeSpriteSize::TITLE_ANIM_W, SnakeSpriteSize::TITLE_ANIM_H, snake_title_anim2_bits);
    }
    display.drawXBM(60, 44, SnakeSpriteSize::HIGHSCORE_W, SnakeSpriteSize::HIGHSCORE_H, snake_highscore_bits);
    display.setFont(u8g2_font_pressstart2p_8u);
    printScore(display, 70, 61, highScore_);
}

void SnakeGameMenu::drawGamePlay(App* app, U8G2& display) {
    display.drawFrame(0, 0, 128, 64);
    display.drawXBM(snake_.head.x * SPRITE_SIZE, snake_.head.y * SPRITE_SIZE, SnakeSpriteSize::BODY_W, SnakeSpriteSize::BODY_H, snake_body_bits);
    for (uint8_t i = 0; i < snake_.body.size(); i++) {
        const SnakePoint& p = snake_.body[i];
        display.drawXBM(p.x * SPRITE_SIZE, p.y * SPRITE_SIZE, SnakeSpriteSize::BODY_W, SnakeSpriteSize::BODY_H, snake_body_bits);
    }
    display.drawXBM(food_.x * SPRITE_SIZE, food_.y * SPRITE_SIZE, SnakeSpriteSize::FOOD_W, SnakeSpriteSize::FOOD_H, snake_food_bits);
}

void SnakeGameMenu::drawGameOverScreen(App* app, U8G2& display) {
    display.drawXBM((128 - SnakeSpriteSize::GAMEOVER_W) / 2, 8, SnakeSpriteSize::GAMEOVER_W, SnakeSpriteSize::GAMEOVER_H, snake_gameover_bits);
    
    display.setFont(u8g2_font_10x20_tr);
    printScore(display, 48, 42, score_);

    if (newHighScoreFlag_ && (animCounter_ / 15) % 2 == 0) {
        display.drawXBM((128 - SnakeSpriteSize::NEW_HIGHSCORE_W) / 2, 56, SnakeSpriteSize::NEW_HIGHSCORE_W, SnakeSpriteSize::NEW_HIGHSCORE_H, snake_newhighscore_bits);
    }
}
