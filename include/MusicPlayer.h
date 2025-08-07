#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <string>
#include <vector>
#include "Logger.h"
#include "AudioOutputPDM.h"

// Forward declarations from ESP8266Audio library
class AudioGeneratorMP3;
class AudioFileSourceSD;
class App;

class MusicPlayer {
public:
    enum class State { STOPPED, PLAYING, PAUSED };
    enum class RepeatMode { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE };

    MusicPlayer();
    ~MusicPlayer();

    void setup(App* app);
    void loop();

    void play(const std::string& path);
    void playPlaylist(const std::string& name, const std::vector<std::string>& tracks);
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
    void startPlayback(const std::string& path);
    void cleanup();
    void playNextInPlaylist(bool songFinishedNaturally = true);
    void generateShuffledIndices();
    void enableAmplifier(bool enable);

    App* app_;
    AudioGeneratorMP3* mp3_;
    AudioFileSourceSD* file_;
    AudioOutputPDM* out_;

    // Buffer to pre-allocate memory for the MP3 decoder, preventing heap allocation failures.
    void* mp3_preAlloc_; 

    State currentState_;
    RepeatMode repeatMode_;
    bool isShuffle_;
    
    std::vector<std::string> currentPlaylist_;
    std::vector<int> shuffledIndices_;
    int playlistTrackIndex_; // Index in the original (or shuffled) playlist
    
    std::string currentTrackPath_;
    std::string currentTrackName_;
    std::string playlistName_;
};

#endif // MUSIC_PLAYER_H