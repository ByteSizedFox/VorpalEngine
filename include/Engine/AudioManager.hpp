#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <portaudio.h>

struct AudioClip {
    std::vector<float> samples; // interleaved stereo float32 at 44100 Hz
};

struct PlayingSound {
    int id;
    int clipId;
    size_t framePos;
    float volume;
    bool looping;
    bool active;
};

class AudioManager {
public:
    static AudioManager& get();

    void init();
    void shutdown();

    // Returns clip ID (>= 0), or -1 on failure
    int loadClip(const std::string& path);

    // Returns sound instance ID (>= 0); safe to call with clipId == -1 (no-op, returns -1)
    int play(int clipId, bool looping = false, float volume = 1.0f);

    void stop(int soundId);
    void stopAll();
    void setVolume(int soundId, float volume);

    // Called by PortAudio — do not call directly
    int paCallback(void* output, unsigned long frames);

private:
    AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    PaStream* stream_ = nullptr;
    std::vector<AudioClip> clips_;
    std::vector<PlayingSound> sounds_;
    std::mutex mutex_;
    int nextSoundId_ = 0;
};
