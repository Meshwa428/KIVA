#ifndef MUSIC_LIBRARY_MANAGER_H
#define MUSIC_LIBRARY_MANAGER_H

#include <string>
#include "SdCardManager.h"

class App;

#include "Service.h"

class MusicLibraryManager : public Service {
public:
    MusicLibraryManager();
    void setup(App* app) override;

    // Main indexing function to be called at boot
    void buildIndex();

    // The name of our index files
    static constexpr const char* INDEX_FILENAME = "_index.txt";

private:
    // Recursive function to create index files in each directory
    void createIndexForDirectory(const char* path);

    App* app_;
};

#endif // MUSIC_LIBRARY_MANAGER_H