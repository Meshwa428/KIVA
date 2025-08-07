#include "NowPlayingMenu.h"
#include "App.h"
#include "MusicPlayer.h"
#include "UI_Utils.h"

NowPlayingMenu::NowPlayingMenu() :
    marqueeActive_(false),
    marqueeScrollLeft_(true)
{}

void NowPlayingMenu::onEnter(App* app, bool isForwardNav) {
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
}

void NowPlayingMenu::onUpdate(App* app) {
    // UI is driven by MusicPlayer state, no specific updates needed here
}

void NowPlayingMenu::onExit(App* app) {
    // Stop music when leaving the player UI
    app->getMusicPlayer().stop();
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
            player.prevTrack();
            break;
        case InputEvent::BTN_RIGHT_PRESS:
        case InputEvent::ENCODER_CW:
            player.nextTrack();
            break;
        case InputEvent::ENCODER_CCW: // Overload encoder CCW for previous track too
            player.prevTrack();
            break;
        case InputEvent::BTN_UP_PRESS:
            player.cycleRepeatMode();
            break;
        case InputEvent::BTN_DOWN_PRESS:
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

    // 1. Playlist Name
    display.setFont(u8g2_font_5x7_tf);
    std::string playlistName = player.getPlaylistName();
    char* truncatedPlaylist = truncateText(playlistName.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncatedPlaylist)) / 2, STATUS_BAR_H + 7, truncatedPlaylist);

    // 2. Track Name (Marquee)
    display.setFont(u8g2_font_6x10_tf);
    std::string trackName = player.getCurrentTrackName();
    if (trackName.empty()) trackName = "Stopped";
    int text_w = 120;
    updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                  marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                  marqueeText_, marqueeTextLenPx_, trackName.c_str(), text_w, display);
    
    display.setClipWindow(4, STATUS_BAR_H + 8, 124, STATUS_BAR_H + 22);
    display.drawStr(4 + (int)marqueeOffset_, STATUS_BAR_H + 18, marqueeText_);
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
    
    IconType playPauseIcon = (player.getState() == MusicPlayer::State::PLAYING) ? IconType::PAUSE : IconType::PLAY;
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
}