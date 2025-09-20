#include "IMenu.h"
#include "Event.h"
#include "App.h" // Required for getInstance()

// The onEvent method filters events and calls the specific handler.
void IMenu::onEvent(const Event& event) {
    if (event.type == EventType::APP_INPUT) {
        // We know App is valid because a menu can't exist without it.
        // We need a way to get the App context. For now, we assume a global or service locator.
        App* app = &App::getInstance();
        // Let's assume App provides a static getInstance() for simplicity here.
        const auto& inputEvent = static_cast<const InputEventData&>(event);
        handleInput(inputEvent.input, app); // We'll need to implement App::getInstance()
        app->requestRedraw();
    }
}
