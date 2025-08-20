#include "DuckyScriptRunner.h"
#include "App.h"
#include "Logger.h"

// Define mappings from generic DuckyScript keys to HIDForge keys
static const struct {
    const char* command;
    uint8_t key;
} duckyKeyMap[] = {
    {"BACKSPACE", KEYBACKSPACE}, {"DELETE", KEY_DELETE}, {"ALT", KEY_LEFT_ALT},
    {"CTRL", KEY_LEFT_CTRL}, {"CONTROL", KEY_LEFT_CTRL}, {"GUI", KEY_LEFT_GUI},
    {"WINDOWS", KEY_LEFT_GUI}, {"SHIFT", KEY_LEFT_SHIFT}, {"ESCAPE", KEY_ESC},
    {"ESC", KEY_ESC}, {"TAB", KEYTAB}, {"ENTER", KEY_RETURN},
    {"DOWNARROW", KEY_DOWN_ARROW}, {"DOWN", KEY_DOWN_ARROW}, {"LEFTARROW", KEY_LEFT_ARROW},
    {"LEFT", KEY_LEFT_ARROW}, {"RIGHTARROW", KEY_RIGHT_ARROW}, {"RIGHT", KEY_RIGHT_ARROW},
    {"UPARROW", KEY_UP_ARROW}, {"UP", KEY_UP_ARROW}, {"PAUSE", KEY_PAUSE},
    {"BREAK", KEY_PAUSE}, {"CAPSLOCK", KEY_CAPS_LOCK}, {"END", KEY_END},
    {"HOME", KEY_HOME}, {"INSERT", KEY_INSERT}, {"MENU", KEY_MENU},
    {"PRINTSCREEN", KEY_PRINT_SCREEN}, {"SCROLLLOCK", KEY_SCROLL_LOCK},
    {"SPACE", ' '}, {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3}, {"F4", KEY_F4},
    {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7}, {"F8", KEY_F8}, {"F9", KEY_F9},
    {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12}
};

struct DuckyCombination {
    const char* command;
    uint8_t key1;
    uint8_t key2;
};

static const DuckyCombination duckyCombos[] = {
    {"CTRL-ALT",   KEY_LEFT_CTRL, KEY_LEFT_ALT},
    {"CTRL-SHIFT", KEY_LEFT_CTRL, KEY_LEFT_SHIFT},
    {"ALT-SHIFT",  KEY_LEFT_ALT,  KEY_LEFT_SHIFT},
    {"CTRL-GUI",   KEY_LEFT_CTRL, KEY_LEFT_GUI},
    {"ALT-GUI",    KEY_LEFT_ALT,  KEY_LEFT_GUI},
    {"GUI-SHIFT",  KEY_LEFT_GUI,  KEY_LEFT_SHIFT}
};

DuckyScriptRunner::DuckyScriptRunner() 
    : app_(nullptr), activeHid_(nullptr), isUsb_(false), state_(State::IDLE), 
      delayUntil_(0), defaultDelay_(100) {}

void DuckyScriptRunner::setup(App* app) { 
    app_ = app; 
}

bool DuckyScriptRunner::startScript(const std::string& scriptPath, HIDInterface* hid, bool isUsb) {
    if (isActive() || !hid) {
        return false;
    }
    
    activeHid_ = hid;
    isUsb_ = isUsb; // Store the type
    LOG(LogLevel::INFO, "DuckyRunner", "Starting script %s", scriptPath.c_str());

    // Initialize the specific device and set its layout
    activeHid_->begin(app_->getConfigManager().getHidLayout());
    
    // Start the global USB stack ONLY if we were given a USB device
    if (isUsb_) {
        USB.begin();
    }
    
    scriptReader_ = SdCardManager::openLineReader(scriptPath.c_str());
    if (!scriptReader_.isOpen()) {
        LOG(LogLevel::ERROR, "DuckyRunner", "Failed to open script file.");
        stopScript();
        return false;
    }
    
    state_ = State::WAITING_FOR_CONNECTION;
    scriptName_ = scriptPath.substr(scriptPath.find_last_of('/') + 1);
    linesExecuted_ = 0;
    delayUntil_ = 0;
    defaultDelay_ = 100;
    connectionTime_ = 0;
    lastLine_ = "";
    
    return true;
}

