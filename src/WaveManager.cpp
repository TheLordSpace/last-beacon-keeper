#include "WaveManager.h"
#include "Audio.h"
#include "Particles.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

WaveManager::WaveManager() = default;

void WaveManager::clear() {
    m_pendingSpawns.clear();
    m_nightActive = false;
    m_waveActive = false;
    m_currentWave = 0;
    m_wavesCompleted = 0;
    m_currentWaveId = 0;
    m_waveEnemiesScheduled = 0;
    m_waveEnemiesSpawned = 0;
    m_waveEnemiesAlive = 0;
    m_timeUntilNextWave = 0.0f;
    m_waveBannerTimer = 0.0f;
    m_waveClearedTimer = 0.0f;
    m_lastClearedWave = 0;
    m_waveActiveTimer = 0.0f;
    m_isBossWave = false;
    m_enemiesDefeatedThisNight = 0;
}

int WaveManager::getTotalWavesForDay(int day) {
    if (day <= 1) return 3;
    if (day == 2) return 4;
    if (day == 3) return 5;
    return 6; // Day 4+: 5 standard waves + 1 final escalation boss wave
}

void WaveManager::resetForDay(int dayNumber) {
    m_dayNumber = dayNumber;
    m_currentWave = 0;
    m_totalWaves = getTotalWavesForDay(dayNumber);
    m_wavesCompleted = 0;
    m_currentWaveId = 0;
    m_waveEnemiesScheduled = 0;
    m_waveEnemiesSpawned = 0;
    m_waveEnemiesAlive = 0;
    m_nightActive = false;
    m_waveActive = false;
    m_isBossWave = false;
    m_nightTimer = 0.0f;
    m_waveActiveTimer = 0.0f;
    m_timeUntilNextWave = 4.0f;
    m_waveBannerTimer = 0.0f;
    m_waveClearedTimer = 0.0f;
    m_lastClearedWave = 0;
    m_pendingSpawns.clear();
    m_enemiesDefeatedThisNight = 0;
}

void WaveManager::startNight() {
    m_nightActive = true;
    m_nightTimer = 0.0f;
    m_waveActiveTimer = 0.0f;
    m_currentWave = 0;
    m_wavesCompleted = 0;
    m_totalWaves = getTotalWavesForDay(m_dayNumber);
    m_currentWaveId = 0;
    m_waveEnemiesScheduled = 0;
    m_waveEnemiesSpawned = 0;
    m_waveEnemiesAlive = 0;
    m_timeUntilNextWave = 4.0f;
    m_waveActive = false;
    m_isBossWave = false;
    m_waveBannerTimer = 0.0f;
    m_waveClearedTimer = 0.0f;
    m_lastClearedWave = 0;
    m_pendingSpawns.clear();
    m_enemiesDefeatedThisNight = 0;
}

void WaveManager::endNight() {
    m_nightActive = false;
    m_waveActive = false;
    m_pendingSpawns.clear();
    m_timeUntilNextWave = 0.0f;
    m_waveBannerTimer = 0.0f;
    m_waveClearedTimer = 0.0f;
}

void WaveManager::notifyEnemyKilled(int waveId) {
    m_enemiesDefeatedThisNight++;
    if (waveId == m_currentWaveId) {
        m_waveEnemiesAlive = std::max(0, m_waveEnemiesAlive - 1);
    }
}

void WaveManager::getWaveComposition(int day, int wave, int& crawlers, int& eaters, int& brutes, int& leviathans) {
    crawlers = 0;
    eaters = 0;
    brutes = 0;
    leviathans = 0;

    if (day <= 1) {
        // Day 1: 3 waves (Introductory)
        if (wave == 1) {
            crawlers = 5;
        } else if (wave == 2) {
            crawlers = 5;
            eaters = 1;
        } else {
            crawlers = 6;
            eaters = 2;
        }
    } else if (day == 2) {
        // Day 2: 4 waves (Moderate, introduce Brute)
        if (wave == 1) {
            crawlers = 6;
            eaters = 2;
        } else if (wave == 2) {
            crawlers = 6;
            eaters = 2;
            brutes = 1;
        } else if (wave == 3) {
            crawlers = 8;
            eaters = 3;
            brutes = 1;
        } else {
            crawlers = 8;
            eaters = 3;
            brutes = 2;
        }
    } else if (day == 3) {
        // Day 3: 5 waves (Hard, high pressure on mirrors & citadel)
        if (wave == 1) {
            crawlers = 8;
            eaters = 3;
            brutes = 1;
        } else if (wave == 2) {
            crawlers = 9;
            eaters = 3;
            brutes = 2;
        } else if (wave == 3) {
            crawlers = 10;
            eaters = 4;
            brutes = 2;
        } else if (wave == 4) {
            crawlers = 12;
            eaters = 4;
            brutes = 3;
        } else {
            crawlers = 14;
            eaters = 5;
            brutes = 3;
        }
    } else {
        // Day 4+: 6 waves (5 waves + Final Escalation Leviathan wave)
        if (wave == 1) {
            crawlers = 10;
            eaters = 3;
            brutes = 2;
        } else if (wave == 2) {
            crawlers = 12;
            eaters = 4;
            brutes = 3;
        } else if (wave == 3) {
            crawlers = 13;
            eaters = 4;
            brutes = 3;
        } else if (wave == 4) {
            crawlers = 14;
            eaters = 5;
            brutes = 4;
        } else if (wave == 5) {
            crawlers = 16;
            eaters = 5;
            brutes = 4;
        } else {
            // Wave 6: Final Escalation Boss Wave
            leviathans = 1;
            crawlers = 8;
            eaters = 3;
            brutes = 2;
        }
    }
}

