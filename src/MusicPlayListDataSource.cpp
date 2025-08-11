#include "MusicPlayListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "MusicLibraryManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Config.h"

MusicPlayListDataSource::MusicPlayListDataSource() :
    currentPath_(SD_ROOT::USER_MUSIC)
{}

void MusicPlayListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    if (isForwardNav) {
        currentPath_ = SD_ROOT::USER_MUSIC;
    }
    isReindexing_ = false; // Ensure flag is reset on enter
    loadItemsFromPath(currentPath_.c_str());
}

void MusicPlayListDataSource::onExit(App* app, ListMenu* menu) {
    items_.clear();
    isReindexing_ = false; // Ensure flag is reset on exit
}

void MusicPlayListDataSource::loadItemsFromPath(const char* directoryPath) {
    items_.clear();
    currentPath_ = directoryPath;
    
    std::string indexPath = std::string(directoryPath) + "/" + MusicLibraryManager::INDEX_FILENAME;
    auto reader = SdCardManager::openLineReader(indexPath.c_str());
    if (!reader.isOpen()) {
        // Index doesn't exist.
    } else {
        while(true) {
            String line = reader.readLine();
            if (line.isEmpty()) break;
            
            int firstSep = line.indexOf(';');
            int secondSep = line.indexOf(';', firstSep + 1);
            if (firstSep != -1 && secondSep != -1) {
                ItemType type = (line.charAt(0) == 'P') ? ItemType::PLAYLIST : ItemType::TRACK;
                items_.push_back({
                    line.substring(firstSep + 1, secondSep).c_str(),
                    line.substring(secondSep + 1).c_str(),
                    type
                });
            }
        }
        reader.close();
    }

    if (currentPath_ != SD_ROOT::USER_MUSIC) {
        items_.push_back({"Back", "", ItemType::BACK});
    }
    items_.push_back({"Re-index Library", "", ItemType::REINDEX});
}

int MusicPlayListDataSource::getNumberOfItems(App* app) {
    // If we are re-indexing, report 0 items to trigger the custom message.
    if (isReindexing_) {
        return 0;
    }
    return items_.size();
}

// --- ADD THIS NEW FUNCTION ---
bool MusicPlayListDataSource::drawCustomEmptyMessage(App* app, U8G2& display) {
    if (isReindexing_) {
        const char* msg = "Re-indexing Library...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return true; // We handled the drawing
    }
    return false; // Let the ListMenu draw its default "No items" message
}

void MusicPlayListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    
    switch(item.type) {
        // (Cases for PLAYLIST, TRACK, and BACK remain unchanged)
        case ItemType::PLAYLIST: {
            loadItemsFromPath(item.path.c_str());
            menu->reloadData(app, true);
            break;
        }
        case ItemType::TRACK: {
            // --- THIS IS THE FINAL, CORRECTED LOGIC ---
            
            // 1. Prepare the playlist data (fast)
            std::vector<std::string> playlistTracks;
            for (const auto& trackItem : items_) {
                if (trackItem.type == ItemType::TRACK) {
                    playlistTracks.push_back(trackItem.path);
                }
            }
            
            int currentTrackIndexInPlaylist = 0;
            for(size_t i=0; i < playlistTracks.size(); ++i) {
                if (playlistTracks[i] == item.path) {
                    currentTrackIndexInPlaylist = i;
                    break;
                }
            }

            std::string parentPath = currentPath_;
            size_t lastSlash = parentPath.find_last_of('/');
            std::string playlistName = (lastSlash != std::string::npos) ? parentPath.substr(lastSlash + 1) : "Music";

            // 2. Queue the playlist for playback. This is non-blocking.
            app->getMusicPlayer().queuePlaylist(playlistName, playlistTracks, currentTrackIndexInPlaylist);
            
            // 3. Change the menu. This is also non-blocking.
            // The UI will now instantly switch to the NowPlayingMenu, which will
            // see the "LOADING" state and display the correct message.
            app->changeMenu(MenuType::NOW_PLAYING);
            break;
        }
        case ItemType::BACK: {
            size_t lastSlash = currentPath_.find_last_of('/');
            if (lastSlash != std::string::npos && lastSlash > 0) {
                currentPath_ = currentPath_.substr(0, lastSlash);
            } else {
                currentPath_ = SD_ROOT::USER_MUSIC;
            }
            loadItemsFromPath(currentPath_.c_str());
            menu->reloadData(app, true);
            break;
        }
        // --- THIS IS THE MODIFIED CASE ---
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
            app->showPopUp("Success", "Music library has been re-indexed.", 
                [this, menu](App* cb_app) {
                    this->loadItemsFromPath(this->currentPath_.c_str());
                    menu->reloadData(cb_app, true);
                }, 
                "OK", "", true);
            break;
        }
    }
}

// The drawItem function remains unchanged
void MusicPlayListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon;
    switch(item.type) {
        case ItemType::PLAYLIST: icon = IconType::PLAYLIST; break;
        case ItemType::TRACK:    icon = IconType::MUSIC_NOTE; break;
        case ItemType::REINDEX:  icon = IconType::UI_REFRESH; break;
        case ItemType::BACK:     icon = IconType::NAV_BACK; break;
        default:                 icon = IconType::NONE; break;
    }
    
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, item.name.c_str(), text_x, text_y, text_w, isSelected);
}