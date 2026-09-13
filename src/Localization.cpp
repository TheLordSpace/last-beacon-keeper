#include "Localization.h"
#include "Entities.h"
#include "Lighthouse.h"
#include <fribidi/fribidi.h>
#include <sstream>
#include <iostream>
#include <algorithm>

Localization& Localization::instance() {
    static Localization s_inst;
    return s_inst;
}

Localization::Localization() {
    initStrings();
}

std::string Localization::applyFriBidi(const std::string& input) const {
    if (input.empty()) return "";

    int utf8_len = static_cast<int>(input.length());
    std::vector<FriBidiChar> ustr(utf8_len + 1);
    FriBidiStrIndex len = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, input.c_str(), utf8_len, ustr.data());

    std::vector<FriBidiCharType> bidi_types(len);
    fribidi_get_bidi_types(ustr.data(), len, bidi_types.data());

    std::vector<FriBidiBracketType> bracket_types(len);
    fribidi_get_bracket_types(ustr.data(), len, bidi_types.data(), bracket_types.data());

    FriBidiParType base_dir = FRIBIDI_PAR_RTL;
    std::vector<FriBidiLevel> embedding_levels(len);
    FriBidiLevel lvl1 = fribidi_get_par_embedding_levels_ex(bidi_types.data(), bracket_types.data(), len, &base_dir, embedding_levels.data());
    (void)lvl1;

    std::vector<FriBidiArabicProp> ar_props(len);
    fribidi_get_joining_types(ustr.data(), len, ar_props.data());
    fribidi_join_arabic(bidi_types.data(), len, embedding_levels.data(), ar_props.data());

    fribidi_shape(FRIBIDI_FLAGS_DEFAULT | FRIBIDI_FLAGS_ARABIC, embedding_levels.data(), len, ar_props.data(), ustr.data());

    FriBidiLevel lvl2 = fribidi_reorder_line(FRIBIDI_FLAGS_DEFAULT, bidi_types.data(), len, 0, base_dir, embedding_levels.data(), ustr.data(), nullptr);
    (void)lvl2;

    std::vector<char> output(len * 4 + 1);
    fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, ustr.data(), len, output.data());

    return std::string(output.data());
}

std::string Localization::shapeText(const std::string& text) const {
    if (m_lang == Language::Arabic) {
        return applyFriBidi(text);
    }
    return text;
}

std::string Localization::get(const std::string& key) const {
    const auto& map = (m_lang == Language::Arabic) ? m_stringsAR : m_stringsEN;
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    // Fallback to English if not found
    auto itEn = m_stringsEN.find(key);
    if (itEn != m_stringsEN.end()) {
        return itEn->second;
    }
    return key;
}

std::string Localization::getClockText(int day, DayPhase phase, int secondsLeft, int currentWave, int totalWaves) const {
    if (m_lang == Language::Arabic) {
        std::string pStr;
        switch (phase) {
        case DayPhase::Day: pStr = "استكشاف النهار"; break;
        case DayPhase::Dusk: pStr = "الغسق (استعد للظلام!)"; break;
        case DayPhase::Night:
            if (currentWave > 0 && totalWaves > 0) {
                pStr = "المد الظلي | الموجة " + std::to_string(currentWave) + "/" + std::to_string(totalWaves);
            } else {
                pStr = "المد الظلي";
            }
            break;
        case DayPhase::Dawn: pStr = "بزوغ الفجر ومكافأة الصمود"; break;
        }
        std::ostringstream ss;
        ss << "اليوم " << day << " - " << pStr << " (" << secondsLeft << " ث)";
        return ss.str();
    } else {
        std::string pStr;
        switch (phase) {
        case DayPhase::Day: pStr = "DAY (EXPLORATION)"; break;
        case DayPhase::Dusk: pStr = "DUSK (PREPARE!)"; break;
        case DayPhase::Night:
            if (currentWave > 0 && totalWaves > 0) {
                pStr = "SHADOW TIDE | WAVE " + std::to_string(currentWave) + "/" + std::to_string(totalWaves);
            } else {
                pStr = "SHADOW TIDE";
            }
            break;
        case DayPhase::Dawn: pStr = "DAWN REWARD"; break;
        }
        std::ostringstream ss;
        ss << "Day " << day << " - " << pStr << " (" << secondsLeft << "s)";
        return ss.str();
    }
}

std::string Localization::getWaveBannerText(int currentWave, int totalWaves, bool isBoss) const {
    if (m_lang == Language::Arabic) {
        if (isBoss) {
            return "وحش الأعماق العظيم يخرج من المياه المظلمة!";
        }
        return "انطلاق الموجة " + std::to_string(currentWave) + " من " + std::to_string(totalWaves) + "! احمِ المنارة!";
    } else {
        if (isBoss) {
            return "THE ABYSSAL LEVIATHAN EMERGES FROM THE TIDE!";
        }
        return "WAVE " + std::to_string(currentWave) + "/" + std::to_string(totalWaves) + " INCOMING! Defend the Beacon!";
    }
}

std::string Localization::getDawnSummaryTitle() const {
    return (m_lang == Language::Arabic) ? "تم الصمود في الليل بنجاح!" : "NIGHT SURVIVED - DAWN BREAKS!";
}

std::string Localization::getDawnSummaryStats(int kills, int hpPercent, int mirrors) const {
    std::ostringstream ss;
    if (m_lang == Language::Arabic) {
        ss << "صرعى الظل: " << kills
           << " | سلامة المنارة: " << hpPercent << "%"
           << " | مرايا باقية: " << mirrors;
    } else {
        ss << "Shadows Slain: " << kills
           << " | Beacon Integrity: " << hpPercent << "%"
           << " | Mirrors Intact: " << mirrors;
    }
    return ss.str();
}

