#pragma once

#include "Common.h"
#include "Entities.h"
#include "Lighthouse.h"
#include "Map.h"
#include "Localization.h"
#include "DayNightManager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>

enum class FontSize {
    Small,
    Medium,
    Large
};

class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    bool init(SDL_Renderer* renderer);
    void cleanup();

    void update(float dt);
    void setStatus(const std::string& msg, float time = 3.0f);

    void drawText(const std::string& text, int x, int y, SDL_Color color, FontSize size, bool alignRight = false);

    void renderHUD(
        const Player& player,
        const Lighthouse& lighthouse,
        const IslandMap& map,
        const DayNightManager& dayNight
    );

    void renderWorkshop(int selectedIndex);
    void renderJournal();
    void renderSettings(int selectedIndex, bool fullscreen, int soundVolumePercent);
    void renderPaused();
    void renderGameOver();
    void renderVictory();

private:
    SDL_Renderer* m_renderer = nullptr;

    // Fonts: English
    TTF_Font* m_fontEnSmall = nullptr;
    TTF_Font* m_fontEnMedium = nullptr;
    TTF_Font* m_fontEnLarge = nullptr;

    // Fonts: Arabic
    TTF_Font* m_fontArSmall = nullptr;
    TTF_Font* m_fontArMedium = nullptr;
    TTF_Font* m_fontArLarge = nullptr;

    std::string m_statusMessage = "";
    float m_statusMessageTimer = 0.0f;
};
