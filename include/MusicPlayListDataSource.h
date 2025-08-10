#ifndef MUSIC_PLAYLIST_DATA_SOURCE_H
#define MUSIC_PLAYLIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <vector>
#include <string>

class MusicPlayListDataSource : public IListMenuDataSource {
public:
    MusicPlayListDataSource(); // Add constructor

    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int h, int w, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;

private:
    struct PlaylistItem {
        std::string name;
        std::string path;
        bool isPlaylist;
    };

    void loadItemsFromPath(const char* directoryPath);

    std::vector<PlaylistItem> items_;
    std::string currentPath_; // The directory we are currently viewing
};

#endif // MUSIC_PLAYLIST_DATA_SOURCE_H