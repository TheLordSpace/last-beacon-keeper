#include "Map.h"
#include "Particles.h"
#include "Audio.h"
#include <cmath>
#include <cstdlib>

IslandMap::IslandMap() {
    init();
}

void IslandMap::init() {
    m_nodes.clear();
    m_altars.clear();

    // 1. Setup 3 Ancient Altars
    // Altar 1: Sun Shrine on West Island
    AncientAltar a1;
    a1.id = 1;
    a1.pos = Vec2(450.0f, 950.0f);
    a1.name = "Altar of the Solar Ray";
    a1.ignited = false;
    a1.color = ColorRGBA{ 255, 200, 50, 255 };
    m_altars.push_back(a1);

    // Altar 2: Crystal Spires on North Island
    AncientAltar a2;
    a2.id = 2;
    a2.pos = Vec2(1300.0f, 350.0f);
    a2.name = "Altar of Crystal Resonance";
    a2.ignited = false;
    a2.color = ColorRGBA{ 80, 220, 255, 255 };
    m_altars.push_back(a2);

    // Altar 3: Shipwreck Atoll on East Island
    AncientAltar a3;
    a3.id = 3;
    a3.pos = Vec2(2150.0f, 1050.0f);
    a3.name = "Altar of the Sea Beacon";
    a3.ignited = false;
    a3.color = ColorRGBA{ 255, 120, 80, 255 };
    m_altars.push_back(a3);

    // 2. Populate Resource Nodes
    int nextId = 1;

    // Central Island Trees & Rocks (center around 1300, 1050)
    for (int i = 0; i < 28; ++i) {
        float angle = (float)i * 0.22f;
        float radius = 90.0f + (float)(i * 9 % 240);
        Vec2 p = Vec2(1300.0f, 1050.0f) + Vec2(std::cos(angle) * radius, std::sin(angle) * (radius * 0.8f));
        if (p.dist(Vec2(1300.0f, 1050.0f)) > 80.0f && pointInIslands(p)) {
            ResourceNode node;
            node.id = nextId++;
            node.type = NodeType::Tree;
            node.pos = p;
            node.health = 80.0f;
            node.maxHealth = 80.0f;
            m_nodes.push_back(node);
        }
    }

    // North Island Crystals
    for (int i = 0; i < 18; ++i) {
        float angle = (float)i * 0.35f;
        float radius = 50.0f + (float)(i * 7 % 160);
        Vec2 p = Vec2(1300.0f, 350.0f) + Vec2(std::cos(angle) * radius, std::sin(angle) * radius * 0.7f);
        if (p.dist(Vec2(1300.0f, 350.0f)) > 50.0f && pointInIslands(p)) {
            ResourceNode node;
            node.id = nextId++;
            node.type = NodeType::Crystal;
            node.pos = p;
            node.health = 100.0f;
            node.maxHealth = 100.0f;
            m_nodes.push_back(node);
        }
    }

    // East Island Oil Barrels & Wreckage
    for (int i = 0; i < 14; ++i) {
        float angle = (float)i * 0.45f;
        float radius = 60.0f + (float)(i * 8 % 180);
        Vec2 p = Vec2(2150.0f, 1050.0f) + Vec2(std::cos(angle) * radius, std::sin(angle) * radius * 0.75f);
        if (p.dist(Vec2(2150.0f, 1050.0f)) > 50.0f && pointInIslands(p)) {
            ResourceNode node;
            node.id = nextId++;
            node.type = (i % 2 == 0) ? NodeType::OilBarrel : NodeType::Tree;
            node.pos = p;
            node.health = 60.0f;
            node.maxHealth = 60.0f;
            m_nodes.push_back(node);
        }
    }

    // West Island Ancient Ruins trees and stone crystals
    for (int i = 0; i < 16; ++i) {
        float angle = (float)i * 0.4f;
        float radius = 70.0f + (float)(i * 11 % 170);
        Vec2 p = Vec2(450.0f, 950.0f) + Vec2(std::cos(angle) * radius, std::sin(angle) * radius * 0.8f);
        if (p.dist(Vec2(450.0f, 950.0f)) > 60.0f && pointInIslands(p)) {
            ResourceNode node;
            node.id = nextId++;
            node.type = (i % 3 == 0) ? NodeType::Crystal : NodeType::Tree;
            node.pos = p;
            node.health = 80.0f;
            node.maxHealth = 80.0f;
            m_nodes.push_back(node);
        }
    }
}

