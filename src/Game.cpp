#include "Game.h"
#include "Audio.h"
#include "Particles.h"
#include <iostream>
#include <cmath>
#include <algorithm>

Game::Game() = default;

Game::~Game() {
    cleanup();
}

bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_window = SDL_CreateWindow(
        "The Last Beacon Keeper - حارس المنارة الأخير",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!m_window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(
        m_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
    );
    if (!m_renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set fixed virtual logical resolution (1280x720) for perfect fullscreen scaling
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_RenderSetLogicalSize(m_renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize UI Manager & Fonts
    if (!m_ui.init(m_renderer)) {
        std::cerr << "UIManager init failed!" << std::endl;
    }

    // Create lighting render target texture
    m_lightTexture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        WINDOW_WIDTH, WINDOW_HEIGHT
    );
    if (m_lightTexture) {
        SDL_SetTextureBlendMode(m_lightTexture, SDL_BLENDMODE_MOD);
    }

    // Create ultra-smooth radial light texture (zero scanlines / zero dark stripes)
    m_radialLightTexture = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STATIC,
        256, 256
    );
    if (m_radialLightTexture) {
        std::vector<Uint32> pixels(256 * 256);
        float center = 127.5f;
        for (int y = 0; y < 256; ++y) {
            for (int x = 0; x < 256; ++x) {
                float dx = (x - center) / center;
                float dy = (y - center) / center;
                float dist = std::sqrt(dx * dx + dy * dy);
                Uint8 alpha = 0;
                if (dist < 1.0f) {
                    float falloff = 1.0f - dist;
                    float smooth = falloff * falloff * (3.0f - 2.0f * falloff);
                    alpha = static_cast<Uint8>(smooth * 255.0f);
                }
                Uint8 r = 255, g = 250, b = 220;
                pixels[y * 256 + x] = (r << 24) | (g << 16) | (b << 8) | alpha;
            }
        }
        SDL_UpdateTexture(m_radialLightTexture, nullptr, pixels.data(), 256 * sizeof(Uint32));
        SDL_SetTextureBlendMode(m_radialLightTexture, SDL_BLENDMODE_ADD);
    }

    // Initialize procedural audio
    AudioManager::instance().init();

    // Initial placed mirrors for player demonstration
    PlacedMirror m1;
    m1.id = 1;
    m1.pos = Vec2(1100.0f, 1020.0f);
    m1.angle = -PI * 0.25f;
    m_mirrors.push_back(m1);

    PlacedMirror m2;
    m2.id = 2;
    m2.pos = Vec2(1500.0f, 1020.0f);
    m2.angle = PI * 0.25f;
    m_mirrors.push_back(m2);

    setStatus(Localization::instance().get("MSG_WELCOME"), 6.0f);

    return true;
}

void Game::cleanup() {
    m_ui.cleanup();

    if (m_lightTexture) SDL_DestroyTexture(m_lightTexture);
    if (m_radialLightTexture) SDL_DestroyTexture(m_radialLightTexture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);

    AudioManager::instance().cleanup();
    SDL_Quit();
}

void Game::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;
    if (m_fullscreen) {
        SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(m_window, 0);
        SDL_SetWindowSize(m_window, WINDOW_WIDTH, WINDOW_HEIGHT);
        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    setStatus(m_fullscreen ? (Localization::instance().isArabic() ? "تم تفعيل وضع ملء الشاشة" : "Fullscreen enabled") :
                             (Localization::instance().isArabic() ? "تم تفعيل وضع النافذة" : "Windowed mode enabled"), 2.0f);
}

void Game::placeMirror() {
    if (m_player.mirrorsInBag <= 0) {
        setStatus(Localization::instance().get("MSG_NO_MIRRORS"), 2.5f);
        return;
    }

    Vec2 placePos = m_player.getPos() + Vec2::fromAngle(m_player.getAimAngle(), 45.0f);
    if (!m_map.isWalkable(placePos)) {
        setStatus(Localization::instance().get("MSG_WATER_PLACE"), 2.0f);
        return;
    }

    PlacedMirror m;
    m.id = static_cast<int>(m_mirrors.size()) + 1;
    m.pos = placePos;
    m.angle = m_player.getAimAngle() + PI * 0.5f;
    m.maxHealth = m_lighthouse.getMirrorMaxHealth();
    m.health = m.maxHealth;
    m_mirrors.push_back(m);

    m_player.mirrorsInBag--;
    AudioManager::instance().playSound(SoundID::Craft, 0.6f);
    setStatus(Localization::instance().get("MSG_MIRROR_PLACED"), 3.0f);
}

