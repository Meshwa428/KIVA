#include "SongListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "MusicLibraryManager.h" // For INDEX_FILENAME constant
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Config.h"
#include "Event.h"
#include "EventDispatcher.h"

SongListDataSource::SongListDataSource() {}

void SongListDataSource::setPlaylistPath(const std::string& path) {
    playlistPath_ = path;
    size_t lastSlash = path.find_last_of('/');
    playlistName_ = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : "Songs";
}

void SongListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    // This is now the ONLY place where we load data.
    loadTracksFromIndex();
}

void SongListDataSource::onExit(App* app, ListMenu* menu) {
    tracks_.clear();
}

// --- THIS IS THE CORRECT, FAST IMPLEMENTATION ---
void SongListDataSource::loadTracksFromIndex() {
    tracks_.clear();
    if (playlistPath_.empty()) return;

    // Construct the path to the index file *inside* the chosen playlist directory.
    std::string indexPath = playlistPath_ + "/" + MusicLibraryManager::INDEX_FILENAME;

    auto reader = SdCardManager::openLineReader(indexPath.c_str());
    if (!reader.isOpen()) {
        // Fallback or error handling can go here if needed
        return;
    }

    while (true) {
        String line = reader.readLine();
        if (line.isEmpty()) break;

        // Only parse lines for Tracks ('T')
        if (line.startsWith("T;")) {
            int firstSep = line.indexOf(';');
            int secondSep = line.indexOf(';', firstSep + 1);
            if (firstSep != -1 && secondSep != -1) {
                tracks_.push_back({
                    line.substring(firstSep + 1, secondSep).c_str(),
                    line.substring(secondSep + 1).c_str()
                });
            }
        }
    }
    reader.close();
}

int SongListDataSource::getNumberOfItems(App* app) {
    return tracks_.size();
}

void SongListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= tracks_.size()) return;
    const auto& selectedTrack = tracks_[index];

    std::vector<std::string> playlistPaths;
    for (const auto& track : tracks_) {
        playlistPaths.push_back(track.path);
    }
    
    app->getMusicPlayer().queuePlaylist(playlistName_, playlistPaths, index);
    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::NOW_PLAYING));
}

void SongListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= tracks_.size()) return;
    const auto& item = tracks_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, IconType::MUSIC_NOTE);
    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, item.name.c_str(), text_x, text_y, text_w, isSelected);
}