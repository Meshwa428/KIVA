#include "DuckyScriptRunner.h"
#include "App.h"
#include "Logger.h"
#include <BleKeyboard.h> 

static const DuckyCommand duckyCmds[] = {
    {"STRING", 0, DuckyCommand::Type::PRINT},
    {"STRINGLN", 0, DuckyCommand::Type::PRINT},
    {"REM", 0, DuckyCommand::Type::PRINT},
    {"DELAY", 0, DuckyCommand::Type::DELAY},
    {"DEFAULTDELAY", 0, DuckyCommand::Type::DELAY},
    {"DEFAULT_DELAY", 0, DuckyCommand::Type::DELAY},
    {"REPEAT", 0, DuckyCommand::Type::REPEAT},
    {"BACKSPACE", KEY_BACKSPACE, DuckyCommand::Type::CMD},
    {"DELETE", KEY_DELETE, DuckyCommand::Type::CMD},
    {"ALT", KEY_LEFT_ALT, DuckyCommand::Type::CMD},
    {"CTRL", KEY_LEFT_CTRL, DuckyCommand::Type::CMD},
    {"CONTROL", KEY_LEFT_CTRL, DuckyCommand::Type::CMD},
    {"GUI", KEY_LEFT_GUI, DuckyCommand::Type::CMD},
    {"WINDOWS", KEY_LEFT_GUI, DuckyCommand::Type::CMD},
    {"SHIFT", KEY_LEFT_SHIFT, DuckyCommand::Type::CMD},
    {"ESCAPE", KEY_ESC, DuckyCommand::Type::CMD},
    {"ESC", KEY_ESC, DuckyCommand::Type::CMD},
    {"TAB", KEY_TAB, DuckyCommand::Type::CMD},
    {"ENTER", KEY_RETURN, DuckyCommand::Type::CMD},
    {"DOWNARROW", KEY_DOWN_ARROW, DuckyCommand::Type::CMD},
    {"DOWN", KEY_DOWN_ARROW, DuckyCommand::Type::CMD},
    {"LEFTARROW", KEY_LEFT_ARROW, DuckyCommand::Type::CMD},
    {"LEFT", KEY_LEFT_ARROW, DuckyCommand::Type::CMD},
    {"RIGHTARROW", KEY_RIGHT_ARROW, DuckyCommand::Type::CMD},
    {"RIGHT", KEY_RIGHT_ARROW, DuckyCommand::Type::CMD},
    {"UPARROW", KEY_UP_ARROW, DuckyCommand::Type::CMD},
    {"UP", KEY_UP_ARROW, DuckyCommand::Type::CMD},
    {"PAUSE", 0x48, DuckyCommand::Type::CMD},
    {"BREAK", 0x48, DuckyCommand::Type::CMD},
    {"CAPSLOCK", KEY_CAPS_LOCK, DuckyCommand::Type::CMD},
    {"END", KEY_END, DuckyCommand::Type::CMD},
    {"HOME", KEY_HOME, DuckyCommand::Type::CMD},
    {"INSERT", KEY_INSERT, DuckyCommand::Type::CMD},
    {"MENU", 0x65, DuckyCommand::Type::CMD},
    {"NUMLOCK", 0x53, DuckyCommand::Type::CMD},
    {"PAGEUP", KEY_PAGE_UP, DuckyCommand::Type::CMD},
    {"PAGEDOWN", KEY_PAGE_DOWN, DuckyCommand::Type::CMD},
    {"PRINTSCREEN", 0x46, DuckyCommand::Type::CMD},
    {"SCROLLLOCK", 0x47, DuckyCommand::Type::CMD},
    {"SPACE", ' ', DuckyCommand::Type::CMD},
    {"F1", KEY_F1, DuckyCommand::Type::CMD},
    {"F2", KEY_F2, DuckyCommand::Type::CMD},
    {"F3", KEY_F3, DuckyCommand::Type::CMD},
    {"F4", KEY_F4, DuckyCommand::Type::CMD},
    {"F5", KEY_F5, DuckyCommand::Type::CMD},
    {"F6", KEY_F6, DuckyCommand::Type::CMD},
    {"F7", KEY_F7, DuckyCommand::Type::CMD},
    {"F8", KEY_F8, DuckyCommand::Type::CMD},
    {"F9", KEY_F9, DuckyCommand::Type::CMD},
    {"F10", KEY_F10, DuckyCommand::Type::CMD},
    {"F11", KEY_F11, DuckyCommand::Type::CMD},
    {"F12", KEY_F12, DuckyCommand::Type::CMD},
};
static const DuckyCombination duckyCombos[] = {
    {"CTRL-ALT",   KEY_LEFT_CTRL, KEY_LEFT_ALT,   0},
    {"CTRL-SHIFT", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 0},
    {"ALT-SHIFT",  KEY_LEFT_ALT,  KEY_LEFT_SHIFT, 0},
    {"CTRL-GUI",   KEY_LEFT_CTRL, KEY_LEFT_GUI,   0},
    {"ALT-GUI",    KEY_LEFT_ALT,  KEY_LEFT_GUI,   0},
    {"GUI-SHIFT",  KEY_LEFT_GUI,  KEY_LEFT_SHIFT, 0}
};

DuckyScriptRunner::DuckyScriptRunner() : app_(nullptr), state_(State::IDLE), delayUntil_(0), defaultDelay_(100) {}