void Game::rotateNearbyMirror() {
    Vec2 ppos = m_player.getPos();
    PlacedMirror* closest = nullptr;
    float bestDistSq = 75.0f * 75.0f;

    for (auto& m : m_mirrors) {
        float dsq = (m.pos - ppos).lengthSq();
        if (dsq < bestDistSq) {
            bestDistSq = dsq;
            closest = &m;
        }
    }

    if (closest) {
        closest->angle += PI * 0.25f;
        if (closest->angle >= 6.28318f) closest->angle -= 6.28318f;
        closest->rotateFeedbackTimer = 0.25f;

        AudioManager::instance().playSound(SoundID::MirrorReflect, 0.5f);

        // Circular sparkle ring around the rotated mirror
        for (int i = 0; i < 10; ++i) {
            float a = (float)i * (2.0f * PI / 10.0f);
            Vec2 dir = Vec2::fromAngle(a);
            Vec2 pos = closest->pos + dir * 14.0f;
            Vec2 vel = dir * 40.0f;
            ColorRGBA col{ 180, 235, 255, 240 };
            ParticleSystem::instance().spawn(pos, vel, col, 0.25f, 2.5f, true);
        }

        // Endpoint sparks indicating newly aligned reflective surface
        Vec2 p1, p2;
        closest->getEndpoints(p1, p2);
        ParticleSystem::instance().spawnSparks(p1, 3, ColorRGBA{ 210, 245, 255, 255 });
        ParticleSystem::instance().spawnSparks(p2, 3, ColorRGBA{ 210, 245, 255, 255 });
    }
}

void Game::interactNearby() {
    Vec2 ppos = m_player.getPos();

    // 1. Lighthouse Workshop
    if ((m_lighthouse.getPos() - ppos).length() < 95.0f) {
        m_state = GameState::Workshop;
        return;
    }

    // 2. Ancient Altar
    AncientAltar* altar = m_map.getNearbyAltar(ppos, 90.0f);
    if (altar) {
        if (!altar->ignited) {
            if (m_player.relicCores >= 1) {
                m_player.relicCores--;
                altar->ignited = true;
                AudioManager::instance().playSound(SoundID::AltarIgnite, 1.0f);
                ParticleSystem::instance().spawnSparks(altar->pos, 30, altar->color);
                setStatus(Localization::instance().get("MSG_ALTAR_IGNITED"), 5.0f);
            } else {
                setStatus(Localization::instance().get("MSG_ALTAR_NEED_CORE"), 3.0f);
            }
        } else {
            setStatus(Localization::instance().get("MSG_ALTAR_ALREADY"), 2.0f);
        }
        return;
    }

    // 3. Pick up placed mirror
    for (auto it = m_mirrors.begin(); it != m_mirrors.end(); ++it) {
        if ((it->pos - ppos).length() < 40.0f) {
            m_mirrors.erase(it);
            m_player.mirrorsInBag++;
            AudioManager::instance().playSound(SoundID::Craft, 0.5f);
            setStatus(Localization::instance().get("MSG_MIRROR_RETRIEVED"), 2.0f);
            return;
        }
    }
}

