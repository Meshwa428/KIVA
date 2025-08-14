#include "MusicPlayer.h"
#include "App.h"
#include "SdCardManager.h"
#include <algorithm>
#include <random>
#include <chrono>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"

// ... (Constructor and other setup methods are unchanged) ...
MusicPlayer::MusicPlayer() : 
    app_(nullptr), resourcesAllocated_(false),
    out_(nullptr), mixer_(nullptr), currentSlot_(-1),
    currentState_(State::STOPPED), repeatMode_(RepeatMode::REPEAT_OFF), requestedAction_(PlaybackAction::NONE), isShuffle_(false),
    _isLoadingTrack(false), currentGain_(1.75f),
    playlistTrackIndex_(-1),
    mixerTaskHandle_(nullptr)
{
    for (int i = 0; i < 2; ++i) {
        file_[i] = nullptr;
        mp3_[i] = nullptr;
        stub_[i] = nullptr;
    }
}

MusicPlayer::~MusicPlayer() {
    releaseResources();
}

void MusicPlayer::setup(App* app) {
    app_ = app;
}

bool MusicPlayer::isServiceRunning() const {
    return mixerTaskHandle_ != nullptr;
}

bool MusicPlayer::allocateResources() {
    if (resourcesAllocated_) return true;
    LOG(LogLevel::INFO, "PLAYER", "Allocating audio resources (Mixer)...");
    app_->getHardwareManager().setAmplifier(true);
    out_ = new AudioOutputPDM(Pins::AMPLIFIER_PIN);
    mixer_ = new AudioOutputMixer(32, out_);
    BaseType_t result = xTaskCreatePinnedToCore(
        mixerTaskWrapper, "AudioMixerTask", 4096, this, 5, &mixerTaskHandle_, 1
    );
    if (result != pdPASS) {
        LOG(LogLevel::ERROR, "PLAYER", "FATAL: Failed to create mixer task!");
        delete mixer_; mixer_ = nullptr;
        delete out_; out_ = nullptr;
        app_->getHardwareManager().setAmplifier(false);
        return false;
    }
    resourcesAllocated_ = true;
    LOG(LogLevel::INFO, "PLAYER", "Audio resources allocated successfully.");
    return true;
}

void MusicPlayer::releaseResources() {
    if (!resourcesAllocated_) return;
    LOG(LogLevel::INFO, "PLAYER", "Releasing audio resources (Mixer)...");
    if (mixerTaskHandle_ != nullptr) {
        vTaskDelete(mixerTaskHandle_);
        mixerTaskHandle_ = nullptr;
    }
    stopPlayback();
    // Destructor of out_ will handle driver uninstall
    delete mixer_; mixer_ = nullptr;
    delete out_; out_ = nullptr;
    app_->getHardwareManager().setAmplifier(false);
    resourcesAllocated_ = false;
    LOG(LogLevel::INFO, "PLAYER", "Audio resources released.");
}

void MusicPlayer::mixerTaskWrapper(void* param) {
    static_cast<MusicPlayer*>(param)->mixerTaskLoop();
}

