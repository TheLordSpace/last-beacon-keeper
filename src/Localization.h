#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <unordered_map>

enum class Language {
    English,
    Arabic
};

class Player;
class Lighthouse;

struct WorkshopItemInfo {
    std::string name;
    std::string category;
    std::string currentLevelStr;
    std::string nextLevelStr;
    std::string currentStatStr;
    std::string nextStatStr;
    int costWood = 0;
    int costCrystals = 0;
    int costOil = 0;
    bool isMaxed = false;
    bool isUpgrade = false;
};

class Localization {
public:
    static Localization& instance();

    Language getLanguage() const { return m_lang; }
    void setLanguage(Language lang) { m_lang = lang; }
    void toggleLanguage() {
        m_lang = (m_lang == Language::Arabic) ? Language::English : Language::Arabic;
    }

    bool isArabic() const { return m_lang == Language::Arabic; }

    std::string get(const std::string& key) const;
    std::string shapeText(const std::string& text) const;

    // Localized formatted text helpers
    std::string getClockText(int day, DayPhase phase, int secondsLeft, int currentWave = 0, int totalWaves = 0) const;
    std::string getWaveBannerText(int currentWave, int totalWaves, bool isBoss) const;
    std::string getDawnSummaryTitle() const;
    std::string getDawnSummaryStats(int kills, int hpPercent, int mirrors) const;
    std::string getDawnSummaryBounty(int wood, int crystals, int oil, int cores, int salves) const;
    std::string getLighthouseAttackWarning() const;
    std::string getInventoryText(int wood, int crystals, int oil, int mirrors, int cores, int salves) const;
    std::string getControlsText() const;
    std::string getWorkshopTitle() const;
    std::string getWorkshopSubtitle() const;
    std::vector<std::string> getWorkshopItems(const Lighthouse& lighthouse, const Player& player) const;
    WorkshopItemInfo getWorkshopItemInfo(int index, const Lighthouse& lighthouse, const Player& player) const;
    std::vector<std::string> getJournalLines() const;

private:
    Localization();
    Language m_lang = Language::Arabic; // Default to Arabic as requested by user

    std::unordered_map<std::string, std::string> m_stringsEN;
    std::unordered_map<std::string, std::string> m_stringsAR;

    void initStrings();
    std::string applyFriBidi(const std::string& input) const;
};