void Game::buyWorkshopItem(int index) {
    bool isAr = Localization::instance().isArabic();

    switch (index) {
    case 0: // Craft Mirror (10 Wood, 5 Crystals)
        if (m_player.wood >= 10 && m_player.crystals >= 5) {
            m_player.wood -= 10;
            m_player.crystals -= 5;
            m_player.mirrorsInBag++;
            AudioManager::instance().playSound(SoundID::Craft, 0.8f);
            setStatus(isAr ? "تمت صناعة مرآة نحاسية عاكسة!" : "Crafted 1 Reflective Mirror!", 2.5f);
        } else {
            setStatus(isAr ? "الموارد غير كافية! (يلزم 10 خشب، 5 بلورات)" : "Not enough materials! (Needs 10 Wood, 5 Crystals)", 2.5f);
        }
        break;

    case 1: // Refuel Lighthouse (10 Oil)
        if (m_player.oil >= 10) {
            m_player.oil -= 10;
            m_lighthouse.addFuel(40.0f);
            AudioManager::instance().playSound(SoundID::HarvestOil, 0.8f);
            setStatus(isAr ? "تم تزويد المنارة +40 وقود!" : "Lighthouse refueled +40 Fuel!", 2.5f);
        } else {
            setStatus(isAr ? "الوقود غير كافٍ! (يلزم 10 وقود)" : "Not enough Oil! (Needs 10 Oil)", 2.5f);
        }
        break;

    case 2: // Repair Lighthouse (15 Wood, 10 Crystals)
        if (m_player.wood >= 15 && m_player.crystals >= 10) {
            m_player.wood -= 15;
            m_player.crystals -= 10;
            m_lighthouse.repair(120.0f);
            AudioManager::instance().playSound(SoundID::Craft, 0.8f);
            setStatus(isAr ? "تم إصلاح هيكل المنارة +120 نقطة حياة!" : "Lighthouse hull repaired +120 HP!", 2.5f);
        } else {
            setStatus(isAr ? "الموارد غير كافية! (يلزم 15 خشب، 10 بلورات)" : "Not enough materials! (Needs 15 Wood, 10 Crystals)", 2.5f);
        }
        break;

    case 3: // Craft Relic Core (20 Crystals, 15 Wood)
        if (m_player.crystals >= 20 && m_player.wood >= 15) {
            m_player.crystals -= 20;
            m_player.wood -= 15;
            m_player.relicCores++;
            AudioManager::instance().playSound(SoundID::AltarIgnite, 0.8f);
            setStatus(isAr ? "تمت صناعة نواة أثرية! خذها إلى أحد المذابح الأثرية." : "Crafted Relic Core! Take it to an Ancient Altar.", 4.0f);
        } else {
            setStatus(isAr ? "يلزم 20 بلورة و 15 خشباً لصنع النواة الأثرية!" : "Needs 20 Crystals, 15 Wood for a Relic Core!", 2.5f);
        }
        break;

    case 4: // Unlock Amber Wide Lens (15 Crystals)
        if (!m_lighthouse.unlockWideLens) {
            if (m_player.crystals >= 15) {
                m_player.crystals -= 15;
                m_lighthouse.unlockWideLens = true;
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? "تم فتح عدسة العنبر الواقية! اضغط [2] للتفعيل." : "Unlocked Amber Wide Lens! Press [2] to equip.", 3.5f);
            } else {
                setStatus(isAr ? "يلزم 15 بلورة لفتح عدسة العنبر!" : "Needs 15 Crystals to unlock Amber Lens!", 2.5f);
            }
        }
        break;

    case 5: // Unlock UV Pulse Lens (25 Crystals)
        if (!m_lighthouse.unlockUVLens) {
            if (m_player.crystals >= 25) {
                m_player.crystals -= 25;
                m_lighthouse.unlockUVLens = true;
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? "تم فتح عدسة الصدمة فوق البنفسجية! اضغط [3] للتفعيل." : "Unlocked UV Pulse Lens! Press [3] to equip.", 3.5f);
            } else {
                setStatus(isAr ? "يلزم 25 بلورة لفتح عدسة UV!" : "Needs 25 Crystals to unlock UV Lens!", 2.5f);
            }
        }
        break;

    case 6: // Craft Healing Salve (5 Wood, 5 Crystals)
        if (m_player.wood >= 5 && m_player.crystals >= 5) {
            m_player.wood -= 5;
            m_player.crystals -= 5;
            m_player.salves++;
            AudioManager::instance().playSound(SoundID::Craft, 0.7f);
            setStatus(isAr ? "تمت صناعة مرهم شفاء! اضغط [H] للاستخدام." : "Crafted Healing Salve! Press [H] to use.", 2.5f);
        } else {
            setStatus(isAr ? "يلزم 5 خشب و 5 بلورات!" : "Needs 5 Wood, 5 Crystals!", 2.5f);
        }
        break;

    case 7: // Beam Power Upgrade (Max 3)
        if (m_lighthouse.beamPowerLevel >= 3) {
            setStatus(isAr ? "تم بلوغ المستوى الأقصى لقوة الشعاع!" : "Beam Power is already at MAX level!", 2.5f);
        } else {
            int needCry = (m_lighthouse.beamPowerLevel == 1) ? 20 : 30;
            int needWood = (m_lighthouse.beamPowerLevel == 1) ? 10 : 15;
            if (m_player.crystals >= needCry && m_player.wood >= needWood) {
                m_player.crystals -= needCry;
                m_player.wood -= needWood;
                m_lighthouse.beamPowerLevel++;
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? ("تمت ترقية قوة الشعاع إلى المستوى " + std::to_string(m_lighthouse.beamPowerLevel) + "! (+25% ضرر)") :
                                 ("Upgraded Beam Power to Level " + std::to_string(m_lighthouse.beamPowerLevel) + "! (+25% Damage)"), 3.5f);
            } else {
                setStatus(isAr ? ("الموارد غير كافية! يلزم " + std::to_string(needCry) + " بلورات و " + std::to_string(needWood) + " خشب") :
                                 ("Not enough materials! Needs " + std::to_string(needCry) + " Crystals, " + std::to_string(needWood) + " Wood"), 2.5f);
            }
        }
        break;

    case 8: // Beam Efficiency Upgrade (Max 3)
        if (m_lighthouse.beamEfficiencyLevel >= 3) {
            setStatus(isAr ? "تم بلوغ المستوى الأقصى لكفاءة الوقود!" : "Beam Efficiency is already at MAX level!", 2.5f);
        } else {
            int needCry = (m_lighthouse.beamEfficiencyLevel == 1) ? 15 : 25;
            int needOil = (m_lighthouse.beamEfficiencyLevel == 1) ? 10 : 18;
            if (m_player.crystals >= needCry && m_player.oil >= needOil) {
                m_player.crystals -= needCry;
                m_player.oil -= needOil;
                m_lighthouse.beamEfficiencyLevel++;
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? ("تمت ترقية كفاءة الوقود إلى المستوى " + std::to_string(m_lighthouse.beamEfficiencyLevel) + "! (-25% استهلاك)") :
                                 ("Upgraded Beam Efficiency to Level " + std::to_string(m_lighthouse.beamEfficiencyLevel) + "! (-25% Fuel Drain)"), 3.5f);
            } else {
                setStatus(isAr ? ("الموارد غير كافية! يلزم " + std::to_string(needCry) + " بلورات و " + std::to_string(needOil) + " وقود") :
                                 ("Not enough materials! Needs " + std::to_string(needCry) + " Crystals, " + std::to_string(needOil) + " Oil"), 2.5f);
            }
        }
        break;

    case 9: // Mirror Durability Upgrade (Max 3)
        if (m_lighthouse.mirrorDurabilityLevel >= 3) {
            setStatus(isAr ? "تم بلوغ المستوى الأقصى لصلابة المرايا!" : "Mirror Durability is already at MAX level!", 2.5f);
        } else {
            int needWood = (m_lighthouse.mirrorDurabilityLevel == 1) ? 15 : 25;
            int needCry = (m_lighthouse.mirrorDurabilityLevel == 1) ? 10 : 15;
            if (m_player.wood >= needWood && m_player.crystals >= needCry) {
                m_player.wood -= needWood;
                m_player.crystals -= needCry;
                m_lighthouse.mirrorDurabilityLevel++;
                float newMax = m_lighthouse.getMirrorMaxHealth();
                for (auto& mir : m_mirrors) {
                    float diff = newMax - mir.maxHealth;
                    mir.maxHealth = newMax;
                    mir.health += diff;
                }
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? ("تمت ترقية متانة المرايا إلى المستوى " + std::to_string(m_lighthouse.mirrorDurabilityLevel) + "! (صحة المرآة: " + std::to_string((int)newMax) + ")") :
                                 ("Upgraded Mirror Durability to Level " + std::to_string(m_lighthouse.mirrorDurabilityLevel) + "! (Mirror HP: " + std::to_string((int)newMax) + ")"), 3.5f);
            } else {
                setStatus(isAr ? ("الموارد غير كافية! يلزم " + std::to_string(needWood) + " خشب و " + std::to_string(needCry) + " بلورات") :
                                 ("Not enough materials! Needs " + std::to_string(needWood) + " Wood, " + std::to_string(needCry) + " Crystals"), 2.5f);
            }
        }
        break;

    case 10: // Lighthouse Armor Upgrade (Max 3)
        if (m_lighthouse.lighthouseArmorLevel >= 3) {
            setStatus(isAr ? "تم بلوغ المستوى الأقصى لدروع المنارة!" : "Lighthouse Armor is already at MAX level!", 2.5f);
        } else {
            int needWood = (m_lighthouse.lighthouseArmorLevel == 1) ? 25 : 40;
            int needCry = (m_lighthouse.lighthouseArmorLevel == 1) ? 20 : 35;
            if (m_player.wood >= needWood && m_player.crystals >= needCry) {
                m_player.wood -= needWood;
                m_player.crystals -= needCry;
                m_lighthouse.upgradeLighthouseArmor();
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? ("تمت ترقية دروع المنارة إلى المستوى " + std::to_string(m_lighthouse.lighthouseArmorLevel) + "! (أقصى صحة: " + std::to_string((int)m_lighthouse.getMaxHealth()) + ")") :
                                 ("Upgraded Lighthouse Armor to Level " + std::to_string(m_lighthouse.lighthouseArmorLevel) + "! (Max HP: " + std::to_string((int)m_lighthouse.getMaxHealth()) + ")"), 3.5f);
            } else {
                setStatus(isAr ? ("الموارد غير كافية! يلزم " + std::to_string(needWood) + " خشب و " + std::to_string(needCry) + " بلورات") :
                                 ("Not enough materials! Needs " + std::to_string(needWood) + " Wood, " + std::to_string(needCry) + " Crystals"), 2.5f);
            }
        }
        break;

    case 11: // Swift Boots Upgrade (Max 3)
        if (m_lighthouse.swiftBootsLevel >= 3) {
            setStatus(isAr ? "تم بلوغ المستوى الأقصى لحذاء الحارس!" : "Swift Boots are already at MAX level!", 2.5f);
        } else {
            int needWood = (m_lighthouse.swiftBootsLevel == 1) ? 15 : 25;
            int needOil = (m_lighthouse.swiftBootsLevel == 1) ? 10 : 18;
            if (m_player.wood >= needWood && m_player.oil >= needOil) {
                m_player.wood -= needWood;
                m_player.oil -= needOil;
                m_lighthouse.swiftBootsLevel++;
                m_player.setBonuses(m_lighthouse.getPlayerSpeedBonus(), m_lighthouse.getPlayerDashCooldownBonus());
                AudioManager::instance().playSound(SoundID::Craft, 0.9f);
                setStatus(isAr ? ("تمت ترقية حذاء الحارس إلى المستوى " + std::to_string(m_lighthouse.swiftBootsLevel) + "! (+سرعة وتفادٍ أسرع)") :
                                 ("Upgraded Swift Boots to Level " + std::to_string(m_lighthouse.swiftBootsLevel) + "! (+Speed & Faster Dash)"), 3.5f);
            } else {
                setStatus(isAr ? ("الموارد غير كافية! يلزم " + std::to_string(needWood) + " خشب و " + std::to_string(needOil) + " وقود") :
                                 ("Not enough materials! Needs " + std::to_string(needWood) + " Wood, " + std::to_string(needOil) + " Oil"), 2.5f);
            }
        }
        break;
    }
}

