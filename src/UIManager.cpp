#include "UIManager.h"
#include <iostream>
#include <algorithm>

UIManager::~UIManager() {
    cleanup();
}

bool UIManager::init(SDL_Renderer* renderer) {
    m_renderer = renderer;

    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return false;
    }

    // Load English Fonts
    m_fontEnSmall = TTF_OpenFont("assets/font.ttf", 15);
    m_fontEnMedium = TTF_OpenFont("assets/font.ttf", 20);
    m_fontEnLarge = TTF_OpenFont("assets/font.ttf", 32);

    // Load Arabic Fonts (Amiri Naskh for authentic calligraphy and proper shaping)
    m_fontArSmall = TTF_OpenFont("assets/font_amiri.ttf", 18);
    m_fontArMedium = TTF_OpenFont("assets/font_amiri.ttf", 23);
    m_fontArLarge = TTF_OpenFont("assets/font_amiri_bold.ttf", 32);

    // Ensure fallback if one font fails
    if (!m_fontArSmall) m_fontArSmall = m_fontEnSmall;
    if (!m_fontArMedium) m_fontArMedium = m_fontEnMedium;
    if (!m_fontArLarge) m_fontArLarge = m_fontEnLarge;

    return true;
}

void UIManager::cleanup() {
    if (m_fontEnSmall) {
        TTF_CloseFont(m_fontEnSmall);
        m_fontEnSmall = nullptr;
    }
    if (m_fontEnMedium) {
        TTF_CloseFont(m_fontEnMedium);
        m_fontEnMedium = nullptr;
    }
    if (m_fontEnLarge) {
        TTF_CloseFont(m_fontEnLarge);
        m_fontEnLarge = nullptr;
    }

    if (m_fontArSmall && m_fontArSmall != m_fontEnSmall) {
        TTF_CloseFont(m_fontArSmall);
        m_fontArSmall = nullptr;
    }
    if (m_fontArMedium && m_fontArMedium != m_fontEnMedium) {
        TTF_CloseFont(m_fontArMedium);
        m_fontArMedium = nullptr;
    }
    if (m_fontArLarge && m_fontArLarge != m_fontEnLarge) {
        TTF_CloseFont(m_fontArLarge);
        m_fontArLarge = nullptr;
    }

    TTF_Quit();
}

void UIManager::update(float dt) {
    if (m_statusMessageTimer > 0.0f) {
        m_statusMessageTimer -= dt;
    }
}

void UIManager::setStatus(const std::string& msg, float time) {
    m_statusMessage = msg;
    m_statusMessageTimer = time;
}