bool IslandMap::pointInIslands(const Vec2& p) const {
    // 1. Central Island (ellipse)
    float dxC = (p.x - 1300.0f) / 380.0f;
    float dyC = (p.y - 1050.0f) / 290.0f;
    if (dxC * dxC + dyC * dyC <= 1.0f) return true;

    // 2. West Island
    float dxW = (p.x - 450.0f) / 260.0f;
    float dyW = (p.y - 950.0f) / 220.0f;
    if (dxW * dxW + dyW * dyW <= 1.0f) return true;

    // 3. North Island
    float dxN = (p.x - 1300.0f) / 260.0f;
    float dyN = (p.y - 350.0f) / 200.0f;
    if (dxN * dxN + dyN * dyN <= 1.0f) return true;

    // 4. East Island
    float dxE = (p.x - 2150.0f) / 270.0f;
    float dyE = (p.y - 1050.0f) / 220.0f;
    if (dxE * dxE + dyE * dyE <= 1.0f) return true;

    // Bridges / Sandbars connecting islands:
    // West to Center sandbar
    if (p.x >= 650.0f && p.x <= 1000.0f && std::abs(p.y - 1020.0f) <= 45.0f) return true;

    // North to Center sandbar
    if (p.y >= 500.0f && p.y <= 800.0f && std::abs(p.x - 1300.0f) <= 45.0f) return true;

    // East to Center sandbar
    if (p.x >= 1600.0f && p.x <= 1950.0f && std::abs(p.y - 1050.0f) <= 45.0f) return true;

    return false;
}

bool IslandMap::isWalkable(const Vec2& worldPos) const {
    if (worldPos.x < 50.0f || worldPos.x > WORLD_WIDTH - 50.0f ||
        worldPos.y < 50.0f || worldPos.y > WORLD_HEIGHT - 50.0f) {
        return false;
    }
    return pointInIslands(worldPos);
}

bool IslandMap::isWater(const Vec2& worldPos) const {
    return !isWalkable(worldPos);
}

ResourceNode* IslandMap::getNearbyNode(const Vec2& pos, float maxDist) {
    ResourceNode* best = nullptr;
    float bestDistSq = maxDist * maxDist;
    for (auto& node : m_nodes) {
        if (!node.active) continue;
        float dsq = (node.pos - pos).lengthSq();
        if (dsq < bestDistSq) {
            bestDistSq = dsq;
            best = &node;
        }
    }
    return best;
}

AncientAltar* IslandMap::getNearbyAltar(const Vec2& pos, float maxDist) {
    AncientAltar* best = nullptr;
    float bestDistSq = maxDist * maxDist;
    for (auto& altar : m_altars) {
        float dsq = (altar.pos - pos).lengthSq();
        if (dsq < bestDistSq) {
            bestDistSq = dsq;
            best = &altar;
        }
    }
    return best;
}

int IslandMap::getIgnitedAltarsCount() const {
    int count = 0;
    for (const auto& a : m_altars) {
        if (a.ignited) ++count;
    }
    return count;
}

void IslandMap::update(float dt) {
    m_waterTimer += dt;
    for (auto& a : m_altars) {
        if (a.ignited) {
            a.glowPhase += dt * 3.0f;
            if (a.glowPhase > 6.28318f) a.glowPhase -= 6.28318f;

            // Spawn celestial light particles around altar
            if ((rand() % 10) < 3) {
                float angle = (float)(rand() % 360) * (PI / 180.0f);
                float dist = (float)(rand() % 35);
                Vec2 spawnPos = a.pos + Vec2::fromAngle(angle, dist);
                Vec2 vel(0.0f, -40.0f - (rand() % 40));
                ParticleSystem::instance().spawn(spawnPos, vel, a.color, 1.2f, 3.5f, true);
            }
        }
    }

    // Respawn nodes slowly over time
    for (auto& node : m_nodes) {
        if (!node.active) {
            node.respawnTimer -= dt;
            if (node.respawnTimer <= 0.0f) {
                node.active = true;
                node.health = node.maxHealth;
            }
        }
    }
}

