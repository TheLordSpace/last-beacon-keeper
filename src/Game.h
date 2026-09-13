#pragma once

#include "Common.h"
#include "Map.h"
#include "Entities.h"
#include "Lighthouse.h"
#include "Localization.h"
#include "UIManager.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>

enum class GameState {
    Playing,
    Workshop,
    Journal,
    Settings,
    Paused,
    GameOver,
    Victory
};

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();
    void cleanup();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_lightTexture = nullptr;
    SDL_Texture* m_radialLightTexture = nullptr;

    bool m_running = true;
    GameState m_state = GameState::Playing;

    // UI and HUD Subsystem
    UIManager m_ui;

    // Game Objects
    IslandMap m_map;
    Player m_player;
    Lighthouse m_lighthouse;
    std::vector<Enemy> m_enemies;
    std::vector<PlacedMirror> m_mirrors;
    std::vector<ItemDrop> m_drops;

    // Day/Night Cycle
    int m_dayNumber = 1;
    DayPhase m_phase = DayPhase::Day;
    float m_phaseTimer = 0.0f;
    float m_dayDuration = 60.0f;
    float m_duskDuration = 12.0f;
    float m_nightDuration = 55.0f;
    float m_dawnDuration = 10.0f;
    float m_spawnTimer = 0.0f;

    // Camera
    Vec2 m_cameraPos{ 1300.0f - WINDOW_WIDTH * 0.5f, 1000.0f - WINDOW_HEIGHT * 0.5f };

    // UI state
    int m_workshopSelected = 0;
    int m_settingsSelected = 0;
    int m_soundVolumePercent = 70;
    bool m_fullscreen = false;

    void toggleFullscreen();

    // Methods
    void processEvents();
    void update(float dt);
    void render();

    void updateDayNight(float dt);
    void spawnNightEnemies(float dt);

    void renderLightingPass();
    void renderMirrors();

    void drawCircleLight(SDL_Renderer* ren, int cx, int cy, int radius, uint8_t alpha);
    void setStatus(const std::string& msg, float time = 3.0f) { m_ui.setStatus(msg, time); }

    void placeMirror();
    void rotateNearbyMirror();
    void interactNearby();
    void buyWorkshopItem(int index);
};
