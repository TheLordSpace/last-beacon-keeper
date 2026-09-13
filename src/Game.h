#pragma once

#include "Common.h"
#include "Map.h"
#include "Entities.h"
#include "Lighthouse.h"
#include "Localization.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

enum class FontSize {
    Small,
    Medium,
    Large
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

    // Fonts: English
    TTF_Font* m_fontEnSmall = nullptr;
    TTF_Font* m_fontEnMedium = nullptr;
    TTF_Font* m_fontEnLarge = nullptr;

    // Fonts: Arabic
    TTF_Font* m_fontArSmall = nullptr;
    TTF_Font* m_fontArMedium = nullptr;
    TTF_Font* m_fontArLarge = nullptr;

    bool m_running = true;
    GameState m_state = GameState::Playing;

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
    std::string m_statusMessage = "";
    float m_statusMessageTimer = 0.0f;

    void toggleFullscreen();

    // Methods
    void processEvents();
    void update(float dt);
    void render();

    void updateDayNight(float dt);
    void spawnNightEnemies(float dt);

    void renderHUD();
    void renderLightingPass();
    void renderMirrors();
    void renderWorkshop();
    void renderJournal();
    void renderSettings();
    void renderPaused();
    void renderGameOver();
    void renderVictory();

    void drawText(const std::string& text, int x, int y, SDL_Color color, FontSize size, bool alignRight = false);
    void drawCircleLight(SDL_Renderer* ren, int cx, int cy, int radius, uint8_t alpha);
    void setStatus(const std::string& msg, float time = 3.0f);

    void placeMirror();
    void rotateNearbyMirror();
    void interactNearby();
    void buyWorkshopItem(int index);
};
