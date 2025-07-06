#include "TextInputMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <Arduino.h>

TextInputMenu::TextInputMenu() : 
    onSubmit_(nullptr), 
    maskInput_(false),
    cursorPosition_(0),
    currentLayer_(KeyboardLayer::LOWERCASE),
    focusRow_(0),
    focusCol_(0),
    capsLock_(false)
{
    inputBuffer_[0] = '\0';
}

void TextInputMenu::configure(std::string title, OnSubmitCallback callback, bool maskInput, const char* initial_text) {
    title_ = title;
    onSubmit_ = callback;
    maskInput_ = maskInput;
    strncpy(inputBuffer_, initial_text, sizeof(inputBuffer_) - 1);
    inputBuffer_[sizeof(inputBuffer_) - 1] = '\0';
    cursorPosition_ = strlen(inputBuffer_);
}

void TextInputMenu::onEnter(App* app) {
    // Reset keyboard state upon entering
    currentLayer_ = KeyboardLayer::LOWERCASE;
    capsLock_ = false;
    focusRow_ = 0;
    focusCol_ = 0;
}

void TextInputMenu::onUpdate(App* app) {
    // The main loop handles redrawing, no specific update logic needed here.
}

void TextInputMenu::onExit(App* app) {
    // Clear buffer and callback when leaving
    inputBuffer_[0] = '\0';
    onSubmit_ = nullptr;
}

void TextInputMenu::handleInput(App* app, InputEvent event) {
    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_RIGHT_PRESS:
            moveFocus(0, 1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
            moveFocus(0, -1);
            break;
        case InputEvent::BTN_UP_PRESS:
            moveFocus(-1, 0);
            break;
        case InputEvent::BTN_DOWN_PRESS:
            moveFocus(1, 0);
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                const KeyboardKey& key = KeyboardLayout::getKey(KeyboardLayout::getLayout(currentLayer_), focusRow_, focusCol_);
                if (key.value == KB_KEY_ENTER) {
                    if (onSubmit_) {
                        // Correct: call the callback with the app context and buffer
                        onSubmit_(app, inputBuffer_);
                    }
                } else {
                    processKeyPress(key.value);
                }
            }
            break;
        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;
        default:
            break;
    }
}

void TextInputMenu::draw(App* app, U8G2& display) {
    // If the display passed is the small display, draw the keyboard.
    // Otherwise, draw the input field on the main display.
    if (display.getDisplayHeight() < 40) { // Heuristic to detect small display
        drawKeyboard(display);
    } else {
        // --- Draw Input Field on Main Display ---
        display.setFont(u8g2_font_6x10_tf);
        display.setDrawColor(1);

        display.drawStr((display.getDisplayWidth() - display.getStrWidth(title_.c_str())) / 2, 22, title_.c_str());

        int fieldX = 5, fieldY = 32, fieldW = 118, fieldH = 12;
        display.drawFrame(fieldX, fieldY, fieldW, fieldH);
        
        char displayBuffer[TEXT_INPUT_MAX_LEN + 1];
        if (maskInput_) {
            memset(displayBuffer, '*', strlen(inputBuffer_));
            displayBuffer[strlen(inputBuffer_)] = '\0';
        } else {
            strcpy(displayBuffer, inputBuffer_);
        }

        int textX = fieldX + 3;
        int textY = fieldY + 9;
        display.drawStr(textX, textY, displayBuffer);

        // Draw blinking cursor
        if ((millis() / 500) % 2 == 0) {
            char sub[cursorPosition_ + 1];
            strncpy(sub, displayBuffer, cursorPosition_);
            sub[cursorPosition_] = '\0';
            int cursorX = textX + display.getStrWidth(sub);
            display.drawVLine(cursorX, fieldY + 2, fieldH - 4);
        }

        display.setFont(u8g2_font_5x7_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth("Use aux for keyboard"))/2, 62, "Use aux for keyboard");
    }
}