void UIManager::drawText(const std::string& text, int x, int y, SDL_Color color, FontSize size, bool alignRight) {
    if (text.empty() || !m_renderer) return;

    bool isAr = Localization::instance().isArabic();
    std::string shaped = Localization::instance().shapeText(text);

    TTF_Font* font = nullptr;
    if (isAr) {
        if (size == FontSize::Small) font = m_fontArSmall;
        else if (size == FontSize::Medium) font = m_fontArMedium;
        else font = m_fontArLarge;
    } else {
        if (size == FontSize::Small) font = m_fontEnSmall;
        else if (size == FontSize::Medium) font = m_fontEnMedium;
        else font = m_fontEnLarge;
    }

    if (!font) return;

    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, shaped.c_str(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surf);
    if (tex) {
        int renderX = alignRight ? (x - surf->w) : x;
        SDL_Rect dst{ renderX, y, surf->w, surf->h };
        SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void UIManager::renderHUD(
    const Player& player,
    const Lighthouse& lighthouse,
    const IslandMap& map,
    int dayNumber,
    DayPhase phase,
    float phaseTimer,
    float dayDuration,
    float duskDuration,
    float nightDuration,
    float dawnDuration
) {
    if (!m_renderer) return;

    // 1. Top Status Bar Container
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 15, 20, 30, 225);
    SDL_Rect topBar{ 15, 12, WINDOW_WIDTH - 30, 68 };
    SDL_RenderFillRect(m_renderer, &topBar);
    SDL_SetRenderDrawColor(m_renderer, 70, 85, 110, 255);
    SDL_RenderDrawRect(m_renderer, &topBar);

    // Player HP Bar
    float phpRatio = player.getHealth() / player.getMaxHealth();
    std::string keeperLabel = Localization::instance().get("KEEPER_HP");
    drawText(keeperLabel, 25, 20, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect phpBg{ 125, 24, 110, 14 };
    SDL_RenderFillRect(m_renderer, &phpBg);
    SDL_SetRenderDrawColor(m_renderer, 220, 60, 60, 255);
    SDL_Rect phpFg{ 125, 24, static_cast<int>(110 * phpRatio), 14 };
    SDL_RenderFillRect(m_renderer, &phpFg);

    // Lighthouse HP Bar
    float lhpRatio = lighthouse.getHealth() / lighthouse.getMaxHealth();
    std::string beaconLabel = Localization::instance().get("BEACON_HP");
    drawText(beaconLabel, 250, 20, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect lhpBg{ 350, 24, 110, 14 };
    SDL_RenderFillRect(m_renderer, &lhpBg);
    SDL_SetRenderDrawColor(m_renderer, 60, 190, 80, 255);
    SDL_Rect lhpFg{ 350, 24, static_cast<int>(110 * lhpRatio), 14 };
    SDL_RenderFillRect(m_renderer, &lhpFg);

    // Fuel Bar
    float fuelRatio = lighthouse.getFuel() / lighthouse.getMaxFuel();
    std::string fuelLabel = Localization::instance().get("FUEL");
    drawText(fuelLabel, 475, 20, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect fuelBg{ 530, 24, 85, 14 };
    SDL_RenderFillRect(m_renderer, &fuelBg);
    SDL_SetRenderDrawColor(m_renderer, 240, 180, 40, 255);
    SDL_Rect fuelFg{ 530, 24, static_cast<int>(85 * fuelRatio), 14 };
    SDL_RenderFillRect(m_renderer, &fuelFg);

    // Day & Phase Clock
    float pDuration = dayDuration;
    if (phase == DayPhase::Dusk) pDuration = duskDuration;
    else if (phase == DayPhase::Night) pDuration = nightDuration;
    else if (phase == DayPhase::Dawn) pDuration = dawnDuration;

    int timeLeft = std::max(0, static_cast<int>(pDuration - phaseTimer));
    std::string clockStr = Localization::instance().getClockText(dayNumber, phase, timeLeft);
    SDL_Color phaseCol{ 255, 220, 90, 255 };
    if (phase == DayPhase::Dusk) phaseCol = { 255, 130, 50, 255 };
    else if (phase == DayPhase::Night) phaseCol = { 210, 90, 255, 255 };
    else if (phase == DayPhase::Dawn) phaseCol = { 100, 225, 255, 255 };

    drawText(clockStr, 630, 18, phaseCol, FontSize::Medium);

    // Altars Count
    int altarsLit = map.getIgnitedAltarsCount();
    std::string altarStr = Localization::instance().get("ALTARS") + " " + std::to_string(altarsLit) + "/3";
    drawText(altarStr, 1120, 20, { 240, 200, 80, 255 }, FontSize::Small);

    // Inventory Line (bottom row of top bar)
    std::string invStr = Localization::instance().getInventoryText(
        player.wood, player.crystals, player.oil,
        player.mirrorsInBag, player.relicCores, player.salves
    );
    drawText(invStr, 25, 48, { 185, 205, 225, 255 }, FontSize::Small);

    // 2. Controls Footer
    SDL_SetRenderDrawColor(m_renderer, 15, 20, 30, 215);
    SDL_Rect botBar{ 15, WINDOW_HEIGHT - 44, WINDOW_WIDTH - 30, 36 };
    SDL_RenderFillRect(m_renderer, &botBar);

    std::string controls = Localization::instance().getControlsText();
    drawText(controls, 25, WINDOW_HEIGHT - 38, { 205, 215, 230, 255 }, FontSize::Small);

    // 3. Status Notification Banner
    if (m_statusMessageTimer > 0.0f) {
        SDL_SetRenderDrawColor(m_renderer, 10, 15, 25, 235);
        SDL_Rect notif{ WINDOW_WIDTH / 2 - 340, 86, 680, 42 };
        SDL_RenderFillRect(m_renderer, &notif);
        SDL_SetRenderDrawColor(m_renderer, 240, 190, 60, 255);
        SDL_RenderDrawRect(m_renderer, &notif);

        // Center text in notification banner
        int tx = WINDOW_WIDTH / 2 - 310;
        drawText(m_statusMessage, tx, 94, { 255, 240, 160, 255 }, FontSize::Small);
    }
}

void UIManager::renderWorkshop(int selectedIndex) {
    if (!m_renderer) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 10, 15, 25, 240);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    SDL_SetRenderDrawColor(m_renderer, 30, 40, 55, 255);
    SDL_Rect panel{ WINDOW_WIDTH / 2 - 370, 65, 740, 590 };
    SDL_RenderFillRect(m_renderer, &panel);
    SDL_SetRenderDrawColor(m_renderer, 220, 180, 80, 255);
    SDL_RenderDrawRect(m_renderer, &panel);

    std::string title = Localization::instance().getWorkshopTitle();
    std::string sub = Localization::instance().getWorkshopSubtitle();
    drawText(title, WINDOW_WIDTH / 2 - 220, 85, { 255, 220, 90, 255 }, FontSize::Large);
    drawText(sub, WINDOW_WIDTH / 2 - 240, 135, { 180, 195, 210, 255 }, FontSize::Small);

    std::vector<std::string> items = Localization::instance().getWorkshopItems();

    for (size_t i = 0; i < items.size(); ++i) {
        int y = 185 + static_cast<int>(i) * 56;
        bool isSel = (selectedIndex == static_cast<int>(i));

        if (isSel) {
            SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
            SDL_Rect itemBg{ WINDOW_WIDTH / 2 - 340, y - 5, 680, 46 };
            SDL_RenderFillRect(m_renderer, &itemBg);
            SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
            SDL_RenderDrawRect(m_renderer, &itemBg);
        }

        SDL_Color c = isSel ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 215, 255 };
        drawText(items[i], WINDOW_WIDTH / 2 - 320, y + 6, c, FontSize::Small);
    }
}

void UIManager::renderJournal() {
    if (!m_renderer) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 8, 12, 20, 245);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    SDL_SetRenderDrawColor(m_renderer, 24, 30, 42, 255);
    SDL_Rect book{ WINDOW_WIDTH / 2 - 420, 45, 840, 630 };
    SDL_RenderFillRect(m_renderer, &book);
    SDL_SetRenderDrawColor(m_renderer, 180, 150, 80, 255);
    SDL_RenderDrawRect(m_renderer, &book);

    bool isAr = Localization::instance().isArabic();
    std::string title = isAr ? "مذكرات الحارس وسجل المهام" : "THE KEEPER'S LOGS & EXPEDITION MAP";
    drawText(title, WINDOW_WIDTH / 2 - 240, 65, { 240, 210, 90, 255 }, FontSize::Large);

    std::vector<std::string> lines = Localization::instance().getJournalLines();
    int y = 130;
    for (const auto& line : lines) {
        drawText(line, WINDOW_WIDTH / 2 - 380, y, { 215, 225, 235, 255 }, FontSize::Small);
        y += 32;
    }
}

void UIManager::renderSettings(int selectedIndex, bool fullscreen, int soundVolumePercent) {
    if (!m_renderer) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 8, 12, 20, 245);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    SDL_SetRenderDrawColor(m_renderer, 28, 36, 50, 255);
    SDL_Rect box{ WINDOW_WIDTH / 2 - 310, WINDOW_HEIGHT / 2 - 230, 620, 460 };
    SDL_RenderFillRect(m_renderer, &box);
    SDL_SetRenderDrawColor(m_renderer, 220, 180, 80, 255);
    SDL_RenderDrawRect(m_renderer, &box);

    std::string title = Localization::instance().get("SETTINGS_TITLE");
    drawText(title, WINDOW_WIDTH / 2 - 190, WINDOW_HEIGHT / 2 - 195, { 255, 220, 90, 255 }, FontSize::Large);

    bool isAr = Localization::instance().isArabic();

    // Option 0: Language
    int y0 = WINDOW_HEIGHT / 2 - 120;
    bool sel0 = (selectedIndex == 0);
    if (sel0) {
        SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
        SDL_Rect r{ WINDOW_WIDTH / 2 - 270, y0 - 6, 540, 42 };
        SDL_RenderFillRect(m_renderer, &r);
        SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
        SDL_RenderDrawRect(m_renderer, &r);
    }
    std::string langLabel = Localization::instance().get("SETTINGS_LANGUAGE") + " " + (isAr ? "< العربية (Arabic) >" : "< English >");
    drawText(langLabel, WINDOW_WIDTH / 2 - 240, y0 + 3, sel0 ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 220, 255 }, FontSize::Medium);

    // Option 1: Fullscreen Mode
    int y1 = WINDOW_HEIGHT / 2 - 60;
    bool sel1 = (selectedIndex == 1);
    if (sel1) {
        SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
        SDL_Rect r{ WINDOW_WIDTH / 2 - 270, y1 - 6, 540, 42 };
        SDL_RenderFillRect(m_renderer, &r);
        SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
        SDL_RenderDrawRect(m_renderer, &r);
    }
    std::string fsState = fullscreen ? Localization::instance().get("SETTINGS_FS_ON") : Localization::instance().get("SETTINGS_FS_OFF");
    std::string fsLabel = Localization::instance().get("SETTINGS_FULLSCREEN") + " < " + fsState + " > [F11]";
    drawText(fsLabel, WINDOW_WIDTH / 2 - 240, y1 + 3, sel1 ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 220, 255 }, FontSize::Medium);

    // Option 2: Volume
    int y2 = WINDOW_HEIGHT / 2;
    bool sel2 = (selectedIndex == 2);
    if (sel2) {
        SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
        SDL_Rect r{ WINDOW_WIDTH / 2 - 270, y2 - 6, 540, 42 };
        SDL_RenderFillRect(m_renderer, &r);
        SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
        SDL_RenderDrawRect(m_renderer, &r);
    }
    std::string volLabel = Localization::instance().get("SETTINGS_VOLUME") + " < " + std::to_string(soundVolumePercent) + "% >";
    drawText(volLabel, WINDOW_WIDTH / 2 - 240, y2 + 3, sel2 ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 220, 255 }, FontSize::Medium);

    // Option 3: Back
    int y3 = WINDOW_HEIGHT / 2 + 65;
    bool sel3 = (selectedIndex == 3);
    if (sel3) {
        SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
        SDL_Rect r{ WINDOW_WIDTH / 2 - 270, y3 - 6, 540, 42 };
        SDL_RenderFillRect(m_renderer, &r);
        SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
        SDL_RenderDrawRect(m_renderer, &r);
    }
    std::string backLabel = Localization::instance().get("SETTINGS_BACK");
    drawText(backLabel, WINDOW_WIDTH / 2 - 100, y3 + 3, sel3 ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 220, 255 }, FontSize::Medium);

    std::string tip = isAr ? "استخدم [W/S] للتنقل، [A/D أو Enter] للتبديل، [ESC] للعودة" : "Use [W/S] to navigate, [A/D or Enter] to toggle, [ESC] to return";
    drawText(tip, WINDOW_WIDTH / 2 - 230, WINDOW_HEIGHT / 2 + 155, { 170, 180, 195, 255 }, FontSize::Small);
}

