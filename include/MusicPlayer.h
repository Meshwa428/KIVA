#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <string>
#include <vector>
#include "Logger.h"
#include "AudioOutputPDM.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- ADD MIXER HEADERS ---
#include "AudioOutputMixer.h"

class AudioGeneratorMP3;
class AudioFileSourceSD;
class App;

class MusicPlayer {
public:
    enum class State { STOPPED, LOADING, PLAYING, PAUSED };
    enum class RepeatMode { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE };
    enum class PlaybackAction { NONE, NEXT, PREV };

    MusicPlayer();
    ~MusicPlayer();

    void setup(App* app);
    
    // --- Resource management is now for the MIXER ---
    bool allocateResources();
    void releaseResources();

    void queuePlaylist(const std::string& name, const std::vector<std::string>& tracks, int startIndex);
    void startQueuedPlayback();
    void pause();
    void resume();
    void stop();
    void nextTrack();
    void prevTrack();
    void serviceRequest();
    void toggleShuffle();
    void cycleRepeatMode();
    void setVolume(uint8_t volumePercent); // New public method
    void songFinished();

    State getState() const;
    RepeatMode getRepeatMode() const;
    PlaybackAction getRequestedAction() const;
    bool isLoadingTrack() const;
    bool isShuffle() const;
    std::string getCurrentTrackName() const;
    std::string getPlaylistName() const;
    float getPlaybackProgress() const;
    bool isServiceRunning() const;

private:
    // --- NEW: Mixer task and loop ---
    static void mixerTaskWrapper(void* param);
    void mixerTaskLoop();

    // --- REVISED: Internal playback management ---
    void startPlayback(const std::string& path);
    void stopPlayback();
    void playNextInPlaylist(bool songFinishedNaturally = true);
    void generateShuffledIndices();

    App* app_;
    bool resourcesAllocated_;

    // --- NEW: Mixer and I/O components ---
    AudioOutputPDM* out_;
    AudioOutputMixer* mixer_;
    
    // --- NEW: Two slots for seamless track changes ---
    AudioFileSourceSD* file_[2];
    AudioGeneratorMP3* mp3_[2];
    AudioOutputMixerStub* stub_[2];
    volatile int currentSlot_; // 0 or 1

    volatile State currentState_;
    RepeatMode repeatMode_;
    PlaybackAction requestedAction_;
    bool isShuffle_;
    bool _isLoadingTrack;
    float currentGain_; // Current gain level (0.0 to 2.5)
    
    std::vector<std::string> currentPlaylist_;
    std::vector<int> shuffledIndices_;
    int playlistTrackIndex_;
    
    std::string currentTrackPath_;
    std::string currentTrackName_;
    std::string playlistName_;

    TaskHandle_t mixerTaskHandle_; // Handle to the persistent mixer task
};

#endif // MUSIC_PLAYER_H