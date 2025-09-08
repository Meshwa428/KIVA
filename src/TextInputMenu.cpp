#include "Event.h"
#include "EventDispatcher.h"
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
    
    memset(inputBuffer_, 0, sizeof(inputBuffer_));
    if (initial_text) {
        strncpy(inputBuffer_, initial_text, sizeof(inputBuffer_) - 1);
    }
    cursorPosition_ = strlen(inputBuffer_);
}

void TextInputMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    currentLayer_ = KeyboardLayer::LOWERCASE;
    capsLock_ = false;
    focusRow_ = 0;
    focusCol_ = 0;
}

void TextInputMenu::onUpdate(App* app) {}

void TextInputMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    memset(inputBuffer_, 0, sizeof(inputBuffer_));
    onSubmit_ = nullptr;
}

void TextInputMenu::handleInput(InputEvent event, App* app) {
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
        case InputEvent::BTN_A_PRESS: 
            processKeyPress(KB_KEY_SHIFT);
            break;
        case InputEvent::BTN_B_PRESS: 
            processKeyPress(KB_KEY_CYCLE_LAYOUT);
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                const KeyboardKey& key = KeyboardLayout::getKey(KeyboardLayout::getLayout(currentLayer_), focusRow_, focusCol_);
                
                if (key.value == KB_KEY_ENTER) {
                    if (onSubmit_) {
                        OnSubmitCallback callback_copy = onSubmit_;
                        callback_copy(app, inputBuffer_);
                    }
                } else {
                    processKeyPress(key.value);
                }
            }
            break;
        case InputEvent::BTN_BACK_PRESS:
            {
                EventDispatcher::getInstance().publish(NavigateBackEvent());
            }
            break;
        default:
            break;
    }
}

void TextInputMenu::processKeyPress(int keyValue) {
    if (keyValue > 0 && keyValue < 127) { // Printable ASCII
        if (cursorPosition_ < TEXT_INPUT_MAX_LEN) {
            memmove(&inputBuffer_[cursorPosition_ + 1], &inputBuffer_[cursorPosition_], TEXT_INPUT_MAX_LEN - cursorPosition_ - 1);
            inputBuffer_[cursorPosition_] = (char)keyValue;
            cursorPosition_++;
            inputBuffer_[TEXT_INPUT_MAX_LEN] = '\0';
        }
    } else {
        switch (keyValue) {
            case KB_KEY_BACKSPACE:
                if (cursorPosition_ > 0) {
                    memmove(&inputBuffer_[cursorPosition_ - 1], &inputBuffer_[cursorPosition_], TEXT_INPUT_MAX_LEN - cursorPosition_);
                    cursorPosition_--;
                }
                break;
            case KB_KEY_SPACE:
                 if (cursorPosition_ < TEXT_INPUT_MAX_LEN) {
                    memmove(&inputBuffer_[cursorPosition_ + 1], &inputBuffer_[cursorPosition_], TEXT_INPUT_MAX_LEN - cursorPosition_ - 1);
                    inputBuffer_[cursorPosition_] = ' ';
                    cursorPosition_++;
                    inputBuffer_[TEXT_INPUT_MAX_LEN] = '\0';
                 }
                break;
            case KB_KEY_SHIFT:
                if (currentLayer_ == KeyboardLayer::LOWERCASE) currentLayer_ = KeyboardLayer::UPPERCASE;
                else if (currentLayer_ == KeyboardLayer::UPPERCASE) currentLayer_ = KeyboardLayer::LOWERCASE;
                break;
            case KB_KEY_CYCLE_LAYOUT: 
                if (currentLayer_ == KeyboardLayer::LOWERCASE || currentLayer_ == KeyboardLayer::UPPERCASE) {
                    currentLayer_ = KeyboardLayer::NUMBERS;
                } else if (currentLayer_ == KeyboardLayer::NUMBERS) {
                    currentLayer_ = KeyboardLayer::SYMBOLS;
                } else if (currentLayer_ == KeyboardLayer::SYMBOLS) {
                    currentLayer_ = KeyboardLayer::LOWERCASE;
                }
                capsLock_ = false;
                focusRow_ = 0;
                focusCol_ = 0;
                break;
            case KB_KEY_TO_NUM: currentLayer_ = KeyboardLayer::NUMBERS; break;
            case KB_KEY_TO_SYM: currentLayer_ = KeyboardLayer::SYMBOLS; break;
            case KB_KEY_TO_LOWER: currentLayer_ = KeyboardLayer::LOWERCASE; break;
        }
    }
}


