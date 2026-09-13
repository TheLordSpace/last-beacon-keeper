#pragma once

#include "Common.h"
#include "Entities.h"
#include <vector>

struct PendingEnemySpawn {
    EnemyType type;
    float delay;
    float angle;
};

class WaveManager {
public:
    WaveManager();

    void resetForDay(int dayNumber);
    void startNight();
    void update(float dt, std::vector<Enemy>& enemies, const Vec2& lighthousePos);
    void endNight();

    int getCurrentWave() const { return m_currentWave; }
    int getTotalWaves() const { return m_totalWaves; }
    bool isWaveActive() const { return m_waveActive; }
    float getTimeUntilNextWave() const { return m_timeUntilNextWave; }
    float getWaveBannerTimer() const { return m_waveBannerTimer; }
    bool isBossWave() const { return m_isBossWave; }

    int getEnemiesDefeatedThisNight() const { return m_enemiesDefeatedThisNight; }
    void notifyEnemyKilled() { m_enemiesDefeatedThisNight++; }

    void clear();

private:
    int m_dayNumber = 1;
    int m_currentWave = 0;
    int m_totalWaves = 3;
    bool m_nightActive = false;
    bool m_waveActive = false;
    bool m_isBossWave = false;

    float m_nightTimer = 0.0f;
    float m_timeUntilNextWave = 0.0f;
    float m_waveBannerTimer = 0.0f;

    std::vector<PendingEnemySpawn> m_pendingSpawns;
    int m_enemiesDefeatedThisNight = 0;

    void triggerWave(int waveIndex);
};
