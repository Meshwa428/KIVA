#ifndef I_LIST_MENU_DATA_SOURCE_H
#define I_LIST_MENU_DATA_SOURCE_H

#include <U8g2lib.h>

// Forward declarations to avoid circular dependencies
class App;
class ListMenu;

/**
 * @brief An interface for providing data and rendering logic to a generic ListMenu.
 * 
 * Any class that wants to display its data in a ListMenu must implement this interface.
 * This decouples the ListMenu's UI/animation logic from the data-specific logic.
 */
class IListMenuDataSource {
public:
    virtual ~IListMenuDataSource() {}

    /**
     * @brief Called by ListMenu to get the total number of items in the list.
     */
    virtual int getNumberOfItems(App* app) = 0;

    /**
     * @brief Called by ListMenu to draw the content of a single item.
     * 
     * The ListMenu handles drawing the selection box and calculating the position (x, y)
     * and dimensions (w, h) of the item. This function only needs to draw the
     * specific content (icons, text, etc.) *inside* that box.
     * 
     * @param app The main App context.
     * @param display The U8G2 display instance.
     * @param menu A const pointer to the calling ListMenu.
     * @param index The index of the item to draw.
     * @param x The top-left x-coordinate of the drawing area for this item.
     * @param y The top-left y-coordinate of the drawing area for this item.
     * @param w The width of the drawing area.
     * @param h The height of the drawing area.
     * @param isSelected True if this item is the currently selected one.
     */
    virtual void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) = 0;

    /**
     * @brief Called when the user presses the OK/Enter button on an item.
     * The implementation should handle the specific action for the selected item.
     */
    virtual void onItemSelected(App* app, ListMenu* menu, int index) = 0;

    /**
     * @brief Called when the ListMenu is first entered. 
     * The implementation can use this to refresh its data (e.g., start a new Wi-Fi scan).
     */
    virtual void onEnter(App* app, ListMenu* menu) = 0;
    
    /**
     * @brief Called when the ListMenu is exited.
     * The implementation can use this for cleanup.
     */
    virtual void onExit(App* app, ListMenu* menu) = 0;

    /**
     * @brief [Optional] Called on every frame.
     * Can be used for continuous updates, like checking for a completed scan.
     */
    virtual void onUpdate(App* app, ListMenu* menu) { /* Default implementation does nothing */ };
    
    /**
     * @brief [NEW] Allows the data source to draw a custom message when the item list is empty.
     * @return true if a custom message was drawn, false to allow the ListMenu to draw its default message.
     */
    virtual bool drawCustomEmptyMessage(App* app, U8G2& display) { return false; }
};

#endif // I_LIST_MENU_DATA_SOURCE_H