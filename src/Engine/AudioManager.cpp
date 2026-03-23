#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "Engine/AudioManager.hpp"
#include <cstring>
#include <cstdio>

static constexpr int    SAMPLE_RATE  = 44100;
static constexpr int    CHANNELS     = 2;
static constexpr unsigned long FRAMES_PER_BUFFER = 512;

// PortAudio C callback forwards to the AudioManager instance
static int paCallbackStatic(const void* /*input*/, void* output,
                             unsigned long frames,
                             const PaStreamCallbackTimeInfo* /*ti*/,
                             PaStreamCallbackFlags /*flags*/,
                             void* userData)
{
    return static_cast<AudioManager*>(userData)->paCallback(output, frames);
}

// ---------------------------------------------------------------------------

AudioManager& AudioManager::get() {
    static AudioManager instance;
    return instance;
}

void AudioManager::init() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "[Audio] Pa_Initialize: %s\n", Pa_GetErrorText(err));
        return;
    }

    err = Pa_OpenDefaultStream(&stream_,
                               0, CHANNELS,
                               paFloat32,
                               SAMPLE_RATE,
                               FRAMES_PER_BUFFER,
                               paCallbackStatic,
                               this);
    if (err != paNoError) {
        fprintf(stderr, "[Audio] Pa_OpenDefaultStream: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return;
    }

    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        fprintf(stderr, "[Audio] Pa_StartStream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        Pa_Terminate();
    }
}

void AudioManager::shutdown() {
    if (stream_) {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
    Pa_Terminate();
}

int AudioManager::loadClip(const std::string& path) {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 frameCount;

    // Load as float32 — dr_wav converts and resamples channel count for us
    float* raw = drwav_open_file_and_read_pcm_frames_f32(
        path.c_str(), &channels, &sampleRate, &frameCount, nullptr);

    if (!raw) {
        fprintf(stderr, "[Audio] Failed to load: %s\n", path.c_str());
        return -1;
    }

    AudioClip clip;
    size_t totalSamples = (size_t)frameCount * CHANNELS;
    clip.samples.resize(totalSamples);

    if (channels == 1) {
        // Mono → interleaved stereo
        for (drwav_uint64 i = 0; i < frameCount; i++) {
            clip.samples[i * 2]     = raw[i];
            clip.samples[i * 2 + 1] = raw[i];
        }
    } else {
        // Already stereo (or more channels: take first two)
        for (drwav_uint64 i = 0; i < frameCount; i++) {
            clip.samples[i * 2]     = raw[i * channels];
            clip.samples[i * 2 + 1] = raw[i * channels + (channels > 1 ? 1 : 0)];
        }
    }

    drwav_free(raw, nullptr);

    std::lock_guard<std::mutex> lock(mutex_);
    int id = (int)clips_.size();
    clips_.push_back(std::move(clip));
    return id;
}

int AudioManager::play(int clipId, bool looping, float volume) {
    if (clipId < 0) return -1;

    std::lock_guard<std::mutex> lock(mutex_);
    if (clipId >= (int)clips_.size()) return -1;

    // Reuse a finished slot if available
    for (auto& s : sounds_) {
        if (!s.active) {
            s = { nextSoundId_++, clipId, 0, volume, looping, true };
            return s.id;
        }
    }

    PlayingSound ps{ nextSoundId_++, clipId, 0, volume, looping, true };
    int id = ps.id;
    sounds_.push_back(ps);
    return id;
}

void AudioManager::stop(int soundId) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& s : sounds_) {
        if (s.id == soundId) { s.active = false; break; }
    }
}

void AudioManager::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& s : sounds_) s.active = false;
}

void AudioManager::setVolume(int soundId, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& s : sounds_) {
        if (s.id == soundId) { s.volume = volume; break; }
    }
}

// ---------------------------------------------------------------------------
// Mixing callback — runs in PortAudio real-time thread
// ---------------------------------------------------------------------------

int AudioManager::paCallback(void* output, unsigned long frames) {
    float* out = static_cast<float*>(output);
    memset(out, 0, frames * CHANNELS * sizeof(float));

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& s : sounds_) {
        if (!s.active) continue;

        const AudioClip& clip = clips_[s.clipId];
        size_t clipFrames = clip.samples.size() / CHANNELS;

        for (unsigned long f = 0; f < frames; f++) {
            if (s.framePos >= clipFrames) {
                if (s.looping) {
                    s.framePos = 0;
                } else {
                    s.active = false;
                    break;
                }
            }
            out[f * 2]     += clip.samples[s.framePos * 2]     * s.volume;
            out[f * 2 + 1] += clip.samples[s.framePos * 2 + 1] * s.volume;
            s.framePos++;
        }
    }

    // Soft clip to prevent distortion from summed voices
    for (unsigned long i = 0; i < frames * CHANNELS; i++) {
        float v = out[i];
        if      (v >  1.0f) out[i] =  1.0f;
        else if (v < -1.0f) out[i] = -1.0f;
    }

    return paContinue;
}
