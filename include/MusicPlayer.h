#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <string>
#include <vector>
#include "Logger.h"
#include "AudioOutputPDM.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Forward declarations
class AudioGeneratorMP3;
class AudioFileSourceSD;
class App;

class MusicPlayer {
public:
    enum class State { STOPPED, LOADING, PLAYING, PAUSED };
    enum class RepeatMode { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE };

    MusicPlayer();
    ~MusicPlayer();

    void setup(App* app);

    void queuePlaylist(const std::string& name, const std::vector<std::string>& tracks, int startIndex);
    void startQueuedPlayback();
    void pause();
    void resume();
    void stop();
    void nextTrack();
    void prevTrack();
    void toggleShuffle();
    void cycleRepeatMode();

    State getState() const;
    RepeatMode getRepeatMode() const;
    bool isShuffle() const;
    std::string getCurrentTrackName() const;
    std::string getPlaylistName() const;
    float getPlaybackProgress() const;

private:
    static void audioTaskWrapper(void* param);
    void audioTaskLoop();

    void startPlayback(const std::string& path);
    void cleanupCurrentTrack();
    void playNextInPlaylist(bool songFinishedNaturally = true);
    void generateShuffledIndices();
    void enableAmplifier(bool enable);

    App* app_;
    AudioGeneratorMP3* mp3_;
    AudioFileSourceSD* file_;
    AudioOutputPDM* out_;
    void* mp3_preAlloc_; 

    // --- START OF FIX ---
    // These variables are shared between the UI task (Core 0) and the audio task (Core 1).
    // They MUST be declared volatile to prevent compiler optimizations that would
    // cause one core to read a stale value from a CPU register.
    volatile State currentState_;
    // --- END OF FIX ---
    
    RepeatMode repeatMode_;
    bool isShuffle_;
    
    std::vector<std::string> currentPlaylist_;
    std::vector<int> shuffledIndices_;
    int playlistTrackIndex_;
    
    std::string currentTrackPath_;
    std::string currentTrackName_;
    std::string playlistName_;

    TaskHandle_t audioTaskHandle_;

    enum class AudioCommand { NONE, PLAY_QUEUED, STOP, NEXT, PREV };
    // --- START OF FIX ---
    volatile AudioCommand pendingCommand_;
    // --- END OF FIX ---

    std::string nextPlaylistName_;
    std::vector<std::string> nextPlaylistTracks_;
    int nextPlaylistStartIndex_;
};

#endif // MUSIC_PLAYER_H