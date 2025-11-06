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
        // Use the original, non-bold font for a cleaner look
        display.setFont(u8g2_font_6x10_tf);

        const char* line1 = "Re-indexing";
        const char* line2 = "Library...";
        const char* line3 = "Please wait";
        
        // Define layout constants
        const int fontHeight = 10;
        const int lineSpacing = 2;
        const int totalLines = 3;
        
        // Calculate the total height of the entire text block
        const int totalBlockHeight = (totalLines * fontHeight) + ((totalLines - 1) * lineSpacing);
        
        // Calculate the dimensions of the area *below* the status bar
        const int drawableAreaY = STATUS_BAR_H;
        const int drawableAreaHeight = display.getDisplayHeight() - drawableAreaY;

        // Calculate the starting Y position to perfectly center the block within the drawable area
        const int startY = drawableAreaY + (drawableAreaHeight - totalBlockHeight) / 2;

        // Calculate the baseline Y position for the first line
        // The baseline is where the bottom of the characters sit. For u8g2, it's relative to the top.
        int baselineY = startY + display.getAscent();

        // Calculate X positions for each line to center them horizontally
        int x1 = (display.getDisplayWidth() - display.getStrWidth(line1)) / 2;
        int x2 = (display.getDisplayWidth() - display.getStrWidth(line2)) / 2;
        int x3 = (display.getDisplayWidth() - display.getStrWidth(line3)) / 2;

        // Draw the three centered lines at their calculated positions
        display.drawStr(x1, baselineY, line1);
        display.drawStr(x2, baselineY + fontHeight + lineSpacing, line2);
        display.drawStr(x3, baselineY + 2 * (fontHeight + lineSpacing), line3);

        return true;
    }
    return false;
}

void MusicLibraryDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    
    PlaylistItem item_copy = items_[index];

    switch(item_copy.type) {
        case ItemType::PLAYLIST: {
            auto& songListDataSource = app->getSongListDataSource();
            songListDataSource.setPlaylistPath(item_copy.path);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::SONG_LIST));
            break;
        }
        case ItemType::REINDEX: {
            isReindexing_ = true;
            menu->reloadData(app, true);
            
            U8G2& display = app->getHardwareManager().getMainDisplay();
            display.clearBuffer();
            app->drawStatusBar();
            menu->draw(app, display); 
            display.sendBuffer();
            
            app->getMusicLibraryManager().buildIndex();
            isReindexing_ = false;
            
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