void IslandMap::renderTerrain(SDL_Renderer* ren, const Vec2& cameraOffset) {
    // 1. Ocean Background (deep navy ocean)
    SDL_SetRenderDrawColor(ren, 14, 28, 48, 255);
    SDL_RenderClear(ren);

    // Water wave grid pattern
    SDL_SetRenderDrawColor(ren, 20, 42, 70, 180);
    int tileSize = 64;
    int startX = static_cast<int>(cameraOffset.x) / tileSize;
    int endX = startX + (WINDOW_WIDTH / tileSize) + 2;
    int startY = static_cast<int>(cameraOffset.y) / tileSize;
    int endY = startY + (WINDOW_HEIGHT / tileSize) + 2;

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            float waveOff = std::sin(m_waterTimer * 1.5f + (float)x * 0.5f + (float)y * 0.7f) * 4.0f;
            float px = x * tileSize - cameraOffset.x;
            float py = y * tileSize - cameraOffset.y + waveOff;
            SDL_RenderDrawLine(ren, (int)px + 8, (int)py, (int)px + 24, (int)py);
        }
    }

    // 2. Render Island Sand & Land
    // We render using layered filled circles/rects for islands and sandbars
    auto drawIsland = [&](const Vec2& center, float rx, float ry, ColorRGBA sand, ColorRGBA grass, ColorRGBA inner) {
        // Sand border (slightly larger)
        float srx = rx + 24.0f;
        float sry = ry + 20.0f;
        SDL_SetRenderDrawColor(ren, sand.r, sand.g, sand.b, sand.a);
        for (int dy = -(int)sry; dy <= (int)sry; dy += 4) {
            float normY = (float)dy / sry;
            if (std::abs(normY) <= 1.0f) {
                float halfW = srx * std::sqrt(1.0f - normY * normY);
                int sx = static_cast<int>(center.x - halfW - cameraOffset.x);
                int sy = static_cast<int>(center.y + dy - cameraOffset.y);
                int w = static_cast<int>(halfW * 2.0f);
                SDL_Rect r{ sx, sy, w, 5 };
                SDL_RenderFillRect(ren, &r);
            }
        }

        // Coastal Grass
        SDL_SetRenderDrawColor(ren, grass.r, grass.g, grass.b, grass.a);
        for (int dy = -(int)ry; dy <= (int)ry; dy += 4) {
            float normY = (float)dy / ry;
            if (std::abs(normY) <= 1.0f) {
                float halfW = rx * std::sqrt(1.0f - normY * normY);
                int sx = static_cast<int>(center.x - halfW - cameraOffset.x);
                int sy = static_cast<int>(center.y + dy - cameraOffset.y);
                int w = static_cast<int>(halfW * 2.0f);
                SDL_Rect r{ sx, sy, w, 5 };
                SDL_RenderFillRect(ren, &r);
            }
        }

        // Inner plateau
        float irx = rx * 0.75f;
        float iry = ry * 0.75f;
        SDL_SetRenderDrawColor(ren, inner.r, inner.g, inner.b, inner.a);
        for (int dy = -(int)iry; dy <= (int)iry; dy += 4) {
            float normY = (float)dy / iry;
            if (std::abs(normY) <= 1.0f) {
                float halfW = irx * std::sqrt(1.0f - normY * normY);
                int sx = static_cast<int>(center.x - halfW - cameraOffset.x);
                int sy = static_cast<int>(center.y + dy - cameraOffset.y);
                int w = static_cast<int>(halfW * 2.0f);
                SDL_Rect r{ sx, sy, w, 5 };
                SDL_RenderFillRect(ren, &r);
            }
        }
    };

    // Connecting Sandbars
    auto drawSandbar = [&](float x1, float y1, float x2, float y2, float thickness) {
        SDL_SetRenderDrawColor(ren, 194, 168, 114, 255); // sand
        SDL_Rect bar{
            static_cast<int>(std::min(x1, x2) - cameraOffset.x),
            static_cast<int>(std::min(y1, y2) - thickness * 0.5f - cameraOffset.y),
            static_cast<int>(std::abs(x2 - x1) + thickness),
            static_cast<int>(std::abs(y2 - y1) + thickness)
        };
        SDL_RenderFillRect(ren, &bar);

        // Path stones
        SDL_SetRenderDrawColor(ren, 140, 130, 110, 255);
        SDL_Rect stone{
            bar.x + 8,
            bar.y + static_cast<int>(thickness * 0.25f),
            bar.w - 16,
            static_cast<int>(thickness * 0.5f)
        };
        SDL_RenderFillRect(ren, &stone);
    };

    // Draw Sandbars
    drawSandbar(650.0f, 1020.0f, 1000.0f, 1020.0f, 65.0f);
    drawSandbar(1300.0f, 500.0f, 1300.0f, 800.0f, 65.0f);
    drawSandbar(1600.0f, 1050.0f, 1950.0f, 1050.0f, 65.0f);

    // Central Island
    drawIsland(Vec2(1300.0f, 1050.0f), 380.0f, 290.0f,
               ColorRGBA{ 205, 180, 120, 255 },
               ColorRGBA{ 55, 115, 60, 255 },
               ColorRGBA{ 40, 85, 45, 255 });

    // West Island (Sun Ruins)
    drawIsland(Vec2(450.0f, 950.0f), 260.0f, 220.0f,
               ColorRGBA{ 190, 165, 110, 255 },
               ColorRGBA{ 90, 95, 80, 255 },
               ColorRGBA{ 110, 110, 95, 255 });

    // North Island (Crystal Spires)
    drawIsland(Vec2(1300.0f, 350.0f), 260.0f, 200.0f,
               ColorRGBA{ 180, 170, 150, 255 },
               ColorRGBA{ 60, 80, 120, 255 },
               ColorRGBA{ 45, 55, 95, 255 });

    // East Island (Shipwreck Atoll)
    drawIsland(Vec2(2150.0f, 1050.0f), 270.0f, 220.0f,
               ColorRGBA{ 215, 195, 135, 255 },
               ColorRGBA{ 175, 155, 105, 255 },
               ColorRGBA{ 145, 125, 85, 255 });
}

