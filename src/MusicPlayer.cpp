#include "MusicPlayer.h"
#include "App.h"
#include "SdCardManager.h"
#include <algorithm>
#include <random>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"

MusicPlayer::MusicPlayer() : 
    app_(nullptr), mp3_(nullptr), file_(nullptr), out_(nullptr),
    mp3_preAlloc_(nullptr),
    currentState_(State::STOPPED), repeatMode_(RepeatMode::REPEAT_OFF), isShuffle_(false),
    playlistTrackIndex_(-1),
    audioTaskHandle_(nullptr),
    pendingCommand_(AudioCommand::NONE),
    nextPlaylistStartIndex_(0)
{
    mp3_preAlloc_ = malloc(AudioGeneratorMP3::preAllocSize());
    if (!mp3_preAlloc_) {
        LOG(LogLevel::ERROR, "PLAYER", "FATAL: Could not pre-allocate memory for MP3 decoder!");
    }
}

MusicPlayer::~MusicPlayer() {
    if (audioTaskHandle_ != nullptr) {
        vTaskDelete(audioTaskHandle_);
    }
    if (mp3_) { delete mp3_; mp3_ = nullptr; }
    if (file_) { delete file_; file_ = nullptr; }
    if (out_) { delete out_; out_ = nullptr; }
    if (mp3_preAlloc_) { free(mp3_preAlloc_); mp3_preAlloc_ = nullptr; }
}

void MusicPlayer::setup(App* app) {
    app_ = app;
    pinMode(Pins::AMPLIFIER_PIN, OUTPUT);
    digitalWrite(Pins::AMPLIFIER_PIN, LOW);

    out_ = new AudioOutputPDM(Pins::AMPLIFIER_PIN);
    out_->SetGain(0.5f);
    mp3_ = new AudioGeneratorMP3(mp3_preAlloc_, AudioGeneratorMP3::preAllocSize());

    xTaskCreatePinnedToCore(
        audioTaskWrapper, "AudioPlayerTask", 8192, this, 5, &audioTaskHandle_, 1
    );
    if (audioTaskHandle_ == nullptr) {
        LOG(LogLevel::ERROR, "PLAYER", "FATAL: Failed to create audio task!");
    } else {
        LOG(LogLevel::INFO, "PLAYER", "Audio task created and pinned to Core 1.");
    }
}

void MusicPlayer::audioTaskWrapper(void* param) {
    static_cast<MusicPlayer*>(param)->audioTaskLoop();
}

