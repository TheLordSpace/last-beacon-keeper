#include "DayNightManager.h"
#include "Game.h"
#include "Audio.h"
#include "Particles.h"
#include "Localization.h"
#include <algorithm>

DayNightManager::DayNightManager() {
    init();
}

void DayNightManager::init() {
    m_dayNumber = 1;
    m_phase = DayPhase::Day;
    m_phaseTimer = 0.0f;
    m_phaseDuration = m_dayDuration;
    m_dawnSummaryTimer = 0.0f;
    m_lighthouseHurtAlertTimer = 0.0f;
    m_totalEnemiesDefeated = 0;
    m_lastDawnReward = DawnReward();
    m_waveManager.resetForDay(1);

    m_fuelAlertCooldown = 0.0f;
    m_lastFuelThreshold = 0;
    m_hpAlertCooldown = 0.0f;
    m_lastHpThreshold = 0;
}

void DayNightManager::resetGame() {
    init();
}

uint8_t DayNightManager::getAmbientDarkness() const {
    switch (m_phase) {
    case DayPhase::Day:
        return 16;
    case DayPhase::Dusk: {
        float t = std::clamp(m_phaseTimer / m_duskDuration, 0.0f, 1.0f);
        return static_cast<uint8_t>(16.0f + t * 226.0f);
    }
    case DayPhase::Night:
        return 242;
    case DayPhase::Dawn: {
        float t = std::clamp(m_phaseTimer / m_dawnDuration, 0.0f, 1.0f);
        return static_cast<uint8_t>(242.0f * (1.0f - t * 0.93f));
    }
    }
    return 16;
}

ColorRGBA DayNightManager::getAmbientColor() const {
    uint8_t darkness = getAmbientDarkness();
    if (m_phase == DayPhase::Dusk) {
        return ColorRGBA{
            static_cast<uint8_t>(255 - darkness),
            static_cast<uint8_t>(std::max(10, 240 - darkness)),
            static_cast<uint8_t>(std::max(5, 190 - darkness)),
            255
        };
    } else if (m_phase == DayPhase::Dawn) {
        return ColorRGBA{
            static_cast<uint8_t>(255 - darkness),
            static_cast<uint8_t>(255 - darkness),
            static_cast<uint8_t>(std::max(35, 245 - darkness)),
            255
        };
    }
    return ColorRGBA{
        static_cast<uint8_t>(255 - darkness),
        static_cast<uint8_t>(255 - darkness),
        static_cast<uint8_t>(std::max(20, 255 - darkness)),
        255
    };
}

WavePreview DayNightManager::getUpcomingWavePreview() const {
    if (m_phase == DayPhase::Dusk) {
        float duskTimeLeft = std::max(0.0f, m_phaseDuration - m_phaseTimer);
        return m_waveManager.getUpcomingWavePreview(m_dayNumber, duskTimeLeft);
    } else if (m_phase == DayPhase::Night) {
        return m_waveManager.getUpcomingWavePreview(m_dayNumber, -1.0f);
    }
    WavePreview empty;
    empty.active = false;
    return empty;
}

