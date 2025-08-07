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
    mp3_preAlloc_(nullptr), // Initialize the new pointer
    currentState_(State::STOPPED), repeatMode_(RepeatMode::REPEAT_OFF), isShuffle_(false),
    playlistTrackIndex_(-1)
{
    // --- THIS IS THE FIX ---
    // Pre-allocate the memory needed for the MP3 decoder.
    mp3_preAlloc_ = malloc(AudioGeneratorMP3::preAllocSize());
    if (!mp3_preAlloc_) {
        LOG(LogLevel::ERROR, "PLAYER", "FATAL: Could not pre-allocate memory for MP3 decoder!");
    }
    // --- END OF FIX ---
}

MusicPlayer::~MusicPlayer() {
    cleanup();
    // --- THIS IS THE FIX ---
    // Free the pre-allocated memory.
    if (mp3_preAlloc_) {
        free(mp3_preAlloc_);
        mp3_preAlloc_ = nullptr;
    }
    // --- END OF FIX ---
}

void MusicPlayer::setup(App* app) {
    app_ = app;
    pinMode(Pins::AMPLIFIER_PIN, OUTPUT);
    digitalWrite(Pins::AMPLIFIER_PIN, LOW);
}

void MusicPlayer::loop() {
    if (currentState_ == State::PLAYING && mp3_ && mp3_->isRunning()) {
        if (!mp3_->loop()) {
            // Song has finished, decide what to do next.
            // This call now correctly uses the default value of 'true'.
            playNextInPlaylist(); 
        }
    }
}

void MusicPlayer::play(const std::string& path) {
    stop();
    currentPlaylist_.clear();
    playlistName_ = "Single Track";
    startPlayback(path);
}

void MusicPlayer::playPlaylist(const std::string& name, const std::vector<std::string>& tracks) {
    if (tracks.empty()) return;
    stop();
    playlistName_ = name;
    currentPlaylist_ = tracks;
    playlistTrackIndex_ = 0;
    
    if (isShuffle_) {
        generateShuffledIndices();
        startPlayback(currentPlaylist_[shuffledIndices_[0]]);
    } else {
        startPlayback(currentPlaylist_[0]);
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
        cleanup();
        currentState_ = State::STOPPED;
        currentTrackPath_ = "";
        currentTrackName_ = "";
        playlistName_ = "";
        currentPlaylist_.clear();
        playlistTrackIndex_ = -1;
    }
}

void MusicPlayer::nextTrack() {
    if (currentPlaylist_.empty()) return;
    // This call is now valid.
    playNextInPlaylist(false); // Force next, don't check repeat mode
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
        // To avoid repeating the current song, find it in the new shuffled list
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

// --- Private Methods ---

void MusicPlayer::startPlayback(const std::string& path) {
    cleanup();
    if (!mp3_preAlloc_) {
        LOG(LogLevel::ERROR, "PLAYER", "Playback failed, no memory for decoder.");
        return; // Can't play if memory allocation failed in the constructor
    }

    LOG(LogLevel::INFO, "PLAYER", "Starting playback for: %s", path.c_str());

    file_ = new AudioFileSourceSD(path.c_str());
    if (!file_->isOpen()) {
        LOG(LogLevel::ERROR, "PLAYER", "Failed to open audio file.");
        delete file_; file_ = nullptr;
        return;
    }

    enableAmplifier(true);
    out_ = new AudioOutputPDM(Pins::AMPLIFIER_PIN);
    out_->SetGain(2.0f);
    
    // --- THIS IS THE FIX ---
    // Use the constructor that accepts our pre-allocated memory buffer.
    mp3_ = new AudioGeneratorMP3(mp3_preAlloc_, AudioGeneratorMP3::preAllocSize());
    // --- END OF FIX ---
    
    if (mp3_->begin(file_, out_)) {
        currentState_ = State::PLAYING;
        currentTrackPath_ = path;
        size_t last_slash = path.find_last_of('/');
        currentTrackName_ = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    } else {
        LOG(LogLevel::ERROR, "PLAYER", "Failed to start MP3 generator. Check MP3 format and memory.");
        cleanup();
    }
}

void MusicPlayer::cleanup() {
    enableAmplifier(false);
    if (mp3_) {
        if (mp3_->isRunning()) mp3_->stop();
        delete mp3_; mp3_ = nullptr;
    }
    // The MP3 generator deletes the file and out objects, so we just null them
    file_ = nullptr;
    out_ = nullptr;
}

void MusicPlayer::playNextInPlaylist(bool songFinishedNaturally) {
    if (currentPlaylist_.empty()) {
        stop();
        return;
    }

    if (songFinishedNaturally && repeatMode_ == RepeatMode::REPEAT_ONE) {
        // Repeat the same song
        stop();
        startPlayback(currentTrackPath_);
        return;
    }
    
    playlistTrackIndex_++;
    
    if (playlistTrackIndex_ >= (int)currentPlaylist_.size()) {
        // Reached end of playlist
        if (repeatMode_ == RepeatMode::REPEAT_ALL) {
            playlistTrackIndex_ = 0;
            if(isShuffle_) generateShuffledIndices(); // Re-shuffle for the next loop
        } else {
            stop();
            return; // End of playlist, no repeat
        }
    }
    
    // Stop cleans up old resources before starting the new track
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

// --- UI Getters ---
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