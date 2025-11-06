#include "MusicPlayer.h"
#include "App.h"
#include "SdCardManager.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <algorithm> // For std::min

#include "AudioFileSourceKivaSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"

MusicPlayer::MusicPlayer() : 
    app_(nullptr), resourcesAllocated_(false),
    out_(nullptr), mixer_(nullptr), currentSlot_(-1),
    currentState_(State::STOPPED), repeatMode_(RepeatMode::REPEAT_OFF), requestedAction_(PlaybackAction::NONE), isShuffle_(false),
    _isLoadingTrack(false), currentGain_(1.75f),
    playlistTrackIndex_(-1), currentTrackDuration_(0),
    playbackStartTimeMs_(0), accumulatedPlayedTimeMs_(0),
    mixerTaskHandle_(nullptr)
{
    for (int i = 0; i < 2; ++i) {
        source_file_[i] = nullptr;
        id3_filter_[i] = nullptr;
        mp3_[i] = nullptr;
        stub_[i] = nullptr;
    }
    audioSlotMutex_ = xSemaphoreCreateMutex();
}

MusicPlayer::~MusicPlayer() {
    releaseResources();
    vSemaphoreDelete(audioSlotMutex_);
}

void MusicPlayer::setup(App* app) {
    app_ = app;
}

int MusicPlayer::getTotalDuration() const {
    return currentTrackDuration_;
}

int MusicPlayer::getCurrentTime() const {
    if (currentState_ == State::STOPPED || currentState_ == State::LOADING) {
        return 0;
    }

    unsigned long totalPlayedMs = accumulatedPlayedTimeMs_;

    if (currentState_ == State::PLAYING) {
        totalPlayedMs += (millis() - playbackStartTimeMs_);
    }

    int seconds = totalPlayedMs / 1000;
    
    return std::min(seconds, currentTrackDuration_);
}


