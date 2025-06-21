#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#define KB_ROWS 4
#define KB_LOGICAL_COLS 10 // Max logical columns for navigation

// Special Key Identifiers
#define KB_KEY_NONE        0
#define KB_KEY_BACKSPACE   -1
#define KB_KEY_ENTER       -2
#define KB_KEY_SHIFT       -3
#define KB_KEY_CAPSLOCK    -4
#define KB_KEY_SPACE       -5
#define KB_KEY_TO_NUM      -6
#define KB_KEY_TO_SYM      -7
#define KB_KEY_TO_LOWER    -8
#define KB_KEY_TO_UPPER    -9
#define KB_KEY_DEL         -10
#define KB_KEY_CYCLE_LAYOUT -11 // <-- ADD THIS DEFINITION

// Keyboard Layers
enum KeyboardLayer {
    KB_LAYER_LOWERCASE,
    KB_LAYER_UPPERCASE,
    KB_LAYER_NUMBERS,
    KB_LAYER_SYMBOLS
};

struct KeyboardKey {
    char displayChar;
    int  value;
    int  colSpan;
};

// Declare the keyboard layout arrays as extern const
// The actual data will be in keyboard_layout.cpp
extern const KeyboardKey keyboard_lowercase[KB_ROWS][KB_LOGICAL_COLS];
extern const KeyboardKey keyboard_uppercase[KB_ROWS][KB_LOGICAL_COLS];
extern const KeyboardKey keyboard_numbers[KB_ROWS][KB_LOGICAL_COLS];
extern const KeyboardKey keyboard_symbols[KB_ROWS][KB_LOGICAL_COLS];

// Function Declarations
const KeyboardKey* getCurrentKeyboardLayout(KeyboardLayer layer);
const KeyboardKey& getKeyFromLayout(const KeyboardKey* layout_ptr, int row, int col);

#endif // KEYBOARD_LAYOUT_H