void DayNightManager::calculateDawnReward(
    const Lighthouse& lighthouse,
    const std::vector<PlacedMirror>& mirrors,
    Player& player
) {
    m_lastDawnReward = DawnReward();
    m_lastDawnReward.dayNumber = m_dayNumber;
    m_lastDawnReward.wavesCompleted = m_waveManager.getWavesCompleted();
    m_lastDawnReward.totalWaves = m_waveManager.getTotalWaves();
    m_lastDawnReward.enemiesDefeated = m_waveManager.getEnemiesDefeatedThisNight();
    m_lastDawnReward.mirrorsPreserved = static_cast<int>(mirrors.size());
    m_lastDawnReward.lighthouseHp = lighthouse.getHealth();
    m_lastDawnReward.lighthouseMaxHp = lighthouse.getMaxHealth();
    m_lastDawnReward.lighthouseHpPercent = (lighthouse.getHealth() / lighthouse.getMaxHealth()) * 100.0f;
    m_lastDawnReward.fuelRemaining = lighthouse.getFuel();

    // Base survival rewards
    m_lastDawnReward.wood = 8;
    m_lastDawnReward.crystals = 6;
    m_lastDawnReward.oil = 12;
    m_lastDawnReward.salves = 1;

    // Defeated enemies bonus
    if (m_lastDawnReward.enemiesDefeated >= 8) {
        m_lastDawnReward.wood += 4;
        m_lastDawnReward.crystals += 3;
    }
    if (m_lastDawnReward.enemiesDefeated >= 16) {
        m_lastDawnReward.wood += 4;
        m_lastDawnReward.crystals += 4;
        m_lastDawnReward.oil += 6;
    }

    // Lighthouse integrity bonus
    if (m_lastDawnReward.lighthouseHpPercent >= 70.0f) {
        m_lastDawnReward.wood += 4;
        m_lastDawnReward.crystals += 4;
    }
    if (m_lastDawnReward.lighthouseHpPercent >= 90.0f) {
        m_lastDawnReward.relicCores += 1;
    }

    // Preserved mirrors bonus
    int preservedBonus = std::min(5, m_lastDawnReward.mirrorsPreserved);
    m_lastDawnReward.wood += preservedBonus * 2;
    m_lastDawnReward.crystals += preservedBonus * 1;

    // Progression helper: from Day 3 onward, ensure at least 1 relic core is earned
    if (m_dayNumber >= 3 && m_lastDawnReward.relicCores == 0) {
        m_lastDawnReward.relicCores += 1;
    }

    // Apply to player inventory
    player.wood += m_lastDawnReward.wood;
    player.crystals += m_lastDawnReward.crystals;
    player.oil += m_lastDawnReward.oil;
    player.relicCores += m_lastDawnReward.relicCores;
    player.salves += m_lastDawnReward.salves;
}

void DayNightManager::transitionTo(
    DayPhase nextPhase,
    Lighthouse& lighthouse,
    Player& player,
    const std::vector<PlacedMirror>& mirrors,
    std::vector<Enemy>& enemies,
    IslandMap& map,
    GameState& gameState,
    std::string& statusOut,
    float& statusTimerOut
) {
    m_phase = nextPhase;
    m_phaseTimer = 0.0f;

    switch (m_phase) {
    case DayPhase::Day:
        m_phaseDuration = m_dayDuration;
        m_dayNumber++;
        m_waveManager.resetForDay(m_dayNumber);
        {
            bool isAr = Localization::instance().isArabic();
            statusOut = isAr ? ("اليوم " + std::to_string(m_dayNumber) + ": اجمع الموارد وحصن دفاعاتك.") :
                               ("Day " + std::to_string(m_dayNumber) + ": Gather resources and prepare defenses.");
            statusTimerOut = 5.0f;
        }
        break;

    case DayPhase::Dusk:
        m_phaseDuration = m_duskDuration;
        AudioManager::instance().playSound(SoundID::NightAlarm, 0.85f);
        statusOut = Localization::instance().get("MSG_DUSK");
        statusTimerOut = 5.0f;
        break;

    case DayPhase::Night:
        m_phaseDuration = m_nightDuration;
        AudioManager::instance().setMusicNight(true);
        m_waveManager.startNight();
        statusOut = Localization::instance().get("MSG_NIGHT");
        statusTimerOut = 5.0f;
        break;

    case DayPhase::Dawn:
        m_phaseDuration = m_dawnDuration;
        m_waveManager.endNight();
        AudioManager::instance().playSound(SoundID::DawnChime, 1.0f);
        AudioManager::instance().setMusicNight(false);

        // Vaporize remaining shadow enemies in golden light
        for (auto& e : enemies) {
            e.takeDamage(9999.0f, true);
            ParticleSystem::instance().spawnSparks(e.getPos(), 20, ColorRGBA{ 255, 240, 160, 255 });
        }

        calculateDawnReward(lighthouse, mirrors, player);
        m_dawnSummaryTimer = 12.0f;
        statusOut = Localization::instance().get("MSG_DAWN");
        statusTimerOut = 5.0f;

        // Check Victory: all 3 ancient altars ignited and Night 4 completed
        if (map.getIgnitedAltarsCount() >= 3 && m_dayNumber >= 4) {
            gameState = GameState::Victory;
            AudioManager::instance().playSound(SoundID::DawnChime, 1.0f);
        }
        break;
    }
}