void Game::toggleLanguage() {
    Localization::instance().toggleLanguage();
    setStatus(Localization::instance().isArabic() ? "تم تغيير اللغة إلى العربية" : "Language switched to English", 2.0f);
}

void Game::takeScreenshot() {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(m_renderer, &outW, &outH);
    SDL_Surface* sshot = SDL_CreateRGBSurfaceWithFormat(0, outW, outH, 32, SDL_PIXELFORMAT_RGBA8888);
    if (sshot) {
        SDL_Rect entireWindow{ 0, 0, outW, outH };
        SDL_RenderReadPixels(m_renderer, &entireWindow, SDL_PIXELFORMAT_RGBA8888, sshot->pixels, sshot->pitch);
        SDL_SaveBMP(sshot, "screenshot.bmp");
        SDL_FreeSurface(sshot);
        setStatus(Localization::instance().isArabic() ? "تم حفظ لقطة الشاشة في screenshot.bmp!" : "Screenshot saved to screenshot.bmp!", 2.5f);
    }
}

void Game::playerDash() {
    m_player.tryDash();
}

void Game::useHealingSalve() {
    if (m_player.salves > 0 && m_player.getHealth() < m_player.getMaxHealth()) {
        m_player.salves--;
        m_player.heal(50.0f);
        AudioManager::instance().playSound(SoundID::DawnChime, 0.5f);
        setStatus(Localization::instance().get("MSG_SALVE_USED"), 2.0f);
    }
}