void DuckyScriptRunner::stopScript() {
    if (!isActive()) return;
    
    LOG(LogLevel::INFO, "DuckyRunner", "Stopping script...");

    if (activeHid_) {
        // Check if it was a USB device before trying to end the global stack
        bool wasUsb = isUsb_;

        activeHid_->releaseAll();
        activeHid_->end(); // This now correctly handles deinit for BLE
        
        if (wasUsb) {
            // USB.end();
            // TODO: USB.end() isnt available, figure out how to disable USB stack
        }
    }

    activeHid_ = nullptr;
    scriptReader_.close();
    state_ = State::IDLE;
    scriptName_ = "";
}

void DuckyScriptRunner::loop() {
    if (!isActive() || !activeHid_) return;
    
    if (state_ == State::WAITING_FOR_CONNECTION) {
        if (activeHid_->isConnected()) {
            LOG(LogLevel::INFO, "DuckyRunner", "HID connected. Starting post-connection delay.");
            connectionTime_ = millis();
            state_ = State::RUNNING;
        }
        return;
    }

    if (state_ == State::RUNNING) {
        if (millis() < delayUntil_) return;
        
        activeHid_->releaseAll();
        delay(defaultDelay_);
        
        std::string currentLine = scriptReader_.readLine().c_str(); 

        if (currentLine.empty()) {
            state_ = State::FINISHED;
            return;
        }
        
        linesExecuted_++;
        parseAndExecute(currentLine);
    }
}

void DuckyScriptRunner::parseAndExecute(const std::string& line) {
    if (!activeHid_) return;

    std::string command;
    std::string arg;
    size_t firstSpace = line.find(' ');
    if (firstSpace != std::string::npos) {
        command = line.substr(0, firstSpace);
        arg = line.substr(firstSpace + 1);
    } else {
        command = line;
    }

    if (command == "REM") return;
    if (command != "REPEAT") lastLine_ = line;

    if (command == "STRING") {
        activeHid_->print(arg.c_str());
        return;
    }
    if (command == "STRINGLN") {
        activeHid_->println(arg.c_str());
        return;
    }
    if (command == "DELAY") {
        delayUntil_ = millis() + std::stoi(arg);
        return;
    }
    if (command == "DEFAULTDELAY" || command == "DEFAULT_DELAY") {
        defaultDelay_ = std::stoi(arg);
        return;
    }
    if (command == "REPEAT") {
        if (!lastLine_.empty()) {
            int count = std::stoi(arg);
            for (int i = 0; i < count; ++i) {
                if (i > 0) { 
                    activeHid_->releaseAll();
                    delay(defaultDelay_);
                }
                parseAndExecute(lastLine_);
            }
        }
        return;
    }

    for (const auto& combo : duckyCombos) {
        if (command == combo.command) {
            activeHid_->press(combo.key1);
            activeHid_->press(combo.key2);
            if (!arg.empty()) {
                activeHid_->press(arg[0]);
            }
            return;
        }
    }

    std::vector<uint8_t> keysToPress;
    size_t start = 0;
    size_t end = command.find('-');
    bool isCombo = (end != std::string::npos);
    
    while (end != std::string::npos) {
        std::string key_str = command.substr(start, end - start);
        bool found = false;
        for (const auto& key : duckyKeyMap) {
            if (key_str == key.command) {
                keysToPress.push_back(key.key);
                found = true;
                break;
            }
        }
        if (!found) {
            isCombo = false;
            break;
        }
        start = end + 1;
        end = command.find('-', start);
    }
    
    // Process the last (or only) part of the command
    if (keysToPress.empty() || start < command.length()) {
        std::string last_key_str = command.substr(start);
        bool found = false;
        for (const auto& key : duckyKeyMap) {
            if (last_key_str == key.command) {
                keysToPress.push_back(key.key);
                found = true;
                break;
            }
        }
        if(!found) isCombo = false;
    }

    if (isCombo) {
        for (uint8_t key : keysToPress) activeHid_->press(key);
        if (!arg.empty()) activeHid_->press(arg[0]);
    } else {
        bool commandFound = false;
        for (const auto& key : duckyKeyMap) {
            if (command == key.command) {
                activeHid_->press(key.key);
                if (!arg.empty()) activeHid_->print(arg.c_str());
                commandFound = true;
                break;
            }
        }
        if (!commandFound) {
            activeHid_->println(line.c_str());
        }
    }
}

// --- START OF FIX: Add missing function definitions ---

DuckyScriptRunner::State DuckyScriptRunner::getState() const {
    return state_;
}

bool DuckyScriptRunner::isActive() const {
    // A script is considered "active" if it's doing anything other than sitting idle.
    return state_ != State::IDLE;
}

const std::string& DuckyScriptRunner::getScriptName() const {
    return scriptName_;
}

uint32_t DuckyScriptRunner::getLinesExecuted() const {
    return linesExecuted_;
}

// --- END OF FIX ---