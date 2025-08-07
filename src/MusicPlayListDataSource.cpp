#include "MusicPlayListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"

void MusicPlayListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    items_.clear();
    const char* dirPath = SD_ROOT::USER_MUSIC;

    if (!SdCardManager::exists(dirPath)) {
        SdCardManager::createDir(dirPath);
    }
    
    File root = SdCardManager::openFile(dirPath);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while(file) {
        std::string fileName = file.name();
        if (fileName == "." || fileName == "..") {
            file.close(); file = root.openNextFile(); continue;
        }

        if (file.isDirectory()) {
            items_.push_back({fileName, file.path(), true});
        } else if (String(file.name()).endsWith(".mp3")) {
            items_.push_back({fileName, file.path(), false});
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

void MusicPlayListDataSource::onExit(App* app, ListMenu* menu) {}

int MusicPlayListDataSource::getNumberOfItems(App* app) {
    return items_.size();
}

void MusicPlayListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= items_.size()) return;
    const auto& item = items_[index];
    auto& player = app->getMusicPlayer();

    if (item.isPlaylist) {
        std::vector<std::string> tracks;
        File playlistDir = SdCardManager::openFile(item.path.c_str());
        if(playlistDir && playlistDir.isDirectory()) {
            File trackFile = playlistDir.openNextFile();
            while(trackFile) {
                if (!trackFile.isDirectory() && String(trackFile.name()).endsWith(".mp3")) {
                    tracks.push_back(trackFile.path());
                }
                trackFile.close();
                trackFile = playlistDir.openNextFile();
            }
        }
        playlistDir.close();
        
        if (!tracks.empty()) {
            player.playPlaylist(item.name, tracks);
            app->changeMenu(MenuType::NOW_PLAYING);
        } else {
            app->showPopUp("Empty Playlist", "No .mp3 files found in this folder.", nullptr, "OK", "", true);
        }

    } else { // Is a single track
        player.play(item.path);
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