void IslandMap::renderNodes(SDL_Renderer* ren, const Vec2& cameraOffset) {
    for (const auto& node : m_nodes) {
        if (!node.active) continue;

        float sx = node.pos.x - cameraOffset.x;
        float sy = node.pos.y - cameraOffset.y;

        if (sx < -40 || sx > WINDOW_WIDTH + 40 || sy < -40 || sy > WINDOW_HEIGHT + 40) continue;

        switch (node.type) {
        case NodeType::Tree: {
            // Shadow
            SDL_SetRenderDrawColor(ren, 20, 30, 25, 100);
            SDL_Rect shadow{ (int)sx - 14, (int)sy + 8, 28, 12 };
            SDL_RenderFillRect(ren, &shadow);

            // Trunk
            SDL_SetRenderDrawColor(ren, 105, 65, 35, 255);
            SDL_Rect trunk{ (int)sx - 5, (int)sy - 8, 10, 20 };
            SDL_RenderFillRect(ren, &trunk);

            // Foliage (layers)
            SDL_SetRenderDrawColor(ren, 28, 85, 40, 255);
            SDL_Rect f1{ (int)sx - 20, (int)sy - 28, 40, 22 };
            SDL_RenderFillRect(ren, &f1);

            SDL_SetRenderDrawColor(ren, 40, 115, 55, 255);
            SDL_Rect f2{ (int)sx - 15, (int)sy - 42, 30, 18 };
            SDL_RenderFillRect(ren, &f2);

            SDL_SetRenderDrawColor(ren, 55, 145, 70, 255);
            SDL_Rect f3{ (int)sx - 10, (int)sy - 52, 20, 14 };
            SDL_RenderFillRect(ren, &f3);
            break;
        }

        case NodeType::Crystal: {
            // Glowing Crystal Formation
            SDL_SetRenderDrawColor(ren, 20, 50, 70, 120);
            SDL_Rect shadow{ (int)sx - 12, (int)sy + 6, 24, 8 };
            SDL_RenderFillRect(ren, &shadow);

            // Main crystal spike
            SDL_SetRenderDrawColor(ren, 40, 180, 240, 255);
            SDL_Rect sp1{ (int)sx - 6, (int)sy - 24, 12, 30 };
            SDL_RenderFillRect(ren, &sp1);

            // Side spikes
            SDL_SetRenderDrawColor(ren, 120, 220, 255, 255);
            SDL_Rect sp2{ (int)sx - 14, (int)sy - 15, 8, 20 };
            SDL_RenderFillRect(ren, &sp2);

            SDL_SetRenderDrawColor(ren, 180, 240, 255, 255);
            SDL_Rect sp3{ (int)sx + 6, (int)sy - 18, 9, 23 };
            SDL_RenderFillRect(ren, &sp3);
            break;
        }

        case NodeType::OilBarrel: {
            // Oil Barrel
            SDL_SetRenderDrawColor(ren, 30, 30, 30, 120);
            SDL_Rect shadow{ (int)sx - 10, (int)sy + 6, 20, 8 };
            SDL_RenderFillRect(ren, &shadow);

            SDL_SetRenderDrawColor(ren, 140, 40, 30, 255); // rusty red
            SDL_Rect barrel{ (int)sx - 10, (int)sy - 14, 20, 22 };
            SDL_RenderFillRect(ren, &barrel);

            // Yellow hazardous stripe
            SDL_SetRenderDrawColor(ren, 240, 190, 40, 255);
            SDL_Rect stripe{ (int)sx - 10, (int)sy - 6, 20, 6 };
            SDL_RenderFillRect(ren, &stripe);
            break;
        }

        default:
            break;
        }
    }
}

