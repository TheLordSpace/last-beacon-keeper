#include "Audio.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

constexpr int SAMPLE_RATE = 44100;
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;

static void audioCallback(void* userdata, Uint8* stream, int len) {
    auto* mgr = static_cast<AudioManager*>(userdata);
    float* floatStream = reinterpret_cast<float*>(stream);
    int sampleCount = len / sizeof(float);
    mgr->fillAudio(floatStream, sampleCount);
}

AudioManager& AudioManager::instance() {
    static AudioManager s_inst;
    return s_inst;
}

AudioManager::AudioManager() = default;

AudioManager::~AudioManager() {
    cleanup();
}

bool AudioManager::init() {
    SDL_AudioSpec desired{};
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = audioCallback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    m_device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (m_device == 0) {
        return false;
    }

    SDL_PauseAudioDevice(m_device, 0); // start audio
    return true;
}

void AudioManager::cleanup() {
    if (m_device != 0) {
        SDL_CloseAudioDevice(m_device);
        m_device = 0;
    }
}

void AudioManager::playSound(SoundID id, float volume) {
    if (m_device == 0) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    ActiveVoice v;
    v.volume = volume * m_masterVolume;

    switch (id) {
    case SoundID::Step:
        v.type = 2; // filtered noise
        v.freq = 280.0f;
        v.freqEnd = 120.0f;
        v.duration = 0.08f;
        v.volume *= 0.25f;
        m_voices.push_back(v);
        break;

    case SoundID::Swing:
        v.type = 4; // saw whoosh
        v.freq = 400.0f;
        v.freqEnd = 150.0f;
        v.duration = 0.14f;
        v.volume *= 0.4f;
        m_voices.push_back(v);
        break;

    case SoundID::HarvestTree:
        v.type = 1; // square impact thud
        v.freq = 180.0f;
        v.freqEnd = 60.0f;
        v.duration = 0.12f;
        v.volume *= 0.5f;
        m_voices.push_back(v);
        break;

    case SoundID::HarvestCrystal:
        // Crystalline sparkle: 2 harmonized sine pings
        v.type = 0;
        v.freq = 880.0f;
        v.freqEnd = 1320.0f;
        v.duration = 0.35f;
        v.volume *= 0.5f;
        m_voices.push_back(v);

        v.freq = 1760.0f;
        v.freqEnd = 2200.0f;
        v.duration = 0.25f;
        v.volume *= 0.35f;
        m_voices.push_back(v);
        break;

    case SoundID::HarvestOil:
        v.type = 0;
        v.freq = 240.0f;
        v.freqEnd = 110.0f;
        v.duration = 0.22f;
        v.volume *= 0.45f;
        m_voices.push_back(v);
        break;

    case SoundID::BeamHum:
        v.type = 1;
        v.freq = 110.0f;
        v.freqEnd = 115.0f;
        v.duration = 0.1f;
        v.volume *= 0.3f;
        m_voices.push_back(v);
        break;

    case SoundID::BeamSizzle:
        v.type = 2; // noise sizzle
        v.freq = 800.0f;
        v.freqEnd = 1200.0f;
        v.duration = 0.08f;
        v.volume *= 0.35f;
        m_voices.push_back(v);
        break;

    case SoundID::MirrorReflect:
        v.type = 0; // high pure ping
        v.freq = 1200.0f;
        v.freqEnd = 1800.0f;
        v.duration = 0.3f;
        v.volume *= 0.5f;
        m_voices.push_back(v);
        break;

    case SoundID::EnemyHurt:
        v.type = 2; // high noise screech
        v.freq = 450.0f;
        v.freqEnd = 200.0f;
        v.duration = 0.15f;
        v.volume *= 0.45f;
        m_voices.push_back(v);
        break;

    case SoundID::EnemyDie:
        v.type = 2;
        v.freq = 300.0f;
        v.freqEnd = 60.0f;
        v.duration = 0.4f;
        v.volume *= 0.6f;
        m_voices.push_back(v);
        break;

    case SoundID::Craft:
        v.type = 1;
        v.freq = 523.25f; // C5
        v.freqEnd = 659.25f; // E5
        v.duration = 0.25f;
        v.volume *= 0.5f;
        m_voices.push_back(v);
        break;

    case SoundID::AltarIgnite:
        // Harmonic triad
        {
            float notes[] = { 261.63f, 329.63f, 392.00f, 523.25f }; // C major chord
            for (float n : notes) {
                ActiveVoice chordVoice;
                chordVoice.type = 0;
                chordVoice.freq = n;
                chordVoice.freqEnd = n * 1.01f;
                chordVoice.duration = 2.0f;
                chordVoice.volume = 0.35f * m_masterVolume;
                m_voices.push_back(chordVoice);
            }
        }
        break;

    case SoundID::NightAlarm:
        v.type = 5; // deep foghorn
        v.freq = 95.0f;
        v.freqEnd = 85.0f;
        v.duration = 2.5f;
        v.volume *= 0.75f;
        m_voices.push_back(v);
        break;

    case SoundID::DawnChime:
        {
            float notes[] = { 392.00f, 493.88f, 587.33f, 783.99f }; // G major sunrise
            for (int i = 0; i < 4; ++i) {
                ActiveVoice cv;
                cv.type = 0;
                cv.freq = notes[i];
                cv.freqEnd = notes[i];
                cv.duration = 1.5f + i * 0.3f;
                cv.volume = 0.4f * m_masterVolume;
                m_voices.push_back(cv);
            }
        }
        break;

    case SoundID::PlayerHurt:
        v.type = 1;
        v.freq = 150.0f;
        v.freqEnd = 70.0f;
        v.duration = 0.22f;
        v.volume *= 0.6f;
        m_voices.push_back(v);
        break;

    case SoundID::PlayerDash:
        v.type = 4;
        v.freq = 280.0f;
        v.freqEnd = 550.0f;
        v.duration = 0.16f;
        v.volume *= 0.4f;
        m_voices.push_back(v);
        break;
    }
}