void DayNightManager::update(
    float dt,
    std::vector<Enemy>& enemies,
    std::vector<PlacedMirror>& mirrors,
    Lighthouse& lighthouse,
    Player& player,
    IslandMap& map,
    GameState& gameState,
    std::string& statusOut,
    float& statusTimerOut
) {
    m_phaseTimer += dt;
    if (m_dawnSummaryTimer > 0.0f) {
        m_dawnSummaryTimer -= dt;
    }
    if (m_lighthouseHurtAlertTimer > 0.0f) {
        m_lighthouseHurtAlertTimer -= dt;
    }

    // Lighthouse HP danger feedback
    if (m_hpAlertCooldown > 0.0f) {
        m_hpAlertCooldown -= dt;
    }
    float hpPercent = (lighthouse.getHealth() / lighthouse.getMaxHealth()) * 100.0f;
    if (hpPercent < 25.0f && lighthouse.getHealth() > 0.0f) {
        if (m_lastHpThreshold != 2 || m_hpAlertCooldown <= 0.0f) {
            m_lastHpThreshold = 2;
            m_hpAlertCooldown = 10.0f;
            statusOut = Localization::instance().get("ALERT_HP_CRITICAL");
            statusTimerOut = 4.0f;
            AudioManager::instance().playSound(SoundID::NightAlarm, 1.0f);
        }
    } else if (hpPercent < 50.0f && lighthouse.getHealth() > 0.0f) {
        if (m_lastHpThreshold < 1 || m_hpAlertCooldown <= 0.0f) {
            m_lastHpThreshold = 1;
            m_hpAlertCooldown = 14.0f;
            statusOut = Localization::instance().get("ALERT_HP_LOW");
            statusTimerOut = 3.5f;
            AudioManager::instance().playSound(SoundID::NightAlarm, 0.8f);
        }
    } else if (hpPercent >= 55.0f) {
        m_lastHpThreshold = 0;
    }

    // Low fuel feedback
    if (m_fuelAlertCooldown > 0.0f) {
        m_fuelAlertCooldown -= dt;
    }
    float fuelPercent = (lighthouse.getFuel() / lighthouse.getMaxFuel()) * 100.0f;
    if (fuelPercent < 15.0f && lighthouse.getFuel() > 0.0f) {
        if (m_lastFuelThreshold != 2 || m_fuelAlertCooldown <= 0.0f) {
            m_lastFuelThreshold = 2;
            m_fuelAlertCooldown = 12.0f;
            statusOut = Localization::instance().get("ALERT_FUEL_CRITICAL");
            statusTimerOut = 4.0f;
            AudioManager::instance().playSound(SoundID::NightAlarm, 0.9f);
        }
    } else if (fuelPercent < 30.0f && lighthouse.getFuel() > 0.0f) {
        if (m_lastFuelThreshold < 1 || m_fuelAlertCooldown <= 0.0f) {
            m_lastFuelThreshold = 1;
            m_fuelAlertCooldown = 16.0f;
            statusOut = Localization::instance().get("ALERT_FUEL_LOW");
            statusTimerOut = 3.5f;
            AudioManager::instance().playSound(SoundID::NightAlarm, 0.75f);
        }
    } else if (fuelPercent >= 35.0f) {
        m_lastFuelThreshold = 0;
    }

    switch (m_phase) {
    case DayPhase::Day:
        if (m_phaseTimer >= m_dayDuration) {
            transitionTo(DayPhase::Dusk, lighthouse, player, mirrors, enemies, map, gameState, statusOut, statusTimerOut);
        }
        break;

    case DayPhase::Dusk:
        if (m_phaseTimer >= m_duskDuration) {
            transitionTo(DayPhase::Night, lighthouse, player, mirrors, enemies, map, gameState, statusOut, statusTimerOut);
        }
        break;

    case DayPhase::Night:
        m_waveManager.update(dt, enemies, lighthouse.getLanternPos());
        if (m_phaseTimer >= m_nightDuration) {
            transitionTo(DayPhase::Dawn, lighthouse, player, mirrors, enemies, map, gameState, statusOut, statusTimerOut);
        }
        break;

    case DayPhase::Dawn:
        if (m_phaseTimer >= m_dawnDuration) {
            transitionTo(DayPhase::Day, lighthouse, player, mirrors, enemies, map, gameState, statusOut, statusTimerOut);
        }
        break;
    }
}