void Game::selectLens(LensType type) {
    if (type == LensType::Focused) {
        m_lighthouse.setLens(LensType::Focused);
        setStatus(Localization::instance().get("MSG_LENS_FOCUSED"), 2.0f);
    } else if (type == LensType::WideAmber) {
        if (m_lighthouse.unlockWideLens) {
            m_lighthouse.setLens(LensType::WideAmber);
            setStatus(Localization::instance().get("MSG_LENS_AMBER"), 2.0f);
        } else {
            setStatus(Localization::instance().get("MSG_LENS_AMBER_LOCKED"), 2.0f);
        }
    } else if (type == LensType::UVPulse) {
        if (m_lighthouse.unlockUVLens) {
            m_lighthouse.setLens(LensType::UVPulse);
            setStatus(Localization::instance().get("MSG_LENS_UV"), 2.0f);
        } else {
            setStatus(Localization::instance().get("MSG_LENS_UV_LOCKED"), 2.0f);
        }
    }
}

void Game::toggleMannedLighthouse() {
    if ((m_lighthouse.getPos() - m_player.getPos()).length() < 90.0f) {
        m_lighthouse.setManned(!m_lighthouse.isManned());
        setStatus(m_lighthouse.isManned() ? Localization::instance().get("MSG_MANNED") : Localization::instance().get("MSG_DISMOUNTED"), 2.5f);
    }
}

void Game::playerAttack() {
    m_player.swingTool(m_map, m_enemies);
}

void Game::settingsNavigateUp() {
    m_settingsSelected = (m_settingsSelected - 1 + 4) % 4;
}

void Game::settingsNavigateDown() {
    m_settingsSelected = (m_settingsSelected + 1) % 4;
}

