#pragma once

#include "Common.h"
#include <SDL2/SDL.h>
#include <vector>

struct ResourceNode {
    int id = 0;
    NodeType type = NodeType::Tree;
    Vec2 pos;
    float health = 100.0f;
    float maxHealth = 100.0f;
    bool active = true;
    float respawnTimer = 0.0f;
    float radius = 22.0f;
};

struct AncientAltar {
    int id = 0;
    Vec2 pos;
    std::string name;
    bool ignited = false;
    float glowPhase = 0.0f;
    ColorRGBA color;
};

class IslandMap {
public:
    IslandMap();

    void init();
    void update(float dt);
    void renderTerrain(SDL_Renderer* ren, const Vec2& cameraOffset);
    void renderNodes(SDL_Renderer* ren, const Vec2& cameraOffset);
    void renderAltars(SDL_Renderer* ren, const Vec2& cameraOffset);

    bool isWalkable(const Vec2& worldPos) const;
    bool isWater(const Vec2& worldPos) const;

    ResourceNode* getNearbyNode(const Vec2& pos, float maxDist);
    AncientAltar* getNearbyAltar(const Vec2& pos, float maxDist);

    const std::vector<AncientAltar>& getAltars() const { return m_altars; }
    std::vector<AncientAltar>& getAltars() { return m_altars; }
    const std::vector<ResourceNode>& getNodes() const { return m_nodes; }
    std::vector<ResourceNode>& getNodes() { return m_nodes; }

    int getIgnitedAltarsCount() const;

private:
    float m_waterTimer = 0.0f;
    std::vector<ResourceNode> m_nodes;
    std::vector<AncientAltar> m_altars;

    // Helper to check if point is on land
    bool pointInIslands(const Vec2& p) const;
};
