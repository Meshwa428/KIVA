#include "MusicPlayListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "MusicLibraryManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Config.h"

MusicPlayListDataSource::MusicPlayListDataSource() :
    currentPath_(SD_ROOT::USER_MUSIC),
    isReindexing_(false),
    needsReload_(false)
{}

void MusicPlayListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    if (isForwardNav) {
        currentPath_ = SD_ROOT::USER_MUSIC;
    }
    isReindexing_ = false;
    needsReload_ = false;
    loadItemsFromPath(currentPath_.c_str());
}

void MusicPlayListDataSource::onExit(App* app, ListMenu* menu) {
    items_.clear();
    isReindexing_ = false;
    needsReload_ = false;
}

void MusicPlayListDataSource::onUpdate(App* app, ListMenu* menu) {
    if (needsReload_) {
        needsReload_ = false;
        menu->reloadData(app, true);
    }
}

void MusicPlayListDataSource::loadItemsFromPath(const char* directoryPath) {
    items_.clear();
    currentPath_ = directoryPath;
    
    std::string indexPath = std::string(directoryPath) + "/" + MusicLibraryManager::INDEX_FILENAME;
    auto reader = SdCardManager::openLineReader(indexPath.c_str());
    if (!reader.isOpen()) {
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

    items_.push_back({"Re-index Library", "", ItemType::REINDEX});
}

int MusicPlayListDataSource::getNumberOfItems(App* app) {
    if (isReindexing_) {
        return 0;
    }
    return items_.size();
}

bool MusicPlayListDataSource::drawCustomEmptyMessage(App* app, U8G2& display) {
    if (isReindexing_) {
        const char* msg = "Re-indexing Library...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return true;
    }
    return false;
}

void MusicPlayListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    
    switch(item.type) {
        case ItemType::PLAYLIST: {
            // --- THIS IS THE FIX ---
            // Copy the path to a local variable BEFORE calling loadItemsFromPath,
            // which clears the vector that 'item' refers to.
            std::string path_copy = item.path;
            loadItemsFromPath(path_copy.c_str());
            needsReload_ = true; 
            break;
            // --- END OF FIX ---
        }
        case ItemType::TRACK: {
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

            app->getMusicPlayer().queuePlaylist(playlistName, playlistTracks, currentTrackIndexInPlaylist);
            app->changeMenu(MenuType::NOW_PLAYING);
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
            app->showPopUp("Success", "Music library has been re-indexed.", 
                [this, menu](App* cb_app) {
                    this->loadItemsFromPath(this->currentPath_.c_str());
                    this->needsReload_ = true;
                }, 
                "OK", "", true);
            break;
        }
    }
}

void MusicPlayListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon;
    switch(item.type) {
        case ItemType::PLAYLIST: icon = IconType::PLAYLIST; break;
        case ItemType::TRACK:    icon = IconType::MUSIC_NOTE; break;
        case ItemType::REINDEX:  icon = IconType::UI_REFRESH; break;
        default:                 icon = IconType::NONE; break;
    }
    
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, item.name.c_str(), text_x, text_y, text_w, isSelected);
}

bool MusicPlayListDataSource::onBackPress(App* app, ListMenu* menu) {
    if (currentPath_ != SD_ROOT::USER_MUSIC) {
        // Navigate up one directory
        size_t lastSlash = currentPath_.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0) {
            currentPath_ = currentPath_.substr(0, lastSlash);
        } else {
            currentPath_ = SD_ROOT::USER_MUSIC;
        }
        loadItemsFromPath(currentPath_.c_str());
        needsReload_ = true;
        return true; // We handled the back press
    }
    return false; // We are at the root, let the menu handle it (will go to MainMenu)
}