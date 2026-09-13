#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <unordered_map>

enum class Language {
    English,
    Arabic
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
    std::string getClockText(int day, DayPhase phase, int secondsLeft) const;
    std::string getInventoryText(int wood, int crystals, int oil, int mirrors, int cores, int salves) const;
    std::string getControlsText() const;
    std::string getWorkshopTitle() const;
    std::string getWorkshopSubtitle() const;
    std::vector<std::string> getWorkshopItems() const;
    std::vector<std::string> getJournalLines() const;

private:
    Localization();
    Language m_lang = Language::Arabic; // Default to Arabic as requested by user

    std::unordered_map<std::string, std::string> m_stringsEN;
    std::unordered_map<std::string, std::string> m_stringsAR;

    void initStrings();
    std::string applyFriBidi(const std::string& input) const;
};
