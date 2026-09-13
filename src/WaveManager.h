#pragma once

#include "Common.h"
#include "Entities.h"
#include <vector>

struct PendingEnemySpawn {
    EnemyType type;
    float delay;
    float angle;
    int waveId = 0;
};

struct WavePreview {
    int dayNumber = 1;
    int waveNumber = 1;
    int totalWaves = 3;
    int crawlers = 0;
    int eaters = 0;
    int brutes = 0;
    int leviathans = 0;
    float countdown = 0.0f;
    bool isBoss = false;
    bool active = false;

    int totalEnemies() const {
        return crawlers + eaters + brutes + leviathans;
    }
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
    int getWavesCompleted() const { return m_wavesCompleted; }
    bool isWaveActive() const { return m_waveActive; }
    bool isNightFinished() const { return m_wavesCompleted >= m_totalWaves && !m_waveActive && m_pendingSpawns.empty(); }
    float getTimeUntilNextWave() const { return m_timeUntilNextWave; }
    float getWaveBannerTimer() const { return m_waveBannerTimer; }
    float getWaveClearedTimer() const { return m_waveClearedTimer; }
    int getLastClearedWave() const { return m_lastClearedWave; }
    bool isBossWave() const { return m_isBossWave; }

    int getEnemiesDefeatedThisNight() const { return m_enemiesDefeatedThisNight; }
    void notifyEnemyKilled(int waveId = 0);

    static int getTotalWavesForDay(int day);
    static void getWaveComposition(int day, int wave, int& crawlers, int& eaters, int& brutes, int& leviathans);
    WavePreview getUpcomingWavePreview(int dayNumber, float duskCountdown = -1.0f) const;

    void clear();

private:
    int m_dayNumber = 1;
    int m_currentWave = 0;
    int m_totalWaves = 3;
    int m_wavesCompleted = 0;
    int m_currentWaveId = 0;
    int m_waveEnemiesScheduled = 0;
    int m_waveEnemiesSpawned = 0;
    int m_waveEnemiesAlive = 0;

    bool m_nightActive = false;
    bool m_waveActive = false;
    bool m_isBossWave = false;

    float m_nightTimer = 0.0f;
    float m_waveActiveTimer = 0.0f;
    float m_timeUntilNextWave = 0.0f;
    float m_waveBannerTimer = 0.0f;
    float m_waveClearedTimer = 0.0f;
    int m_lastClearedWave = 0;

    std::vector<PendingEnemySpawn> m_pendingSpawns;
    int m_enemiesDefeatedThisNight = 0;

    void triggerWave(int waveIndex);
};