std::string Localization::getDawnSummaryBounty(int wood, int crystals, int oil, int cores, int salves) const {
    std::ostringstream ss;
    if (m_lang == Language::Arabic) {
        ss << "الغنائم: +" << wood << " خشب | +" << crystals << " بلورات | +" << oil << " وقود";
        if (cores > 0) ss << " | +" << cores << " أنوية أثرية";
        if (salves > 0) ss << " | +" << salves << " مراهم";
    } else {
        ss << "Bounty: +" << wood << " Wood | +" << crystals << " Crystals | +" << oil << " Oil";
        if (cores > 0) ss << " | +" << cores << " Relic Core";
        if (salves > 0) ss << " | +" << salves << " Salve";
    }
    return ss.str();
}

std::string Localization::getLighthouseAttackWarning() const {
    return (m_lang == Language::Arabic) ? "تحت الهجوم!" : "UNDER ATTACK!";
}

std::string Localization::getInventoryText(int wood, int crystals, int oil, int mirrors, int cores, int salves) const {
    std::ostringstream ss;
    if (m_lang == Language::Arabic) {
        ss << "خشب: " << wood
           << " | بلورات: " << crystals
           << " | وقود: " << oil
           << " | مرايا: " << mirrors
           << " | أنوية أثرية: " << cores
           << " | مراهم: " << salves;
    } else {
        ss << "Wood: " << wood
           << " | Crystals: " << crystals
           << " | Oil: " << oil
           << " | Mirrors: " << mirrors
           << " | Cores: " << cores
           << " | Salves: " << salves;
    }
    return ss.str();
}

std::string Localization::getControlsText() const {
    if (m_lang == Language::Arabic) {
        return "[WASD] حركة | [نقر أيسر] جمع/ضرب | [F] وضع مرآة | [R/نقر أيمن] تدوير | [Space] تفادي | [E] تفاعل/ورشة | [O] إعدادات | [M] مذكرات";
    } else {
        return "[WASD] Move | [L-Click] Chop/Mine/Attack | [F] Place Mirror | [R/R-Click] Rotate | [Space] Dash | [E] Interact | [O] Settings | [M] Journal";
    }
}

std::string Localization::getWorkshopTitle() const {
    return (m_lang == Language::Arabic) ? "ورشة المنارة والترقيات الدفاعية" : "THE LIGHTHOUSE WORKSHOP & UPGRADES";
}

std::string Localization::getWorkshopSubtitle() const {
    if (m_lang == Language::Arabic) {
        return "استخدم [W/S] للتنقل، [ENTER] للصنع أو الترقية، [ESC/E] للإغلاق";
    } else {
        return "Use [W/S] to Navigate, [ENTER] to Craft / Upgrade, [ESC/E] to Close";
    }
}

std::vector<std::string> Localization::getWorkshopItems(const Lighthouse& lighthouse, const Player& player) const {
    bool isAr = (m_lang == Language::Arabic);
    std::vector<std::string> list;

    if (isAr) {
        list.push_back("1. مرآة نحاسية عاكسة [" + std::to_string(player.mirrorsInBag) + " بالحقيبة]");
        list.push_back("2. تعبئة وقود المنارة (+40 وقود)");
        list.push_back("3. ترميم هيكل المنارة (+120 نقطة)");
        list.push_back("4. نواة أثرية للمذابح [" + std::to_string(player.relicCores) + " بالحقيبة]");
        list.push_back(std::string("5. عدسة العنبر الواقية ") + (lighthouse.unlockWideLens ? "[مفعلة]" : "[مقفلة]"));
        list.push_back(std::string("6. عدسة صدمة UV ") + (lighthouse.unlockUVLens ? "[مفعلة]" : "[مقفلة]"));
        list.push_back("7. مرهم شفاء الحارس [" + std::to_string(player.salves) + " بالحقيبة]");
        list.push_back("8. ترقية قوة الشعاع [مستوى " + std::to_string(lighthouse.beamPowerLevel) + "/3]");
        list.push_back("9. ترقية كفاءة الوقود [مستوى " + std::to_string(lighthouse.beamEfficiencyLevel) + "/3]");
        list.push_back("10. ترقية صلابة المرايا [مستوى " + std::to_string(lighthouse.mirrorDurabilityLevel) + "/3]");
        list.push_back("11. ترقية دروع المنارة [مستوى " + std::to_string(lighthouse.lighthouseArmorLevel) + "/3]");
        list.push_back("12. ترقية حذاء السرعة [مستوى " + std::to_string(lighthouse.swiftBootsLevel) + "/3]");
    } else {
        list.push_back("1. Reflective Brass Mirror [" + std::to_string(player.mirrorsInBag) + " in bag]");
        list.push_back("2. Beacon Fuel Tank (+40 Fuel)");
        list.push_back("3. Lighthouse Hull Repair (+120 HP)");
        list.push_back("4. Ancient Relic Core [" + std::to_string(player.relicCores) + " in bag]");
        list.push_back(std::string("5. Amber Wide-Defense Lens ") + (lighthouse.unlockWideLens ? "[Unlocked]" : "[Locked]"));
        list.push_back(std::string("6. UV Pulse Shockwave Lens ") + (lighthouse.unlockUVLens ? "[Unlocked]" : "[Locked]"));
        list.push_back("7. Keeper's Healing Salve [" + std::to_string(player.salves) + " in bag]");
        list.push_back("8. Upgrade: Beam Power [Lvl " + std::to_string(lighthouse.beamPowerLevel) + "/3]");
        list.push_back("9. Upgrade: Beam Efficiency [Lvl " + std::to_string(lighthouse.beamEfficiencyLevel) + "/3]");
        list.push_back("10. Upgrade: Mirror Durability [Lvl " + std::to_string(lighthouse.mirrorDurabilityLevel) + "/3]");
        list.push_back("11. Upgrade: Lighthouse Armor [Lvl " + std::to_string(lighthouse.lighthouseArmorLevel) + "/3]");
        list.push_back("12. Upgrade: Swift Boots [Lvl " + std::to_string(lighthouse.swiftBootsLevel) + "/3]");
    }

    return list;
}

