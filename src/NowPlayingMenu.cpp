#include "Event.h"
#include "EventDispatcher.h"
#include "NowPlayingMenu.h"
#include "App.h"
#include "MusicPlayer.h"
#include "UI_Utils.h"
#include "Logger.h"

NowPlayingMenu::NowPlayingMenu() :
    marqueeActive_(false),
    marqueeScrollLeft_(true),
    entryTime_(0),
    playbackTriggered_(false),
    _serviceRequestPending(false),
    volumeDisplayUntil_(0),
    _songFinished(false)
{}

void NowPlayingMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    // Allocate resources when entering the player screen
    if (!app->getMusicPlayer().allocateResources()) {
        app->showPopUp("Error", "Could not init audio.", [app](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "OK", "", false);
        return;
    }

    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
    entryTime_ = millis();
    playbackTriggered_ = false;
    _serviceRequestPending = false;
    volumeDisplayUntil_ = 0;
    _songFinished = false;

    // Register callback to know when a song finishes
    app->getMusicPlayer().setSongFinishedCallback([this](){
        this->_songFinished = true;
    });
}

void NowPlayingMenu::onUpdate(App* app) {
    // We give a tiny delay before starting playback to ensure the UI has drawn its first frame.
    if (!playbackTriggered_ && (millis() - entryTime_ > 50)) {
        app->getMusicPlayer().startQueuedPlayback();
        playbackTriggered_ = true;
    }

    // --- Deferred track change logic ---
    auto& player = app->getMusicPlayer();
    bool requestIsPending = player.getRequestedAction() != MusicPlayer::PlaybackAction::NONE;

    // If we have a pending request to service, do it now.
    if (_serviceRequestPending) {
        player.serviceRequest();
        _serviceRequestPending = false; // Reset the flag
    }
    // If a new request was just made, set the flag to service it on the *next* update cycle.
    else if (requestIsPending) {
        _serviceRequestPending = true;
    }

    // --- Auto-play next song (via callback) ---
    if (_songFinished) {
        _songFinished = false;
        player.songFinished();
    }
}

void NowPlayingMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    // When leaving the "Now Playing" screen, stop the current song.
    app->getMusicPlayer().stop();
    // Release resources when leaving the player screen entirely.
    app->getMusicPlayer().releaseResources();
    // Un-register the callback
    app->getMusicPlayer().setSongFinishedCallback(nullptr);
}

void NowPlayingMenu::handleInput(InputEvent event, App* app) {
    auto& player = app->getMusicPlayer();
    switch (event) {
        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS:
            if (player.getState() == MusicPlayer::State::PLAYING) player.pause();
            else player.resume();
            break;
        case InputEvent::BTN_LEFT_PRESS:
        case InputEvent::ENCODER_CCW:
            LOG(LogLevel::INFO, "UI", "Next/Prev: Calling player.prevTrack()"); 
            player.prevTrack();
            app->clearInputQueue();
            break;
        case InputEvent::BTN_RIGHT_PRESS:
        case InputEvent::ENCODER_CW:
            LOG(LogLevel::INFO, "UI", "Next/Prev: Calling player.nextTrack()");
            player.nextTrack();
            app->clearInputQueue();
            break;
        case InputEvent::BTN_UP_PRESS:
        case InputEvent::BTN_DOWN_PRESS:
            {
                auto& settings = app->getConfigManager().getSettings();
                int newVolume = settings.volume;
                if (event == InputEvent::BTN_UP_PRESS) newVolume += 5;
                else {
                    if (newVolume <= 10) newVolume -= 2;
                    else newVolume -= 5;
                }
                
                if (newVolume < 0) newVolume = 0;
                if (newVolume > 200) newVolume = 200;
                
                settings.volume = newVolume;
                app->getConfigManager().saveSettings(); // This also calls applySettings() which calls player.setVolume()
                volumeDisplayUntil_ = millis() + 2000; // Show volume for 2 seconds
            }
            break;
        case InputEvent::BTN_A_PRESS:
            player.cycleRepeatMode();
            break;
        case InputEvent::BTN_B_PRESS:
            player.toggleShuffle();
            break;
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
            break;
        default:
            break;
    }
}