void Game::settingsAdjustLeft() {
    if (m_settingsSelected == 2) {
        m_soundVolumePercent = std::max(0, m_soundVolumePercent - 10);
    } else {
        settingsConfirmOrRight();
    }
}

void Game::settingsConfirmOrRight() {
    if (m_settingsSelected == 0) {
        toggleLanguage();
    } else if (m_settingsSelected == 1) {
        toggleFullscreen();
    } else if (m_settingsSelected == 2) {
        m_soundVolumePercent = std::min(100, m_soundVolumePercent + 10);
    } else if (m_settingsSelected == 3) {
        m_state = GameState::Playing;
    }
}

void Game::settingsClick() {
    if (m_settingsSelected == 0) {
        toggleLanguage();
    } else if (m_settingsSelected == 1) {
        toggleFullscreen();
    } else if (m_settingsSelected == 3) {
        m_state = GameState::Playing;
    }
}

void Game::workshopNavigateUp() {
    m_workshopSelected = (m_workshopSelected - 1 + 12) % 12;
}

void Game::workshopNavigateDown() {
    m_workshopSelected = (m_workshopSelected + 1) % 12;
}

void Game::workshopConfirm() {
    buyWorkshopItem(m_workshopSelected);
}

void Game::restartGame() {
    m_map.init();
    m_player = Player();
    m_lighthouse = Lighthouse();
    m_enemies.clear();
    m_mirrors.clear();
    PlacedMirror im1; im1.id = 1; im1.pos = Vec2(1100.0f, 1020.0f); im1.angle = -PI * 0.25f;
    im1.maxHealth = im1.health = m_lighthouse.getMirrorMaxHealth();
    m_mirrors.push_back(im1);
    m_dayNight.resetGame();
    m_state = GameState::Playing;
    setStatus(Localization::instance().isArabic() ? "أشرق يوم جديد. احمِ شعلة المنارة!" : "A new dawn arrives. Defend the Beacon!", 4.0f);
}

void Game::updateDayNight(float dt) {
    std::string statusMsg;
    float statusTime = 0.0f;
    m_dayNight.update(dt, m_enemies, m_mirrors, m_lighthouse, m_player, m_map, m_state, statusMsg, statusTime);
    if (!statusMsg.empty()) {
        setStatus(statusMsg, statusTime);
    }
}

void Game::spawnNightEnemies(float dt) {
    (void)dt;
}

