#include "Lighthouse.h"
#include "Audio.h"
#include "Particles.h"
#include <cmath>
#include <limits>
#include <algorithm>

// Line segment intersection helper
static bool lineIntersection(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec2& p4, Vec2& hit) {
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    if (std::abs(d) < 0.0001f) return false;

    float t = ((p1.x - p3.x) * (p3.y - p4.y) - (p1.y - p3.y) * (p3.x - p4.x)) / d;
    float u = -((p1.x - p2.x) * (p1.y - p3.y) - (p1.y - p2.y) * (p1.x - p3.x)) / d;

    if (t >= 0.001f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        hit = p1 + (p2 - p1) * t;
        return true;
    }
    return false;
}

// Distance from point to segment
static float distToSegment(const Vec2& p, const Vec2& a, const Vec2& b) {
    Vec2 ab = b - a;
    float l2 = ab.lengthSq();
    if (l2 < 0.0001f) return (p - a).length();
    float t = std::clamp((p - a).dot(ab) / l2, 0.0f, 1.0f);
    Vec2 proj = a + ab * t;
    return (p - proj).length();
}

Lighthouse::Lighthouse() = default;

void Lighthouse::addFuel(float amount) {
    m_fuel = std::min(m_maxFuel, m_fuel + amount);
}

