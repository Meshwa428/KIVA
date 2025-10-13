#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <string>
#include <vector>
#include <functional>
#include "Logger.h"
#include "AudioOutputPDM.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // MODIFICATION: Include semaphore header

// --- ADD MIXER HEADERS ---
#include "AudioOutputMixer.h"

class AudioGeneratorMP3;
class AudioFileSource; 
class App;

#include "Service.h"

class MusicPlayer : public Service {
public:
    using SongFinishedCallback = std::function<void()>;
    enum class State { STOPPED, LOADING, PLAYING, PAUSED };
    enum class RepeatMode { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE };
    enum class PlaybackAction { NONE, NEXT, PREV };

    MusicPlayer();
    ~MusicPlayer();

    void setup(App* app) override;
    
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
    void setVolume(uint8_t volumePercent); 
    void setSongFinishedCallback(SongFinishedCallback cb);
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
    static void mixerTaskWrapper(void* param);
    void mixerTaskLoop();

    void startPlayback(const std::string& path);
    void stopPlayback();
    void playNextInPlaylist(bool songFinishedNaturally = true);
    void generateShuffledIndices();

    App* app_;
    bool resourcesAllocated_;

    AudioOutputPDM* out_;
    AudioOutputMixer* mixer_;
    
    AudioFileSource* source_file_[2]; 
    AudioFileSource* id3_filter_[2]; 
    AudioGeneratorMP3* mp3_[2];
    AudioOutputMixerStub* stub_[2];
    volatile int currentSlot_;

    volatile State currentState_;
    RepeatMode repeatMode_;
    PlaybackAction requestedAction_;
    bool isShuffle_;
    bool _isLoadingTrack;
    float currentGain_; 
    
    std::vector<std::string> currentPlaylist_;
    std::vector<int> shuffledIndices_;
    int playlistTrackIndex_;
    
    std::string currentTrackPath_;
    std::string currentTrackName_;
    std::string playlistName_;

    TaskHandle_t mixerTaskHandle_; 
    SongFinishedCallback songFinishedCallback_;

    // MODIFICATION: Add the mutex handle
    SemaphoreHandle_t audioSlotMutex_;
};

#endif // MUSIC_PLAYER_H