void DuckyScriptRunner::setup(App* app) { app_ = app; }

bool DuckyScriptRunner::startScript(const std::string& scriptPath, Mode mode) {
    if (isActive()) stopScript();
    
    LOG(LogLevel::INFO, "DuckyRunner", "Starting script %s in %s mode", scriptPath.c_str(), (mode == Mode::USB ? "USB" : "BLE"));
    currentMode_ = mode;

    if (mode == Mode::BLE) {
        // <-- FIX: The call to startKeyboard() is now void. 
        // It just initiates the process.
        app_->getBleManager().startKeyboard();
        activeHid_.reset(new BleHid(app_->getBleManager().getKeyboard()));
    } else { // USB Mode
        activeHid_.reset(new UsbHid());
    }
    
    if (!activeHid_ || !activeHid_->begin()) {
        LOG(LogLevel::ERROR, "DuckyRunner", "Failed to begin active HID component.");
        if (mode == Mode::BLE) {
            app_->getBleManager().stopKeyboard();
        }
        activeHid_.reset();
        return false;
    }

    scriptReader_ = SdCardManager::openLineReader(scriptPath.c_str());
    if (!scriptReader_.isOpen()) {
        LOG(LogLevel::ERROR, "DuckyRunner", "Failed to open script file.");
        stopScript();
        return false;
    }
    
    state_ = State::WAITING_FOR_CONNECTION;
    scriptName_ = scriptPath.substr(scriptPath.find_last_of('/') + 1);
    currentLine_ = "Connecting...";
    linesExecuted_ = 0;
    delayUntil_ = 0;
    defaultDelay_ = 100;
    lastLine_ = "";
    
    return true;
}

void DuckyScriptRunner::stopScript() {
    if (!isActive()) return;
    
    LOG(LogLevel::INFO, "DuckyRunner", "Stopping script...");

    if (activeHid_) {
        activeHid_->releaseAll();
        activeHid_->end();
    }
    
    if (currentMode_ == Mode::BLE) {
        app_->getBleManager().stopKeyboard();
    }

    activeHid_.reset();
    scriptReader_.close();
    state_ = State::IDLE;
    currentLine_ = "";
    scriptName_ = "";
    LOG(LogLevel::INFO, "DuckyRunner", "Script stopped and resources released.");
}

void DuckyScriptRunner::loop() {
    if (!isActive() || !activeHid_) return;
    
    if (state_ == State::WAITING_FOR_CONNECTION) {
        if (activeHid_->isConnected()) {
            LOG(LogLevel::INFO, "DuckyRunner", "loop: HID connected. Changing state to RUNNING.");
            state_ = State::RUNNING;
        }
        return;
    }

    if (state_ == State::RUNNING) {
        if (millis() < delayUntil_) return;
        
        activeHid_->releaseAll();
        delay(defaultDelay_);
        
        currentLine_ = scriptReader_.readLine().c_str(); 

        if (currentLine_.empty()) {
            state_ = State::FINISHED;
            currentLine_ = "Finished";
            return;
        }
        
        linesExecuted_++;
        parseAndExecute(currentLine_);
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
    for (const auto& cmd : duckyCmds) {
        if (command == cmd.command) {
            switch (cmd.type) {
                case DuckyCommand::Type::PRINT:
                    if (command == "STRING") {
                        activeHid_->write((const uint8_t*)arg.c_str(), arg.length());
                    } else if (command == "STRINGLN") {
                        activeHid_->write((const uint8_t*)arg.c_str(), arg.length());
                        activeHid_->press(KEY_RETURN);
                    }
                    return;
                case DuckyCommand::Type::DELAY:
                    if (command == "DELAY") delayUntil_ = millis() + std::stoi(arg);
                    else if (command == "DEFAULTDELAY" || command == "DEFAULT_DELAY") defaultDelay_ = std::stoi(arg);
                    return;
                case DuckyCommand::Type::CMD:
                    executeCombination(command, arg);
                    return;
                case DuckyCommand::Type::REPEAT:
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
                default: break;
            }
        }
    }
    activeHid_->write((const uint8_t*)line.c_str(), line.length());
}

void DuckyScriptRunner::executeCombination(const std::string& command, const std::string& arg) {
    if (!activeHid_) return;
    for (const auto& combo : duckyCombos) {
        if (command == combo.command) {
            activeHid_->press(combo.key1);
            activeHid_->press(combo.key2);
            if (!arg.empty()) activeHid_->write((const uint8_t*)arg.c_str(), arg.length());
            return;
        }
    }
    uint8_t keyToPress = 0;
    for (const auto& cmd : duckyCmds) {
        if (command == cmd.command && cmd.type == DuckyCommand::Type::CMD) {
            keyToPress = cmd.key;
            break;
        }
    }
    if (keyToPress) {
        activeHid_->press(keyToPress);
        if (!arg.empty()) activeHid_->write((const uint8_t*)arg.c_str(), arg.length());
    }
}

DuckyScriptRunner::State DuckyScriptRunner::getState() const { return state_; }
bool DuckyScriptRunner::isActive() const { return state_ != State::IDLE; }
const std::string& DuckyScriptRunner::getCurrentLine() const { return currentLine_; }
const std::string& DuckyScriptRunner::getScriptName() const { return scriptName_; }
uint32_t DuckyScriptRunner::getLinesExecuted() const { return linesExecuted_; }