WorkshopItemInfo Localization::getWorkshopItemInfo(int index, const Lighthouse& lighthouse, const Player& player) const {
    (void)player;
    bool isAr = (m_lang == Language::Arabic);
    WorkshopItemInfo info;

    switch (index) {
    case 0: // Mirror
        info.name = isAr ? "صناعة مرآة نحاسية عاكسة" : "Craft Reflective Brass Mirror";
        info.category = isAr ? "معدات بصرية قابلة للوضع" : "DEPLOYABLE OPTICS";
        info.currentLevelStr = isAr ? ("في الحقيبة: " + std::to_string(player.mirrorsInBag)) : ("In Bag: " + std::to_string(player.mirrorsInBag));
        info.nextLevelStr = isAr ? "توضع على الأرض [F] وتعكس شعاع النور بزاوية 90° [R]" : "Placed via [F] to redirect the lighthouse beam [R]";
        info.currentStatStr = isAr ? ("صحة المرآة: " + std::to_string((int)lighthouse.getMirrorMaxHealth()) + " HP") : ("Mirror Durability: " + std::to_string((int)lighthouse.getMirrorMaxHealth()) + " HP");
        info.nextStatStr = isAr ? "توجه الضوء لحرق كائنات الظل وحماية الشواطئ" : "Vaporizes crawling hordes along reflective corridors";
        info.costWood = 10;
        info.costCrystals = 5;
        info.isUpgrade = false;
        info.isMaxed = false;
        break;

    case 1: // Refuel
        info.name = isAr ? "تزويد المنارة بالوقود (+40)" : "Refuel Lighthouse Tank (+40 Fuel)";
        info.category = isAr ? "صيانة المنارة الأساسية" : "BEACON MAINTENANCE";
        info.currentLevelStr = isAr ? ("الوقود الحالي: " + std::to_string((int)lighthouse.getFuel()) + " / 100") : ("Current Fuel: " + std::to_string((int)lighthouse.getFuel()) + " / 100");
        info.nextLevelStr = isAr ? "يغذي شعلة النور للحفاظ على الشعاع مشتعلاً طوال الليل" : "Keeps the central flame burning through dark nights";
        info.currentStatStr = isAr ? "استهلاك الوقود: عادي" : "Fuel Burn Rate: Standard";
        info.nextStatStr = isAr ? "+40 إلى خزان وقود المنارة فوراً" : "+40 Fuel added immediately";
        info.costOil = 10;
        info.isUpgrade = false;
        info.isMaxed = false;
        break;

    case 2: // Repair
        info.name = isAr ? "إصلاح هيكل المنارة (+120 HP)" : "Lighthouse Hull Repair (+120 HP)";
        info.category = isAr ? "صيانة المنارة الأساسية" : "BEACON MAINTENANCE";
        info.currentLevelStr = isAr ? ("صحة الهيكل: " + std::to_string((int)lighthouse.getHealth()) + " / " + std::to_string((int)lighthouse.getMaxHealth())) : ("Hull Integrity: " + std::to_string((int)lighthouse.getHealth()) + " / " + std::to_string((int)lighthouse.getMaxHealth()));
        info.nextLevelStr = isAr ? "ترميم الأضرار الناتجة عن هجمات كواسر الظل" : "Patches breaches caused by shadow abominations";
        info.currentStatStr = isAr ? "الهيكل الأساسي للمنارة" : "Lighthouse Citadel Foundation";
        info.nextStatStr = isAr ? "+120 نقطة صحة لهيكل المنارة" : "+120 HP restored to Lighthouse";
        info.costWood = 15;
        info.costCrystals = 10;
        info.isUpgrade = false;
        info.isMaxed = false;
        break;

    case 3: // Relic Core
        info.name = isAr ? "صناعة نواة أثرية قديمة" : "Forge Ancient Relic Core";
        info.category = isAr ? "مقتنيات النصر المقدسة" : "SACRED ARTIFACT";
        info.currentLevelStr = isAr ? ("في الحقيبة: " + std::to_string(player.relicCores)) : ("In Bag: " + std::to_string(player.relicCores));
        info.nextLevelStr = isAr ? "تستخدم لإشعال المذابح الأثرية الثلاثة في أرجاء الجزيرة" : "Required to rekindle the 3 Ancient Altars across the isle";
        info.currentStatStr = isAr ? "تحتوي على طاقة شمسية مركزة" : "Contains dormant celestial solar power";
        info.nextStatStr = isAr ? "إشعال المذابح الثلاثة يوقظ الفجر الأول ويحقق النصر!" : "Igniting 3 Altars awakens the First Dawn and wins the game!";
        info.costWood = 15;
        info.costCrystals = 20;
        info.isUpgrade = false;
        info.isMaxed = false;
        break;

    case 4: // Amber Lens
        info.name = isAr ? "عدسة العنبر الواقية العريضة" : "Amber Wide-Defense Lens";
        info.category = isAr ? "عدسات بصرية متطورة" : "OPTICAL UPGRADE";
        info.currentLevelStr = lighthouse.unlockWideLens ? (isAr ? "مفتوحة ومفعلة [2]" : "Unlocked [2]") : (isAr ? "مقفلة" : "Locked");
        info.nextLevelStr = isAr ? "شعاع ضوئي عريض بزاوية تغطية واسعة يدفع الأعداء للخلف" : "Emits a wide-angle arc beam that slows & repels crowds";
        info.currentStatStr = isAr ? "العدسة الأساسية: شعاع شمسي مركز" : "Current: Narrow Solar Beam";
        info.nextStatStr = isAr ? "تبديل سريع بالمفتاح [2]" : "Quick-swap using [2]";
        info.costCrystals = 15;
        info.isUpgrade = false;
        info.isMaxed = lighthouse.unlockWideLens;
        break;

    case 5: // UV Lens
        info.name = isAr ? "عدسة الصدمة فوق البنفسجية UV" : "UV Pulse Shockwave Lens";
        info.category = isAr ? "عدسات بصرية متطورة" : "OPTICAL UPGRADE";
        info.currentLevelStr = lighthouse.unlockUVLens ? (isAr ? "مفتوحة ومفعلة [3]" : "Unlocked [3]") : (isAr ? "مقفلة" : "Locked");
        info.nextLevelStr = isAr ? "نبضات موجية كهرومغناطيسية تصعق كائنات الظل وتشلها" : "Discharges periodic UV bursts that stun shadow abominations";
        info.currentStatStr = isAr ? "مدافع نبضية عالية التردد" : "High-frequency defensive pulse";
        info.nextStatStr = isAr ? "تبديل سريع بالمفتاح [3]" : "Quick-swap using [3]";
        info.costCrystals = 25;
        info.isUpgrade = false;
        info.isMaxed = lighthouse.unlockUVLens;
        break;

    case 6: // Salve
        info.name = isAr ? "صناعة مرهم شفاء الحارس" : "Craft Keeper's Healing Salve";
        info.category = isAr ? "مواد إسعافية" : "SURVIVAL CONSUMABLE";
        info.currentLevelStr = isAr ? ("في الحقيبة: " + std::to_string(player.salves)) : ("In Bag: " + std::to_string(player.salves));
        info.nextLevelStr = isAr ? "يستخدم بالمفتاح [H] لاستعادة 50 نقطة من صحة الحارس" : "Press [H] during exploration or combat to restore 50 HP";
        info.currentStatStr = isAr ? "صحة الحارس: 100 HP" : "Keeper Vitality: 100 HP";
        info.nextStatStr = isAr ? "علاج فوري لحالات الطوارئ" : "Instant tactical survival heal";
        info.costWood = 5;
        info.costCrystals = 5;
        info.isUpgrade = false;
        info.isMaxed = false;
        break;

    case 7: // Beam Power Upgrade
        info.name = isAr ? "ترقية قوة ضرر الشعاع" : "Lighthouse Beam Power Upgrade";
        info.category = isAr ? "ترقية منظومة المنارة" : "PERMANENT DEFENSE UPGRADE";
        info.isUpgrade = true;
        info.isMaxed = (lighthouse.beamPowerLevel >= 3);
        info.currentLevelStr = isAr ? ("المستوى: " + std::to_string(lighthouse.beamPowerLevel) + " / 3") : ("Level: " + std::to_string(lighthouse.beamPowerLevel) + " / 3");
        if (info.isMaxed) {
            info.nextLevelStr = isAr ? "تم الوصول للحد الأقصى (المستوى 3/3)" : "MAX Level Reached (Level 3/3)";
            info.currentStatStr = isAr ? "ضرر الشعاع: +50% ضرر فتاك (1.5x)" : "Beam Damage: +50% Overcharged (1.5x)";
            info.nextStatStr = isAr ? "أقصى طاقة مشعة ممكنة" : "Max radiant output";
        } else {
            int nextLvl = lighthouse.beamPowerLevel + 1;
            info.nextLevelStr = isAr ? ("ترقية إلى المستوى " + std::to_string(nextLvl) + " / 3") : ("Upgrade to Level " + std::to_string(nextLvl) + " / 3");
            int curBonus = (lighthouse.beamPowerLevel - 1) * 25;
            int nextBonus = curBonus + 25;
            info.currentStatStr = isAr ? ("مضاعف الضرر الحالي: +" + std::to_string(curBonus) + "%") : ("Current Damage Bonus: +" + std::to_string(curBonus) + "%");
            info.nextStatStr = isAr ? ("المستوى التالي: +" + std::to_string(nextBonus) + "% ضرر لشعاع المنارة") : ("Next Level: +" + std::to_string(nextBonus) + "% Beam Damage");
            info.costCrystals = (lighthouse.beamPowerLevel == 1) ? 20 : 30;
            info.costWood = (lighthouse.beamPowerLevel == 1) ? 10 : 15;
        }
        break;

    case 8: // Beam Efficiency Upgrade
        info.name = isAr ? "ترقية كفاءة استهلاك الوقود" : "Beam Fuel Efficiency Upgrade";
        info.category = isAr ? "ترقية منظومة المنارة" : "PERMANENT DEFENSE UPGRADE";
        info.isUpgrade = true;
        info.isMaxed = (lighthouse.beamEfficiencyLevel >= 3);
        info.currentLevelStr = isAr ? ("المستوى: " + std::to_string(lighthouse.beamEfficiencyLevel) + " / 3") : ("Level: " + std::to_string(lighthouse.beamEfficiencyLevel) + " / 3");
        if (info.isMaxed) {
            info.nextLevelStr = isAr ? "تم الوصول للحد الأقصى (المستوى 3/3)" : "MAX Level Reached (Level 3/3)";
            info.currentStatStr = isAr ? "استهلاك الوقود: -50% (نصف الاستهلاك)" : "Fuel Burn: -50% (Eco Mode)";
            info.nextStatStr = isAr ? "كفاءة حرق وقود مثالية" : "Max fuel conservation reached";
        } else {
            int nextLvl = lighthouse.beamEfficiencyLevel + 1;
            info.nextLevelStr = isAr ? ("ترقية إلى المستوى " + std::to_string(nextLvl) + " / 3") : ("Upgrade to Level " + std::to_string(nextLvl) + " / 3");
            int curSav = (lighthouse.beamEfficiencyLevel - 1) * 25;
            int nextSav = curSav + 25;
            info.currentStatStr = isAr ? ("توفير الوقود الحالي: -" + std::to_string(curSav) + "%") : ("Current Fuel Saving: -" + std::to_string(curSav) + "%");
            info.nextStatStr = isAr ? ("المستوى التالي: -" + std::to_string(nextSav) + "% استهلاك وقود المنارة") : ("Next Level: -" + std::to_string(nextSav) + "% Fuel Consumption");
            info.costCrystals = (lighthouse.beamEfficiencyLevel == 1) ? 15 : 25;
            info.costOil = (lighthouse.beamEfficiencyLevel == 1) ? 10 : 18;
        }
        break;

    case 9: // Mirror Durability Upgrade
        info.name = isAr ? "ترقية صلابة ومقاومة المرايا" : "Mirror Durability Upgrade";
        info.category = isAr ? "ترقية منظومة المنارة" : "PERMANENT DEFENSE UPGRADE";
        info.isUpgrade = true;
        info.isMaxed = (lighthouse.mirrorDurabilityLevel >= 3);
        info.currentLevelStr = isAr ? ("المستوى: " + std::to_string(lighthouse.mirrorDurabilityLevel) + " / 3") : ("Level: " + std::to_string(lighthouse.mirrorDurabilityLevel) + " / 3");
        if (info.isMaxed) {
            info.nextLevelStr = isAr ? "تم الوصول للحد الأقصى (المستوى 3/3)" : "MAX Level Reached (Level 3/3)";
            info.currentStatStr = isAr ? "صحة المرآة: 170 HP (مصفحة بنحاس مقوى)" : "Mirror Max HP: 170 HP (Reinforced Brass)";
            info.nextStatStr = isAr ? "أقصى مقاومة ضد هجمات كائنات النور" : "Maximum durability against eaters";
        } else {
            int nextLvl = lighthouse.mirrorDurabilityLevel + 1;
            info.nextLevelStr = isAr ? ("ترقية إلى المستوى " + std::to_string(nextLvl) + " / 3") : ("Upgrade to Level " + std::to_string(nextLvl) + " / 3");
            int curHp = static_cast<int>(lighthouse.getMirrorMaxHealth());
            int nextHp = curHp + 45;
            info.currentStatStr = isAr ? ("صحة المرآة الحالية: " + std::to_string(curHp) + " HP") : ("Current Mirror HP: " + std::to_string(curHp) + " HP");
            info.nextStatStr = isAr ? ("المستوى التالي: " + std::to_string(nextHp) + " HP (+45 صحة لكل المرايا)") : ("Next Level: " + std::to_string(nextHp) + " HP (+45 HP for all mirrors)");
            info.costWood = (lighthouse.mirrorDurabilityLevel == 1) ? 15 : 25;
            info.costCrystals = (lighthouse.mirrorDurabilityLevel == 1) ? 10 : 15;
        }
        break;

    case 10: // Lighthouse Armor Upgrade
        info.name = isAr ? "ترقية دروع وتحصينات المنارة" : "Lighthouse Citadel Armor";
        info.category = isAr ? "ترقية منظومة المنارة" : "PERMANENT DEFENSE UPGRADE";
        info.isUpgrade = true;
        info.isMaxed = (lighthouse.lighthouseArmorLevel >= 3);
        info.currentLevelStr = isAr ? ("المستوى: " + std::to_string(lighthouse.lighthouseArmorLevel) + " / 3") : ("Level: " + std::to_string(lighthouse.lighthouseArmorLevel) + " / 3");
        if (info.isMaxed) {
            info.nextLevelStr = isAr ? "تم الوصول للحد الأقصى (المستوى 3/3)" : "MAX Level Reached (Level 3/3)";
            info.currentStatStr = isAr ? "صحة المنارة: 850 HP (حصن منيع)" : "Citadel Integrity: 850 HP (Fortress)";
            info.nextStatStr = isAr ? "أعلى مستوى تدريع للمنارة" : "Ultimate citadel reinforcement";
        } else {
            int nextLvl = lighthouse.lighthouseArmorLevel + 1;
            info.nextLevelStr = isAr ? ("ترقية إلى المستوى " + std::to_string(nextLvl) + " / 3") : ("Upgrade to Level " + std::to_string(nextLvl) + " / 3");
            int curHp = static_cast<int>(lighthouse.getMaxHealth());
            int bonus = (lighthouse.lighthouseArmorLevel == 1) ? 150 : 200;
            int nextHp = curHp + bonus;
            info.currentStatStr = isAr ? ("صحة المنارة القصوى: " + std::to_string(curHp) + " HP") : ("Max Citadel HP: " + std::to_string(curHp) + " HP");
            info.nextStatStr = isAr ? ("المستوى التالي: " + std::to_string(nextHp) + " HP (+" + std::to_string(bonus) + " HP)") : ("Next Level: " + std::to_string(nextHp) + " HP (+" + std::to_string(bonus) + " HP)");
            info.costWood = (lighthouse.lighthouseArmorLevel == 1) ? 25 : 40;
            info.costCrystals = (lighthouse.lighthouseArmorLevel == 1) ? 20 : 35;
        }
        break;

    case 11: // Swift Boots Upgrade
        info.name = isAr ? "ترقية حذاء الحارس السريع" : "Keeper's Swift Boots Upgrade";
        info.category = isAr ? "ترقية قدرات الحارس" : "PERMANENT DEFENSE UPGRADE";
        info.isUpgrade = true;
        info.isMaxed = (lighthouse.swiftBootsLevel >= 3);
        info.currentLevelStr = isAr ? ("المستوى: " + std::to_string(lighthouse.swiftBootsLevel) + " / 3") : ("Level: " + std::to_string(lighthouse.swiftBootsLevel) + " / 3");
        if (info.isMaxed) {
            info.nextLevelStr = isAr ? "تم الوصول للحد الأقصى (المستوى 3/3)" : "MAX Level Reached (Level 3/3)";
            info.currentStatStr = isAr ? "السرعة: +56 | تفادي أسرع: 0.54 ثانية" : "Speed: +56 | Dash Cooldown: 0.54s";
            info.nextStatStr = isAr ? "أقصى رشاقة وحركة ميدانية" : "Maximum agility and mobility";
        } else {
            int nextLvl = lighthouse.swiftBootsLevel + 1;
            info.nextLevelStr = isAr ? ("ترقية إلى المستوى " + std::to_string(nextLvl) + " / 3") : ("Upgrade to Level " + std::to_string(nextLvl) + " / 3");
            int curSpdBonus = static_cast<int>(lighthouse.getPlayerSpeedBonus());
            int nextSpdBonus = curSpdBonus + 28;
            info.currentStatStr = isAr ? ("علاوة السرعة الحالية: +" + std::to_string(curSpdBonus)) : ("Current Speed Bonus: +" + std::to_string(curSpdBonus));
            info.nextStatStr = isAr ? ("المستوى التالي: +" + std::to_string(nextSpdBonus) + " سرعة وتفادٍ أسرع بـ 0.18 ث") : ("Next Level: +" + std::to_string(nextSpdBonus) + " Speed & -0.18s Dash CD");
            info.costWood = (lighthouse.swiftBootsLevel == 1) ? 15 : 25;
            info.costOil = (lighthouse.swiftBootsLevel == 1) ? 10 : 18;
        }
        break;
    }

    return info;
}