void MusicPlayer::audioTaskLoop() {
    LOG(LogLevel::INFO, "PLAYER_TASK", "Audio task loop started on Core %d.", xPortGetCoreID());
    unsigned long last_heartbeat = 0;

    for (;;) {
        // Heartbeat log to show the loop is running, even if nothing else happens
        if (millis() - last_heartbeat > 2000) {
            LOG(LogLevel::DEBUG, "PLAYER_TASK", "Heartbeat. State: %d, isRunning: %d", (int)currentState_, (mp3_ ? mp3_->isRunning() : 0));
            last_heartbeat = millis();
        }

        if (pendingCommand_ != AudioCommand::NONE) {
            LOG(LogLevel::INFO, "PLAYER_TASK", "[L1] Pending command detected: %d", (int)pendingCommand_);
            AudioCommand cmd = pendingCommand_;
            pendingCommand_ = AudioCommand::NONE;
            LOG(LogLevel::INFO, "PLAYER_TASK", "[L2] Command consumed. Processing...");

            switch (cmd) {
                case AudioCommand::PLAY_QUEUED:
                    LOG(LogLevel::INFO, "PLAYER_TASK", "[L3] Matched PLAY_QUEUED.");
                    if (!nextPlaylistTracks_.empty()) {
                        LOG(LogLevel::INFO, "PLAYER_TASK", "[L4] Playlist is not empty. Assigning...");
                        currentPlaylist_ = nextPlaylistTracks_;
                        playlistName_ = nextPlaylistName_;
                        playlistTrackIndex_ = nextPlaylistStartIndex_;
                        if (isShuffle_) {
                            LOG(LogLevel::INFO, "PLAYER_TASK", "[L5] Shuffle is ON. Generating indices and starting playback.");
                            generateShuffledIndices();
                            std::string targetPath = currentPlaylist_[playlistTrackIndex_];
                            for (size_t i = 0; i < shuffledIndices_.size(); ++i) {
                                if (currentPlaylist_[shuffledIndices_[i]] == targetPath) {
                                    playlistTrackIndex_ = i; break;
                                }
                            }
                            startPlayback(currentPlaylist_[shuffledIndices_[playlistTrackIndex_]]);
                        } else {
                            LOG(LogLevel::INFO, "PLAYER_TASK", "[L6] Shuffle is OFF. Starting playback.");
                            startPlayback(currentPlaylist_[playlistTrackIndex_]);
                        }
                    } else {
                        LOG(LogLevel::WARN, "PLAYER_TASK", "[L7] PLAY_QUEUED command received, but playlist was empty.");
                    }
                    break;
                case AudioCommand::STOP:
                    LOG(LogLevel::INFO, "PLAYER_TASK", "[L8] Matched STOP.");
                    cleanupCurrentTrack();
                    currentState_ = State::STOPPED;
                    currentTrackPath_ = ""; currentTrackName_ = ""; playlistName_ = "";
                    currentPlaylist_.clear(); playlistTrackIndex_ = -1;
                    break;
                case AudioCommand::NEXT:
                    LOG(LogLevel::INFO, "PLAYER_TASK", "[L9] Matched NEXT. Calling playNextInPlaylist.");
                    playNextInPlaylist(false);
                    break;
                case AudioCommand::PREV:
                    LOG(LogLevel::INFO, "PLAYER_TASK", "[L10] Matched PREV.");
                    if (!currentPlaylist_.empty() && playlistTrackIndex_ >= 0) {
                        LOG(LogLevel::INFO, "PLAYER_TASK", "[L11] Playlist not empty. Decrementing index.");
                        playlistTrackIndex_--;
                        if (playlistTrackIndex_ < 0) playlistTrackIndex_ = currentPlaylist_.size() - 1;
                        const std::string& path = isShuffle_ ? currentPlaylist_[shuffledIndices_[playlistTrackIndex_]] : currentPlaylist_[playlistTrackIndex_];
                        LOG(LogLevel::INFO, "PLAYER_TASK", "[L12] Starting playback for previous track.");
                        startPlayback(path);
                    } else {
                        LOG(LogLevel::WARN, "PLAYER_TASK", "[L13] PREV command ignored, playlist empty or index invalid.");
                    }
                    break;
                default:
                    LOG(LogLevel::WARN, "PLAYER_TASK", "[L14] Unknown command in switch: %d", (int)cmd);
                    break;
            }
            LOG(LogLevel::INFO, "PLAYER_TASK", "[L15] Finished processing command.");
        }

        if (currentState_ == State::PLAYING && mp3_ && mp3_->isRunning()) {
            // This is the most performance-critical part, so logging is minimal here.
            if (!mp3_->loop()) {
                LOG(LogLevel::INFO, "PLAYER_TASK", "[L16] mp3->loop() returned false. Song has finished.");
                playNextInPlaylist(true);
            }
        } else {
            // If not playing, log why not.
            if (millis() - last_heartbeat > 1990) { // Throttled log
                 LOG(LogLevel::DEBUG, "PLAYER_TASK", "Skipping mp3->loop(). State=%d, mp3_valid=%d, isRunning=%d", 
                    (int)currentState_, (mp3_ != nullptr), (mp3_ ? mp3_->isRunning() : 0) );
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void MusicPlayer::queuePlaylist(const std::string& name, const std::vector<std::string>& tracks, int startIndex) {
    if (tracks.empty() || startIndex >= (int)tracks.size()) return;
    currentState_ = State::LOADING;
    nextPlaylistName_ = name;
    nextPlaylistTracks_ = tracks;
    nextPlaylistStartIndex_ = startIndex;
}

void MusicPlayer::startQueuedPlayback() {
    if (currentState_ == State::LOADING) {
        LOG(LogLevel::INFO, "PLAYER", "Queued playback requested.");
        pendingCommand_ = AudioCommand::PLAY_QUEUED;
    }
}

void MusicPlayer::pause() {
    if (currentState_ == State::PLAYING) {
        currentState_ = State::PAUSED;
        enableAmplifier(false);
    }
}

void MusicPlayer::resume() {
    if (currentState_ == State::PAUSED) {
        currentState_ = State::PLAYING;
        enableAmplifier(true);
    }
}

void MusicPlayer::stop() {
    if (currentState_ != State::STOPPED) {
        pendingCommand_ = AudioCommand::STOP;
    }
}

void MusicPlayer::nextTrack() {
    if (!currentPlaylist_.empty()) {
        LOG(LogLevel::INFO, "PLAYER", "nextTrack() called. Setting pending command.");
        pendingCommand_ = AudioCommand::NEXT;
    }
}

void MusicPlayer::prevTrack() {
    if (!currentPlaylist_.empty()) {
        LOG(LogLevel::INFO, "PLAYER", "prevTrack() called. Setting pending command.");
        pendingCommand_ = AudioCommand::PREV;
    }
}

void MusicPlayer::cleanupCurrentTrack() {
    LOG(LogLevel::INFO, "PLAYER", "cleanupCurrentTrack: Starting...");
    if (mp3_ && mp3_->isRunning()) {
        LOG(LogLevel::INFO, "PLAYER", "cleanupCurrentTrack: MP3 is running, calling stop().");
        mp3_->stop();
        // --- THIS IS THE CRITICAL FIX ---
        // This delay gives the I2S DMA buffer time to flush and the driver
        // to enter a stable, stopped state before we try to re-initialize it.
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
    if (file_) {
        file_->close();
        delete file_;
        file_ = nullptr;
    }
    enableAmplifier(false);
    LOG(LogLevel::INFO, "PLAYER", "cleanupCurrentTrack: Finished.");
}

void MusicPlayer::startPlayback(const std::string& path) {
    cleanupCurrentTrack();
    
    LOG(LogLevel::INFO, "PLAYER", "startPlayback: Starting for: %s", path.c_str());
    file_ = new AudioFileSourceSD(path.c_str());
    
    if (!file_ || !file_->isOpen()) {
        LOG(LogLevel::ERROR, "PLAYER", "startPlayback: Failed to open audio file: %s", path.c_str());
        if (file_) { delete file_; file_ = nullptr; }
        currentState_ = State::STOPPED;
        playNextInPlaylist(false);
        return;
    }

    enableAmplifier(true);
    bool success = mp3_ && mp3_->begin(file_, out_);
    LOG(LogLevel::INFO, "PLAYER", "startPlayback: mp3->begin() returned %s", success ? "true" : "false");

    if (success) {
        currentState_ = State::PLAYING;
        currentTrackPath_ = path;
        size_t last_slash = path.find_last_of('/');
        currentTrackName_ = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    } else {
        LOG(LogLevel::ERROR, "PLAYER", "startPlayback: Failed to start MP3 generator for: %s", path.c_str());
        cleanupCurrentTrack();
        currentState_ = State::STOPPED;
    }
}

void MusicPlayer::playNextInPlaylist(bool songFinishedNaturally) {
    LOG(LogLevel::INFO, "PLAYER", "playNextInPlaylist: Entered. Finished naturally: %d", songFinishedNaturally);
    if (currentPlaylist_.empty()) {
        pendingCommand_ = AudioCommand::STOP;
        return;
    }
    if (songFinishedNaturally && repeatMode_ == RepeatMode::REPEAT_ONE) {
        startPlayback(currentTrackPath_);
        return;
    }
    
    playlistTrackIndex_++;
    LOG(LogLevel::INFO, "PLAYER", "playNextInPlaylist: New index: %d", playlistTrackIndex_);
    
    if (playlistTrackIndex_ >= (int)currentPlaylist_.size()) {
        if (repeatMode_ == RepeatMode::REPEAT_ALL) {
            playlistTrackIndex_ = 0;
            if (isShuffle_) generateShuffledIndices();
        } else {
            pendingCommand_ = AudioCommand::STOP;
            return;
        }
    }
    
    const std::string& path = isShuffle_ ? currentPlaylist_[shuffledIndices_[playlistTrackIndex_]] : currentPlaylist_[playlistTrackIndex_];
    startPlayback(path);
}

void MusicPlayer::toggleShuffle() {
    isShuffle_ = !isShuffle_;
    if (isShuffle_ && !currentPlaylist_.empty()) {
        generateShuffledIndices();
        for(size_t i = 0; i < shuffledIndices_.size(); ++i) {
            if (currentPlaylist_[shuffledIndices_[i]] == currentTrackPath_) {
                playlistTrackIndex_ = i;
                break;
            }
        }
    }
}

void MusicPlayer::cycleRepeatMode() {
    repeatMode_ = static_cast<RepeatMode>((static_cast<int>(repeatMode_) + 1) % 3);
}

void MusicPlayer::generateShuffledIndices() {
    shuffledIndices_.resize(currentPlaylist_.size());
    for(size_t i=0; i < shuffledIndices_.size(); ++i) shuffledIndices_[i] = i;
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(shuffledIndices_.begin(), shuffledIndices_.end(), g);
}

void MusicPlayer::enableAmplifier(bool enable) {
    digitalWrite(Pins::AMPLIFIER_PIN, enable ? HIGH : LOW);
}

MusicPlayer::State MusicPlayer::getState() const { return currentState_; }
MusicPlayer::RepeatMode MusicPlayer::getRepeatMode() const { return repeatMode_; }
bool MusicPlayer::isShuffle() const { return isShuffle_; }
std::string MusicPlayer::getCurrentTrackName() const { return currentTrackName_; }
std::string MusicPlayer::getPlaylistName() const { return playlistName_; }

float MusicPlayer::getPlaybackProgress() const {
    if (currentState_ == State::PLAYING && file_ && file_->isOpen()) {
        uint32_t pos = file_->getPos();
        uint32_t size = file_->getSize();
        if (size > 0) {
            return (float)pos / (float)size;
        }
    }
    return 0.0f;
}