void Lighthouse::repair(float amount) {
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Lighthouse::takeDamage(float amount) {
    m_health -= amount;
    AudioManager::instance().playSound(SoundID::PlayerHurt, 0.9f);
    ParticleSystem::instance().spawnSparks(m_pos, 15, ColorRGBA{ 255, 100, 50, 255 });
}

void Lighthouse::calculateBeams(std::vector<PlacedMirror>& mirrors) {
    m_beamSegments.clear();
    if (m_fuel <= 0.0f || m_isOverheated) return;

    Vec2 origin = getLanternPos(); // Center of glass lantern room

    auto traceRay = [&](const Vec2& startPos, float initialAngle, ColorRGBA color, float width) {
        Vec2 curStart = startPos;
        Vec2 curDir = Vec2::fromAngle(initialAngle);
        float remainingDist = 950.0f;
        int maxBounces = 4;
        float currentPulse = 0.0f;

        for (int bounce = 0; bounce < maxBounces; ++bounce) {
            Vec2 curEnd = curStart + curDir * remainingDist;
            PlacedMirror* hitMirror = nullptr;
            Vec2 bestHit = curEnd;
            float bestDistSq = remainingDist * remainingDist;

            // Check collision against all placed mirrors
            for (auto& m : mirrors) {
                if (m.health <= 0.0f) continue;
                Vec2 mp1, mp2;
                m.getEndpoints(mp1, mp2);
                Vec2 hit;
                if (lineIntersection(curStart, curEnd, mp1, mp2, hit)) {
                    float dsq = (hit - curStart).lengthSq();
                    if (dsq < bestDistSq && dsq > 1.0f) {
                        bestDistSq = dsq;
                        bestHit = hit;
                        hitMirror = &m;
                    }
                }
            }

            BeamSegment seg;
            seg.start = curStart;
            seg.end = bestHit;
            seg.width = width;
            seg.color = color;
            seg.pulse = currentPulse;
            m_beamSegments.push_back(seg);

            // Shimmering light photons along newly redirected beam
            if (currentPulse > 0.0f && (rand() % 100 < 30)) {
                float t = (float)(rand() % 90 + 5) / 100.0f;
                Vec2 pt = curStart + (bestHit - curStart) * t;
                Vec2 pVel = curDir * 60.0f + Vec2((float)(rand() % 20 - 10), (float)(rand() % 20 - 10));
                ColorRGBA pCol{ color.r, color.g, color.b, 220 };
                ParticleSystem::instance().spawn(pt, pVel, pCol, 0.18f, 2.5f, true);
            }

            if (hitMirror) {
                // Reflect ray
                Vec2 normal = hitMirror->getNormal();
                if (curDir.dot(normal) > 0.0f) {
                    normal = normal * -1.0f;
                }
                curDir = curDir.reflect(normal).normalized();
                float distUsed = std::sqrt(bestDistSq);
                remainingDist -= distUsed;
                curStart = bestHit + curDir * 2.0f; // slight nudge

                // If hitMirror was recently rotated, propagate pulse to the reflected beam
                if (hitMirror->rotateFeedbackTimer > 0.0f) {
                    float mirrorPulse = std::clamp(hitMirror->rotateFeedbackTimer / 0.25f, 0.0f, 1.0f);
                    currentPulse = std::max(currentPulse, mirrorPulse);

                    // Forward sparkle burst along new reflected beam direction on initial rotation
                    if (hitMirror->rotateFeedbackTimer > 0.20f) {
                        for (int s = 0; s < 4; ++s) {
                            float sparkAngle = curDir.angle() + ((float)(rand() % 40 - 20) / 100.0f);
                            float speed = 100.0f + (float)(rand() % 80);
                            Vec2 vel = Vec2::fromAngle(sparkAngle, speed);
                            ColorRGBA sparkCol{ color.r, color.g, color.b, 255 };
                            ParticleSystem::instance().spawn(bestHit, vel, sparkCol, 0.22f, 2.5f, true);
                        }
                    }
                }

                // Spawn reflection sparks
                ParticleSystem::instance().spawnBeamSparks(bestHit, curDir);
                if (bounce == 0) {
                    AudioManager::instance().playSound(SoundID::MirrorReflect, 0.4f);
                }

                if (remainingDist <= 20.0f) break;
            } else {
                break;
            }
        }
    };

    if (m_currentLens == LensType::Focused) {
        ColorRGBA col{ 255, 245, 140, 255 };
        traceRay(origin, m_beamAngle, col, 10.0f);
    } else if (m_currentLens == LensType::WideAmber) {
        ColorRGBA col{ 255, 175, 50, 240 };
        traceRay(origin, m_beamAngle, col, 14.0f);
        traceRay(origin, m_beamAngle - 0.28f, col, 10.0f);
        traceRay(origin, m_beamAngle + 0.28f, col, 10.0f);
    } else if (m_currentLens == LensType::UVPulse) {
        ColorRGBA col{ 120, 230, 255, 255 };
        traceRay(origin, m_beamAngle, col, 12.0f);
    }
}

void Lighthouse::update(float dt, std::vector<Enemy>& enemies, std::vector<PlacedMirror>& mirrors) {
    // If not manned, sweep slowly back and forth automatically
    if (!m_manned) {
        m_autoSweepTimer += dt * 0.4f;
        m_beamAngle = std::sin(m_autoSweepTimer) * 1.8f + (PI * 0.5f);
    }

    // Heat & Fuel management
    if (m_fuel > 0.0f) {
        float fuelDrainRate = m_manned ? 1.5f : 0.8f;
        m_fuel = std::max(0.0f, m_fuel - fuelDrainRate * dt);

        if (m_manned) {
            m_heat = std::min(m_maxHeat, m_heat + 18.0f * dt);
            if (m_heat >= m_maxHeat) {
                m_isOverheated = true;
                AudioManager::instance().playSound(SoundID::EnemyHurt, 0.7f);
            }
        } else {
            m_heat = std::max(0.0f, m_heat - 25.0f * dt);
            if (m_heat <= 10.0f) m_isOverheated = false;
        }
    }

    if (m_isOverheated) {
        m_heat = std::max(0.0f, m_heat - 20.0f * dt);
        if (m_heat <= 5.0f) {
            m_isOverheated = false;
        }
    }

    // Calculate light raycast and reflections
    calculateBeams(mirrors);

    float baseDamage = (m_currentLens == LensType::Focused ? 85.0f : 45.0f) * (1.0f + (beamLevel - 1) * 0.4f);

    // Test beam segments against all enemies
    for (const auto& seg : m_beamSegments) {
        for (auto& e : enemies) {
            if (e.isDead()) continue;
            float dist = distToSegment(e.getPos(), seg.start, seg.end);
            if (dist <= (e.getRadius() + seg.width * 0.7f)) {
                e.takeDamage(baseDamage * dt, true);
                ParticleSystem::instance().spawn(e.getPos(), Vec2((rand() % 40 - 20), (rand() % 40 - 20)),
                                                 seg.color, 0.2f, 3.0f, true);
            }
        }
    }

    AudioManager::instance().setBeamActive(m_beamSegments.size() > 0);
}

void Lighthouse::renderBase(SDL_Renderer* ren, const Vec2& cameraOffset) {
    float sx = m_pos.x - cameraOffset.x;
    float sy = m_pos.y - cameraOffset.y;

    // Stone Cliff Foundation
    SDL_SetRenderDrawColor(ren, 45, 48, 55, 255);
    SDL_Rect foundation{ (int)sx - 45, (int)sy - 15, 90, 30 };
    SDL_RenderFillRect(ren, &foundation);

    // Lighthouse Tower Body (Striped White and Maritime Red)
    // Segment 1 (Base - stone grey)
    SDL_SetRenderDrawColor(ren, 85, 90, 100, 255);
    SDL_Rect b1{ (int)sx - 32, (int)sy - 35, 64, 25 };
    SDL_RenderFillRect(ren, &b1);

    // Segment 2 (Red stripe)
    SDL_SetRenderDrawColor(ren, 195, 45, 45, 255);
    SDL_Rect b2{ (int)sx - 28, (int)sy - 55, 56, 22 };
    SDL_RenderFillRect(ren, &b2);

    // Segment 3 (White stripe)
    SDL_SetRenderDrawColor(ren, 235, 235, 240, 255);
    SDL_Rect b3{ (int)sx - 24, (int)sy - 75, 48, 22 };
    SDL_RenderFillRect(ren, &b3);

    // Segment 4 (Red stripe top)
    SDL_SetRenderDrawColor(ren, 195, 45, 45, 255);
    SDL_Rect b4{ (int)sx - 20, (int)sy - 90, 40, 18 };
    SDL_RenderFillRect(ren, &b4);

    // Lantern Gallery Railing & Balcony
    SDL_SetRenderDrawColor(ren, 35, 38, 45, 255);
    SDL_Rect balcony{ (int)sx - 26, (int)sy - 96, 52, 7 };
    SDL_RenderFillRect(ren, &balcony);

    // Glass Lantern Room
    if (m_fuel > 0.0f && !m_isOverheated) {
        SDL_SetRenderDrawColor(ren, 255, 240, 120, 255);
    } else {
        SDL_SetRenderDrawColor(ren, 70, 75, 80, 255);
    }
    SDL_Rect glass{ (int)sx - 16, (int)sy - 114, 32, 20 };
    SDL_RenderFillRect(ren, &glass);

    // Dome Roof
    SDL_SetRenderDrawColor(ren, 160, 40, 35, 255);
    SDL_Rect dome{ (int)sx - 18, (int)sy - 124, 36, 12 };
    SDL_RenderFillRect(ren, &dome);

    // Spire
    SDL_SetRenderDrawColor(ren, 220, 210, 120, 255);
    SDL_Rect spire{ (int)sx - 2, (int)sy - 134, 4, 12 };
    SDL_RenderFillRect(ren, &spire);

    // Health / Fuel status bar above tower
    float hpRatio = m_health / m_maxHealth;
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 220);
    SDL_Rect hbBg{ (int)sx - 40, (int)sy - 146, 80, 6 };
    SDL_RenderFillRect(ren, &hbBg);

    SDL_SetRenderDrawColor(ren, 50, 205, 50, 255);
    SDL_Rect hbFg{ (int)sx - 40, (int)sy - 146, static_cast<int>(80 * hpRatio), 6 };
    SDL_RenderFillRect(ren, &hbFg);

    // Fuel bar
    float fuelRatio = m_fuel / m_maxFuel;
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 220);
    SDL_Rect fbBg{ (int)sx - 40, (int)sy - 139, 80, 4 };
    SDL_RenderFillRect(ren, &fbBg);

    SDL_SetRenderDrawColor(ren, 240, 175, 40, 255);
    SDL_Rect fbFg{ (int)sx - 40, (int)sy - 139, static_cast<int>(80 * fuelRatio), 4 };
    SDL_RenderFillRect(ren, &fbFg);
}

