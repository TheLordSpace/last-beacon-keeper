#include "UIManager.h"
#include "Common.h"
#include "Audio.h"
#include <iostream>
#include <sstream>
#include <algorithm>

UIManager::~UIManager() {
    cleanup();
}

bool UIManager::init(SDL_Renderer* renderer) {
    m_renderer = renderer;

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    // Load English Fonts
    m_fontEnSmall = TTF_OpenFont("assets/font.ttf", 15);
    m_fontEnMedium = TTF_OpenFont("assets/font.ttf", 20);
    m_fontEnLarge = TTF_OpenFont("assets/font.ttf", 30);

    // Load Arabic Fonts
    m_fontArSmall = TTF_OpenFont("assets/font_amiri.ttf", 18);
    m_fontArMedium = TTF_OpenFont("assets/font_amiri.ttf", 23);
    m_fontArLarge = TTF_OpenFont("assets/font_amiri_bold.ttf", 32);

    if (!m_fontArSmall) m_fontArSmall = m_fontEnSmall;
    if (!m_fontArMedium) m_fontArMedium = m_fontEnMedium;
    if (!m_fontArLarge) m_fontArLarge = m_fontEnLarge;

    return true;
}

void UIManager::cleanup() {
    if (m_fontArSmall) { TTF_CloseFont(m_fontArSmall); m_fontArSmall = nullptr; }
    if (m_fontArMedium) { TTF_CloseFont(m_fontArMedium); m_fontArMedium = nullptr; }
    if (m_fontArLarge) { TTF_CloseFont(m_fontArLarge); m_fontArLarge = nullptr; }

    if (m_fontEnSmall) { TTF_CloseFont(m_fontEnSmall); m_fontEnSmall = nullptr; }
    if (m_fontEnMedium) { TTF_CloseFont(m_fontEnMedium); m_fontEnMedium = nullptr; }
    if (m_fontEnLarge) { TTF_CloseFont(m_fontEnLarge); m_fontEnLarge = nullptr; }

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
    if (!m_renderer || text.empty()) return;

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
    const DayNightManager& dayNight
) {
    if (!m_renderer) return;

    // 1. Top Status Bar Container
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 15, 20, 30, 225);
    SDL_Rect topBar{ 15, 10, WINDOW_WIDTH - 30, 72 };
    SDL_RenderFillRect(m_renderer, &topBar);
    SDL_SetRenderDrawColor(m_renderer, 70, 85, 110, 255);
    SDL_RenderDrawRect(m_renderer, &topBar);

    // Player HP Bar
    float phpRatio = std::clamp(player.getHealth() / player.getMaxHealth(), 0.0f, 1.0f);
    std::string keeperLabel = Localization::instance().get("KEEPER_HP");
    drawText(keeperLabel, 25, 16, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect phpBg{ 125, 20, 105, 14 };
    SDL_RenderFillRect(m_renderer, &phpBg);
    SDL_SetRenderDrawColor(m_renderer, 220, 60, 60, 255);
    SDL_Rect phpFg{ 125, 20, static_cast<int>(105 * phpRatio), 14 };
    SDL_RenderFillRect(m_renderer, &phpFg);

    // Lighthouse HP Bar
    float lhpRatio = std::clamp(lighthouse.getHealth() / lighthouse.getMaxHealth(), 0.0f, 1.0f);
    std::string beaconLabel = Localization::instance().get("BEACON_HP");
    drawText(beaconLabel, 245, 16, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect lhpBg{ 345, 20, 115, 14 };
    SDL_RenderFillRect(m_renderer, &lhpBg);

    // Dynamic color for lighthouse HP
    if (lhpRatio < 0.25f) {
        SDL_SetRenderDrawColor(m_renderer, 240, 40, 40, 255); // critical red
    } else if (lhpRatio < 0.5f) {
        SDL_SetRenderDrawColor(m_renderer, 240, 160, 40, 255); // warning orange
    } else {
        SDL_SetRenderDrawColor(m_renderer, 60, 195, 80, 255); // healthy green
    }
    SDL_Rect lhpFg{ 345, 20, static_cast<int>(115 * lhpRatio), 14 };
    SDL_RenderFillRect(m_renderer, &lhpFg);

    // Lighthouse Under Attack Alert
    if (dayNight.isLighthouseUnderAttack()) {
        SDL_SetRenderDrawColor(m_renderer, 255, 50, 50, 255);
        SDL_RenderDrawRect(m_renderer, &lhpBg);
        std::string atkLabel = Localization::instance().getLighthouseAttackWarning();
        drawText(atkLabel, 345, 2, { 255, 70, 70, 255 }, FontSize::Small);
    }

    // Fuel Bar
    float fuelRatio = std::clamp(lighthouse.getFuel() / lighthouse.getMaxFuel(), 0.0f, 1.0f);
    std::string fuelLabel = Localization::instance().get("FUEL");
    drawText(fuelLabel, 475, 16, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect fuelBg{ 530, 20, 85, 14 };
    SDL_RenderFillRect(m_renderer, &fuelBg);
    if (fuelRatio < 0.15f) {
        SDL_SetRenderDrawColor(m_renderer, 240, 50, 50, 255);
    } else if (fuelRatio < 0.3f) {
        SDL_SetRenderDrawColor(m_renderer, 240, 140, 30, 255);
    } else {
        SDL_SetRenderDrawColor(m_renderer, 240, 190, 40, 255);
    }
    SDL_Rect fuelFg{ 530, 20, static_cast<int>(85 * fuelRatio), 14 };
    SDL_RenderFillRect(m_renderer, &fuelFg);

    // Day & Phase & Wave Clock
    DayPhase phase = dayNight.getPhase();
    int dayNumber = dayNight.getDayNumber();
    int timeLeft = dayNight.getTimeLeft();
    int curWave = dayNight.getWaveManager().getCurrentWave();
    int totalWaves = dayNight.getWaveManager().getTotalWaves();

    std::string clockStr = Localization::instance().getClockText(dayNumber, phase, timeLeft, curWave, totalWaves);
    SDL_Color phaseCol{ 255, 220, 90, 255 };
    if (phase == DayPhase::Dusk) phaseCol = { 255, 140, 50, 255 };
    else if (phase == DayPhase::Night) phaseCol = { 220, 110, 255, 255 };
    else if (phase == DayPhase::Dawn) phaseCol = { 110, 235, 255, 255 };

    drawText(clockStr, 630, 15, phaseCol, FontSize::Medium);

    // Altars Count
    int altarsLit = map.getIgnitedAltarsCount();
    std::string altarStr = Localization::instance().get("ALTARS") + " " + std::to_string(altarsLit) + "/3";
    drawText(altarStr, 1120, 16, { 240, 200, 80, 255 }, FontSize::Small);

    // Inventory Line (bottom row of top bar)
    std::string invStr = Localization::instance().getInventoryText(
        player.wood, player.crystals, player.oil,
        player.mirrorsInBag, player.relicCores, player.salves
    );
    drawText(invStr, 25, 48, { 185, 205, 225, 255 }, FontSize::Small);

    // 2. Controls Footer
    SDL_SetRenderDrawColor(m_renderer, 15, 20, 30, 215);
    SDL_Rect botBar{ 15, WINDOW_HEIGHT - 42, WINDOW_WIDTH - 30, 34 };
    SDL_RenderFillRect(m_renderer, &botBar);

    std::string controls = Localization::instance().getControlsText();
    drawText(controls, 25, WINDOW_HEIGHT - 36, { 205, 215, 230, 255 }, FontSize::Small);

    int notifY = 88;

    // 3. Wave Incoming Announcement Banner
    if (dayNight.getWaveManager().getWaveBannerTimer() > 0.0f) {
        int bannerW = 620;
        SDL_SetRenderDrawColor(m_renderer, 35, 15, 25, 240);
        SDL_Rect waveBanner{ WINDOW_WIDTH / 2 - bannerW / 2, notifY, bannerW, 38 };
        SDL_RenderFillRect(m_renderer, &waveBanner);
        SDL_SetRenderDrawColor(m_renderer, 255, 80, 60, 255);
        SDL_RenderDrawRect(m_renderer, &waveBanner);

        std::string waveMsg = Localization::instance().getWaveBannerText(
            dayNight.getWaveManager().getCurrentWave(),
            dayNight.getWaveManager().getTotalWaves(),
            dayNight.getWaveManager().isBossWave()
        );
        drawText(waveMsg, WINDOW_WIDTH / 2 - bannerW / 2 + 15, notifY + 8, { 255, 230, 130, 255 }, FontSize::Small);
        notifY += 44;
    }

    // 4. Wave Cleared Banner
    if (dayNight.getWaveManager().getWaveClearedTimer() > 0.0f) {
        int bannerW = 560;
        SDL_SetRenderDrawColor(m_renderer, 15, 35, 30, 240);
        SDL_Rect clearBanner{ WINDOW_WIDTH / 2 - bannerW / 2, notifY, bannerW, 38 };
        SDL_RenderFillRect(m_renderer, &clearBanner);
        SDL_SetRenderDrawColor(m_renderer, 90, 230, 140, 255);
        SDL_RenderDrawRect(m_renderer, &clearBanner);

        bool isAr = Localization::instance().isArabic();
        int clearedWave = dayNight.getWaveManager().getLastClearedWave();
        int totalWaves = dayNight.getWaveManager().getTotalWaves();
        std::string clearMsg = isAr ?
            ("تم صد الموجة " + std::to_string(clearedWave) + " بنجاح! استعد للمرحلة التالية.") :
            ("Wave " + std::to_string(clearedWave) + "/" + std::to_string(totalWaves) + " Survived! Preparing next assault.");
        drawText(clearMsg, WINDOW_WIDTH / 2 - bannerW / 2 + 18, notifY + 8, { 180, 255, 200, 255 }, FontSize::Small);
        notifY += 44;
    }

    // 5. Status Notification Banner
    if (m_statusMessageTimer > 0.0f) {
        SDL_SetRenderDrawColor(m_renderer, 10, 15, 25, 235);
        SDL_Rect notif{ WINDOW_WIDTH / 2 - 340, notifY, 680, 38 };
        SDL_RenderFillRect(m_renderer, &notif);
        SDL_SetRenderDrawColor(m_renderer, 240, 190, 60, 255);
        SDL_RenderDrawRect(m_renderer, &notif);

        int tx = WINDOW_WIDTH / 2 - 310;
        drawText(m_statusMessage, tx, notifY + 8, { 255, 240, 160, 255 }, FontSize::Small);
    }

    // 6. Tactical Wave Telegraphing Card (during Dusk or Intermission)
    WavePreview preview = dayNight.getUpcomingWavePreview();
    if (preview.active && dayNight.getWaveManager().getWaveBannerTimer() <= 0.0f) {
        renderWaveTelegraph(preview);
    }

    // 7. Dedicated Dawn Summary Modal Card
    if (dayNight.hasDawnReward()) {
        renderDawnSummary(dayNight.getLastDawnReward(), dayNight.getDayNumber());
    }
}

void UIManager::renderWaveTelegraph(const WavePreview& preview) {
    if (!m_renderer || !preview.active) return;

    bool isAr = Localization::instance().isArabic();
    int cardW = 340;
    int cardH = 150;
    int x = WINDOW_WIDTH - cardW - 20;
    int y = 92;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 18, 20, 32, 235);
    SDL_Rect card{ x, y, cardW, cardH };
    SDL_RenderFillRect(m_renderer, &card);

    // Glowing border (crimson/orange)
    SDL_SetRenderDrawColor(m_renderer, 245, 130, 45, 255);
    SDL_RenderDrawRect(m_renderer, &card);

    // Header: Wave / Total + Countdown
    int countInt = static_cast<int>(preview.countdown + 0.99f);
    std::ostringstream headSS;
    if (isAr) {
        headSS << "رصد الموجة " << preview.waveNumber << " / " << preview.totalWaves << " (خلال " << countInt << " ث)";
    } else {
        headSS << "WAVE " << preview.waveNumber << " / " << preview.totalWaves << " INCOMING (" << countInt << "s)";
    }
    drawText(headSS.str(), x + 12, y + 8, { 255, 215, 80, 255 }, FontSize::Small);

    // Enemy Breakdown
    int lineY = y + 36;
    if (isAr) {
        drawText("كائنات الظل المتوقعة في الهجوم:", x + 12, lineY, { 200, 215, 230, 255 }, FontSize::Small);
    } else {
        drawText("Incoming Wave Composition:", x + 12, lineY, { 200, 215, 230, 255 }, FontSize::Small);
    }
    lineY += 24;

    std::vector<std::string> enemyLines;
    if (isAr) {
        if (preview.crawlers > 0) enemyLines.push_back("• " + std::to_string(preview.crawlers) + " زواحف سريعة");
        if (preview.eaters > 0) enemyLines.push_back("• " + std::to_string(preview.eaters) + " ملتهمو نور (يستهدفون المرايا)");
        if (preview.brutes > 0) enemyLines.push_back("• " + std::to_string(preview.brutes) + " كواسر مدرعة (مقاومة للضربات)");
        if (preview.leviathans > 0) enemyLines.push_back("• 1 وحش الأعماق العظيم (زعيم!)");
    } else {
        if (preview.crawlers > 0) enemyLines.push_back("• " + std::to_string(preview.crawlers) + " Crawlers (Swarmers)");
        if (preview.eaters > 0) enemyLines.push_back("• " + std::to_string(preview.eaters) + " Eaters (Target Mirrors)");
        if (preview.brutes > 0) enemyLines.push_back("• " + std::to_string(preview.brutes) + " Brutes (Resist Melee)");
        if (preview.leviathans > 0) enemyLines.push_back("• 1 Leviathan (Abyssal Boss!)");
    }

    for (const auto& el : enemyLines) {
        drawText(el, x + 16, lineY, { 255, 170, 100, 255 }, FontSize::Small);
        lineY += 20;
    }

    std::string tipStr = isAr ? "استعد بالدفاعات وتوجيه المرايا!" : "Prepare defenses & position mirrors!";
    drawText(tipStr, x + 12, y + cardH - 24, { 180, 200, 220, 255 }, FontSize::Small);
}

void UIManager::renderDawnSummary(const DawnReward& r, int nextDay) {
    if (!m_renderer) return;

    bool isAr = Localization::instance().isArabic();
    int cardW = 660;
    int cardH = 380;
    int x = WINDOW_WIDTH / 2 - cardW / 2;
    int y = WINDOW_HEIGHT / 2 - cardH / 2 + 10;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 12, 18, 30, 245);
    SDL_Rect card{ x, y, cardW, cardH };
    SDL_RenderFillRect(m_renderer, &card);

    // Golden decorative border
    SDL_SetRenderDrawColor(m_renderer, 245, 210, 80, 255);
    SDL_RenderDrawRect(m_renderer, &card);
    SDL_Rect innerBorder{ x + 4, y + 4, cardW - 8, cardH - 8 };
    SDL_SetRenderDrawColor(m_renderer, 60, 85, 120, 180);
    SDL_RenderDrawRect(m_renderer, &innerBorder);

    // Title & Day Header
    std::string title = isAr ? "انبلج الفجر - تم الصمود في وجه الظلام!" : "DAWN BREAKS - NIGHT DEFENSE SURVIVED!";
    drawText(title, x + 24, y + 16, { 255, 225, 90, 255 }, FontSize::Large);

    std::string subTitle = isAr ?
        ("اكتمل اليوم " + std::to_string(r.dayNumber) + " | الموجات التي تم صدها: " + std::to_string(r.wavesCompleted) + " / " + std::to_string(r.totalWaves)) :
        ("Day " + std::to_string(r.dayNumber) + " Complete | Waves Survived: " + std::to_string(r.wavesCompleted) + " / " + std::to_string(r.totalWaves));
    drawText(subTitle, x + 24, y + 54, { 190, 220, 245, 255 }, FontSize::Small);

    // Separator line
    SDL_SetRenderDrawColor(m_renderer, 70, 95, 130, 200);
    SDL_RenderDrawLine(m_renderer, x + 24, y + 80, x + cardW - 24, y + 80);

    // Left Column: Defense Performance Stats
    int col1X = x + 28;
    int statsY = y + 96;
    std::string statHead = isAr ? "إحصائيات الصمود والدفاع:" : "DEFENSE & SURVIVAL REPORT:";
    drawText(statHead, col1X, statsY, { 255, 200, 120, 255 }, FontSize::Medium);
    statsY += 34;

    std::string killStr = isAr ? ("• صرعى كائنات الظل: " + std::to_string(r.enemiesDefeated)) :
                                 ("• Shadows Defeated: " + std::to_string(r.enemiesDefeated));
    drawText(killStr, col1X, statsY, { 220, 230, 240, 255 }, FontSize::Small);
    statsY += 26;

    std::string hpStr = isAr ?
        ("• سلامة المنارة: " + std::to_string((int)r.lighthouseHp) + " / " + std::to_string((int)r.lighthouseMaxHp) + " (" + std::to_string((int)r.lighthouseHpPercent) + "%)") :
        ("• Beacon Integrity: " + std::to_string((int)r.lighthouseHp) + " / " + std::to_string((int)r.lighthouseMaxHp) + " (" + std::to_string((int)r.lighthouseHpPercent) + "%)");
    drawText(hpStr, col1X, statsY, { 220, 230, 240, 255 }, FontSize::Small);
    statsY += 26;

    std::string fuelStr = isAr ? ("• الوقود المتبقي في الخزان: " + std::to_string((int)r.fuelRemaining) + " / 100") :
                                  ("• Fuel Remaining: " + std::to_string((int)r.fuelRemaining) + " / 100");
    drawText(fuelStr, col1X, statsY, { 220, 230, 240, 255 }, FontSize::Small);
    statsY += 26;

    std::string mirStr = isAr ? ("• المرايا السليمة الباقية: " + std::to_string(r.mirrorsPreserved)) :
                                 ("• Mirrors Preserved Intact: " + std::to_string(r.mirrorsPreserved));
    drawText(mirStr, col1X, statsY, { 220, 230, 240, 255 }, FontSize::Small);

    // Right Column: Dawn Rewards & Bounty Breakdown
    int col2X = x + 350;
    int rewY = y + 96;
    std::string rewHead = isAr ? "غنائم ومكافآت الفجر:" : "HARVEST BOUNTY REWARDS:";
    drawText(rewHead, col2X, rewY, { 255, 200, 120, 255 }, FontSize::Medium);
    rewY += 34;

    std::string wStr = isAr ? ("+ " + std::to_string(r.wood) + " خشب للأبنية والمرايا") : ("+ " + std::to_string(r.wood) + " Wood for crafting");
    drawText(wStr, col2X, rewY, { 200, 255, 170, 255 }, FontSize::Small);
    rewY += 26;

    std::string cStr = isAr ? ("+ " + std::to_string(r.crystals) + " بلورات للترقيات والعدسات") : ("+ " + std::to_string(r.crystals) + " Crystals for upgrades");
    drawText(cStr, col2X, rewY, { 180, 235, 255, 255 }, FontSize::Small);
    rewY += 26;

    std::string oStr = isAr ? ("+ " + std::to_string(r.oil) + " وقود لشعلة المنارة") : ("+ " + std::to_string(r.oil) + " Oil for the Beacon flame");
    drawText(oStr, col2X, rewY, { 255, 230, 150, 255 }, FontSize::Small);
    rewY += 26;

    if (r.relicCores > 0) {
        std::string coreStr = isAr ? ("+ " + std::to_string(r.relicCores) + " نواة أثرية قديمة!") : ("+ " + std::to_string(r.relicCores) + " Ancient Relic Core!");
        drawText(coreStr, col2X, rewY, { 255, 160, 255, 255 }, FontSize::Small);
        rewY += 26;
    }
    if (r.salves > 0) {
        std::string salveStr = isAr ? ("+ " + std::to_string(r.salves) + " مرهم شفاء الحارس [H]") : ("+ " + std::to_string(r.salves) + " Healing Salve [H]");
        drawText(salveStr, col2X, rewY, { 160, 255, 200, 255 }, FontSize::Small);
        rewY += 26;
    }

    // Bottom Transition Card: NEXT DAY
    int botBoxY = y + cardH - 74;
    SDL_SetRenderDrawColor(m_renderer, 24, 38, 58, 255);
    SDL_Rect nextBox{ x + 16, botBoxY, cardW - 32, 54 };
    SDL_RenderFillRect(m_renderer, &nextBox);
    SDL_SetRenderDrawColor(m_renderer, 100, 160, 220, 255);
    SDL_RenderDrawRect(m_renderer, &nextBox);

    std::string nextStr = isAr ?
        ("المرحلة التالية: اليوم " + std::to_string(nextDay) + " (استكشاف، ترقيات، وإشعال المذابح)") :
        ("NEXT: DAY " + std::to_string(nextDay) + " (Gather resources, craft upgrades & ignite altars)");
    drawText(nextStr, x + 32, botBoxY + 8, { 255, 230, 110, 255 }, FontSize::Medium);

    std::string tipStr = isAr ? "تمت إضافة الموارد لحقيبتك. تحرك بحرية واستعد لليوم التالي." :
                                "All rewards placed in inventory. Move freely and explore the island.";
    drawText(tipStr, x + 32, botBoxY + 32, { 180, 205, 225, 255 }, FontSize::Small);
}

