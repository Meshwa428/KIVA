#include "ActionListDataSource.h"
#include "App.h"
#include "ListMenu.h"
#include "UI_Utils.h"

ActionListDataSource::ActionListDataSource(const std::vector<MenuItem>& items) : items_(items) {}

int ActionListDataSource::getNumberOfItems(App* app) { 
    return items_.size(); 
}

void ActionListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    if (item.action) {
        item.action(app);
    } else {
        app->changeMenu(item.targetMenu);
    }
}

void ActionListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, item.icon);
    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    menu->updateAndDrawText(display, item.label, text_x, text_y, text_w, isSelected);
}