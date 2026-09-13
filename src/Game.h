#pragma once

#include "Common.h"
#include "Map.h"
#include "Entities.h"
#include "Lighthouse.h"
#include "Localization.h"
#include "UIManager.h"
#include "InputHandler.h"
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

    // State & Lifecycle
    GameState getState() const { return m_state; }
    void setState(GameState state) { m_state = state; }
    void quit() { m_running = false; }

    // State Navigation Actions
    void pauseGame() { m_state = GameState::Paused; }
    void resumeGame() { m_state = GameState::Playing; }
    void openSettings() { m_state = GameState::Settings; }
    void closeSettings() { m_state = GameState::Playing; }
    void openJournal() { m_state = GameState::Journal; }
    void closeJournal() { m_state = GameState::Playing; }
    void closeWorkshop() { m_state = GameState::Playing; }

    // Display & System Actions
    void toggleFullscreen();
    void takeScreenshot();
    void toggleLanguage();

    // Gameplay Actions
    void placeMirror();
    void rotateNearbyMirror();
    void interactNearby();
    void playerDash();
    void useHealingSalve();
    void selectLens(LensType type);
    void toggleMannedLighthouse();
    void playerAttack();

    // Settings Navigation & Input
    void settingsNavigateUp();
    void settingsNavigateDown();
    void settingsAdjustLeft();
    void settingsConfirmOrRight();
    void settingsClick();

    // Workshop Navigation & Input
    void workshopNavigateUp();
    void workshopNavigateDown();
    void workshopConfirm();

    // Game Reset
    void restartGame();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_lightTexture = nullptr;
    SDL_Texture* m_radialLightTexture = nullptr;

    bool m_running = true;
    GameState m_state = GameState::Playing;

    // Subsystems
    UIManager m_ui;
    InputHandler m_inputHandler;

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

    // Methods
    void update(float dt);
    void render();

    void updateDayNight(float dt);
    void spawnNightEnemies(float dt);

    void renderLightingPass();
    void renderMirrors();

    void drawCircleLight(SDL_Renderer* ren, int cx, int cy, int radius, uint8_t alpha);
    void setStatus(const std::string& msg, float time = 3.0f) { m_ui.setStatus(msg, time); }

    void buyWorkshopItem(int index);
};