void Lighthouse::renderBeams(SDL_Renderer* ren, const Vec2& cameraOffset) {
    if (m_beamSegments.empty()) return;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);

    for (const auto& seg : m_beamSegments) {
        Vec2 p1 = seg.start - cameraOffset;
        Vec2 p2 = seg.end - cameraOffset;

        // 1. Soft wide outer glow quad
        SDL_Color glowCol = { seg.color.r, seg.color.g, seg.color.b, 90 };
        drawThickBeam(ren, p1, p2, seg.width * 2.2f, glowCol);

        // 2. Solid colored beam quad
        SDL_Color beamCol = { seg.color.r, seg.color.g, seg.color.b, 220 };
        drawThickBeam(ren, p1, p2, seg.width, beamCol);

        // 3. Crisp white laser core quad
        SDL_Color coreCol = { 255, 255, 255, 255 };
        drawThickBeam(ren, p1, p2, std::max(2.5f, seg.width * 0.32f), coreCol);

        // 4. Directional pulse feedback when reflected beam changes direction
        if (seg.pulse > 0.0f) {
            // Expanded soft outer glow pulse
            uint8_t pulseGlowA = static_cast<uint8_t>(110.0f * seg.pulse);
            SDL_Color pulseGlowCol = { seg.color.r, seg.color.g, seg.color.b, pulseGlowA };
            drawThickBeam(ren, p1, p2, seg.width * (2.2f + 1.8f * seg.pulse), pulseGlowCol);

            // Bright white energy flash core
            uint8_t pulseCoreA = static_cast<uint8_t>(200.0f * seg.pulse);
            SDL_Color pulseCoreCol = { 255, 255, 255, pulseCoreA };
            drawThickBeam(ren, p1, p2, seg.width * (0.5f + 0.5f * seg.pulse), pulseCoreCol);
        }
    }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
}
