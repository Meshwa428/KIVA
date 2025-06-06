#include "keyboard_layout.h"

// Define your keyboard layouts here
// These are the actual definitions of the arrays.
const KeyboardKey keyboard_lowercase[KB_ROWS][KB_LOGICAL_COLS] = {
    // Row 0
    {{'q', 'q', 1}, {'w', 'w', 1}, {'e', 'e', 1}, {'r', 'r', 1}, {'t', 't', 1}, {'y', 'y', 1}, {'u', 'u', 1}, {'i', 'i', 1}, {'o', 'o', 1}, {'p', 'p', 1}},
    // Row 1
    {{'a', 'a', 1}, {'s', 's', 1}, {'d', 'd', 1}, {'f', 'f', 1}, {'g', 'g', 1}, {'h', 'h', 1}, {'j', 'j', 1}, {'k', 'k', 1}, {'l', 'l', 1}, {'E', KB_KEY_ENTER, 1}}, // Changed displayChar for Enter for clarity
    // Row 2
    {{'S', KB_KEY_SHIFT, 1}, {'z', 'z', 1}, {'x', 'x', 1}, {'c', 'c', 1}, {'v', 'v', 1}, {'b', 'b', 1}, {'n', 'n', 1}, {'m', 'm', 1}, {',', ',', 1}, {'.', '.', 1}},
    // Row 3
    {{'1', KB_KEY_TO_NUM, 1}, {'%', KB_KEY_TO_SYM, 1}, {' ', KB_KEY_SPACE, 4}, {KB_KEY_NONE,KB_KEY_NONE,0}, {KB_KEY_NONE,KB_KEY_NONE,0}, {KB_KEY_NONE,KB_KEY_NONE,0}, {'<', KB_KEY_BACKSPACE, 2}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}}
};

const KeyboardKey keyboard_uppercase[KB_ROWS][KB_LOGICAL_COLS] = {
    {{'Q', 'Q', 1}, {'W', 'W', 1}, {'E', 'E', 1}, {'R', 'R', 1}, {'T', 'T', 1}, {'Y', 'Y', 1}, {'U', 'U', 1}, {'I', 'I', 1}, {'O', 'O', 1}, {'P', 'P', 1}},
    {{'A', 'A', 1}, {'S', 'S', 1}, {'D', 'D', 1}, {'F', 'F', 1}, {'G', 'G', 1}, {'H', 'H', 1}, {'J', 'J', 1}, {'K', 'K', 1}, {'L', 'L', 1}, {'E', KB_KEY_ENTER, 1}},
    {{'s', KB_KEY_SHIFT, 1}, {'Z', 'Z', 1}, {'X', 'X', 1}, {'C', 'C', 1}, {'V', 'V', 1}, {'B', 'B', 1}, {'N', 'N', 1}, {'M', 'M', 1}, {',', ',', 1}, {'.', '.', 1}},
    {{'1', KB_KEY_TO_NUM, 1}, {'%', KB_KEY_TO_SYM, 1}, {' ', KB_KEY_SPACE, 4}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}, {'<', KB_KEY_BACKSPACE, 2}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}}
};

const KeyboardKey keyboard_numbers[KB_ROWS][KB_LOGICAL_COLS] = {
    {{'1', '1', 1}, {'2', '2', 1}, {'3', '3', 1}, {'4', '4', 1}, {'5', '5', 1}, {'6', '6', 1}, {'7', '7', 1}, {'8', '8', 1}, {'9', '9', 1}, {'0', '0', 1}},
    {{'-', '-', 1}, {'/', '/', 1}, {':', ':', 1}, {';', ';', 1}, {'(', '(', 1}, {')', ')', 1}, {'$', '$', 1}, {'&', '&', 1}, {'@', '@', 1}, {'"', '"', 1}},
    {{'.', '.', 1}, {',', ',', 1}, {'?', '?', 1}, {'!', '!', 1}, {'\'', '\'', 1}, {'_', '_', 1}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}, {'E', KB_KEY_ENTER, 1}},
    {{'a', KB_KEY_TO_LOWER, 1}, {'%', KB_KEY_TO_SYM, 1}, {' ', KB_KEY_SPACE, 4}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}, {'<', KB_KEY_BACKSPACE, 2}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}}
};

const KeyboardKey keyboard_symbols[KB_ROWS][KB_LOGICAL_COLS] = {
    {{'[', '[', 1}, {']', ']', 1}, {'{', '{', 1}, {'}', '}', 1}, {'#', '#', 1}, {'%', '%', 1}, {'^', '^', 1}, {'*', '*', 1}, {'+', '+', 1}, {'=', '=', 1}},
    {{'_', '_', 1}, {'\\', '\\', 1}, {'|', '|', 1}, {'~', '~', 1}, {'<', '<', 1}, {'>', '>', 1}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}, {'E', KB_KEY_ENTER, 1}},
    {{'.', '.', 1}, {',', ',', 1}, {'?', '?', 1}, {'!', '!', 1}, {'\'', '\'', 1}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}},
    {{'a', KB_KEY_TO_LOWER, 1}, {'1', KB_KEY_TO_NUM, 1}, {' ', KB_KEY_SPACE, 4}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}, {'<', KB_KEY_BACKSPACE, 2}, {KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0},{KB_KEY_NONE,KB_KEY_NONE,0}}
};

// Function Definitions
const KeyboardKey* getCurrentKeyboardLayout(KeyboardLayer layer) {
    switch (layer) {
        case KB_LAYER_LOWERCASE: return &keyboard_lowercase[0][0];
        case KB_LAYER_UPPERCASE: return &keyboard_uppercase[0][0];
        case KB_LAYER_NUMBERS:   return &keyboard_numbers[0][0];
        case KB_LAYER_SYMBOLS:   return &keyboard_symbols[0][0];
        default:                 return &keyboard_lowercase[0][0]; // Fallback
    }
}

const KeyboardKey& getKeyFromLayout(const KeyboardKey* layout_ptr, int row, int col) {
    // Basic bounds checking (optional, but good practice if row/col could be invalid)
    // if (row < 0 || row >= KB_ROWS || col < 0 || col >= KB_LOGICAL_COLS) {
    //     // Handle error, e.g., return a default key or assert
    //     // For simplicity, assuming valid row/col based on navigation logic
    // }
    return *(layout_ptr + row * KB_LOGICAL_COLS + col);
}