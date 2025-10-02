#include "MusicLibraryDataSource.h" // Renamed header
#include "App.h"
#include "SdCardManager.h"
#include "MusicLibraryManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Config.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "SongListDataSource.h" // Include the correctly named header

MusicLibraryDataSource::MusicLibraryDataSource() : isReindexing_(false), needsReload_(false) {}

void MusicLibraryDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    isReindexing_ = false;
    needsReload_ = false;
    loadPlaylists();
}

void MusicLibraryDataSource::onExit(App* app, ListMenu* menu) {
    items_.clear();
}

void MusicLibraryDataSource::onUpdate(App* app, ListMenu* menu) {
    if (needsReload_) {
        needsReload_ = false;
        menu->reloadData(app, true);
    }
}

void MusicLibraryDataSource::loadPlaylists() {
    items_.clear();
    
    // This now only loads PLAYLISTS (directories) from the root music folder
    std::string indexPath = std::string(SD_ROOT::USER_MUSIC) + "/" + MusicLibraryManager::INDEX_FILENAME;
    auto reader = SdCardManager::getInstance().openLineReader(indexPath.c_str());
    if (reader.isOpen()) {
        while(true) {
            String line = reader.readLine();
            if (line.isEmpty()) break;
            
            // Only parse lines for Playlists ('P')
            if (line.startsWith("P;")) {
                 int firstSep = line.indexOf(';');
                 int secondSep = line.indexOf(';', firstSep + 1);
                 items_.push_back({
                     line.substring(firstSep + 1, secondSep).c_str(),
                     line.substring(secondSep + 1).c_str(),
                     ItemType::PLAYLIST
                 });
            }
        }
        reader.close();
    }
    items_.push_back({"Re-index Library", "", ItemType::REINDEX});
}

int MusicLibraryDataSource::getNumberOfItems(App* app) {
    return isReindexing_ ? 0 : items_.size();
}

bool MusicLibraryDataSource::drawCustomEmptyMessage(App* app, U8G2& display) {
    if (isReindexing_) {
        const char* msg = "Re-indexing Library...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return true;
    }
    return false;
}

void MusicLibraryDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    
    // --- THE CRITICAL FIX ---
    // Make a full copy of the item data immediately.
    // This makes the rest of the function immune to the `items_.clear()`
    // call that happens mid-execution during the event dispatch.
    PlaylistItem item_copy = items_[index];

    switch(item_copy.type) { // Use the safe copy from this point on
        case ItemType::PLAYLIST: {
            // 1. Get the new song list data source instance from the App
            auto& songListDataSource = app->getSongListDataSource();
            
            // 2. Configure it with the path from our safe, local copy
            songListDataSource.setPlaylistPath(item_copy.path);
            
            // 3. Now it is safe to publish the navigation event.
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::SONG_LIST));
            break;
        }
        case ItemType::REINDEX: {
            // This case was already safe, but for consistency, we'll leave the pattern.
            isReindexing_ = true;
            menu->reloadData(app, true);
            // Redraw the "re-indexing" message before the blocking call
            U8G2& display = app->getHardwareManager().getMainDisplay();
            display.clearBuffer();
            app->drawStatusBar();
            menu->draw(app, display); 
            display.sendBuffer();
            // Blocking call
            app->getMusicLibraryManager().buildIndex();
            isReindexing_ = false;
            // Show popup and reload data on confirm
            app->showPopUp("Success", "Library re-indexed.", 
                [this](App* cb_app) {
                    this->loadPlaylists();
                    this->needsReload_ = true;
                }, 
                "OK", "", true);
            break;
        }
    }
}

void MusicLibraryDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon;
    switch(item.type) {
        case ItemType::PLAYLIST: icon = IconType::PLAYLIST; break;
        case ItemType::REINDEX:  icon = IconType::UI_REFRESH; break;
        default:                 icon = IconType::NONE; break;
    }
    
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, item.name.c_str(), text_x, text_y, text_w, isSelected);
}