WavePreview WaveManager::getUpcomingWavePreview(int dayNumber, float duskCountdown) const {
    WavePreview preview;
    preview.dayNumber = dayNumber;
    preview.totalWaves = getTotalWavesForDay(dayNumber);

    if (duskCountdown >= 0.0f) {
        // During Dusk: preview wave 1
        preview.waveNumber = 1;
        preview.countdown = duskCountdown;
        getWaveComposition(dayNumber, 1, preview.crawlers, preview.eaters, preview.brutes, preview.leviathans);
        preview.isBoss = (preview.leviathans > 0);
        preview.active = true;
        return preview;
    }

    if (m_nightActive) {
        int nextWave = (m_currentWave == 0) ? 1 : (m_currentWave + 1);
        if (nextWave <= m_totalWaves && !m_waveActive && m_timeUntilNextWave > 0.0f) {
            preview.waveNumber = nextWave;
            preview.countdown = m_timeUntilNextWave;
            getWaveComposition(m_dayNumber, nextWave, preview.crawlers, preview.eaters, preview.brutes, preview.leviathans);
            preview.isBoss = (preview.leviathans > 0);
            preview.active = true;
            return preview;
        }
    }

    preview.active = false;
    return preview;
}

void WaveManager::triggerWave(int waveIndex) {
    m_waveActive = true;
    m_waveActiveTimer = 0.0f;
    m_waveBannerTimer = 4.0f;
    m_pendingSpawns.clear();

    m_currentWaveId = m_dayNumber * 100 + waveIndex;

    auto addSpawn = [&](EnemyType type, float delay, float baseAngle) {
        float angleVariance = ((float)(rand() % 40 - 20) / 100.0f);
        m_pendingSpawns.push_back({ type, delay, baseAngle + angleVariance, m_currentWaveId });
    };

    float angles[4] = { 0.0f, PI * 0.5f, PI, PI * 1.5f };
    int angleIdx = 0;

    int crawlers = 0, eaters = 0, brutes = 0, leviathans = 0;
    getWaveComposition(m_dayNumber, waveIndex, crawlers, eaters, brutes, leviathans);
    m_isBossWave = (leviathans > 0);

    m_waveEnemiesScheduled = crawlers + eaters + brutes + leviathans;
    m_waveEnemiesSpawned = 0;
    m_waveEnemiesAlive = m_waveEnemiesScheduled;

    float delay = 0.2f;
    for (int i = 0; i < crawlers; ++i) {
        addSpawn(EnemyType::Crawler, delay, angles[angleIdx++ % 4]);
        delay += 0.35f;
    }
    for (int i = 0; i < eaters; ++i) {
        addSpawn(EnemyType::Eater, delay, angles[angleIdx++ % 4]);
        delay += 0.55f;
    }
    for (int i = 0; i < brutes; ++i) {
        addSpawn(EnemyType::Brute, delay, angles[angleIdx++ % 4]);
        delay += 0.75f;
    }
    for (int i = 0; i < leviathans; ++i) {
        addSpawn(EnemyType::Leviathan, delay + 0.5f, angles[angleIdx++ % 4]);
    }

    if (m_isBossWave) {
        AudioManager::instance().playSound(SoundID::NightAlarm, 1.0f);
    } else {
        AudioManager::instance().playSound(SoundID::NightAlarm, 0.75f);
    }
}

void WaveManager::update(float dt, std::vector<Enemy>& enemies, const Vec2& lighthousePos) {
    if (!m_nightActive) return;

    m_nightTimer += dt;
    if (m_waveBannerTimer > 0.0f) {
        m_waveBannerTimer -= dt;
    }
    if (m_waveClearedTimer > 0.0f) {
        m_waveClearedTimer -= dt;
    }

    // Intermission countdown
    if (!m_waveActive && m_currentWave < m_totalWaves) {
        m_timeUntilNextWave -= dt;
        if (m_timeUntilNextWave <= 0.0f) {
            m_currentWave++;
            triggerWave(m_currentWave);
        }
    }

    // Active wave clear monitoring (No arbitrary timeout! Explicit kill rule)
    if (m_waveActive) {
        m_waveActiveTimer += dt;

        if (m_pendingSpawns.empty()) {
            int waveEnemiesAliveInWorld = 0;
            for (const auto& e : enemies) {
                if (e.getWaveId() == m_currentWaveId && !e.isDead()) {
                    waveEnemiesAliveInWorld++;
                }
            }

            if (waveEnemiesAliveInWorld == 0 && m_waveEnemiesSpawned >= m_waveEnemiesScheduled) {
                m_waveActive = false;
                m_wavesCompleted = m_currentWave;
                m_lastClearedWave = m_currentWave;
                m_waveClearedTimer = 3.5f;
                AudioManager::instance().playSound(SoundID::DawnChime, 0.75f);

                if (m_currentWave < m_totalWaves) {
                    m_timeUntilNextWave = 7.0f; // 7 seconds tactical preparation window before next wave
                } else {
                    m_timeUntilNextWave = 0.0f;
                }
            }
        }
    }

    // Process pending spawns
    for (auto it = m_pendingSpawns.begin(); it != m_pendingSpawns.end(); ) {
        it->delay -= dt;
        if (it->delay <= 0.0f) {
            float dist = 780.0f + (float)(rand() % 80);
            Vec2 spawnPos = lighthousePos + Vec2::fromAngle(it->angle, dist);
            enemies.emplace_back(it->type, spawnPos, it->waveId);
            m_waveEnemiesSpawned++;
            ParticleSystem::instance().spawnShadowBurst(spawnPos, 14);
            it = m_pendingSpawns.erase(it);
        } else {
            ++it;
        }
    }
}
