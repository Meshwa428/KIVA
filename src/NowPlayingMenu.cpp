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
    volumeDisplayUntil_(0)
{}

void NowPlayingMenu::onEnter(App* app, bool isForwardNav) {
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
    volumeDisplayUntil_ = 0;
}

void NowPlayingMenu::onUpdate(App* app) {
    // We give a tiny delay before starting playback to ensure the UI has drawn its first frame.
    if (!playbackTriggered_ && (millis() - entryTime_ > 50)) {
        app->getMusicPlayer().startQueuedPlayback();
        playbackTriggered_ = true;
    }
}

void NowPlayingMenu::onExit(App* app) {
    // When leaving the "Now Playing" screen, stop the current song.
    app->getMusicPlayer().stop();
    // Release resources when leaving the player screen entirely.
    app->getMusicPlayer().releaseResources();
}

void NowPlayingMenu::handleInput(App* app, InputEvent event) {
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
            app->changeMenu(MenuType::BACK);
            break;
        default:
            break;
    }
}

void NowPlayingMenu::draw(App* app, U8G2& display) {
    auto& player = app->getMusicPlayer();
    auto state = player.getState();

    // 1. Playlist Name
    display.setFont(u8g2_font_5x7_tf);
    std::string playlistName = player.getPlaylistName();
    char* truncatedPlaylist = truncateText(playlistName.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncatedPlaylist)) / 2, STATUS_BAR_H + 7, truncatedPlaylist);

    // 2. Track Name (Marquee)
    display.setFont(u8g2_font_6x10_tf);
    std::string trackName;

    if (state == MusicPlayer::State::LOADING) {
        trackName = "Loading...";
        marqueeActive_ = false; 
    } else {
        trackName = player.getCurrentTrackName();
        if (trackName.empty()) trackName = "Stopped";
    }

    int text_w = 120;
    if (state != MusicPlayer::State::LOADING) {
        updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                      marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                      marqueeText_, marqueeTextLenPx_, trackName.c_str(), text_w, display);
    }
    
    display.setClipWindow(4, STATUS_BAR_H + 8, 124, STATUS_BAR_H + 22);
    if (marqueeActive_) {
        display.drawStr(4 + (int)marqueeOffset_, STATUS_BAR_H + 18, marqueeText_);
    } else {
        int text_width = display.getStrWidth(trackName.c_str());
        display.drawStr((display.getDisplayWidth() - text_width) / 2, STATUS_BAR_H + 18, trackName.c_str());
    }
    display.setMaxClipWindow();

    // 3. Progress Bar
    float progress = player.getPlaybackProgress();
    int barX = 14, barW = 100, barH = 5;
    display.drawFrame(barX, STATUS_BAR_H + 24, barW, barH);
    int fillW = progress * (barW - 2);
    if (fillW > 0) {
        display.drawBox(barX + 1, STATUS_BAR_H + 25, fillW, barH - 2);
    }

    // 4. Controls
    int controlsY = 50;
    drawCustomIcon(display, 30, controlsY - 8, IconType::PREV_TRACK);
    
    IconType playPauseIcon = (state == MusicPlayer::State::PLAYING) ? IconType::PAUSE : IconType::PLAY;
    drawCustomIcon(display, 57, controlsY - 8, playPauseIcon);
    
    drawCustomIcon(display, 84, controlsY - 8, IconType::NEXT_TRACK);

    // 5. Status Icons (Shuffle/Repeat)
    int statusY = 63 - IconSize::SMALL_HEIGHT;
    if (player.isShuffle()) {
        drawCustomIcon(display, 4, statusY, IconType::SHUFFLE, IconRenderSize::SMALL);
    }
    
    auto repeatMode = player.getRepeatMode();
    if (repeatMode == MusicPlayer::RepeatMode::REPEAT_ALL) {
        drawCustomIcon(display, 118, statusY, IconType::REPEAT, IconRenderSize::SMALL);
    } else if (repeatMode == MusicPlayer::RepeatMode::REPEAT_ONE) {
        drawCustomIcon(display, 118, statusY, IconType::REPEAT_ONE, IconRenderSize::SMALL);
    }
    
    // 6. Volume Overlay
    if (millis() < volumeDisplayUntil_) {
        uint8_t volume = app->getConfigManager().getSettings().volume;
        int volBarX = 24, volBarY = 25, volBarW = 80, volBarH = 12;
        
        drawRndBox(display, volBarX, volBarY, volBarW, volBarH, 2, true);
        display.setDrawColor(0);
        
        char volStr[8];
        snprintf(volStr, sizeof(volStr), "%d%%", volume);
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr(volBarX + (volBarW - display.getStrWidth(volStr)) / 2, volBarY + 10, volStr);
    }
}