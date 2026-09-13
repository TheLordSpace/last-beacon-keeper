#include "Localization.h"
#include <fribidi/fribidi.h>
#include <sstream>
#include <iostream>

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

std::string Localization::getClockText(int day, DayPhase phase, int secondsLeft) const {
    if (m_lang == Language::Arabic) {
        std::string pStr;
        switch (phase) {
        case DayPhase::Day: pStr = "النهار"; break;
        case DayPhase::Dusk: pStr = "الغسق (إنذار)"; break;
        case DayPhase::Night: pStr = "المد الظلي"; break;
        case DayPhase::Dawn: pStr = "بزوغ الفجر"; break;
        }
        std::ostringstream ss;
        ss << "اليوم " << day << " - " << pStr << " (" << secondsLeft << " ث)";
        return ss.str();
    } else {
        std::string pStr;
        switch (phase) {
        case DayPhase::Day: pStr = "DAY"; break;
        case DayPhase::Dusk: pStr = "DUSK (WARNING)"; break;
        case DayPhase::Night: pStr = "SHADOW TIDE"; break;
        case DayPhase::Dawn: pStr = "DAWN BREAK"; break;
        }
        std::ostringstream ss;
        ss << "Day " << day << " - " << pStr << " (" << secondsLeft << "s)";
        return ss.str();
    }
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
    return (m_lang == Language::Arabic) ? "ورشة المنارة والترقيات" : "LIGHTHOUSE WORKBENCH & CRAFTING";
}

std::string Localization::getWorkshopSubtitle() const {
    if (m_lang == Language::Arabic) {
        return "استخدم [W/S] للاختيار، [ENTER/Space] للصنع، [ESC/E] للعودة";
    } else {
        return "Use [W/S] to Select, [ENTER/SPACE] to Craft, [ESC/E] to Exit";
    }
}

std::vector<std::string> Localization::getWorkshopItems() const {
    if (m_lang == Language::Arabic) {
        return {
            "1. صناعة مرآة نحاسية عاكسة (التكلفة: 10 خشب، 5 بلورات)",
            "2. تزويد خزان المنارة +40 وقود (التكلفة: 10 وقود)",
            "3. إصلاح هيكل المنارة +120 نقطة حياة (التكلفة: 15 خشب، 10 بلورات)",
            "4. صناعة نواة أثرية لإشعال المذابح (التكلفة: 15 خشب، 20 بلورات)",
            "5. فتح عدسة العنبر الواقية العريضة (التكلفة: 15 بلورات)",
            "6. فتح عدسة الصدمة فوق البنفسجية UV (التكلفة: 25 بلورات)",
            "7. صناعة مرهم شفاء [H] (التكلفة: 5 خشب، 5 بلورات)"
        };
    } else {
        return {
            "1. Craft Brass Mirror (Cost: 10 Wood, 5 Crystals)",
            "2. Refuel Beacon Tank +40 (Cost: 10 Oil)",
            "3. Repair Lighthouse Structure +120 HP (Cost: 15 Wood, 10 Crystals)",
            "4. Craft Ancient Relic Core for Altars (Cost: 15 Wood, 20 Crystals)",
            "5. Unlock Amber Wide-Defense Lens (Cost: 15 Crystals)",
            "6. Unlock UV Pulse Shockwave Lens (Cost: 25 Crystals)",
            "7. Craft Healing Salve [H] (Cost: 5 Wood, 5 Crystals)"
        };
    }
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
    m_stringsEN["MSG_DUSK"] = "DUSK FALLS! The Shadow Tide approaches in moments...";
    m_stringsEN["MSG_NIGHT"] = "NIGHTFALL! Survive the horde and protect the Lighthouse!";
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
    m_stringsAR["MSG_DUSK"] = "حلول الغسق! كائنات الظل تقترب من الشواطئ...";
    m_stringsAR["MSG_NIGHT"] = "حل الظلام الدامس! اصمد ودافع عن شعلة المنارة!";
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
