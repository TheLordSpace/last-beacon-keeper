#pragma once

#include "Common.h"
#include "Entities.h"
#include <SDL2/SDL.h>
#include <vector>

struct BeamSegment {
    Vec2 start;
    Vec2 end;
    float width = 8.0f;
    ColorRGBA color{ 255, 240, 150, 255 };
    float pulse = 0.0f;
};

class Lighthouse {
public:
    Lighthouse();

    void update(float dt, std::vector<Enemy>& enemies, std::vector<PlacedMirror>& mirrors);
    void renderBase(SDL_Renderer* ren, const Vec2& cameraOffset);
    void renderBeams(SDL_Renderer* ren, const Vec2& cameraOffset);

    void setBeamAngle(float angle) { m_beamAngle = angle; }
    float getBeamAngle() const { return m_beamAngle; }

    bool isManned() const { return m_manned; }
    void setManned(bool manned) { m_manned = manned; }

    void addFuel(float amount);
    void repair(float amount);
    void takeDamage(float amount);

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    float getFuel() const { return m_fuel; }
    float getMaxFuel() const { return m_maxFuel; }
    float getHeat() const { return m_heat; }
    float getMaxHeat() const { return m_maxHeat; }
    bool isOverheated() const { return m_isOverheated; }

    LensType getLens() const { return m_currentLens; }
    void setLens(LensType lens) { m_currentLens = lens; }

    // Upgrades (Max level 3 each)
    bool unlockWideLens = false;
    bool unlockUVLens = false;
    int beamPowerLevel = 1;       // 1..3: +25% damage per level
    int beamEfficiencyLevel = 1;  // 1..3: -25% fuel drain per level
    int mirrorDurabilityLevel = 1;// 1..3: +45 mirror HP per level
    int lighthouseArmorLevel = 1; // 1..3: +150 / +200 max HP
    int swiftBootsLevel = 1;      // 1..3: +28 speed, -0.18s dash cd

    float getBeamDamageMultiplier() const {
        return 1.0f + (beamPowerLevel - 1) * 0.25f;
    }
    float getFuelDrainMultiplier() const {
        return std::max(0.4f, 1.0f - (beamEfficiencyLevel - 1) * 0.25f);
    }
    float getMirrorMaxHealth() const {
        return 80.0f + (mirrorDurabilityLevel - 1) * 45.0f;
    }
    float getPlayerSpeedBonus() const {
        return (swiftBootsLevel - 1) * 28.0f;
    }
    float getPlayerDashCooldownBonus() const {
        return (swiftBootsLevel - 1) * 0.18f;
    }
    void upgradeLighthouseArmor() {
        if (lighthouseArmorLevel < 3) {
            lighthouseArmorLevel++;
            float bonus = (lighthouseArmorLevel == 2) ? 150.0f : 200.0f;
            m_maxHealth += bonus;
            m_health = std::min(m_maxHealth, m_health + bonus);
        }
    }

    Vec2 getPos() const { return m_pos; }
    Vec2 getLanternPos() const { return m_pos + Vec2(0.0f, -104.0f); }
    const std::vector<BeamSegment>& getBeamSegments() const { return m_beamSegments; }

private:
    Vec2 m_pos{ 1300.0f, 1000.0f };
    float m_health = 500.0f;
    float m_maxHealth = 500.0f;
    float m_fuel = 100.0f;
    float m_maxFuel = 100.0f;
    float m_heat = 0.0f;
    float m_maxHeat = 100.0f;
    bool m_isOverheated = false;

    float m_beamAngle = 0.0f;
    float m_autoSweepTimer = 0.0f;
    bool m_manned = false;
    LensType m_currentLens = LensType::Focused;

    std::vector<BeamSegment> m_beamSegments;

    void calculateBeams(std::vector<PlacedMirror>& mirrors);
};
