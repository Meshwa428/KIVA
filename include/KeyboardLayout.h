#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#define KB_ROWS 4
#define KB_LOGICAL_COLS 10

// Special Key Identifiers
#define KB_KEY_NONE        0
#define KB_KEY_BACKSPACE   -1
#define KB_KEY_ENTER       -2
#define KB_KEY_SHIFT       -3
#define KB_KEY_SPACE       -5
#define KB_KEY_TO_NUM      -6
#define KB_KEY_TO_SYM      -7
#define KB_KEY_TO_LOWER    -8
#define KB_KEY_CYCLE_LAYOUT -11

enum class KeyboardLayer {
    LOWERCASE, UPPERCASE, NUMBERS, SYMBOLS
};

struct KeyboardKey {
    char displayChar;
    int  value;
    int  colSpan;
};

namespace KeyboardLayout {
    const KeyboardKey* getLayout(KeyboardLayer layer);
    const KeyboardKey& getKey(const KeyboardKey* layout_ptr, int row, int col);
}

#endif // KEYBOARD_LAYOUT_H