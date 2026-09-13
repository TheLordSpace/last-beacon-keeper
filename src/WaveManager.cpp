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
    m_timeUntilNextWave = 0.0f;
    m_waveBannerTimer = 0.0f;
    m_isBossWave = false;
    m_enemiesDefeatedThisNight = 0;
}

void WaveManager::resetForDay(int dayNumber) {
    m_dayNumber = dayNumber;
    m_currentWave = 0;
    m_totalWaves = 3;
    m_nightActive = false;
    m_waveActive = false;
    m_isBossWave = false;
    m_nightTimer = 0.0f;
    m_timeUntilNextWave = 4.0f;
    m_waveBannerTimer = 0.0f;
    m_pendingSpawns.clear();
    m_enemiesDefeatedThisNight = 0;
}

void WaveManager::startNight() {
    m_nightActive = true;
    m_nightTimer = 0.0f;
    m_currentWave = 0;
    m_totalWaves = 3;
    m_timeUntilNextWave = 3.5f;
    m_waveActive = false;
    m_isBossWave = false;
    m_waveBannerTimer = 0.0f;
    m_pendingSpawns.clear();
    m_enemiesDefeatedThisNight = 0;
}

void WaveManager::endNight() {
    m_nightActive = false;
    m_waveActive = false;
    m_pendingSpawns.clear();
    m_timeUntilNextWave = 0.0f;
}

void WaveManager::triggerWave(int waveIndex) {
    m_waveActive = true;
    m_waveBannerTimer = 4.0f;
    m_pendingSpawns.clear();

    // Helper to queue an enemy with staggered delay and shore angle
    auto addSpawn = [&](EnemyType type, float delay, float baseAngle) {
        float angleVariance = ((float)(rand() % 40 - 20) / 100.0f);
        m_pendingSpawns.push_back({ type, delay, baseAngle + angleVariance });
    };

    // Shore angle sectors (West ~PI, North ~1.5*PI, East ~0, South ~0.5*PI)
    float angles[4] = { 0.0f, PI * 0.5f, PI, PI * 1.5f };

    if (m_dayNumber == 1) {
        // Day 1: Mostly Crawlers, few Eaters
        if (waveIndex == 1) {
            for (int i = 0; i < 5; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.5f, angles[i % 4]);
            }
        } else if (waveIndex == 2) {
            for (int i = 0; i < 6; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.45f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 1.5f, angles[0]);
        } else {
            // Wave 3
            for (int i = 0; i < 7; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.4f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 1.0f, angles[1]);
            addSpawn(EnemyType::Eater, 2.2f, angles[3]);
        }
    } else if (m_dayNumber == 2) {
        // Day 2: Crawlers, Eaters, and introduction of Brutes
        if (waveIndex == 1) {
            for (int i = 0; i < 6; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.4f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 1.2f, angles[2]);
            addSpawn(EnemyType::Eater, 2.0f, angles[0]);
        } else if (waveIndex == 2) {
            for (int i = 0; i < 7; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.35f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 1.0f, angles[1]);
            addSpawn(EnemyType::Eater, 2.0f, angles[3]);
            addSpawn(EnemyType::Brute, 2.5f, angles[2]);
        } else {
            // Wave 3
            for (int i = 0; i < 8; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.35f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 0.8f, angles[0]);
            addSpawn(EnemyType::Eater, 1.8f, angles[2]);
            addSpawn(EnemyType::Brute, 2.0f, angles[1]);
        }
    } else if (m_dayNumber == 3) {
        // Day 3: Larger waves, aggressive combinations
        if (waveIndex == 1) {
            for (int i = 0; i < 8; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.3f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 0.8f, angles[1]);
            addSpawn(EnemyType::Eater, 1.6f, angles[3]);
            addSpawn(EnemyType::Brute, 2.2f, angles[0]);
        } else if (waveIndex == 2) {
            for (int i = 0; i < 10; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.28f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 0.6f, angles[0]);
            addSpawn(EnemyType::Eater, 1.4f, angles[2]);
            addSpawn(EnemyType::Brute, 1.8f, angles[1]);
            addSpawn(EnemyType::Brute, 2.8f, angles[3]);
        } else {
            // Wave 3
            for (int i = 0; i < 12; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.25f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 0.5f, angles[0]);
            addSpawn(EnemyType::Eater, 1.2f, angles[1]);
            addSpawn(EnemyType::Eater, 2.0f, angles[2]);
            addSpawn(EnemyType::Brute, 2.0f, angles[0]);
            addSpawn(EnemyType::Brute, 3.0f, angles[2]);
        }
    } else {
        // Day 4+ (The Ultimate Tide / Leviathan Night)
        if (waveIndex == 1) {
            for (int i = 0; i < 10; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.28f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 0.8f, angles[0]);
            addSpawn(EnemyType::Eater, 1.6f, angles[2]);
            addSpawn(EnemyType::Brute, 2.0f, angles[1]);
            addSpawn(EnemyType::Brute, 2.8f, angles[3]);
        } else if (waveIndex == 2) {
            for (int i = 0; i < 12; ++i) {
                addSpawn(EnemyType::Crawler, 0.2f + i * 0.25f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 0.5f, angles[1]);
            addSpawn(EnemyType::Eater, 1.2f, angles[3]);
            addSpawn(EnemyType::Brute, 1.8f, angles[0]);
            addSpawn(EnemyType::Brute, 2.5f, angles[2]);
        } else {
            // Wave 3: The Abyssal Leviathan emerges!
            m_isBossWave = true;
            addSpawn(EnemyType::Leviathan, 1.0f, angles[2]);
            for (int i = 0; i < 8; ++i) {
                addSpawn(EnemyType::Crawler, 0.3f + i * 0.3f, angles[i % 4]);
            }
            addSpawn(EnemyType::Eater, 1.5f, angles[0]);
            addSpawn(EnemyType::Eater, 2.5f, angles[1]);
            addSpawn(EnemyType::Brute, 2.8f, angles[3]);
        }
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

    if (m_currentWave < m_totalWaves) {
        m_timeUntilNextWave -= dt;
        if (m_timeUntilNextWave <= 0.0f) {
            m_currentWave++;
            triggerWave(m_currentWave);
            if (m_currentWave < m_totalWaves) {
                m_timeUntilNextWave = 20.0f; // 20s between waves
            }
        }
    }

    // Process pending spawns
    for (auto it = m_pendingSpawns.begin(); it != m_pendingSpawns.end(); ) {
        it->delay -= dt;
        if (it->delay <= 0.0f) {
            float dist = 780.0f + (float)(rand() % 80);
            Vec2 spawnPos = lighthousePos + Vec2::fromAngle(it->angle, dist);
            enemies.emplace_back(it->type, spawnPos);
            ParticleSystem::instance().spawnShadowBurst(spawnPos, 14);
            it = m_pendingSpawns.erase(it);
        } else {
            ++it;
        }
    }
}
