#include "MusicPlayer.h"
#include "App.h"
#include "SdCardManager.h"
#include <algorithm>
#include <random>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputPDM.h"

MusicPlayer::MusicPlayer() : 
    app_(nullptr), mp3_(nullptr), file_(nullptr), out_(nullptr),
    mp3_preAlloc_(nullptr),
    currentState_(State::STOPPED), repeatMode_(RepeatMode::REPEAT_OFF), isShuffle_(false),
    playlistTrackIndex_(-1),
    audioTaskHandle_(nullptr),
    playRequestPending_(false),
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
        audioTaskHandle_ = nullptr;
    }
    cleanup();
    if (mp3_preAlloc_) {
        free(mp3_preAlloc_);
        mp3_preAlloc_ = nullptr;
    }
}

void MusicPlayer::setup(App* app) {
    app_ = app;
    pinMode(Pins::AMPLIFIER_PIN, OUTPUT);
    digitalWrite(Pins::AMPLIFIER_PIN, LOW);

    xTaskCreatePinnedToCore(
        audioTaskWrapper,     
        "AudioPlayerTask",    
        4096,                 
        this,                 
        5,                    
        &audioTaskHandle_,    
        1                     // Pin to Core 1
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
    LOG(LogLevel::INFO, "PLAYER", "Audio task loop started.");
    for (;;) {
        if (playRequestPending_) {
            playRequestPending_ = false; 
            if (!nextPlaylistTracks_.empty()) {
                currentPlaylist_ = nextPlaylistTracks_;
                playlistName_ = nextPlaylistName_;
                // --- USE THE START INDEX ---
                playlistTrackIndex_ = nextPlaylistStartIndex_; 
                
                if (isShuffle_) {
                    generateShuffledIndices();
                    // If shuffle is on, we need to find where our target track ended up
                    std::string targetPath = currentPlaylist_[playlistTrackIndex_];
                    for(size_t i = 0; i < shuffledIndices_.size(); ++i) {
                        if (currentPlaylist_[shuffledIndices_[i]] == targetPath) {
                            playlistTrackIndex_ = i;
                            break;
                        }
                    }
                    startPlayback(currentPlaylist_[shuffledIndices_[playlistTrackIndex_]]);
                } else {
                    startPlayback(currentPlaylist_[playlistTrackIndex_]);
                }
            } else {
                startPlayback(nextTrackToPlay_);
            }
        }

        if (currentState_ == State::PLAYING && mp3_ && mp3_->isRunning()) {
            if (!mp3_->loop()) {
                playNextInPlaylist();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void MusicPlayer::play(const std::string& path) {
    stop();
    currentPlaylist_.clear();
    playlistName_ = "Single Track";
    
    nextTrackToPlay_ = path;
    nextPlaylistTracks_.clear();
    nextPlaylistStartIndex_ = 0;
    currentState_ = State::LOADING;
    playRequestPending_ = true;
}

// THIS METHOD IS RENAMED AND MODIFIED
void MusicPlayer::queuePlaylist(const std::string& name, const std::vector<std::string>& tracks, int startIndex) {
    if (tracks.empty() || startIndex >= (int)tracks.size()) return;
    stop();

    nextPlaylistName_ = name;
    nextPlaylistTracks_ = tracks;
    nextTrackToPlay_ = "";
    nextPlaylistStartIndex_ = startIndex;
    
    // Set the state to LOADING so the UI can show the correct message immediately,
    // but DO NOT set playRequestPending_ here.
    currentState_ = State::LOADING; 
}

// THIS IS THE NEW METHOD
void MusicPlayer::startQueuedPlayback() {
    // This is the only place we trigger the audio task to start a playlist.
    if (currentState_ == State::LOADING) {
        playRequestPending_ = true;
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
        LOG(LogLevel::INFO, "PLAYER", "Stopping playback.");
        playRequestPending_ = false; // Cancel any pending request
        currentState_ = State::PAUSED;
        vTaskDelay(pdMS_TO_TICKS(10));
        cleanup();
        currentState_ = State::STOPPED;
        currentTrackPath_ = "";
        currentTrackName_ = "";
        playlistName_ = "";
        currentPlaylist_.clear();
        playlistTrackIndex_ = -1;
    }
}

void MusicPlayer::startPlayback(const std::string& path) {
    cleanup();
    if (!mp3_preAlloc_) {
        LOG(LogLevel::ERROR, "PLAYER", "Playback failed, no memory for decoder.");
        currentState_ = State::STOPPED;
        return;
    }

    LOG(LogLevel::INFO, "PLAYER", "Starting playback for: %s", path.c_str());

    file_ = new AudioFileSourceSD(path.c_str());
    if (!file_->isOpen()) {
        LOG(LogLevel::ERROR, "PLAYER", "Failed to open audio file.");
        delete file_; file_ = nullptr;
        currentState_ = State::STOPPED;
        return;
    }

    enableAmplifier(true);
    out_ = new AudioOutputPDM(Pins::AMPLIFIER_PIN);
    out_->SetGain(2.0f);
    
    mp3_ = new AudioGeneratorMP3(mp3_preAlloc_, AudioGeneratorMP3::preAllocSize());
    
    // THIS BLOCKING CALL IS NOW SAFELY ON CORE 1
    if (mp3_->begin(file_, out_)) {
        currentState_ = State::PLAYING; // Update state on success
        currentTrackPath_ = path;
        size_t last_slash = path.find_last_of('/');
        currentTrackName_ = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    } else {
        LOG(LogLevel::ERROR, "PLAYER", "Failed to start MP3 generator.");
        cleanup();
        currentState_ = State::STOPPED; // Update state on failure
    }
}

void MusicPlayer::nextTrack() {
    if (currentPlaylist_.empty()) return;
    playNextInPlaylist(false);
}

void MusicPlayer::prevTrack() {
    if (currentPlaylist_.empty() || playlistTrackIndex_ < 0) return;
    
    playlistTrackIndex_--;
    if (playlistTrackIndex_ < 0) {
        playlistTrackIndex_ = currentPlaylist_.size() - 1; // Wrap around
    }
    
    stop();
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

void MusicPlayer::cleanup() {
    enableAmplifier(false);
    if (mp3_) {
        if (mp3_->isRunning()) mp3_->stop();
        delete mp3_; mp3_ = nullptr;
    }
    file_ = nullptr;
    out_ = nullptr;
}

void MusicPlayer::playNextInPlaylist(bool songFinishedNaturally) {
    if (currentPlaylist_.empty()) {
        stop();
        return;
    }

    if (songFinishedNaturally && repeatMode_ == RepeatMode::REPEAT_ONE) {
        stop();
        startPlayback(currentTrackPath_);
        return;
    }
    
    playlistTrackIndex_++;
    
    if (playlistTrackIndex_ >= (int)currentPlaylist_.size()) {
        if (repeatMode_ == RepeatMode::REPEAT_ALL) {
            playlistTrackIndex_ = 0;
            if(isShuffle_) generateShuffledIndices();
        } else {
            stop();
            return;
        }
    }
    
    stop(); 
    const std::string& path = isShuffle_ ? currentPlaylist_[shuffledIndices_[playlistTrackIndex_]] : currentPlaylist_[playlistTrackIndex_];
    startPlayback(path);
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
    if (currentState_ != State::STOPPED && file_ && file_->isOpen()) {
        uint32_t pos = file_->getPos();
        uint32_t size = file_->getSize();
        if (size > 0) {
            return (float)pos / (float)size;
        }
    }
    return 0.0f;
}