void UIManager::renderWorkshop(int selectedIndex, const Player& player, const Lighthouse& lighthouse) {
    if (!m_renderer) return;

    bool isAr = Localization::instance().isArabic();

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 8, 12, 20, 245);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    int panelX = 50;
    int panelY = 40;
    int panelW = WINDOW_WIDTH - 100;
    int panelH = WINDOW_HEIGHT - 80;

    SDL_SetRenderDrawColor(m_renderer, 20, 26, 38, 255);
    SDL_Rect panel{ panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(m_renderer, &panel);
    SDL_SetRenderDrawColor(m_renderer, 225, 185, 75, 255);
    SDL_RenderDrawRect(m_renderer, &panel);

    // Title Header
    std::string title = Localization::instance().getWorkshopTitle();
    std::string sub = Localization::instance().getWorkshopSubtitle();
    drawText(title, panelX + 24, panelY + 14, { 255, 220, 90, 255 }, FontSize::Large);
    drawText(sub, panelX + 24, panelY + 48, { 170, 190, 210, 255 }, FontSize::Small);

    // Separator line
    SDL_SetRenderDrawColor(m_renderer, 60, 80, 110, 200);
    SDL_RenderDrawLine(m_renderer, panelX + 20, panelY + 74, panelX + panelW - 20, panelY + 74);

    // Left Column: Items List (12 items)
    int leftW = 460;
    int leftX = panelX + 24;
    int listY = panelY + 88;

    std::vector<std::string> items = Localization::instance().getWorkshopItems(lighthouse, player);

    for (size_t i = 0; i < items.size(); ++i) {
        int itemY = listY + static_cast<int>(i) * 40;
        bool isSel = (selectedIndex == static_cast<int>(i));

        SDL_Rect itemBg{ leftX, itemY, leftW, 34 };
        if (isSel) {
            SDL_SetRenderDrawColor(m_renderer, 45, 75, 115, 255);
            SDL_RenderFillRect(m_renderer, &itemBg);
            SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
            SDL_RenderDrawRect(m_renderer, &itemBg);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 26, 33, 46, 180);
            SDL_RenderFillRect(m_renderer, &itemBg);
        }

        SDL_Color textCol = isSel ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 190, 205, 220, 255 };
        drawText(items[i], leftX + 12, itemY + 6, textCol, FontSize::Small);
    }

    // Right Column: Inspector Details Card
    int rightX = leftX + leftW + 24;
    int rightW = panelW - leftW - 68;
    int rightY = panelY + 88;
    int rightH = panelH - 110;

    SDL_SetRenderDrawColor(m_renderer, 15, 20, 30, 255);
    SDL_Rect inspectorRect{ rightX, rightY, rightW, rightH };
    SDL_RenderFillRect(m_renderer, &inspectorRect);
    SDL_SetRenderDrawColor(m_renderer, 80, 105, 140, 255);
    SDL_RenderDrawRect(m_renderer, &inspectorRect);

    WorkshopItemInfo info = Localization::instance().getWorkshopItemInfo(selectedIndex, lighthouse, player);

    // Inspector Content
    int curY = rightY + 16;
    drawText(info.category, rightX + 20, curY, { 120, 210, 255, 255 }, FontSize::Small);
    curY += 24;

    drawText(info.name, rightX + 20, curY, { 255, 230, 100, 255 }, FontSize::Large);
    curY += 40;

    // Progression / Level box
    SDL_SetRenderDrawColor(m_renderer, 25, 34, 48, 255);
    SDL_Rect progBox{ rightX + 20, curY, rightW - 40, 48 };
    SDL_RenderFillRect(m_renderer, &progBox);
    SDL_SetRenderDrawColor(m_renderer, 60, 80, 110, 255);
    SDL_RenderDrawRect(m_renderer, &progBox);

    std::string lvlLine = info.currentLevelStr + "   ──▶   " + info.nextLevelStr;
    drawText(lvlLine, rightX + 32, curY + 12, { 255, 240, 180, 255 }, FontSize::Small);
    curY += 60;

    // Stats / Effects
    drawText(info.currentStatStr, rightX + 24, curY, { 210, 220, 235, 255 }, FontSize::Small);
    curY += 26;
    drawText(info.nextStatStr, rightX + 24, curY, { 140, 240, 160, 255 }, FontSize::Small);
    curY += 38;

    // Resource Costs Section
    std::string costTitle = isAr ? "الموارد المطلوبة للصنع / الترقية:" : "REQUIRED MATERIALS:";
    drawText(costTitle, rightX + 24, curY, { 240, 200, 100, 255 }, FontSize::Medium);
    curY += 32;

    bool canAfford = true;

    if (info.costWood > 0) {
        bool haveW = (player.wood >= info.costWood);
        if (!haveW) canAfford = false;
        std::ostringstream ss;
        if (isAr) {
            ss << "• خشب: " << info.costWood << " (المتوفر: " << player.wood << ")";
        } else {
            ss << "• Wood: " << info.costWood << " (Have: " << player.wood << ")";
        }
        drawText(ss.str(), rightX + 32, curY, haveW ? SDL_Color{ 180, 255, 180, 255 } : SDL_Color{ 255, 110, 110, 255 }, FontSize::Small);
        curY += 24;
    }

    if (info.costCrystals > 0) {
        bool haveC = (player.crystals >= info.costCrystals);
        if (!haveC) canAfford = false;
        std::ostringstream ss;
        if (isAr) {
            ss << "• بلورات: " << info.costCrystals << " (المتوفر: " << player.crystals << ")";
        } else {
            ss << "• Crystals: " << info.costCrystals << " (Have: " << player.crystals << ")";
        }
        drawText(ss.str(), rightX + 32, curY, haveC ? SDL_Color{ 180, 255, 180, 255 } : SDL_Color{ 255, 110, 110, 255 }, FontSize::Small);
        curY += 24;
    }

    if (info.costOil > 0) {
        bool haveO = (player.oil >= info.costOil);
        if (!haveO) canAfford = false;
        std::ostringstream ss;
        if (isAr) {
            ss << "• وقود: " << info.costOil << " (المتوفر: " << player.oil << ")";
        } else {
            ss << "• Oil: " << info.costOil << " (Have: " << player.oil << ")";
        }
        drawText(ss.str(), rightX + 32, curY, haveO ? SDL_Color{ 180, 255, 180, 255 } : SDL_Color{ 255, 110, 110, 255 }, FontSize::Small);
        curY += 24;
    }

    // Action / Affordability Button Box
    int btnY = rightY + rightH - 68;
    SDL_Rect actionBtn{ rightX + 24, btnY, rightW - 48, 48 };

    if (info.isMaxed) {
        SDL_SetRenderDrawColor(m_renderer, 40, 48, 60, 255);
        SDL_RenderFillRect(m_renderer, &actionBtn);
        SDL_SetRenderDrawColor(m_renderer, 80, 100, 120, 255);
        SDL_RenderDrawRect(m_renderer, &actionBtn);
        std::string maxLbl = isAr ? "[ تم بلوغ المستوى الأقصى للترقية ]" : "[ MAXIMUM LEVEL ACHIEVED ]";
        drawText(maxLbl, rightX + 60, btnY + 12, { 160, 200, 220, 255 }, FontSize::Medium);
    } else if (canAfford) {
        SDL_SetRenderDrawColor(m_renderer, 30, 80, 45, 255);
        SDL_RenderFillRect(m_renderer, &actionBtn);
        SDL_SetRenderDrawColor(m_renderer, 90, 235, 120, 255);
        SDL_RenderDrawRect(m_renderer, &actionBtn);
        std::string actLbl = info.isUpgrade ?
            (isAr ? "[ اضغط ENTER للترقية ]" : "[ PRESS ENTER TO UPGRADE ]") :
            (isAr ? "[ اضغط ENTER للصنع / الشراء ]" : "[ PRESS ENTER TO CRAFT / BUY ]");
        drawText(actLbl, rightX + 50, btnY + 12, { 255, 240, 140, 255 }, FontSize::Medium);
    } else {
        SDL_SetRenderDrawColor(m_renderer, 65, 25, 25, 255);
        SDL_RenderFillRect(m_renderer, &actionBtn);
        SDL_SetRenderDrawColor(m_renderer, 200, 60, 60, 255);
        SDL_RenderDrawRect(m_renderer, &actionBtn);
        std::string noMatLbl = isAr ? "[ الموارد غير كافية للترقية ]" : "[ INSUFFICIENT RESOURCES ]";
        drawText(noMatLbl, rightX + 60, btnY + 12, { 255, 160, 160, 255 }, FontSize::Medium);
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

    SDL_SetRenderDrawColor(m_renderer, 24, 32, 46, 255);
    SDL_Rect card{ WINDOW_WIDTH / 2 - 320, WINDOW_HEIGHT / 2 - 220, 640, 440 };
    SDL_RenderFillRect(m_renderer, &card);
    SDL_SetRenderDrawColor(m_renderer, 220, 180, 80, 255);
    SDL_RenderDrawRect(m_renderer, &card);

    bool isAr = Localization::instance().isArabic();
    std::string title = Localization::instance().get("SETTINGS_TITLE");
    drawText(title, WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2 - 185, { 255, 220, 90, 255 }, FontSize::Large);

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