void Game::update(float dt) {
    m_ui.update(dt);

    if (m_state != GameState::Playing) return;

    int mx, my;
    SDL_GetMouseState(&mx, &my);
    float lx = (float)mx, ly = (float)my;
    SDL_RenderWindowToLogical(m_renderer, mx, my, &lx, &ly);
    Vec2 mouseWorld = Vec2(lx, ly) + m_cameraPos;

    const Uint8* keystate = SDL_GetKeyboardState(nullptr);
    m_player.setBonuses(m_lighthouse.getPlayerSpeedBonus(), m_lighthouse.getPlayerDashCooldownBonus());
    m_player.handleInput(keystate, mouseWorld);

    m_player.update(dt, m_map);
    m_map.update(dt);

    if (m_lighthouse.isManned()) {
        Vec2 beamDir = mouseWorld - m_lighthouse.getLanternPos();
        m_lighthouse.setBeamAngle(beamDir.angle());
    }
    m_lighthouse.update(dt, m_enemies, m_mirrors);

    for (auto it = m_mirrors.begin(); it != m_mirrors.end(); ) {
        if (it->rotateFeedbackTimer > 0.0f) {
            it->rotateFeedbackTimer -= dt;
        }
        if (it->health <= 0.0f) {
            ParticleSystem::instance().spawnSparks(it->pos, 15, ColorRGBA{ 220, 220, 240, 255 });
            it = m_mirrors.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_enemies.begin(); it != m_enemies.end(); ) {
        it->update(dt, m_player.getPos(), m_lighthouse.getPos(), m_mirrors, m_map);

        if ((it->getPos() - m_player.getPos()).length() < (it->getRadius() + 14.0f)) {
            m_player.takeDamage(it->getDamage() * dt);
        }

        if ((it->getPos() - m_lighthouse.getPos()).length() < (it->getRadius() + 45.0f)) {
            m_lighthouse.takeDamage(it->getDamage() * dt * 0.8f);
            m_dayNight.notifyLighthouseHurt();
        }

        if (it->isDead()) {
            m_dayNight.notifyEnemyKilled();
            int dropRoll = rand() % 100;
            if (dropRoll < 40) m_player.crystals += 1;
            if (dropRoll > 80) m_player.oil += 1;
            it = m_enemies.erase(it);
        } else {
            ++it;
        }
    }

    ParticleSystem::instance().update(dt);
    updateDayNight(dt);

    Vec2 targetCam = m_player.getPos() - Vec2(WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f);
    if (m_lighthouse.isManned()) {
        targetCam = m_lighthouse.getPos() - Vec2(WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f);
    }
    m_cameraPos += (targetCam - m_cameraPos) * (dt * 6.0f);
    m_cameraPos.x = std::clamp(m_cameraPos.x, 0.0f, WORLD_WIDTH - WINDOW_WIDTH);
    m_cameraPos.y = std::clamp(m_cameraPos.y, 0.0f, WORLD_HEIGHT - WINDOW_HEIGHT);

    if (m_player.getHealth() <= 0.0f || m_lighthouse.getHealth() <= 0.0f) {
        m_state = GameState::GameOver;
        AudioManager::instance().playSound(SoundID::EnemyDie, 1.0f);
    }
}

void Game::drawCircleLight(SDL_Renderer* ren, int cx, int cy, int radius, uint8_t alpha) {
    if (!m_radialLightTexture) return;
    SDL_SetTextureAlphaMod(m_radialLightTexture, alpha);
    SDL_Rect dst{ cx - radius, cy - radius, radius * 2, radius * 2 };
    SDL_RenderCopy(ren, m_radialLightTexture, nullptr, &dst);
}

void Game::renderMirrors() {
    for (const auto& m : m_mirrors) {
        float sx = m.pos.x - m_cameraPos.x;
        float sy = m.pos.y - m_cameraPos.y;

        if (sx < -40 || sx > WINDOW_WIDTH + 40 || sy < -40 || sy > WINDOW_HEIGHT + 40) continue;

        Vec2 p1, p2;
        m.getEndpoints(p1, p2);
        float x1 = p1.x - m_cameraPos.x;
        float y1 = p1.y - m_cameraPos.y;
        float x2 = p2.x - m_cameraPos.x;
        float y2 = p2.y - m_cameraPos.y;

        bool inFeedback = (m.rotateFeedbackTimer > 0.0f);

        SDL_SetRenderDrawColor(m_renderer, 15, 20, 25, 100);
        SDL_Rect shadow{ (int)sx - 12, (int)sy + 6, 24, 8 };
        SDL_RenderFillRect(m_renderer, &shadow);

        // Pedestal glow when rotating
        if (inFeedback) {
            SDL_SetRenderDrawColor(m_renderer, 140, 215, 255, 255);
            SDL_Rect pedGlow{ (int)sx - 8, (int)sy - 8, 16, 16 };
            SDL_RenderDrawRect(m_renderer, &pedGlow);
        }

        SDL_SetRenderDrawColor(m_renderer, 70, 75, 85, 255);
        SDL_Rect ped{ (int)sx - 6, (int)sy - 6, 12, 12 };
        SDL_RenderFillRect(m_renderer, &ped);

        // Brass Frame (flashes bright gold during feedback)
        if (inFeedback) {
            SDL_SetRenderDrawColor(m_renderer, 255, 235, 130, 255);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 220, 180, 80, 255);
        }
        SDL_RenderDrawLine(m_renderer, (int)x1 - 1, (int)y1 - 1, (int)x2 - 1, (int)y2 - 1);
        SDL_RenderDrawLine(m_renderer, (int)x1 + 1, (int)y1 + 1, (int)x2 + 1, (int)y2 + 1);

        // Reflective Surface (flashes bright white during feedback)
        if (inFeedback) {
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 180, 240, 255, 255);
        }
        SDL_RenderDrawLine(m_renderer, (int)x1, (int)y1, (int)x2, (int)y2);

        // Normal direction indicator (extended and brightened on rotate to show new facing angle clearly)
        float normLen = inFeedback ? 20.0f : 12.0f;
        Vec2 norm = m.getNormal() * normLen;
        if (inFeedback) {
            SDL_SetRenderDrawColor(m_renderer, 220, 250, 255, 255);
            SDL_RenderDrawLine(m_renderer, (int)sx - 1, (int)sy, (int)(sx + norm.x) - 1, (int)(sy + norm.y));
            SDL_RenderDrawLine(m_renderer, (int)sx + 1, (int)sy, (int)(sx + norm.x) + 1, (int)(sy + norm.y));
        } else {
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 180);
        }
        SDL_RenderDrawLine(m_renderer, (int)sx, (int)sy, (int)(sx + norm.x), (int)(sy + norm.y));
    }
}