void AudioManager::setMusicNight(bool isNight) {
    m_isNight = isNight;
}

void AudioManager::setBeamActive(bool active) {
    m_beamActive = active;
}

void AudioManager::update(float dt) {
    // Keep internal timer
    m_musicTime += dt;
}

void AudioManager::fillAudio(float* stream, int sampleCount) {
    std::fill(stream, stream + sampleCount, 0.0f);

    float dt = 1.0f / static_cast<float>(SAMPLE_RATE);

    // 1. Synthesize background ambient music
    for (int i = 0; i < sampleCount; i += 2) {
        m_musicTime += dt;
        m_oceanPhase += dt * 0.18f;
        if (m_oceanPhase > TWO_PI) m_oceanPhase -= TWO_PI;

        // Ambient surf noise
        float surf = ((float)(rand() % 2000 - 1000) / 1000.0f) * (0.015f + 0.015f * std::sin(m_oceanPhase));

        float musicL = surf;
        float musicR = surf;

        if (!m_isNight) {
            // Day calm vibraphone arpeggios
            // Scale: A minor pentatonic (A3, C4, D4, E4, G4)
            const float pentatonic[] = { 220.0f, 261.63f, 293.66f, 329.63f, 392.0f, 440.0f, 523.25f };
            float stepTime = 0.45f;
            int stepIndex = static_cast<int>(m_musicTime / stepTime) % 16;
            float noteFraction = std::fmod(m_musicTime, stepTime) / stepTime;
            float env = std::exp(-noteFraction * 5.0f);

            int noteMap[16] = { 0, 2, 3, 5, 4, 3, 2, 1, 0, 3, 4, 6, 5, 4, 2, 0 };
            float freq = pentatonic[noteMap[stepIndex]];
            float tone = std::sin(m_musicTime * freq * TWO_PI) * env * 0.045f;

            musicL += tone;
            musicR += tone * 0.95f;
        } else {
            // Night tense pulsating drone
            float pulse = 0.5f + 0.5f * std::sin(m_musicTime * 4.0f);
            float bass = std::sin(m_musicTime * 55.0f * TWO_PI) * (0.05f + 0.03f * pulse);
            float arpStep = 0.20f;
            int arpIndex = static_cast<int>(m_musicTime / arpStep) % 8;
            float arpNotes[8] = { 110.0f, 130.81f, 146.83f, 155.56f, 164.81f, 146.83f, 130.81f, 110.0f };
            float arpFraction = std::fmod(m_musicTime, arpStep) / arpStep;
            float arpEnv = std::exp(-arpFraction * 4.5f);
            float arpTone = std::sin(m_musicTime * arpNotes[arpIndex] * TWO_PI) * arpEnv * 0.04f;

            musicL += bass + arpTone;
            musicR += bass + arpTone * 0.85f;
        }

        // Active beam continuous hum
        if (m_beamActive) {
            float beamTone = (std::fmod(m_musicTime * 120.0f, 1.0f) > 0.5f ? 0.035f : -0.035f);
            float beamSizzle = ((float)(rand() % 1000 - 500) / 1000.0f) * 0.025f;
            musicL += beamTone + beamSizzle;
            musicR += beamTone + beamSizzle;
        }

        stream[i] += musicL;
        stream[i + 1] += musicR;
    }

    // 2. Synthesize active sound effect voices
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it = m_voices.begin(); it != m_voices.end(); ) {
        ActiveVoice& v = *it;
        float progress = v.elapsed / v.duration;
        float currentFreq = v.freq + (v.freqEnd - v.freq) * progress;
        float envelope = 1.0f - progress;
        if (envelope < 0.0f) envelope = 0.0f;

        float invSampleRate = 1.0f / static_cast<float>(SAMPLE_RATE);

        for (int i = 0; i < sampleCount; i += 2) {
            v.elapsed += dt;
            if (v.elapsed >= v.duration) break;

            v.phase += currentFreq * TWO_PI * invSampleRate;
            if (v.phase > TWO_PI) v.phase -= TWO_PI;

            float sample = 0.0f;
            switch (v.type) {
            case 0: // Sine
                sample = std::sin(v.phase);
                break;
            case 1: // Square
                sample = (v.phase < PI) ? 1.0f : -1.0f;
                break;
            case 2: // Noise
                sample = ((float)(rand() % 2000 - 1000)) / 1000.0f;
                break;
            case 3: // Triangle
                sample = 2.0f * std::abs(2.0f * (v.phase / TWO_PI - std::floor(v.phase / TWO_PI + 0.5f))) - 1.0f;
                break;
            case 4: // Saw
                sample = 2.0f * (v.phase / TWO_PI - std::floor(v.phase / TWO_PI + 0.5f));
                break;
            case 5: // Horn (multi-sine organ)
                sample = 0.6f * std::sin(v.phase) + 0.3f * std::sin(v.phase * 2.0f) + 0.1f * std::sin(v.phase * 3.0f);
                break;
            }

            float finalVal = sample * envelope * v.volume;
            stream[i] += finalVal;
            stream[i + 1] += finalVal;
        }

        if (v.elapsed >= v.duration) {
            it = m_voices.erase(it);
        } else {
            ++it;
        }
    }

    // Master clamp to prevent clipping
    for (int i = 0; i < sampleCount; ++i) {
        stream[i] = std::clamp(stream[i], -0.98f, 0.98f);
    }
}
