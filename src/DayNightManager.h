#pragma once

#include "Common.h"
#include "WaveManager.h"
#include "Entities.h"
#include "Lighthouse.h"
#include "Map.h"
#include <string>

// Forward declaration for GameState
enum class GameState;

struct DawnReward {
    int dayNumber = 1;
    int wavesCompleted = 3;
    int totalWaves = 3;
    int enemiesDefeated = 0;
    float lighthouseHp = 500.0f;
    float lighthouseMaxHp = 500.0f;
    float lighthouseHpPercent = 100.0f;
    float fuelRemaining = 0.0f;
    int mirrorsPreserved = 0;
    int wood = 0;
    int crystals = 0;
    int oil = 0;
    int relicCores = 0;
    int salves = 0;
};

class DayNightManager {
public:
    DayNightManager();

    void init();
    void resetGame();

    void update(
        float dt,
        std::vector<Enemy>& enemies,
        std::vector<PlacedMirror>& mirrors,
        Lighthouse& lighthouse,
        Player& player,
        IslandMap& map,
        GameState& gameState,
        std::string& statusOut,
        float& statusTimerOut
    );

    int getDayNumber() const { return m_dayNumber; }
    DayPhase getPhase() const { return m_phase; }
    float getPhaseTimer() const { return m_phaseTimer; }
    float getPhaseDuration() const { return m_phaseDuration; }
    int getTimeLeft() const { return std::max(0, static_cast<int>(m_phaseDuration - m_phaseTimer)); }

    float getDayDuration() const { return m_dayDuration; }
    float getDuskDuration() const { return m_duskDuration; }
    float getNightDuration() const { return m_nightDuration; }
    float getDawnDuration() const { return m_dawnDuration; }

    uint8_t getAmbientDarkness() const;
    ColorRGBA getAmbientColor() const;

    WaveManager& getWaveManager() { return m_waveManager; }
    const WaveManager& getWaveManager() const { return m_waveManager; }

    WavePreview getUpcomingWavePreview() const;

    const DawnReward& getLastDawnReward() const { return m_lastDawnReward; }
    bool hasDawnReward() const { return m_phase == DayPhase::Dawn || m_dawnSummaryTimer > 0.0f; }
    float getDawnSummaryTimer() const { return m_dawnSummaryTimer; }

    bool isLighthouseUnderAttack() const { return m_lighthouseHurtAlertTimer > 0.0f; }
    void notifyLighthouseHurt() { m_lighthouseHurtAlertTimer = 2.0f; }

    int getTotalEnemiesDefeated() const { return m_totalEnemiesDefeated; }
    void notifyEnemyKilled() {
        m_waveManager.notifyEnemyKilled();
        m_totalEnemiesDefeated++;
    }

private:
    int m_dayNumber = 1;
    DayPhase m_phase = DayPhase::Day;
    float m_phaseTimer = 0.0f;
    float m_phaseDuration = 60.0f;

    float m_dayDuration = 60.0f;
    float m_duskDuration = 18.0f;
    float m_nightDuration = 70.0f;
    float m_dawnDuration = 12.0f;

    WaveManager m_waveManager;

    DawnReward m_lastDawnReward;
    float m_dawnSummaryTimer = 0.0f;
    float m_lighthouseHurtAlertTimer = 0.0f;

    float m_fuelAlertCooldown = 0.0f;
    int m_lastFuelThreshold = 0; // 0 = ok, 1 = <30%, 2 = <15%
    float m_hpAlertCooldown = 0.0f;
    int m_lastHpThreshold = 0;   // 0 = ok, 1 = <50%, 2 = <25%

    int m_totalEnemiesDefeated = 0;

    void transitionTo(
        DayPhase nextPhase,
        Lighthouse& lighthouse,
        Player& player,
        const std::vector<PlacedMirror>& mirrors,
        std::vector<Enemy>& enemies,
        IslandMap& map,
        GameState& gameState,
        std::string& statusOut,
        float& statusTimerOut
    );

    void calculateDawnReward(
        const Lighthouse& lighthouse,
        const std::vector<PlacedMirror>& mirrors,
        Player& player
    );
};
