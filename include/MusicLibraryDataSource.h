#ifndef MUSIC_LIBRARY_DATA_SOURCE_H
#define MUSIC_LIBRARY_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <vector>
#include <string>

// Class name updated
class MusicLibraryDataSource : public IListMenuDataSource {
public:
    MusicLibraryDataSource();

    int getNumberOfItems(App* app) override;
    bool drawCustomEmptyMessage(App* app, U8G2& display) override; 
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int h, int w, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;
    void onUpdate(App* app, ListMenu* menu) override;

private:
    enum class ItemType { PLAYLIST, REINDEX };

    struct PlaylistItem {
        std::string name;
        std::string path;
        ItemType type;
    };

    void loadPlaylists();

    std::vector<PlaylistItem> items_;
    bool isReindexing_ = false;
    bool needsReload_ = false;
};

#endif // MUSIC_LIBRARY_DATA_SOURCE_H