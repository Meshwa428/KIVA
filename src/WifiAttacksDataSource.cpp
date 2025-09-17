#include "WifiAttacksDataSource.h"
#include "App.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"

WifiAttacksDataSource::WifiAttacksDataSource() {
    items_ = {
        {"Beacon Spam", IconType::BEACON, MenuType::BEACON_MODE_GRID},
        {"Deauth", IconType::DISCONNECT, MenuType::DEAUTH_MODE_GRID},
        {"Evil Twin", IconType::SKULL, MenuType::PORTAL_LIST},
        {"Probe Flood", IconType::TOOL_PROBE, MenuType::PROBE_FLOOD_MODE_GRID},
        {"Karma Attack", IconType::TOOL_INJECTION, MenuType::KARMA_ACTIVE},
        {"Assoc Sleep", IconType::SKULL, MenuType::ASSOCIATION_SLEEP_MODE_CAROUSEL},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    };
}

int WifiAttacksDataSource::getNumberOfItems(App* app) { return items_.size(); }

void WifiAttacksDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    if (item.action) {
        item.action(app);
    } else if (item.targetMenu == MenuType::BACK) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    } else {
        EventDispatcher::getInstance().publish(NavigateToMenuEvent(item.targetMenu));
    }
}

void WifiAttacksDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, item.icon);
    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    menu->updateAndDrawText(display, item.label, text_x, text_y, text_w, isSelected);
}