void UIManager::renderPaused() {
    if (!m_renderer) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 10, 15, 25, 230);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    std::string title = Localization::instance().get("PAUSE_TITLE");
    std::string resume = Localization::instance().get("PAUSE_RESUME");
    drawText(title, WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 60, { 255, 220, 90, 255 }, FontSize::Large);
    drawText(resume, WINDOW_WIDTH / 2 - 220, WINDOW_HEIGHT / 2 + 20, { 210, 220, 235, 255 }, FontSize::Medium);
}

void UIManager::renderGameOver() {
    if (!m_renderer) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 20, 5, 5, 240);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    std::string title = Localization::instance().get("GAMEOVER_TITLE");
    std::string sub = Localization::instance().get("GAMEOVER_SUB");
    std::string restart = Localization::instance().get("GAMEOVER_RESTART");

    drawText(title, WINDOW_WIDTH / 2 - 290, WINDOW_HEIGHT / 2 - 90, { 240, 50, 50, 255 }, FontSize::Large);
    drawText(sub, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2 - 20, { 220, 180, 180, 255 }, FontSize::Medium);
    drawText(restart, WINDOW_WIDTH / 2 - 170, WINDOW_HEIGHT / 2 + 50, { 255, 220, 90, 255 }, FontSize::Small);
}

void UIManager::renderVictory() {
    if (!m_renderer) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 15, 30, 50, 240);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    std::string title = Localization::instance().get("VICTORY_TITLE");
    std::string sub1 = Localization::instance().get("VICTORY_SUB1");
    std::string sub2 = Localization::instance().get("VICTORY_SUB2");
    std::string restart = Localization::instance().get("VICTORY_RESTART");

    drawText(title, WINDOW_WIDTH / 2 - 240, WINDOW_HEIGHT / 2 - 100, { 255, 230, 100, 255 }, FontSize::Large);
    drawText(sub1, WINDOW_WIDTH / 2 - 320, WINDOW_HEIGHT / 2 - 30, { 220, 240, 255, 255 }, FontSize::Medium);
    drawText(sub2, WINDOW_WIDTH / 2 - 280, WINDOW_HEIGHT / 2 + 15, { 180, 220, 240, 255 }, FontSize::Small);
    drawText(restart, WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 + 80, { 255, 220, 90, 255 }, FontSize::Small);
}