void TextInputMenu::processKeyPress(int keyValue) {
    if (keyValue > 0 && keyValue < 127) { // Printable ASCII
        if (cursorPosition_ < TEXT_INPUT_MAX_LEN) {
            inputBuffer_[cursorPosition_++] = (char)keyValue;
            inputBuffer_[cursorPosition_] = '\0';
        }
    } else {
        switch (keyValue) {
            case KB_KEY_BACKSPACE:
                if (cursorPosition_ > 0) {
                    inputBuffer_[--cursorPosition_] = '\0';
                }
                break;
            case KB_KEY_SPACE:
                if (cursorPosition_ < TEXT_INPUT_MAX_LEN) {
                    inputBuffer_[cursorPosition_++] = ' ';
                    inputBuffer_[cursorPosition_] = '\0';
                }
                break;
            case KB_KEY_ENTER:
                if (onSubmit_) {
                    // This is where we call back to the system to handle the text.
                    // The App object is not available here, so the callback needs it.
                    // This will be fixed when we implement the calling menu (e.g. WifiListMenu)
                }
                break;
            case KB_KEY_SHIFT:
                if (currentLayer_ == KeyboardLayer::LOWERCASE) currentLayer_ = KeyboardLayer::UPPERCASE;
                else if (currentLayer_ == KeyboardLayer::UPPERCASE) currentLayer_ = KeyboardLayer::LOWERCASE;
                break;
            case KB_KEY_TO_NUM: currentLayer_ = KeyboardLayer::NUMBERS; break;
            case KB_KEY_TO_SYM: currentLayer_ = KeyboardLayer::SYMBOLS; break;
            case KB_KEY_TO_LOWER: currentLayer_ = KeyboardLayer::LOWERCASE; break;
        }
    }
}


void TextInputMenu::moveFocus(int dRow, int dCol) {
    const KeyboardKey* layout = KeyboardLayout::getLayout(currentLayer_);
    
    focusRow_ += dRow;
    if (focusRow_ < 0) focusRow_ = KB_ROWS - 1;
    if (focusRow_ >= KB_ROWS) focusRow_ = 0;

    int attempts = 0;
    int direction = dCol > 0 ? 1 : -1;
    do {
        focusCol_ += direction;
        if (focusCol_ < 0) focusCol_ = KB_LOGICAL_COLS - 1;
        if (focusCol_ >= KB_LOGICAL_COLS) focusCol_ = 0;
        
        if (KeyboardLayout::getKey(layout, focusRow_, focusCol_).colSpan > 0) break;
        attempts++;
    } while (attempts < KB_LOGICAL_COLS);
}


void TextInputMenu::drawKeyboard(U8G2& display) {
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);

    const KeyboardKey* layout = KeyboardLayout::getLayout(currentLayer_);
    const int keyHeight = 8;
    const int keyWidthBase = 12;

    for (int r = 0; r < KB_ROWS; ++r) {
        int currentX = 0;
        for (int c = 0; c < KB_LOGICAL_COLS;) {
            const KeyboardKey& key = KeyboardLayout::getKey(layout, r, c);
            if (key.colSpan == 0) { c++; continue; }

            int keyW = key.colSpan * keyWidthBase + (key.colSpan - 1);
            int keyY = r * keyHeight;

            if (r == focusRow_ && c == focusCol_) {
                display.setDrawColor(1);
                display.drawBox(currentX, keyY, keyW, keyHeight);
                display.setDrawColor(0);
            } else {
                display.setDrawColor(1);
                display.drawFrame(currentX, keyY, keyW, keyHeight);
            }

            char str[2] = {key.displayChar, '\0'};
            display.drawStr(currentX + (keyW - display.getStrWidth(str)) / 2, keyY + 6, str);

            currentX += keyW + 1;
            c += key.colSpan;
        }
    }
    display.sendBuffer();
}