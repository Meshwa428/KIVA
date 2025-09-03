#ifndef SNAKE_GAME_SPRITES_H
#define SNAKE_GAME_SPRITES_H

#include <U8g2lib.h>

// --- Sprite Dimensions ---
namespace SnakeSpriteSize {
    constexpr int FOOD_W = 4;
    constexpr int FOOD_H = 4;
    constexpr int BODY_W = 4;
    constexpr int BODY_H = 4;
    constexpr int TITLE_W = 56;
    constexpr int TITLE_H = 41;
    constexpr int TITLE_ANIM_W = 52;
    constexpr int TITLE_ANIM_H = 64;
    constexpr int GAMEOVER_W = 116;
    constexpr int GAMEOVER_H = 16;
    constexpr int HIGHSCORE_W = 40;
    constexpr int HIGHSCORE_H = 6;
    constexpr int NEW_HIGHSCORE_W = 61;
    constexpr int NEW_HIGHSCORE_H = 6;
}

// --- Extern Declarations for Sprite Data ---
extern const unsigned char snake_food_bits[];
extern const unsigned char snake_body_bits[];
extern const unsigned char snake_title_bits[];
extern const unsigned char snake_title_anim1_bits[];
extern const unsigned char snake_title_anim2_bits[];
extern const unsigned char snake_gameover_bits[];
extern const unsigned char snake_highscore_bits[];
extern const unsigned char snake_newhighscore_bits[];

#endif // SNAKE_GAME_SPRITES_H