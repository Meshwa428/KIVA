#include "Event.h"
#include "EventDispatcher.h"
#include "NowPlayingMenu.h"
#include "App.h"
#include "ConfigManager.h"
#include "MusicPlayer.h"
#include "UI_Utils.h"
#include "Logger.h"

// --- NEW HELPER FUNCTION ---
// Formats seconds into a MM:SS string
static void formatTime(char* buf, size_t len, int seconds) {
    if (seconds < 0) seconds = 0;
    snprintf(buf, len, "%d:%02d", seconds / 60, seconds % 60);
}

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
    if (!app->getMusicPlayer().allocateResources()) {
        app->showPopUp("Error", "Could not init audio.", [app](App* app_cb){
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }, "OK", "", false);
        return;
    }

    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
    entryTime_ = millis();
    playbackTriggered_ = false;
    volumeDisplayUntil_ = 0;
    _songFinished = false;

    app->getMusicPlayer().setSongFinishedCallback([this](){
        this->_songFinished = true;
    });
}

void NowPlayingMenu::onUpdate(App* app) {
    app->requestRedraw();
    if (!playbackTriggered_ && (millis() - entryTime_ > 50)) {
        app->getMusicPlayer().startQueuedPlayback();
        playbackTriggered_ = true;
    }

    auto& player = app->getMusicPlayer();

    if (player.getRequestedAction() != MusicPlayer::PlaybackAction::NONE) {
        player.serviceRequest();
    }

    if (_songFinished) {
        _songFinished = false;
        player.songFinished();
    }
}

void NowPlayingMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getMusicPlayer().stop();
    app->getMusicPlayer().releaseResources();
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
            player.prevTrack();
            break;
        case InputEvent::BTN_RIGHT_PRESS:
        case InputEvent::ENCODER_CW:
            player.nextTrack();
            break;
        case InputEvent::BTN_UP_PRESS:
        case InputEvent::BTN_DOWN_PRESS: {
            auto& settings = app->getConfigManager().getSettings();
            int currentVolume = settings.volume;
            int newVolume = currentVolume;
            int step = (currentVolume < 10) ? 2 : 5;
            newVolume += (event == InputEvent::BTN_UP_PRESS) ? step : -step;
            if (newVolume < 0) newVolume = 0;
            if (newVolume > 200) newVolume = 200;
            settings.volume = newVolume;
            app->getConfigManager().saveSettings();
            volumeDisplayUntil_ = millis() + 2000;
            break;
        }
        case InputEvent::BTN_A_PRESS:
            player.cycleRepeatMode();
            break;
        case InputEvent::BTN_B_PRESS:
            player.toggleShuffle();
            break;
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            break;
        default:
            break;
    }
}

bool NowPlayingMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& player = app->getMusicPlayer();
    std::string playlistName = player.getPlaylistName();

    if (playlistName.empty()) {
        playlistName = "Music Player";
    }

    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    char* truncated = truncateText(playlistName.c_str(), 124, display);
    int textWidth = display.getStrWidth(truncated);
    
    display.drawStr((128 - textWidth) / 2, 8, truncated);
    
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);
    return true;
}

void NowPlayingMenu::draw(App* app, U8G2& display) {
    auto& player = app->getMusicPlayer();
    auto state = player.getState();
    const int topY = STATUS_BAR_H;
    const int DISP_W = 128;

    display.setFont(u8g2_font_6x10_tf);
    std::string trackName = player.isLoadingTrack() ? "Loading..." : player.getCurrentTrackName();
    if (trackName.empty()) trackName = "Stopped";

    if (!player.isLoadingTrack()) {
        updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_,
                      marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_,
                      marqueeText_, sizeof(marqueeText_), marqueeTextLenPx_, trackName.c_str(), 120, display);
    } else {
        marqueeActive_ = false; 
    }
    
    int trackNameCenterY = topY + 9;
    int trackNameBaselineY = trackNameCenterY + 4;

    display.setClipWindow(4, trackNameCenterY - 6, 124, trackNameCenterY + 6);
    if (marqueeActive_) {
        display.drawStr(4 + (int)marqueeOffset_, trackNameBaselineY, marqueeText_);
    } else {
        int text_width = display.getStrWidth(trackName.c_str());
        display.drawStr((DISP_W - text_width) / 2, trackNameBaselineY, trackName.c_str());
    }
    display.setMaxClipWindow();

    int barY = topY + 28;
    int barX = 30, barW = 68, barH = 5;
    
    int currentTime = player.getCurrentTime();
    int totalDuration = player.getTotalDuration();
    // LOG(LogLevel::DEBUG, "UI_DRAW", false, "Player state: currentTime=%d, totalDuration=%d", currentTime, totalDuration);

    char currentTimeStr[8];
    char totalTimeStr[8];
    formatTime(currentTimeStr, sizeof(currentTimeStr), currentTime);
    formatTime(totalTimeStr, sizeof(totalTimeStr), totalDuration);

    display.setFont(u8g2_font_5x7_tf);
    display.drawStr(2, barY + 4, currentTimeStr);
    display.drawStr(DISP_W - display.getStrWidth(totalTimeStr) - 2, barY + 4, totalTimeStr);

    drawRndBox(display, barX, barY, barW, barH, 2, false);
    int fillW = (int)(player.getPlaybackProgress() * (barW - 2));
    if (fillW > 0) {
        drawRndBox(display, barX + 1, barY + 1, fillW, barH - 2, 2, true);
    }

    int controlsY = topY + 38;
    const int ICON_W = 15;
    if (player.isShuffle()) {
        drawCustomIcon(display, 4, controlsY, IconType::SHUFFLE);
    }
    drawCustomIcon(display, 20, controlsY, IconType::PREV_TRACK);
    IconType playPauseIcon = (state == MusicPlayer::State::PLAYING) ? IconType::PAUSE : IconType::PLAY;
    drawCustomIcon(display, 56, controlsY, playPauseIcon);
    drawCustomIcon(display, 92, controlsY, IconType::NEXT_TRACK);
    auto repeatMode = player.getRepeatMode();
    if (repeatMode == MusicPlayer::RepeatMode::REPEAT_ALL) {
        drawCustomIcon(display, DISP_W - ICON_W - 4, controlsY, IconType::REPEAT);
    } else if (repeatMode == MusicPlayer::RepeatMode::REPEAT_ONE) {
        drawCustomIcon(display, DISP_W - ICON_W - 4, controlsY, IconType::REPEAT_ONE);
    }

    if (millis() < volumeDisplayUntil_) {
        uint8_t volume = app->getConfigManager().getSettings().volume;
        int volBarW = 60, volBarH = 16;
        int volBarX = (DISP_W - volBarW) / 2;
        int volBarY = (display.getDisplayHeight() - volBarH) / 2;

        display.setDrawColor(1);
        drawRndBox(display, volBarX, volBarY, volBarW, volBarH, 3, true);

        display.setDrawColor(0);
        drawRndBox(display, volBarX, volBarY, volBarW, volBarH, 3, false);

        char volStr[8];
        snprintf(volStr, sizeof(volStr), "%d%%", volume);
        display.setFont(u8g2_font_6x10_tf);
        int tx = volBarX + (volBarW - display.getStrWidth(volStr)) / 2;
        int ty = volBarY + 12; 
        display.drawStr(tx, ty, volStr);

        display.setDrawColor(1);
    }
}