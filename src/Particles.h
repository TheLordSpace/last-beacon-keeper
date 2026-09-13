#pragma once

#include "Common.h"
#include <SDL2/SDL.h>
#include <vector>

struct Particle {
    Vec2 pos;
    Vec2 vel;
    ColorRGBA color;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 3.0f;
    bool isGlow = false;
};

class ParticleSystem {
public:
    static ParticleSystem& instance();

    void update(float dt);
    void render(SDL_Renderer* ren, const Vec2& cameraOffset);

    void spawn(const Vec2& pos, const Vec2& vel, ColorRGBA color, float life, float size, bool isGlow = false);
    void spawnSparks(const Vec2& pos, int count, ColorRGBA color);
    void spawnShadowBurst(const Vec2& pos, int count);
    void spawnFireflies(const Vec2& center, float radius, int count);
    void spawnBeamSparks(const Vec2& pos, const Vec2& dir);

    void clear();

private:
    ParticleSystem() = default;
    std::vector<Particle> m_particles;
};