std::vector<std::string> Localization::getJournalLines() const {
    if (m_lang == Language::Arabic) {
        return {
            "\"سجل الحارس - التدوينة الأولى: انطفاء الشمس\"",
            "غابت الشعلة السماوية وراء الأفق، ولم يبق سوى نور المنارة لحمايتنا.",
            "تخرج كائنات الظلال من لجج البحر كلما أرخى الليل سدوله.",
            "",
            "\"التدوينة الثانية: حكمة الانعكاس والعدسات\"",
            "شعاع المنارة وحده لا يستطيع حراسة جميع الشواطئ في آن واحد.",
            "وزع المرايا النحاسية في سفوح الجزيرة لعكس النور وإبادة جحافل الظلام.",
            "",
            "\"التدوينة الثالثة: المذابح الأثرية الثلاثة\"",
            "- معبد الغرب: مذبح الشمس (يمنح سرعة فائقة للحارس ويزيد قوة النار).",
            "- أبراج الشمال: مذبح البلورات (يغذي خزان المنارة بالوقود تلقائياً).",
            "- جزيرة الشرق: مذبح البحر (يُحطّم دروع وحش الأعماق العظيم).",
            "",
            "\"الهدف الأسمى: أشعل المذابح الثلاثة بالأنوية الأثرية لتبديد الظلام واستدعاء الفجر الأول!\"",
            "",
            "اضغط [ESC] أو [TAB] أو [ENTER] للعودة إلى المراقبة."
        };
    } else {
        return {
            "\"Keeper's Log - Entry 1: The Dying Sun\"",
            "The celestial flame vanished beyond the horizon. Only the Beacon protects us.",
            "Shadowy abominations emerge from the abyss whenever night overtakes the sea.",
            "",
            "\"Entry 2: The Art of Reflection\"",
            "The lighthouse beam alone cannot cover all shores at once.",
            "Position brass mirrors across the island cliffs to redirect the light.",
            "",
            "\"Entry 3: The Three Ancient Altars\"",
            "- West Shrine: The Solar Altar (Grants player speed and lamp intensity).",
            "- North Spires: The Crystal Altar (Recharges the Beacon fuel automatically).",
            "- East Atoll: The Sea Beacon (Shatters the armor of the Abyssal Leviathan).",
            "",
            "\"Final Mission: Rekindle all 3 Altars with Relic Cores to awaken the First Dawn!\"",
            "",
            "Press [ESC], [TAB], or [ENTER] to return to the watch."
        };
    }
}

