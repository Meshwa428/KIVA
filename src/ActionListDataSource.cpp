#include "ActionListDataSource.h"
#include "App.h"
#include "ListMenu.h"
#include "UI_Utils.h"

ActionListDataSource::ActionListDataSource(const std::vector<MenuItem>& items) : items_(items) {}

int ActionListDataSource::getNumberOfItems(App* app) { 
    return items_.size(); 
}

const MenuItem* ActionListDataSource::getItem(int index) const {
    if (index >= 0 && (size_t)index < items_.size()) {
        return &items_[index];
    }
    return nullptr;
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

    // --- Draw Icon (common for all items) ---
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, item.icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;

    // --- UNIFIED DRAWING LOGIC FOR ALL ITEMS ---

    // 1. Get the value text if the item is interactive.
    std::string valueText;
    if (item.isInteractive && item.getValue) {
        valueText = item.getValue(app);
    }

    // 2. Calculate available width for the main label.
    int right_padding = 4;
    int value_width_with_padding = 0;
    if (!valueText.empty()) {
        // Reserve space for the value text (e.g., "< 100% >") plus some padding.
        value_width_with_padding = display.getStrWidth(valueText.c_str()) + 8;
    }
    
    // The available width for the label is from its start position to the right edge of the box,
    // minus any space reserved for the value text.
    int text_w_available = (x + w) - text_x - right_padding - value_width_with_padding;

    // 3. Draw the label using the marquee/truncation helper within the calculated space.
    menu->updateAndDrawText(display, item.label, text_x, text_y, text_w_available, isSelected);

    // 4. If there's a value, draw it on the right side.
    if (!valueText.empty()) {
        int value_x = x + w - display.getStrWidth(valueText.c_str()) - 8;
        display.drawStr(value_x, text_y, valueText.c_str());
    }
}