void MusicPlayer::setSongFinishedCallback(SongFinishedCallback cb) {
    songFinishedCallback_ = cb;
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
        mixerTaskWrapper, "AudioMixerTask", 8192, this, 5, &mixerTaskHandle_, 1
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
            int activeSlot = currentSlot_;
            
            if (mp3_[i] == nullptr) continue;
            
            bool isCurrent = (i == activeSlot);
            
            if (isCurrent) {
                if (currentState_ == State::PLAYING && mp3_[i]->isRunning()) {
                    if (!mp3_[i]->loop()) {
                        mp3_[i]->stop();
                        if (songFinishedCallback_) {
                            songFinishedCallback_();
                        }
                    }
                    any_running = true;
                }
            } else { 
                if (mp3_[i]->isRunning()) {
                    mp3_[i]->stop();
                    any_running = true; 
                } else {
                    if (xSemaphoreTake(audioSlotMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
                        if (i != currentSlot_ && mp3_[i] != nullptr) {
                            LOG(LogLevel::INFO, "PLAYER_TASK", "Cleaning up old slot %d", i);
                            delete mp3_[i]; mp3_[i] = nullptr;
                            delete stub_[i]; stub_[i] = nullptr;
                            delete id3_filter_[i]; id3_filter_[i] = nullptr;
                            delete source_file_[i]; source_file_[i] = nullptr;
                        }
                        xSemaphoreGive(audioSlotMutex_);
                    }
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
        accumulatedPlayedTimeMs_ += (millis() - playbackStartTimeMs_);
        currentState_ = State::PAUSED;
        if (out_) out_->stop();
        LOG(LogLevel::INFO, "PLAYER", "Playback paused.");
    }
}

void MusicPlayer::resume() {
    if (currentState_ == State::PAUSED) {
        playbackStartTimeMs_ = millis();
        currentState_ = State::PLAYING;
        if (out_) out_->begin();
        LOG(LogLevel::INFO, "PLAYER", "Playback resumed.");
    }
}

void MusicPlayer::stop() {
    if (currentState_ == State::PAUSED && out_) {
        out_->begin();
    }
    stopPlayback();
    currentState_ = State::STOPPED;
    currentTrackPath_ = "";
    currentTrackName_ = "";
    currentTrackDuration_ = 0;
    accumulatedPlayedTimeMs_ = 0;
    playbackStartTimeMs_ = 0;
}

void MusicPlayer::queuePlaylist(const std::string& name, const std::vector<PlaylistTrack>& tracks, int startIndex) {
    if (tracks.empty() || startIndex >= (int)tracks.size()) return;
    _isLoadingTrack = true;
    currentState_ = State::LOADING;
    playlistName_ = name;
    currentPlaylist_ = tracks;
    playlistTrackIndex_ = startIndex - 1; 

    if (isShuffle_) {
        generateShuffledIndices();
        std::string targetPath = currentPlaylist_[startIndex].path;
        for(size_t i = 0; i < shuffledIndices_.size(); ++i) {
            if (currentPlaylist_[shuffledIndices_[i]].path == targetPath) {
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
    if (xSemaphoreTake(audioSlotMutex_, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < 2; ++i) {
            if (mp3_[i] && mp3_[i]->isRunning()) {
                mp3_[i]->stop();
            }
            delete mp3_[i]; mp3_[i] = nullptr;
            delete stub_[i]; stub_[i] = nullptr;
            delete id3_filter_[i]; id3_filter_[i] = nullptr;
            delete source_file_[i]; source_file_[i] = nullptr;
        }
        currentSlot_ = -1;
        xSemaphoreGive(audioSlotMutex_);
    }
}

void MusicPlayer::startPlayback(const PlaylistTrack& track) {
    currentState_ = State::LOADING;
    
    accumulatedPlayedTimeMs_ = 0;
    playbackStartTimeMs_ = 0;
    
    int prevSlot;
    int nextSlot;

    if (xSemaphoreTake(audioSlotMutex_, portMAX_DELAY) != pdTRUE) {
        LOG(LogLevel::ERROR, "PLAYER", "Could not take mutex in startPlayback!");
        return;
    }
    
    prevSlot = currentSlot_;
    nextSlot = (currentSlot_ == -1) ? 0 : (currentSlot_ + 1) % 2;
    LOG(LogLevel::INFO, "PLAYER", "Preparing to start '%s' in slot %d", track.path.c_str(), nextSlot);

    delete mp3_[nextSlot]; mp3_[nextSlot] = nullptr;
    delete stub_[nextSlot]; stub_[nextSlot] = nullptr;
    delete id3_filter_[nextSlot]; id3_filter_[nextSlot] = nullptr;
    delete source_file_[nextSlot]; source_file_[nextSlot] = nullptr;
    
    source_file_[nextSlot] = new AudioFileSourceKivaSD(track.path.c_str());
    if (!source_file_[nextSlot]->isOpen()) {
        LOG(LogLevel::ERROR, "PLAYER", "Failed to open file for slot %d", nextSlot);
        delete source_file_[nextSlot];
        source_file_[nextSlot] = nullptr;
        xSemaphoreGive(audioSlotMutex_);
        playNextInPlaylist(false); 
        return;
    }
    id3_filter_[nextSlot] = new AudioFileSourceID3(source_file_[nextSlot]);
    
    stub_[nextSlot] = mixer_->NewInput();
    stub_[nextSlot]->SetGain(currentGain_);
    mp3_[nextSlot] = new AudioGeneratorMP3();
    currentSlot_ = nextSlot;

    xSemaphoreGive(audioSlotMutex_);

    bool success = mp3_[nextSlot]->begin(id3_filter_[nextSlot], stub_[nextSlot]);
    
    if (success) {
        uint32_t audioStartPos = id3_filter_[nextSlot]->getPos();
        LOG(LogLevel::DEBUG, "PLAYER", "Audio data starts at position: %u", audioStartPos);
        
        if (id3_filter_[nextSlot]->seek(audioStartPos, SEEK_SET)) {
            LOG(LogLevel::DEBUG, "PLAYER", "Rewinding to audio start position: %u", audioStartPos);
        } else {
            LOG(LogLevel::ERROR, "PLAYER", "Failed to rewind to audio start position!");
        }
    }
    
    if (!success) {
        LOG(LogLevel::ERROR, "PLAYER", "MP3 begin failed for slot %d", nextSlot);
        if (xSemaphoreTake(audioSlotMutex_, portMAX_DELAY) == pdTRUE) {
            delete mp3_[nextSlot]; mp3_[nextSlot] = nullptr;
            delete stub_[nextSlot]; stub_[nextSlot] = nullptr;
            delete id3_filter_[nextSlot]; id3_filter_[nextSlot] = nullptr;
            delete source_file_[nextSlot]; source_file_[nextSlot] = nullptr;
            currentSlot_ = prevSlot;
            xSemaphoreGive(audioSlotMutex_);
        }
        playNextInPlaylist(false);
        return;
    }

    currentTrackPath_ = track.path;
    currentTrackDuration_ = track.duration;
    size_t last_slash = track.path.find_last_of('/');
    currentTrackName_ = (last_slash == std::string::npos) ? track.path : track.path.substr(last_slash + 1);
    
    _isLoadingTrack = false;

    // This happens *after* all blocking setup calls are finished.
    currentState_ = State::PLAYING;
    playbackStartTimeMs_ = millis();
    
    int startTimeSeconds = getCurrentTime();
    char timeStr[8];
    snprintf(timeStr, sizeof(timeStr), "%d:%02d", startTimeSeconds / 60, startTimeSeconds % 60);

    LOG(LogLevel::INFO, "PLAYER", "Playback initiated for '%s'. Initial calculated time: %s (%d seconds).", 
        currentTrackName_.c_str(), timeStr, startTimeSeconds);

}

void MusicPlayer::playNextInPlaylist(bool songFinishedNaturally) {
    if (currentPlaylist_.empty()) {
        stop();
        return;
    }
    if (songFinishedNaturally && repeatMode_ == RepeatMode::REPEAT_ONE) {
        startPlayback({currentTrackPath_, currentTrackDuration_});
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
    
    const auto& track = isShuffle_ ? currentPlaylist_[shuffledIndices_[playlistTrackIndex_]] : currentPlaylist_[playlistTrackIndex_];
    startPlayback(track);
}

void MusicPlayer::songFinished() {
    playNextInPlaylist(true);
}

void MusicPlayer::serviceRequest() {
    if (requestedAction_ == PlaybackAction::NONE) {
        return;
    }

    if (requestedAction_ == PlaybackAction::NEXT) {
        // Handled by playNextInPlaylist
    } else if (requestedAction_ == PlaybackAction::PREV) {
        playlistTrackIndex_ -= 2;
        if (playlistTrackIndex_ < -1) {
            playlistTrackIndex_ = currentPlaylist_.size() - 2;
        }
    }

    requestedAction_ = PlaybackAction::NONE;
    playNextInPlaylist(false);
}

float MusicPlayer::getPlaybackProgress() const {
    if (currentTrackDuration_ > 0) {
        float progress = (float)getCurrentTime() / (float)currentTrackDuration_;
        return std::min(progress, 1.0f);
    }
    return 0.0f;
}

void MusicPlayer::toggleShuffle() {
    isShuffle_ = !isShuffle_;
    if (isShuffle_ && !currentPlaylist_.empty()) {
        generateShuffledIndices();
        for(size_t i = 0; i < shuffledIndices_.size(); ++i) {
            if (currentPlaylist_[shuffledIndices_[i]].path == currentTrackPath_) {
                playlistTrackIndex_ = i;
                break;
            }
        }
    }
    LOG(LogLevel::INFO, "PLAYER", "Shuffle mode %s", isShuffle_ ? "enabled" : "disabled");
}

void MusicPlayer::cycleRepeatMode() {
    repeatMode_ = static_cast<RepeatMode>((static_cast<int>(repeatMode_) + 1) % 3);
    LOG(LogLevel::INFO, "PLAYER", "Repeat mode %s", repeatMode_ == RepeatMode::REPEAT_OFF ? "off" : repeatMode_ == RepeatMode::REPEAT_ALL ? "all" : "one");
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