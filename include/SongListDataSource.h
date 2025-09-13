#ifndef SONG_LIST_DATA_SOURCE_H
#define SONG_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <vector>
#include <string>

class SongListDataSource : public IListMenuDataSource {
public:
    SongListDataSource();

    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;

    void setPlaylistPath(const std::string& path);
    const std::string& getPlaylistName() const { return playlistName_; } // Getter for the name

private:
    struct TrackItem {
        std::string name;
        std::string path;
    };

    void loadTracksFromIndex(); // Renamed to be specific

    std::vector<TrackItem> tracks_;
    std::string playlistPath_;
    std::string playlistName_;
};

#endif // SONG_LIST_DATA_SOURCE_H