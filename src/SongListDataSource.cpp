#include "SongListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "MusicLibraryManager.h" // For INDEX_FILENAME constant
#include "MusicPlayer.h"         // For MusicPlayer::PlaylistTrack
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
    loadTracksFromIndex();
}

void SongListDataSource::onExit(App* app, ListMenu* menu) {
    tracks_.clear();
}

void SongListDataSource::loadTracksFromIndex() {
    tracks_.clear();
    if (playlistPath_.empty()) return;

    std::string indexPath = playlistPath_ + "/" + MusicLibraryManager::INDEX_FILENAME;

    auto reader = SdCardManager::getInstance().openLineReader(indexPath.c_str());
    if (!reader.isOpen()) {
        return;
    }

    while (true) {
        String line = reader.readLine();
        if (line.isEmpty()) break;

        if (line.startsWith("T;")) {
            int firstSep = line.indexOf(';');
            int secondSep = line.indexOf(';', firstSep + 1);
            int thirdSep = line.indexOf(';', secondSep + 1);
            if (firstSep != -1 && secondSep != -1 && thirdSep != -1) {
                TrackItem newItem;
                newItem.name = line.substring(firstSep + 1, secondSep).c_str();
                newItem.path = line.substring(secondSep + 1, thirdSep).c_str();
                newItem.duration = line.substring(thirdSep + 1).toInt();
                
                tracks_.push_back(newItem);
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

    std::vector<MusicPlayer::PlaylistTrack> playlistTracks;
    for (const auto& track : tracks_) {
        playlistTracks.push_back({track.path, track.duration});
    }
    
    app->getMusicPlayer().queuePlaylist(playlistName_, playlistTracks, index);
    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::NOW_PLAYING));
}

void SongListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= tracks_.size()) return;
    const auto& item = tracks_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, IconType::MUSIC_NOTE);
    
    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4 - 30; // Reserve 30px for duration
    
    menu->updateAndDrawText(display, item.name.c_str(), text_x, text_y, text_w, isSelected);

    if (item.duration > 0) {
        char durationStr[8];
        snprintf(durationStr, sizeof(durationStr), "%d:%02d", item.duration / 60, item.duration % 60);
        int durationW = display.getStrWidth(durationStr);
        display.drawStr(x + w - durationW - 4, text_y, durationStr);
    }
}