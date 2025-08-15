#ifndef MUSIC_PLAYLIST_DATA_SOURCE_H
#define MUSIC_PLAYLIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <vector>
#include <string>

class MusicPlayListDataSource : public IListMenuDataSource {
public:
    MusicPlayListDataSource();

    int getNumberOfItems(App* app) override;
    // Add the override for the custom empty message
    bool drawCustomEmptyMessage(App* app, U8G2& display) override; 
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int h, int w, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;
    void onUpdate(App* app, ListMenu* menu) override; // Add override specifier

    // Handle back button presses for directory navigation
    bool onBackPress(App* app, ListMenu* menu) override;

private:
    enum class ItemType { TRACK, PLAYLIST, REINDEX };

    struct PlaylistItem {
        std::string name;
        std::string path;
        ItemType type;
    };

    void loadItemsFromPath(const char* directoryPath);

    std::vector<PlaylistItem> items_;
    std::string currentPath_; 
    bool isReindexing_ = false;
    bool needsReload_ = false; // Flag to fix loading bug
};

#endif // MUSIC_PLAYLIST_DATA_SOURCE_H