void NowPlayingMenu::draw(App* app, U8G2& display) {
    auto& player = app->getMusicPlayer();
    auto state = player.getState();
    const int topY = STATUS_BAR_H;
    const int DISP_W = 128;

    // --- Playlist Name ---
    display.setFont(u8g2_font_5x7_tf);
    std::string playlistName = player.getPlaylistName();
    char* truncatedPlaylist = truncateText(playlistName.c_str(), 124, display);
    display.drawStr((DISP_W - display.getStrWidth(truncatedPlaylist)) / 2, topY + 7, truncatedPlaylist);
    display.drawHLine(0, topY + 9, DISP_W);

    // --- Track Title (Marquee) ---
    display.setFont(u8g2_font_6x10_tf);
    std::string trackName = player.isLoadingTrack() ? "Loading..." : player.getCurrentTrackName();
    if (trackName.empty()) trackName = "Stopped";

    // The marquee should not run when "Loading..." is displayed
    if (!player.isLoadingTrack()) {
        updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_,
                      marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_,
                      marqueeText_, marqueeTextLenPx_, trackName.c_str(), 120, display);
    } else {
        marqueeActive_ = false; // Stop marquee if it was running
    }
    display.setClipWindow(4, topY + 12, 124, topY + 26);
    if (marqueeActive_) {
        display.drawStr(4 + (int)marqueeOffset_, topY + 22, marqueeText_);
    } else {
        int text_width = display.getStrWidth(trackName.c_str());
        display.drawStr((DISP_W - text_width) / 2, topY + 22, trackName.c_str());
    }
    display.setMaxClipWindow();

    // --- Progress Bar ---
    int barX = 14, barW = 100, barH = 5, barY = topY + 28;
    drawRndBox(display, barX, barY, barW, barH, 2, false);
    int fillW = (int)(player.getPlaybackProgress() * (barW - 2));
    if (fillW > 0) {
        drawRndBox(display, barX + 1, barY + 1, fillW, barH - 2, 2, true);
    }

    // --- Playback Controls + Shuffle/Repeat on same row (no overlap) ---
    int controlsY = topY + 38; // row dedicated to 15x15 icons
    const int ICON_W = 15;

    // Shuffle (left-most)
    if (player.isShuffle()) {
        drawCustomIcon(display, 4, controlsY, IconType::SHUFFLE);
    }

    // Prev / Play / Next (centered-ish)
    drawCustomIcon(display, 20, controlsY, IconType::PREV_TRACK);
    IconType playPauseIcon = (state == MusicPlayer::State::PLAYING) ? IconType::PAUSE : IconType::PLAY;
    drawCustomIcon(display, 56, controlsY, playPauseIcon);
    drawCustomIcon(display, 92, controlsY, IconType::NEXT_TRACK);

    // Repeat (right-most)
    auto repeatMode = player.getRepeatMode();
    if (repeatMode == MusicPlayer::RepeatMode::REPEAT_ALL) {
        drawCustomIcon(display, DISP_W - ICON_W - 4, controlsY, IconType::REPEAT);
    } else if (repeatMode == MusicPlayer::RepeatMode::REPEAT_ONE) {
        drawCustomIcon(display, DISP_W - ICON_W - 4, controlsY, IconType::REPEAT_ONE);
    }

    // --- Volume Overlay (white fill + 1px black rounded border) ---
    if (millis() < volumeDisplayUntil_) {
        uint8_t volume = app->getConfigManager().getSettings().volume;
        int volBarW = 60, volBarH = 16;
        int volBarX = (DISP_W - volBarW) / 2;
        int volBarY = topY + 12; // near the marquee area like in your original aesthetic

        // White filled rounded box
        display.setDrawColor(1);
        drawRndBox(display, volBarX, volBarY, volBarW, volBarH, 3, true);

        // Black 1px rounded border
        display.setDrawColor(0);
        drawRndBox(display, volBarX, volBarY, volBarW, volBarH, 3, false);

        // Volume text in black
        char volStr[8];
        snprintf(volStr, sizeof(volStr), "%d%%", volume);
        display.setFont(u8g2_font_6x10_tf);
        int tx = volBarX + (volBarW - display.getStrWidth(volStr)) / 2;
        int ty = volBarY + 12; // matches previous alignment
        display.drawStr(tx, ty, volStr);

        // restore draw color
        display.setDrawColor(1);
    }
}

