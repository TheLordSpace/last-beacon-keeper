#pragma once

#include "Common.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>

class IslandMap;

struct ItemDrop {
    Vec2 pos;
    Vec2 vel;
    NodeType type;
    int amount = 1;
    float life = 25.0f;
    float bounceTimer = 0.0f;
};

struct PlacedMirror {
    int id = 0;
    Vec2 pos;
    float angle = 0.0f; // in radians
    float health = 80.0f;
    float maxHealth = 80.0f;
    float length = 38.0f;
    float rotateFeedbackTimer = 0.0f;

    Vec2 getNormal() const {
        return Vec2(-std::sin(angle), std::cos(angle));
    }

    void getEndpoints(Vec2& p1, Vec2& p2) const {
        Vec2 dir(std::cos(angle), std::sin(angle));
        p1 = pos - dir * (length * 0.5f);
        p2 = pos + dir * (length * 0.5f);
    }
};

class Enemy {
public:
    Enemy(EnemyType type, const Vec2& pos, int waveId = 0);

    void update(float dt, const Vec2& playerPos, const Vec2& lighthousePos,
                std::vector<PlacedMirror>& mirrors, const IslandMap& map);
    void render(SDL_Renderer* ren, const Vec2& cameraOffset);
    void takeDamage(float dmg, bool isLight);
    void applyHitReaction(const Vec2& knockbackDir);

    bool isDead() const { return m_health <= 0.0f; }
    EnemyType getType() const { return m_type; }
    Vec2 getPos() const { return m_pos; }
    float getRadius() const { return m_radius; }
    float getDamage() const { return m_attackDmg; }
    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    bool isBurning() const { return m_burnTimer > 0.0f; }
    int getWaveId() const { return m_waveId; }
    void setWaveId(int id) { m_waveId = id; }

private:
    EnemyType m_type;
    Vec2 m_pos;
    Vec2 m_vel;
    int m_waveId = 0;
    float m_health = 40.0f;
    float m_maxHealth = 40.0f;
    float m_speed = 90.0f;
    float m_radius = 16.0f;
    float m_attackDmg = 15.0f;
    float m_attackCooldown = 0.0f;
    float m_animTimer = 0.0f;
    float m_burnTimer = 0.0f;
    float m_stunTimer = 0.0f;
    float m_hitFlashTimer = 0.0f;
};

class Player {
public:
    Player();

    void update(float dt, const IslandMap& map);
    void render(SDL_Renderer* ren, const Vec2& cameraOffset);

    void handleInput(const Uint8* keystate, const Vec2& mouseWorld);
    void swingTool(IslandMap& map, std::vector<Enemy>& enemies);
    void tryDash();
    void takeDamage(float dmg);
    void heal(float amount);

    Vec2 getPos() const { return m_pos; }
    Vec2 getCenterPos() const { return m_pos + Vec2(0.0f, -8.0f); }
    Vec2 getLanternPos() const { return m_pos + Vec2::fromAngle(m_aimAngle, 20.0f) + Vec2(0.0f, -4.0f); }
    void setPos(const Vec2& p) { m_pos = p; }
    float getAimAngle() const { return m_aimAngle; }
    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    bool isDashing() const { return m_dashTimer > 0.0f; }
    bool isSwinging() const { return m_swingTimer > 0.0f; }

    void setBonuses(float speedBonus_, float dashCdBonus_) {
        speedBonus = speedBonus_;
        dashCooldownBonus = dashCdBonus_;
    }
    float speedBonus = 0.0f;
    float dashCooldownBonus = 0.0f;

    // Inventory
    int wood = 25;
    int crystals = 15;
    int oil = 20;
    int mirrorsInBag = 2;
    int relicCores = 0;
    int salves = 1;

private:
    Vec2 m_pos{ 1300.0f, 1140.0f };
    Vec2 m_vel{ 0.0f, 0.0f };
    float m_speed = 175.0f;
    float m_health = 100.0f;
    float m_maxHealth = 100.0f;
    float m_aimAngle = 0.0f;

    float m_stepSoundTimer = 0.0f;
    float m_dashTimer = 0.0f;
    float m_dashCooldown = 0.0f;
    Vec2 m_dashDir{ 1.0f, 0.0f };

    float m_swingTimer = 0.0f;
    float m_swingAngle = 0.0f;

    float m_invulnTimer = 0.0f;
};
