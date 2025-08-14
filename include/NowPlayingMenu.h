// KIVA/include/NowPlayingMenu.h
#ifndef NOW_PLAYING_MENU_H
#define NOW_PLAYING_MENU_H

#include "IMenu.h"
#include "MusicPlayer.h" // Needed for MusicPlayer::State

class NowPlayingMenu : public IMenu {
public:
    NowPlayingMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Now Playing"; }
    MenuType getMenuType() const override { return MenuType::NOW_PLAYING; }

private:
    // Marquee state
    char marqueeText_[64];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;

    // --- NEW MEMBERS FOR DELAYED PLAYBACK TRIGGER ---
    unsigned long entryTime_;
    bool playbackTriggered_;
    bool _serviceRequestPending; // Flag to delay servicing track change by one update loop

    // --- NEW MEMBER FOR VOLUME DISPLAY ---
    unsigned long volumeDisplayUntil_;

    // --- NEW MEMBER FOR AUTO-NEXT ---
    MusicPlayer::State lastPlayerState_;
};

#endif // NOW_PLAYING_MENU_H