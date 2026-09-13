#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr float WORLD_WIDTH = 2600.0f;
constexpr float WORLD_HEIGHT = 2000.0f;

constexpr float PI = 3.14159265358979323846f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator-() const { return Vec2(-x, -y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        float l = length();
        if (l > 0.0001f) return Vec2(x / l, y / l);
        return Vec2(0.0f, 0.0f);
    }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }

    float dist(const Vec2& o) const {
        return (*this - o).length();
    }

    Vec2 reflect(const Vec2& normal) const {
        return *this - normal * (2.0f * this->dot(normal));
    }

    static Vec2 fromAngle(float radians, float length = 1.0f) {
        return Vec2(std::cos(radians) * length, std::sin(radians) * length);
    }

    float angle() const {
        return std::atan2(y, x);
    }
};

enum class DayPhase {
    Day,
    Dusk,
    Night,
    Dawn
};

enum class LensType {
    Focused,   // High-damage single concentrated beam (melts heavy units)
    WideAmber, // Wide defensive barrier cone that repels and scorches swarms
    UVPulse    // Reveals cloaked enemies and deals freezing shockwave
};

enum class EnemyType {
    Crawler,  // Fast, low health, scurries from dark
    Eater,    // Targets mirrors and fuel stores
    Brute,    // Heavy armored rock-shadow, requires sustained beam
    Leviathan // Night 4 Boss with shadow arms and huge HP
};

enum class NodeType {
    Tree,
    Crystal,
    OilBarrel,
    Altar
};

struct ColorRGBA {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

#include <SDL2/SDL.h>

inline void drawThickBeam(SDL_Renderer* ren, const Vec2& p1, const Vec2& p2, float width, SDL_Color color) {
    Vec2 dir = p2 - p1;
    float len = dir.length();
    if (len < 0.001f) return;
    Vec2 norm(-dir.y / len, dir.x / len);
    Vec2 off = norm * (width * 0.5f);

    SDL_Vertex verts[4];
    verts[0].position = { p1.x - off.x, p1.y - off.y };
    verts[0].color = color;
    verts[0].tex_coord = { 0.0f, 0.0f };

    verts[1].position = { p1.x + off.x, p1.y + off.y };
    verts[1].color = color;
    verts[1].tex_coord = { 1.0f, 0.0f };

    verts[2].position = { p2.x + off.x, p2.y + off.y };
    verts[2].color = color;
    verts[2].tex_coord = { 1.0f, 1.0f };

    verts[3].position = { p2.x - off.x, p2.y - off.y };
    verts[3].color = color;
    verts[3].tex_coord = { 0.0f, 1.0f };

    int indices[6] = { 0, 1, 2, 0, 2, 3 };
    SDL_RenderGeometry(ren, nullptr, verts, 4, indices, 6);
}
