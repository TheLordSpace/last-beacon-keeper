#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <mutex>
#include <cmath>

enum class SoundID {
    Step,
    Swing,
    HarvestTree,
    HarvestCrystal,
    HarvestOil,
    BeamHum,
    BeamSizzle,
    MirrorReflect,
    EnemyHurt,
    EnemyDie,
    Craft,
    AltarIgnite,
    NightAlarm,
    DawnChime,
    PlayerHurt,
    PlayerDash
};

struct ActiveVoice {
    float freq = 440.0f;
    float phase = 0.0f;
    float duration = 0.2f;
    float elapsed = 0.0f;
    float volume = 0.5f;
    int type = 0; // 0=sine, 1=square, 2=noise, 3=triangle, 4=saw, 5=horn
    float freqEnd = 440.0f;
};

class AudioManager {
public:
    static AudioManager& instance();

    bool init();
    void cleanup();

    void playSound(SoundID id, float volume = 0.7f);
    void setMusicNight(bool isNight);
    void setBeamActive(bool active);
    void update(float dt);

    void fillAudio(float* stream, int len);

private:
    AudioManager();
    ~AudioManager();

    SDL_AudioDeviceID m_device = 0;
    std::mutex m_mutex;
    std::vector<ActiveVoice> m_voices;

    bool m_isNight = false;
    bool m_beamActive = false;
    float m_musicTime = 0.0f;
    float m_oceanPhase = 0.0f;
    float m_masterVolume = 0.7f;
};