void Localization::initStrings() {
    // English Strings
    m_stringsEN["TITLE"] = "The Last Beacon Keeper";
    m_stringsEN["KEEPER_HP"] = "Keeper HP:";
    m_stringsEN["BEACON_HP"] = "Beacon HP:";
    m_stringsEN["FUEL"] = "Fuel:";
    m_stringsEN["ALTARS"] = "Altars:";
    m_stringsEN["MSG_WELCOME"] = "Welcome Keeper! Gather resources by day, survive the Shadow Tide by night.";
    m_stringsEN["MSG_DUSK"] = "DUSK FALLS! Prepare defenses, reposition mirrors, and refuel before nightfall.";
    m_stringsEN["MSG_NIGHT"] = "NIGHTFALL! The horde arrives! Protect the Lighthouse!";
    m_stringsEN["MSG_DAWN"] = "DAWN HAS BROKEN! The shadows dissolve into ash.";
    m_stringsEN["MSG_BOSS"] = "THE ABYSSAL LEVIATHAN EMERGES FROM THE TIDE!";
    m_stringsEN["MSG_NEW_DAY"] = "Day %d: Gather resources and prepare defenses.";
    m_stringsEN["MSG_NO_MIRRORS"] = "No mirrors in inventory! Craft one at the Lighthouse Workshop.";
    m_stringsEN["MSG_WATER_PLACE"] = "Cannot place mirror in deep water!";
    m_stringsEN["MSG_MIRROR_PLACED"] = "Mirror placed! Press [R / Right Click] to rotate.";
    m_stringsEN["MSG_MIRROR_RETRIEVED"] = "Retrieved mirror back to inventory.";
    m_stringsEN["MSG_ALTAR_IGNITED"] = "ANCIENT ALTAR REKINDLED! The island's light strengthens!";
    m_stringsEN["MSG_ALTAR_NEED_CORE"] = "Requires 1 Relic Core to rekindle! Craft it at the Lighthouse.";
    m_stringsEN["MSG_ALTAR_ALREADY"] = "This Altar is already shining brightly.";
    m_stringsEN["MSG_SALVE_USED"] = "Used Healing Salve (+50 HP)!";
    m_stringsEN["MSG_LENS_FOCUSED"] = "Equipped: Concentrated Solar Lens";
    m_stringsEN["MSG_LENS_AMBER"] = "Equipped: Amber Wide-Defense Lens";
    m_stringsEN["MSG_LENS_AMBER_LOCKED"] = "Amber Lens locked! Unlock in Lighthouse Workshop.";
    m_stringsEN["MSG_LENS_UV"] = "Equipped: UV Shockwave Lens";
    m_stringsEN["MSG_LENS_UV_LOCKED"] = "UV Lens locked! Unlock in Lighthouse Workshop.";
    m_stringsEN["MSG_MANNED"] = "MANNING TOWER CONSOLE! Aim with mouse.";
    m_stringsEN["MSG_DISMOUNTED"] = "Dismounted tower.";
    m_stringsEN["MSG_OVERHEATED"] = "BEACON OVERHEATED! Cooling down...";
    m_stringsEN["ALERT_HP_CRITICAL"] = "CRITICAL: Lighthouse Integrity Failing (< 25%)!";
    m_stringsEN["ALERT_HP_LOW"] = "WARNING: Lighthouse taking heavy damage (< 50%)!";
    m_stringsEN["ALERT_FUEL_CRITICAL"] = "CRITICAL: Lighthouse Fuel Exhaustion Imminent (< 15%)!";
    m_stringsEN["ALERT_FUEL_LOW"] = "WARNING: Low Lighthouse Fuel (< 30%)!";
    m_stringsEN["SETTINGS_TITLE"] = "GAME SETTINGS & OPTIONS";
    m_stringsEN["SETTINGS_LANGUAGE"] = "Language:";
    m_stringsEN["SETTINGS_FULLSCREEN"] = "Display Mode:";
    m_stringsEN["SETTINGS_FS_ON"] = "Fullscreen";
    m_stringsEN["SETTINGS_FS_OFF"] = "Windowed";
    m_stringsEN["SETTINGS_VOLUME"] = "Master Volume:";
    m_stringsEN["SETTINGS_BACK"] = "Return to Game";
    m_stringsEN["GAMEOVER_TITLE"] = "THE BEACON HAS BEEN EXTINGUISHED";
    m_stringsEN["GAMEOVER_SUB"] = "Darkness has consumed the archipelago...";
    m_stringsEN["GAMEOVER_RESTART"] = "Press [ENTER] or [R] to Try Again";
    m_stringsEN["VICTORY_TITLE"] = "THE FIRST DAWN BREAKS!";
    m_stringsEN["VICTORY_SUB1"] = "All 3 Ancient Altars have united their sacred beams with the Beacon.";
    m_stringsEN["VICTORY_SUB2"] = "The eternal night has been vanquished. You saved the archipelago!";
    m_stringsEN["VICTORY_RESTART"] = "Press [ENTER] or [R] to Play Again";
    m_stringsEN["PAUSE_TITLE"] = "GAME PAUSED";
    m_stringsEN["PAUSE_RESUME"] = "Press [ESC] to Resume, [O] for Settings";

    // Arabic Strings
    m_stringsAR["TITLE"] = "حارس المنارة الأخير";
    m_stringsAR["KEEPER_HP"] = "صحة الحارس:";
    m_stringsAR["BEACON_HP"] = "صحة المنارة:";
    m_stringsAR["FUEL"] = "الوقود:";
    m_stringsAR["ALTARS"] = "المذابح المشعلة:";
    m_stringsAR["MSG_WELCOME"] = "مرحباً بك أيها الحارس! اجمع الموارد نهاراً ودافع ضد المد الظلي ليلاً.";
    m_stringsAR["MSG_DUSK"] = "حلول الغسق! استعد للظلام: حصّن دفاعاتك، وزّع المرايا، وزوّد الوقود.";
    m_stringsAR["MSG_NIGHT"] = "حل الظلام الدامس! جحافل الظل تهاجم، اصمد ودافع عن شعلة المنارة!";
    m_stringsAR["MSG_DAWN"] = "انبلج الفجر! تبددت كائنات الظل إلى رماد.";
    m_stringsAR["MSG_BOSS"] = "وحش الأعماق العظيم يخرج من المياه المظلمة!";
    m_stringsAR["MSG_NEW_DAY"] = "اليوم %d: اجمع الموارد وحصّن دفاعاتك.";
    m_stringsAR["MSG_NO_MIRRORS"] = "لا توجد مرايا في حقيبتك! اصنع واحدة في ورشة المنارة.";
    m_stringsAR["MSG_WATER_PLACE"] = "لا يمكن تثبيت المرآة في المياه العميقة!";
    m_stringsAR["MSG_MIRROR_PLACED"] = "تم تثبيت المرآة! اضغط [R أو نقر أيمن] لتدويرها.";
    m_stringsAR["MSG_MIRROR_RETRIEVED"] = "تمت استعادة المرآة إلى حقيبتك.";
    m_stringsAR["MSG_ALTAR_IGNITED"] = "تم إشعال المذبح الأثري! تعزز نور الأرخبيل!";
    m_stringsAR["MSG_ALTAR_NEED_CORE"] = "يتطلب نواة أثرية لإشعاله! اصنعها في ورشة المنارة.";
    m_stringsAR["MSG_ALTAR_ALREADY"] = "هذا المذبح مشتعل بالفعل وينير السماء.";
    m_stringsAR["MSG_SALVE_USED"] = "تم استخدام مرهم الشفاء (+50 نقطة صحة)!";
    m_stringsAR["MSG_LENS_FOCUSED"] = "العدسة المفعلة: الشعاع الشمسي المركز الفتاك";
    m_stringsAR["MSG_LENS_AMBER"] = "العدسة المفعلة: عدسة العنبر الواقية العريضة";
    m_stringsAR["MSG_LENS_AMBER_LOCKED"] = "عدسة العنبر مقفلة! افتحها في ورشة المنارة.";
    m_stringsAR["MSG_LENS_UV"] = "العدسة المفعلة: عدسة الصدمة فوق البنفسجية UV";
    m_stringsAR["MSG_LENS_UV_LOCKED"] = "عدسة UV مقفلة! افتحها في ورشة المنارة.";
    m_stringsAR["MSG_MANNED"] = "أنت الآن تتحكم ببرج المنارة! صوّب بالفأرة.";
    m_stringsAR["MSG_DISMOUNTED"] = "نزلت من البرج إلى الأرض.";
    m_stringsAR["MSG_OVERHEATED"] = "حرارة مفرطة في المنارة! انتظر حتى تبرد...";
    m_stringsAR["ALERT_HP_CRITICAL"] = "تحذير حرج: سلامة هيكل المنارة في خطر شديد (< 25%)!";
    m_stringsAR["ALERT_HP_LOW"] = "تحذير: المنارة تتعرض لأضرار بالغة (< 50%)!";
    m_stringsAR["ALERT_FUEL_CRITICAL"] = "تحذير حرج: وقود المنارة قارب على النفاد التام (< 15%)!";
    m_stringsAR["ALERT_FUEL_LOW"] = "تحذير: منسوب وقود المنارة منخفض (< 30%)!";
    m_stringsAR["SETTINGS_TITLE"] = "إعدادات اللعبة والخيارات";
    m_stringsAR["SETTINGS_LANGUAGE"] = "لغة اللعبة:";
    m_stringsAR["SETTINGS_FULLSCREEN"] = "نمط العرض:";
    m_stringsAR["SETTINGS_FS_ON"] = "ملء الشاشة";
    m_stringsAR["SETTINGS_FS_OFF"] = "نافذة";
    m_stringsAR["SETTINGS_VOLUME"] = "مستوى الصوت:";
    m_stringsAR["SETTINGS_BACK"] = "العودة إلى اللعبة";
    m_stringsAR["GAMEOVER_TITLE"] = "انطفأت شعلة المنارة وساد الظلام";
    m_stringsAR["GAMEOVER_SUB"] = "ابتلعت كائنات الظل الأرخبيل للأبد...";
    m_stringsAR["GAMEOVER_RESTART"] = "اضغط [ENTER] أو [R] للمحاولة مجدداً";
    m_stringsAR["VICTORY_TITLE"] = "انبلج الفجر الأول وزال الظلام!";
    m_stringsAR["VICTORY_SUB1"] = "اتحدت أنوار المذابح الثلاثة مع المنارة في سماء الأرخبيل.";
    m_stringsAR["VICTORY_SUB2"] = "لقد طردت الظلام الأبدي وأنقذت العالم. مبارك انتصارك أيها الحارس!";
    m_stringsAR["VICTORY_RESTART"] = "اضغط [ENTER] أو [R] للعب مرة أخرى";
    m_stringsAR["PAUSE_TITLE"] = "اللعبة متوقفة مؤقتاً";
    m_stringsAR["PAUSE_RESUME"] = "اضغط [ESC] للاستئناف، أو [O] للإعدادات";
}
