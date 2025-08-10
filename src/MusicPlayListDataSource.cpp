#include "MusicPlayListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "MusicLibraryManager.h" // For INDEX_FILENAME
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Config.h" // For SD_ROOT

MusicPlayListDataSource::MusicPlayListDataSource() :
    currentPath_(SD_ROOT::USER_MUSIC)
{}

void MusicPlayListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    // When entering for the first time or navigating back to the root,
    // ensure we are at the top-level directory.
    if (isForwardNav) {
        currentPath_ = SD_ROOT::USER_MUSIC;
    }
    loadItemsFromPath(currentPath_.c_str());
}

void MusicPlayListDataSource::onExit(App* app, ListMenu* menu) {
    items_.clear();
}

void MusicPlayListDataSource::loadItemsFromPath(const char* directoryPath) {
    items_.clear();
    currentPath_ = directoryPath;
    
    std::string indexPath = std::string(directoryPath) + "/" + MusicLibraryManager::INDEX_FILENAME;
    auto reader = SdCardManager::openLineReader(indexPath.c_str());
    if (!reader.isOpen()) {
        // Optional: Could trigger a re-index or show an error.
        // For now, the list will just appear empty.
        return;
    }

    while(true) {
        String line = reader.readLine();
        if (line.isEmpty()) break;
        
        int firstSep = line.indexOf(';');
        int secondSep = line.indexOf(';', firstSep + 1);
        if (firstSep != -1 && secondSep != -1) {
            items_.push_back({
                line.substring(firstSep + 1, secondSep).c_str(),
                line.substring(secondSep + 1).c_str(),
                (line.charAt(0) == 'P')
            });
        }
    }
    reader.close();
}

int MusicPlayListDataSource::getNumberOfItems(App* app) {
    return items_.size();
}

void MusicPlayListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    
    if (item.isPlaylist) {
        // --- NAVIGATE INTO A PLAYLIST ---
        // Load the items from the subdirectory's index and reload the menu.
        loadItemsFromPath(item.path.c_str());
        menu->reloadData(app, true); // true = reset selection to the top
    } else {
        // --- PLAY A SINGLE TRACK ---
        // --- THIS IS THE CORRECTED, SAFE LOGIC ---

        // 1. Build a temporary playlist of all tracks in the current view.
        std::vector<std::string> playlistTracks;
        for (const auto& trackItem : items_) {
            if (!trackItem.isPlaylist) {
                playlistTracks.push_back(trackItem.path);
            }
        }
        
        // 2. Find the index of the selected track within our temporary playlist.
        int currentTrackIndexInPlaylist = 0;
        for(size_t i=0; i < playlistTracks.size(); ++i) {
            if (playlistTracks[i] == item.path) {
                currentTrackIndexInPlaylist = i;
                break;
            }
        }

        // 3. Get the playlist name from the current directory path.
        std::string parentPath = currentPath_;
        size_t lastSlash = parentPath.find_last_of('/');
        std::string playlistName = (lastSlash != std::string::npos) ? parentPath.substr(lastSlash + 1) : "Music";

        // 4. Call the new, safe, asynchronous method.
        app->getMusicPlayer().playPlaylistAtIndex(playlistName, playlistTracks, currentTrackIndexInPlaylist);

        // 5. Change the menu immediately. This is now safe.
        app->changeMenu(MenuType::NOW_PLAYING);
    }
}

void MusicPlayListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon = item.isPlaylist ? IconType::PLAYLIST : IconType::MUSIC_NOTE;
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, item.name.c_str(), text_x, text_y, text_w, isSelected);
}