void IslandMap::renderAltars(SDL_Renderer* ren, const Vec2& cameraOffset) {
    for (const auto& a : m_altars) {
        float sx = a.pos.x - cameraOffset.x;
        float sy = a.pos.y - cameraOffset.y;

        if (sx < -80 || sx > WINDOW_WIDTH + 80 || sy < -80 || sy > WINDOW_HEIGHT + 80) continue;

        // Base Stone Pedestal
        SDL_SetRenderDrawColor(ren, 40, 45, 55, 255);
        SDL_Rect base{ (int)sx - 24, (int)sy - 12, 48, 26 };
        SDL_RenderFillRect(ren, &base);

        SDL_SetRenderDrawColor(ren, 65, 72, 85, 255);
        SDL_Rect tier{ (int)sx - 18, (int)sy - 22, 36, 14 };
        SDL_RenderFillRect(ren, &tier);

        // Altar Pillar & Crystal
        if (a.ignited) {
            // Ignited Altar Core
            float pulse = 0.8f + 0.2f * std::sin(a.glowPhase);
            SDL_SetRenderDrawColor(ren, (uint8_t)(a.color.r * pulse),
                                        (uint8_t)(a.color.g * pulse),
                                        (uint8_t)(a.color.b * pulse), 255);
            SDL_Rect core{ (int)sx - 10, (int)sy - 38, 20, 20 };
            SDL_RenderFillRect(ren, &core);

            // Skyward light beam pillar
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(ren, a.color.r, a.color.g, a.color.b, 160);
            SDL_Rect beamCol{ (int)sx - 8, -500, 16, (int)sy + 500 };
            SDL_RenderFillRect(ren, &beamCol);

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 220);
            SDL_Rect beamCore{ (int)sx - 3, -500, 6, (int)sy + 500 };
            SDL_RenderFillRect(ren, &beamCore);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        } else {
            // Dormant / Unlit Altar
            SDL_SetRenderDrawColor(ren, 30, 32, 40, 255);
            SDL_Rect dormantCore{ (int)sx - 8, (int)sy - 34, 16, 16 };
            SDL_RenderFillRect(ren, &dormantCore);

            // Inactive socket indicator
            SDL_SetRenderDrawColor(ren, 90, 80, 70, 255);
            SDL_RenderDrawRect(ren, &dormantCore);
        }
    }
}
