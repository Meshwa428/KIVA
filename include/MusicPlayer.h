#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <string>
#include <vector>
#include "Logger.h"
#include "AudioOutputPDM.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Forward declarations from ESP8266Audio library
class AudioGeneratorMP3;
class AudioFileSourceSD;
class App;

class MusicPlayer {
public:
    // --- ADD LOADING STATE ---
    enum class State { STOPPED, LOADING, PLAYING, PAUSED };
    enum class RepeatMode { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE };

    MusicPlayer();
    ~MusicPlayer();

    void setup(App* app);

    void play(const std::string& path);
    void queuePlaylist(const std::string& name, const std::vector<std::string>& tracks, int startIndex);
    void startQueuedPlayback();
    void pause();
    void resume();
    void stop();
    void nextTrack();
    void prevTrack();
    void toggleShuffle();
    void cycleRepeatMode();

    // UI Getters
    State getState() const;
    RepeatMode getRepeatMode() const;
    bool isShuffle() const;
    std::string getCurrentTrackName() const;
    std::string getPlaylistName() const;
    float getPlaybackProgress() const; // Returns 0.0f to 1.0f

private:
    static void audioTaskWrapper(void* param);
    void audioTaskLoop();

    void startPlayback(const std::string& path);
    void cleanup();
    void playNextInPlaylist(bool songFinishedNaturally = true);
    void generateShuffledIndices();
    void enableAmplifier(bool enable);

    App* app_;
    AudioGeneratorMP3* mp3_;
    AudioFileSourceSD* file_;
    AudioOutputPDM* out_;

    void* mp3_preAlloc_; 

    volatile State currentState_;
    RepeatMode repeatMode_;
    bool isShuffle_;
    
    std::vector<std::string> currentPlaylist_;
    std::vector<int> shuffledIndices_;
    int playlistTrackIndex_;
    
    std::string currentTrackPath_;
    std::string currentTrackName_;
    std::string playlistName_;

    TaskHandle_t audioTaskHandle_;

    // --- ADD THREAD-SAFE REQUEST VARIABLES ---
    volatile bool playRequestPending_;
    std::string nextTrackToPlay_;
    std::string nextPlaylistName_;
    std::vector<std::string> nextPlaylistTracks_;
    int nextPlaylistStartIndex_;
};

#endif // MUSIC_PLAYER_H