void MusicPlayer::mixerTaskLoop() {
    LOG(LogLevel::INFO, "PLAYER_TASK", "Mixer task started.");
    while (true) {
        bool any_running = false;

        for (int i = 0; i < 2; i++) {
            if (mp3_[i] == nullptr) {
                continue;
            }

            bool isCurrent = (i == currentSlot_);

            // This is the currently active track
            if (isCurrent) {
                // And we're in a playing state
                if (currentState_ == State::PLAYING && mp3_[i]->isRunning()) {
                    if (!mp3_[i]->loop()) {
                        // Song has finished naturally
                        mp3_[i]->stop();
                        currentState_ = State::STOPPED;
                    }
                    any_running = true;
                }
            }
            // This is an old track that needs to be stopped and/or cleaned up
            else if (currentSlot_ != -1) {
                if (mp3_[i]->isRunning()) {
                    // If it's still running, stop it first.
                    LOG(LogLevel::INFO, "PLAYER_TASK", "Stopping old slot %d", i);
                    mp3_[i]->stop();
                    any_running = true; // Still running for now, don't sleep
                } else {
                    // If it's not running and not current, it's safe to clean up.
                    LOG(LogLevel::INFO, "PLAYER_TASK", "Cleaning up old slot %d", i);
                    delete mp3_[i]; mp3_[i] = nullptr;
                    delete stub_[i]; stub_[i] = nullptr;
                    delete file_[i]; file_[i] = nullptr;
                }
            }
        }

        if (!any_running) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void MusicPlayer::pause() {
    if (currentState_ == State::PLAYING) {
        currentState_ = State::PAUSED;
        if (out_) out_->stop(); // Use the lightweight stop, as you requested.
        LOG(LogLevel::INFO, "PLAYER", "Playback paused.");
    }
}

void MusicPlayer::resume() {
    if (currentState_ == State::PAUSED) {
        currentState_ = State::PLAYING;
        if (out_) out_->begin(); // Use the lightweight begin/resume, as you requested.
        LOG(LogLevel::INFO, "PLAYER", "Playback resumed.");
    }
}

void MusicPlayer::stop() {
    if (currentState_ == State::PAUSED && out_) {
        // If we stop from a paused state, we must resume the hardware stream
        // so it's ready for the next song.
        out_->begin();
    }
    stopPlayback();
    currentState_ = State::STOPPED;
    currentTrackPath_ = "";
    currentTrackName_ = "";
}

// ... the rest of MusicPlayer.cpp remains unchanged ...
void MusicPlayer::queuePlaylist(const std::string& name, const std::vector<std::string>& tracks, int startIndex) {
    if (tracks.empty() || startIndex >= (int)tracks.size()) return;
    _isLoadingTrack = true;
    currentState_ = State::LOADING;
    playlistName_ = name;
    currentPlaylist_ = tracks;
    playlistTrackIndex_ = startIndex - 1; 

    if (isShuffle_) {
        generateShuffledIndices();
        std::string targetPath = currentPlaylist_[startIndex];
        for(size_t i = 0; i < shuffledIndices_.size(); ++i) {
            if (currentPlaylist_[shuffledIndices_[i]] == targetPath) {
                playlistTrackIndex_ = i - 1;
                break;
            }
        }
    }
}

void MusicPlayer::startQueuedPlayback() {
    if (currentState_ == State::LOADING) {
        playNextInPlaylist(false);
    }
}

void MusicPlayer::nextTrack() {
    if (!currentPlaylist_.empty()) {
        _isLoadingTrack = true;
        requestedAction_ = PlaybackAction::NEXT;
    }
}

void MusicPlayer::prevTrack() {
    if (!currentPlaylist_.empty()) {
        _isLoadingTrack = true;
        requestedAction_ = PlaybackAction::PREV;
    }
}

void MusicPlayer::setVolume(uint8_t volumePercent) {
    if (volumePercent > 200) volumePercent = 200;

    if (volumePercent <= 100) {
        currentGain_ = (float)volumePercent / 100.0f * 1.75f;
    } else {
        currentGain_ = 1.75f + ((float)(volumePercent - 100) / 100.0f) * (2.5f - 1.75f);
    }

    LOG(LogLevel::INFO, "PLAYER", "Volume set to %u%%, gain is now %.2f", volumePercent, currentGain_);
    
    if (currentState_ == State::PLAYING && currentSlot_ != -1 && stub_[currentSlot_]) {
        stub_[currentSlot_]->SetGain(currentGain_);
    }
}

void MusicPlayer::stopPlayback() {
    LOG(LogLevel::INFO, "PLAYER", "Stopping all playback and cleaning up both slots.");
    for (int i = 0; i < 2; ++i) {
        if (mp3_[i] && mp3_[i]->isRunning()) {
            mp3_[i]->stop();
        }
        delete mp3_[i]; mp3_[i] = nullptr;
        delete stub_[i]; stub_[i] = nullptr;
        delete file_[i]; file_[i] = nullptr;
    }
    currentSlot_ = -1;
}

void MusicPlayer::startPlayback(const std::string& path) {
    int prevSlot = currentSlot_;
    int nextSlot = (currentSlot_ == -1) ? 0 : (currentSlot_ + 1) % 2;

    LOG(LogLevel::INFO, "PLAYER", "Attempting to start '%s' in slot %d", path.c_str(), nextSlot);

    // --- Prepare the next track ---
    file_[nextSlot] = new AudioFileSourceSD(path.c_str());
    if (!file_[nextSlot]->isOpen()) {
        LOG(LogLevel::ERROR, "PLAYER", "Failed to open file for slot %d", nextSlot);
        delete file_[nextSlot];
        file_[nextSlot] = nullptr;
        playNextInPlaylist(false); // Try next song
        return;
    }

    stub_[nextSlot] = mixer_->NewInput();
    stub_[nextSlot]->SetGain(currentGain_);
    mp3_[nextSlot] = new AudioGeneratorMP3();

    // Set the current slot *before* calling begin() to prevent a race condition
    // where the mixer task sees a running track in a non-current slot and stops it.
    currentSlot_ = nextSlot;

    if (!mp3_[nextSlot]->begin(file_[nextSlot], stub_[nextSlot])) {
        LOG(LogLevel::ERROR, "PLAYER", "MP3 begin failed for slot %d", nextSlot);
        delete mp3_[nextSlot]; mp3_[nextSlot] = nullptr;
        delete stub_[nextSlot]; stub_[nextSlot] = nullptr;
        delete file_[nextSlot]; file_[nextSlot] = nullptr;
        currentSlot_ = prevSlot; // Revert slot on failure
        playNextInPlaylist(false); // Try next song
        return;
    }

    LOG(LogLevel::INFO, "PLAYER", "Playback started successfully in slot %d", nextSlot);

    // --- Update track metadata ---
    currentTrackPath_ = path;
    size_t last_slash = path.find_last_of('/');
    currentTrackName_ = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    currentState_ = State::PLAYING;
    _isLoadingTrack = false;
}

void MusicPlayer::playNextInPlaylist(bool songFinishedNaturally) {
    if (currentPlaylist_.empty()) {
        stop();
        return;
    }
    if (songFinishedNaturally && repeatMode_ == RepeatMode::REPEAT_ONE) {
        startPlayback(currentTrackPath_);
        return;
    }
    
    playlistTrackIndex_++;
    
    if (playlistTrackIndex_ >= (int)currentPlaylist_.size()) {
        if (repeatMode_ == RepeatMode::REPEAT_ALL) {
            playlistTrackIndex_ = 0;
            if (isShuffle_) generateShuffledIndices();
        } else {
            stop();
            return;
        }
    }
    
    const std::string& path = isShuffle_ ? currentPlaylist_[shuffledIndices_[playlistTrackIndex_]] : currentPlaylist_[playlistTrackIndex_];
    startPlayback(path);
}

void MusicPlayer::serviceRequest() {
    if (requestedAction_ == PlaybackAction::NONE) {
        return;
    }

    if (requestedAction_ == PlaybackAction::NEXT) {
        // This logic is taken from the original playNextInPlaylist
        // No changes needed here.
    } else if (requestedAction_ == PlaybackAction::PREV) {
        // This logic is taken from the original prevTrack()
        playlistTrackIndex_ -= 2;
        if (playlistTrackIndex_ < -1) {
            // Wrap around to the end of the playlist
            playlistTrackIndex_ = currentPlaylist_.size() - 2;
        }
    }

    // Reset the action now that we're handling it
    requestedAction_ = PlaybackAction::NONE;

    // Call the actual playback function
    playNextInPlaylist(false);
}

void MusicPlayer::songFinished() {
    playNextInPlaylist(true);
}

float MusicPlayer::getPlaybackProgress() const {
    if (currentSlot_ != -1 && file_[currentSlot_] && file_[currentSlot_]->isOpen()) {
        uint32_t pos = file_[currentSlot_]->getPos();
        uint32_t size = file_[currentSlot_]->getSize();
        if (size > 0) {
            return (float)pos / (float)size;
        }
    }
    return 0.0f;
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
    unsigned int seed = rd() ^ (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 g(seed);

    std::shuffle(shuffledIndices_.begin(), shuffledIndices_.end(), g);
}

MusicPlayer::State MusicPlayer::getState() const { return currentState_; }
MusicPlayer::RepeatMode MusicPlayer::getRepeatMode() const { return repeatMode_; }
MusicPlayer::PlaybackAction MusicPlayer::getRequestedAction() const { return requestedAction_; }
bool MusicPlayer::isLoadingTrack() const { return _isLoadingTrack; }
bool MusicPlayer::isShuffle() const { return isShuffle_; }
std::string MusicPlayer::getCurrentTrackName() const { return currentTrackName_; }
std::string MusicPlayer::getPlaylistName() const { return playlistName_; }