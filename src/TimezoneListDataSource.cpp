#include "TimezoneListDataSource.h"
#include "App.h"
#include "RtcManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ConfigManager.h"
#include "Timezones.h"

TimezoneListDataSource::TimezoneListDataSource() {
    // empty
}

const std::vector<TimezoneInfo>& TimezoneListDataSource::getTimezones() {
    if (timezones_.empty()) {
        timezones_ = TIMEZONE_LIST;
    }
    return timezones_;
}

int TimezoneListDataSource::getNumberOfItems(App* app) {
    return timezones_.size();
}

void TimezoneListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= timezones_.size()) return;

    const auto& selectedTz = timezones_[index];
    
    auto& settings = app->getConfigManager().getSettings();
    strncpy(settings.timezoneString, selectedTz.posixTzString, sizeof(settings.timezoneString) - 1);
    settings.timezoneString[sizeof(settings.timezoneString) - 1] = '\0'; 
    
    app->getConfigManager().saveSettings();
    app->getRtcManager().setTimezone(settings.timezoneString);
    app->getRtcManager().syncInternalClock();

    EventDispatcher::getInstance().publish(NavigateBackEvent());
    app->showPopUp("Timezone Set", "Time has been updated", nullptr, "OK", "", true);
}

void TimezoneListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    getTimezones();
}

void TimezoneListDataSource::onExit(App* app, ListMenu* menu) {
    timezones_.clear();
}

void TimezoneListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= timezones_.size()) return;

    const auto& item = timezones_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    bool isActive = (strcmp(item.posixTzString, app->getConfigManager().getSettings().timezoneString) == 0);
    
    // Use the long, descriptive name in this detailed list view
    std::string label = item.displayName;
    if (isActive) {
        label += " *"; 
    }

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, IconType::SETTINGS);
    
    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, label.c_str(), text_x, text_y, text_w, isSelected);
}