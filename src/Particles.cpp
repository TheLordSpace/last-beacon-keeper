#include "Particles.h"
#include <cstdlib>

ParticleSystem& ParticleSystem::instance() {
    static ParticleSystem s_inst;
    return s_inst;
}

void ParticleSystem::clear() {
    m_particles.clear();
}

void ParticleSystem::spawn(const Vec2& pos, const Vec2& vel, ColorRGBA color, float life, float size, bool isGlow) {
    Particle p;
    p.pos = pos;
    p.vel = vel;
    p.color = color;
    p.life = life;
    p.maxLife = life;
    p.size = size;
    p.isGlow = isGlow;
    m_particles.push_back(p);
}

void ParticleSystem::spawnSparks(const Vec2& pos, int count, ColorRGBA color) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)(rand() % 360) * (PI / 180.0f);
        float speed = 40.0f + (float)(rand() % 160);
        Vec2 vel = Vec2::fromAngle(angle, speed);
        float life = 0.2f + (float)(rand() % 40) / 100.0f;
        float size = 2.0f + (float)(rand() % 4);
        spawn(pos, vel, color, life, size, true);
    }
}

void ParticleSystem::spawnShadowBurst(const Vec2& pos, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)(rand() % 360) * (PI / 180.0f);
        float speed = 30.0f + (float)(rand() % 120);
        Vec2 vel = Vec2::fromAngle(angle, speed);
        float life = 0.3f + (float)(rand() % 50) / 100.0f;
        float size = 3.0f + (float)(rand() % 5);
        ColorRGBA col;
        col.r = 30 + rand() % 50;
        col.g = 10 + rand() % 30;
        col.b = 60 + rand() % 80;
        col.a = 230;
        spawn(pos, vel, col, life, size, false);
    }
}

void ParticleSystem::spawnFireflies(const Vec2& center, float radius, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)(rand() % 360) * (PI / 180.0f);
        float dist = (float)(rand() % (int)radius);
        Vec2 pos = center + Vec2::fromAngle(angle, dist);
        Vec2 vel((float)(rand() % 20 - 10), (float)(rand() % 20 - 10));
        ColorRGBA col;
        col.r = 230 + rand() % 25;
        col.g = 220 + rand() % 35;
        col.b = 100 + rand() % 50;
        col.a = 200;
        spawn(pos, vel, col, 2.0f + (float)(rand() % 30) / 10.0f, 2.5f, true);
    }
}

void ParticleSystem::spawnBeamSparks(const Vec2& pos, const Vec2& dir) {
    for (int i = 0; i < 3; ++i) {
        float angle = dir.angle() + PI + ((float)(rand() % 100 - 50) / 100.0f);
        float speed = 60.0f + (float)(rand() % 140);
        Vec2 vel = Vec2::fromAngle(angle, speed);
        ColorRGBA col{ 255, (uint8_t)(200 + rand() % 55), (uint8_t)(100 + rand() % 100), 255 };
        spawn(pos, vel, col, 0.15f + (float)(rand() % 20) / 100.0f, 2.0f + (rand() % 3), true);
    }
}

void ParticleSystem::update(float dt) {
    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            it->pos += it->vel * dt;
            it->vel *= (1.0f - dt * 2.0f); // friction
            ++it;
        }
    }
}

void ParticleSystem::render(SDL_Renderer* ren, const Vec2& cameraOffset) {
    for (const auto& p : m_particles) {
        float screenX = p.pos.x - cameraOffset.x;
        float screenY = p.pos.y - cameraOffset.y;

        if (screenX < -20 || screenX > WINDOW_WIDTH + 20 ||
            screenY < -20 || screenY > WINDOW_HEIGHT + 20) {
            continue;
        }

        float alphaRatio = (p.life / p.maxLife);
        uint8_t alpha = static_cast<uint8_t>(p.color.a * alphaRatio);

        SDL_SetRenderDrawBlendMode(ren, p.isGlow ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b, alpha);

        SDL_Rect r{
            static_cast<int>(screenX - p.size * 0.5f),
            static_cast<int>(screenY - p.size * 0.5f),
            static_cast<int>(p.size),
            static_cast<int>(p.size)
        };
        SDL_RenderFillRect(ren, &r);
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
}