void TextInputMenu::moveFocus(int dRow, int dCol) {
    const KeyboardKey* layout = KeyboardLayout::getLayout(currentLayer_);
    int oldRow = focusRow_;
    int oldCol = focusCol_;

    int newRow = focusRow_ + dRow;
    newRow = (newRow + KB_ROWS) % KB_ROWS; 

    int newCol = focusCol_ + dCol;
    
    if (dCol != 0) {
        int direction = (dCol > 0) ? 1 : -1;
        int attempts = 0;
        do {
            newCol = (focusCol_ + direction + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
            if (direction > 0) {
                const KeyboardKey& currentKey = KeyboardLayout::getKey(layout, focusRow_, focusCol_);
                if(currentKey.colSpan > 1) {
                    newCol = (focusCol_ + currentKey.colSpan) % KB_LOGICAL_COLS;
                }
            }
            focusCol_ = newCol;
            if (KeyboardLayout::getKey(layout, focusRow_, focusCol_).colSpan > 0) break;
            attempts++;
        } while (attempts < KB_LOGICAL_COLS);
    }
    
    if (dRow != 0) {
        focusRow_ = newRow;
        const KeyboardKey& sourceKey = KeyboardLayout::getKey(layout, oldRow, oldCol);
        int sourceKeyCenter = oldCol * 12 + (sourceKey.colSpan * 12) / 2;
        int bestMatchCol = 0;
        int smallestDist = 1000;

        for (int c = 0; c < KB_LOGICAL_COLS; ) {
            const KeyboardKey& targetKey = KeyboardLayout::getKey(layout, focusRow_, c);
            if(targetKey.colSpan > 0) {
                int targetKeyCenter = c * 12 + (targetKey.colSpan * 12) / 2;
                if(abs(targetKeyCenter - sourceKeyCenter) < smallestDist) {
                    smallestDist = abs(targetKeyCenter - sourceKeyCenter);
                    bestMatchCol = c;
                }
                c += targetKey.colSpan;
            } else {
                c++;
            }
        }
        focusCol_ = bestMatchCol;
    }
}

void TextInputMenu::draw(App* app, U8G2& display) {
    if (display.getDisplayHeight() < 40) {
        drawKeyboard(display);
    } else {
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

void TextInputMenu::drawKeyboard(U8G2& display) {
    display.clearBuffer();
    display.setFont(u8g2_font_5x7_tf);

    const KeyboardKey* layout = KeyboardLayout::getLayout(currentLayer_);
    const int keyHeight = 8;
    const int keyWidthBase = 12;
    const int interKeySpacingX = 1;

    for (int r = 0; r < KB_ROWS; ++r) {
        int currentX = 0;
        int keyY = r * keyHeight;
        for (int c = 0; c < KB_LOGICAL_COLS;) {
            const KeyboardKey& key = KeyboardLayout::getKey(layout, r, c);
            if (key.colSpan == 0) { c++; continue; }
            
            int keyW = key.colSpan * keyWidthBase + (key.colSpan - 1) * interKeySpacingX;
            if (currentX + keyW > display.getDisplayWidth()) {
                keyW = display.getDisplayWidth() - currentX;
            }

            if (r == focusRow_ && c == focusCol_) {
                display.setDrawColor(1);
                display.drawBox(currentX, keyY, keyW, keyHeight);
                display.setDrawColor(0);
            } else {
                display.setDrawColor(1);
                display.drawFrame(currentX, keyY, keyW, keyHeight);
            }

            char str[2] = {key.displayChar, '\0'};
            if (key.value == KB_KEY_ENTER) str[0] = 'E';
            else if (key.value == KB_KEY_BACKSPACE) str[0] = '<';
            else if (key.value == KB_KEY_SHIFT) str[0] = capsLock_ ? 's' : 'S'; 
            else if (key.value == KB_KEY_TO_NUM) str[0] = '1';
            else if (key.value == KB_KEY_TO_SYM) str[0] = '%';
            else if (key.value == KB_KEY_TO_LOWER) str[0] = 'a';
            
            if(str[0] != ' ' && str[0] != '\0'){
                 int charWidth = display.getStrWidth(str);
                 display.drawStr(currentX + (keyW - charWidth) / 2, keyY + 6, str);
            }
           
            currentX += keyW + interKeySpacingX;
            c += key.colSpan;
        }
    }
    display.sendBuffer();
}