void Game::renderLightingPass() {
    if (!m_lightTexture) return;

    SDL_SetRenderTarget(m_renderer, m_lightTexture);

    ColorRGBA ambient = m_dayNight.getAmbientColor();
    SDL_SetRenderDrawColor(m_renderer, ambient.r, ambient.g, ambient.b, ambient.a);
    SDL_RenderClear(m_renderer);

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_ADD);

    // 1. Player Body light (centered on player's torso)
    Vec2 pCenter = m_player.getCenterPos() - m_cameraPos;
    drawCircleLight(m_renderer, static_cast<int>(pCenter.x), static_cast<int>(pCenter.y), 95, 150);

    // 1b. Player Lantern forward glow (centered on handheld lantern)
    Vec2 pLantern = m_player.getLanternPos() - m_cameraPos;
    drawCircleLight(m_renderer, static_cast<int>(pLantern.x), static_cast<int>(pLantern.y), 160, 225);

    // 2. Lighthouse tower glass lantern aura (centered on lantern room)
    Vec2 lPos = m_lighthouse.getLanternPos() - m_cameraPos;
    drawCircleLight(m_renderer, static_cast<int>(lPos.x), static_cast<int>(lPos.y), 230, 240);

    // 3. Ignited Altars celestial glow
    for (const auto& a : m_map.getAltars()) {
        if (a.ignited) {
            Vec2 aPos = a.pos - m_cameraPos;
            drawCircleLight(m_renderer, static_cast<int>(aPos.x), static_cast<int>(aPos.y - 20.0f), 240, 245);
        }
    }

    // 4. Placed Mirrors subtle reflection aura
    for (const auto& m : m_mirrors) {
        Vec2 mPos = m.pos - m_cameraPos;
        drawCircleLight(m_renderer, static_cast<int>(mPos.x), static_cast<int>(mPos.y), 50, 110);
    }

    // 5. Lighthouse Beams light channel on darkness texture (using exact matching geometry)
    for (const auto& seg : m_lighthouse.getBeamSegments()) {
        Vec2 p1 = seg.start - m_cameraPos;
        Vec2 p2 = seg.end - m_cameraPos;

        // Wide light illumination corridor
        SDL_Color wideMask = { 255, 248, 200, 235 };
        float wideWidth = seg.width * (2.6f + (seg.pulse > 0.0f ? 1.2f * seg.pulse : 0.0f));
        drawThickBeam(m_renderer, p1, p2, wideWidth, wideMask);

        // Core bright illumination
        SDL_Color coreMask = { 255, 255, 255, 255 };
        drawThickBeam(m_renderer, p1, p2, seg.width * 1.3f, coreMask);
    }

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(m_renderer, nullptr);

    SDL_Rect screenRect{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderCopy(m_renderer, m_lightTexture, nullptr, &screenRect);
}

void Game::render() {
    // 1. World Terrain
    m_map.renderTerrain(m_renderer, m_cameraPos);

    // 2. Resource Nodes & Altars
    m_map.renderNodes(m_renderer, m_cameraPos);
    m_map.renderAltars(m_renderer, m_cameraPos);

    // 3. Placed Mirrors
    renderMirrors();

    // 4. Lighthouse Structure
    m_lighthouse.renderBase(m_renderer, m_cameraPos);

    // 5. Enemies
    for (auto& e : m_enemies) {
        e.render(m_renderer, m_cameraPos);
    }

    // 6. Player
    m_player.render(m_renderer, m_cameraPos);

    // 7. Dynamic Lighting Pass (Night ambient & Light holes)
    renderLightingPass();

    // 8. Additive Beam & Particle Pass (shines on top of lighting)
    m_lighthouse.renderBeams(m_renderer, m_cameraPos);
    ParticleSystem::instance().render(m_renderer, m_cameraPos);

    // 9. HUD & UI Overlays
    m_ui.renderHUD(
        m_player,
        m_lighthouse,
        m_map,
        m_dayNight
    );

    if (m_state == GameState::Workshop) {
        m_ui.renderWorkshop(m_workshopSelected, m_player, m_lighthouse);
    } else if (m_state == GameState::Journal) {
        m_ui.renderJournal();
    } else if (m_state == GameState::Settings) {
        m_ui.renderSettings(m_settingsSelected, m_fullscreen, m_soundVolumePercent);
    } else if (m_state == GameState::Paused) {
        m_ui.renderPaused();
    } else if (m_state == GameState::GameOver) {
        m_ui.renderGameOver();
    } else if (m_state == GameState::Victory) {
        m_ui.renderVictory();
    }

    SDL_RenderPresent(m_renderer);
}

void Game::run() {
    Uint32 lastTime = SDL_GetTicks();

    while (m_running) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (dt > 0.05f) dt = 0.05f;

        m_inputHandler.processEvents(*this);
        update(dt);
        render();

        SDL_Delay(1);
    }
}
