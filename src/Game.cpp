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

    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
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
    if (m_fontEnSmall) TTF_CloseFont(m_fontEnSmall);
    if (m_fontEnMedium) TTF_CloseFont(m_fontEnMedium);
    if (m_fontEnLarge) TTF_CloseFont(m_fontEnLarge);

    if (m_fontArSmall && m_fontArSmall != m_fontEnSmall) TTF_CloseFont(m_fontArSmall);
    if (m_fontArMedium && m_fontArMedium != m_fontEnMedium) TTF_CloseFont(m_fontArMedium);
    if (m_fontArLarge && m_fontArLarge != m_fontEnLarge) TTF_CloseFont(m_fontArLarge);

    TTF_Quit();

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

void Game::setStatus(const std::string& msg, float time) {
    m_statusMessage = msg;
    m_statusMessageTimer = time;
}

void Game::drawText(const std::string& text, int x, int y, SDL_Color color, FontSize size, bool alignRight) {
    if (text.empty()) return;

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
        AudioManager::instance().playSound(SoundID::MirrorReflect, 0.5f);
        ParticleSystem::instance().spawnSparks(closest->pos, 6, ColorRGBA{ 180, 220, 255, 255 });
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
    }
}

void Game::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            m_running = false;
        } else if (e.type == SDL_KEYDOWN) {
            if (m_state == GameState::Playing) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    m_state = GameState::Paused;
                    break;
                case SDLK_o:
                    m_state = GameState::Settings;
                    break;
                case SDLK_l:
                    // Quick toggle language key
                    Localization::instance().toggleLanguage();
                    setStatus(Localization::instance().isArabic() ? "تم تغيير اللغة إلى العربية" : "Language switched to English", 2.0f);
                    break;
                case SDLK_TAB:
                case SDLK_m:
                    m_state = GameState::Journal;
                    break;
                case SDLK_e:
                    interactNearby();
                    break;
                case SDLK_f:
                case SDLK_c:
                    placeMirror();
                    break;
                case SDLK_r:
                    rotateNearbyMirror();
                    break;
                case SDLK_SPACE:
                    m_player.tryDash();
                    break;
                case SDLK_h:
                    if (m_player.salves > 0 && m_player.getHealth() < m_player.getMaxHealth()) {
                        m_player.salves--;
                        m_player.heal(50.0f);
                        AudioManager::instance().playSound(SoundID::DawnChime, 0.5f);
                        setStatus(Localization::instance().get("MSG_SALVE_USED"), 2.0f);
                    }
                    break;
                case SDLK_1:
                    m_lighthouse.setLens(LensType::Focused);
                    setStatus(Localization::instance().get("MSG_LENS_FOCUSED"), 2.0f);
                    break;
                case SDLK_2:
                    if (m_lighthouse.unlockWideLens) {
                        m_lighthouse.setLens(LensType::WideAmber);
                        setStatus(Localization::instance().get("MSG_LENS_AMBER"), 2.0f);
                    } else {
                        setStatus(Localization::instance().get("MSG_LENS_AMBER_LOCKED"), 2.0f);
                    }
                    break;
                case SDLK_3:
                    if (m_lighthouse.unlockUVLens) {
                        m_lighthouse.setLens(LensType::UVPulse);
                        setStatus(Localization::instance().get("MSG_LENS_UV"), 2.0f);
                    } else {
                        setStatus(Localization::instance().get("MSG_LENS_UV_LOCKED"), 2.0f);
                    }
                    break;
                case SDLK_t:
                    if ((m_lighthouse.getPos() - m_player.getPos()).length() < 90.0f) {
                        m_lighthouse.setManned(!m_lighthouse.isManned());
                        setStatus(m_lighthouse.isManned() ? Localization::instance().get("MSG_MANNED") : Localization::instance().get("MSG_DISMOUNTED"), 2.5f);
                    }
                    break;
                case SDLK_F11:
                    toggleFullscreen();
                    break;
                case SDLK_RETURN:
                    if (e.key.keysym.mod & KMOD_ALT) {
                        toggleFullscreen();
                    }
                    break;
                case SDLK_F12: {
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
                    break;
                }
                }
            } else if (m_state == GameState::Settings) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    m_state = GameState::Playing;
                    break;
                case SDLK_F11:
                    toggleFullscreen();
                    break;
                case SDLK_w:
                case SDLK_UP:
                    m_settingsSelected = (m_settingsSelected - 1 + 4) % 4;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    m_settingsSelected = (m_settingsSelected + 1) % 4;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                case SDLK_d:
                case SDLK_RIGHT:
                case SDLK_RETURN:
                case SDLK_SPACE:
                    if (m_settingsSelected == 0) {
                        // Toggle language
                        Localization::instance().toggleLanguage();
                    } else if (m_settingsSelected == 1) {
                        // Toggle Fullscreen
                        toggleFullscreen();
                    } else if (m_settingsSelected == 2) {
                        // Adjust volume
                        if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_a) {
                            m_soundVolumePercent = std::max(0, m_soundVolumePercent - 10);
                        } else {
                            m_soundVolumePercent = std::min(100, m_soundVolumePercent + 10);
                        }
                    } else if (m_settingsSelected == 3) {
                        m_state = GameState::Playing;
                    }
                    break;
                }
            } else if (m_state == GameState::Workshop) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_e:
                    m_state = GameState::Playing;
                    break;
                case SDLK_w:
                case SDLK_UP:
                    m_workshopSelected = (m_workshopSelected - 1 + 7) % 7;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    m_workshopSelected = (m_workshopSelected + 1) % 7;
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    buyWorkshopItem(m_workshopSelected);
                    break;
                }
            } else if (m_state == GameState::Paused) {
                if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_RETURN) {
                    m_state = GameState::Playing;
                } else if (e.key.keysym.sym == SDLK_o) {
                    m_state = GameState::Settings;
                }
            } else if (m_state == GameState::Journal) {
                if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_TAB || e.key.keysym.sym == SDLK_RETURN) {
                    m_state = GameState::Playing;
                }
            } else if (m_state == GameState::GameOver || m_state == GameState::Victory) {
                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_r) {
                    // Reset game
                    m_map.init();
                    m_player = Player();
                    m_lighthouse = Lighthouse();
                    m_enemies.clear();
                    m_mirrors.clear();
                    PlacedMirror im1; im1.id = 1; im1.pos = Vec2(1100.0f, 1020.0f); im1.angle = -PI * 0.25f;
                    m_mirrors.push_back(im1);
                    m_dayNumber = 1;
                    m_phase = DayPhase::Day;
                    m_phaseTimer = 0.0f;
                    m_state = GameState::Playing;
                    setStatus(Localization::instance().isArabic() ? "أشرق يوم جديد. احمِ شعلة المنارة!" : "A new dawn arrives. Defend the Beacon!", 4.0f);
                }
            }
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (m_state == GameState::Playing) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    m_player.swingTool(m_map, m_enemies);
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    rotateNearbyMirror();
                }
            } else if (m_state == GameState::Workshop) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    buyWorkshopItem(m_workshopSelected);
                }
            } else if (m_state == GameState::Settings) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    if (m_settingsSelected == 0) {
                        Localization::instance().toggleLanguage();
                    } else if (m_settingsSelected == 1) {
                        toggleFullscreen();
                    } else if (m_settingsSelected == 3) {
                        m_state = GameState::Playing;
                    }
                }
            }
        }
    }
}

void Game::updateDayNight(float dt) {
    m_phaseTimer += dt;

    switch (m_phase) {
    case DayPhase::Day:
        if (m_phaseTimer >= m_dayDuration) {
            m_phase = DayPhase::Dusk;
            m_phaseTimer = 0.0f;
            AudioManager::instance().playSound(SoundID::NightAlarm, 0.8f);
            setStatus(Localization::instance().get("MSG_DUSK"), 5.0f);
        }
        break;

    case DayPhase::Dusk:
        if (m_phaseTimer >= m_duskDuration) {
            m_phase = DayPhase::Night;
            m_phaseTimer = 0.0f;
            AudioManager::instance().setMusicNight(true);
            setStatus(Localization::instance().get("MSG_NIGHT"), 5.0f);
        }
        break;

    case DayPhase::Night:
        spawnNightEnemies(dt);
        if (m_phaseTimer >= m_nightDuration) {
            m_phase = DayPhase::Dawn;
            m_phaseTimer = 0.0f;
            AudioManager::instance().playSound(SoundID::DawnChime, 1.0f);
            AudioManager::instance().setMusicNight(false);
            for (auto& e : m_enemies) {
                e.takeDamage(9999.0f, true);
            }
            setStatus(Localization::instance().get("MSG_DAWN"), 5.0f);
        }
        break;

    case DayPhase::Dawn:
        if (m_phaseTimer >= m_dawnDuration) {
            m_phase = DayPhase::Day;
            m_phaseTimer = 0.0f;
            m_dayNumber++;
            m_player.wood += 6;
            m_player.crystals += 6;
            m_player.oil += 10;

            bool isAr = Localization::instance().isArabic();
            std::string msg = isAr ? ("اليوم " + std::to_string(m_dayNumber) + ": اجمع الموارد وحصن دفاعاتك.") :
                                     ("Day " + std::to_string(m_dayNumber) + ": Gather resources and prepare defenses.");
            setStatus(msg, 5.0f);

            if (m_map.getIgnitedAltarsCount() >= 3 && m_dayNumber >= 4) {
                m_state = GameState::Victory;
                AudioManager::instance().playSound(SoundID::DawnChime, 1.0f);
            }
        }
        break;
    }
}

void Game::spawnNightEnemies(float dt) {
    m_spawnTimer += dt;
    float spawnInterval = std::max(1.2f, 3.5f - m_dayNumber * 0.45f);

    if (m_spawnTimer >= spawnInterval) {
        m_spawnTimer = 0.0f;

        float angle = (float)(rand() % 360) * (PI / 180.0f);
        Vec2 spawnPos = Vec2(1300.0f, 1000.0f) + Vec2::fromAngle(angle, 800.0f);

        EnemyType t = EnemyType::Crawler;
        int roll = rand() % 100;
        if (m_dayNumber >= 2 && roll < 35) {
            t = EnemyType::Eater;
        } else if (m_dayNumber >= 3 && roll < 20) {
            t = EnemyType::Brute;
        }

        if (m_dayNumber >= 4 && m_phaseTimer > 15.0f && m_phaseTimer < 18.0f) {
            bool hasBoss = false;
            for (auto& e : m_enemies) {
                if (e.getType() == EnemyType::Leviathan) hasBoss = true;
            }
            if (!hasBoss) {
                t = EnemyType::Leviathan;
                setStatus(Localization::instance().get("MSG_BOSS"), 6.0f);
                AudioManager::instance().playSound(SoundID::NightAlarm, 1.0f);
            }
        }

        m_enemies.emplace_back(t, spawnPos);
        ParticleSystem::instance().spawnShadowBurst(spawnPos, 12);
    }
}

void Game::update(float dt) {
    if (m_statusMessageTimer > 0.0f) {
        m_statusMessageTimer -= dt;
    }

    if (m_state != GameState::Playing) return;

    int mx, my;
    SDL_GetMouseState(&mx, &my);
    float lx = (float)mx, ly = (float)my;
    SDL_RenderWindowToLogical(m_renderer, mx, my, &lx, &ly);
    Vec2 mouseWorld = Vec2(lx, ly) + m_cameraPos;

    const Uint8* keystate = SDL_GetKeyboardState(nullptr);
    m_player.handleInput(keystate, mouseWorld);

    m_player.update(dt, m_map);
    m_map.update(dt);

    if (m_lighthouse.isManned()) {
        Vec2 beamDir = mouseWorld - m_lighthouse.getLanternPos();
        m_lighthouse.setBeamAngle(beamDir.angle());
    }
    m_lighthouse.update(dt, m_enemies, m_mirrors);

    for (auto it = m_mirrors.begin(); it != m_mirrors.end(); ) {
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
        }

        if (it->isDead()) {
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

        SDL_SetRenderDrawColor(m_renderer, 15, 20, 25, 100);
        SDL_Rect shadow{ (int)sx - 12, (int)sy + 6, 24, 8 };
        SDL_RenderFillRect(m_renderer, &shadow);

        SDL_SetRenderDrawColor(m_renderer, 70, 75, 85, 255);
        SDL_Rect ped{ (int)sx - 6, (int)sy - 6, 12, 12 };
        SDL_RenderFillRect(m_renderer, &ped);

        SDL_SetRenderDrawColor(m_renderer, 220, 180, 80, 255);
        SDL_RenderDrawLine(m_renderer, (int)x1 - 1, (int)y1 - 1, (int)x2 - 1, (int)y2 - 1);
        SDL_RenderDrawLine(m_renderer, (int)x1 + 1, (int)y1 + 1, (int)x2 + 1, (int)y2 + 1);

        SDL_SetRenderDrawColor(m_renderer, 180, 240, 255, 255);
        SDL_RenderDrawLine(m_renderer, (int)x1, (int)y1, (int)x2, (int)y2);

        Vec2 norm = m.getNormal() * 12.0f;
        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 180);
        SDL_RenderDrawLine(m_renderer, (int)sx, (int)sy, (int)(sx + norm.x), (int)(sy + norm.y));
    }
}

void Game::renderLightingPass() {
    if (!m_lightTexture) return;

    SDL_SetRenderTarget(m_renderer, m_lightTexture);

    uint8_t darkness = 0;
    if (m_phase == DayPhase::Day) {
        darkness = 18;
    } else if (m_phase == DayPhase::Dusk) {
        darkness = static_cast<uint8_t>(18 + (m_phaseTimer / m_duskDuration) * 202.0f);
    } else if (m_phase == DayPhase::Night) {
        darkness = 238;
    } else if (m_phase == DayPhase::Dawn) {
        darkness = static_cast<uint8_t>(238 - (m_phaseTimer / m_dawnDuration) * 220.0f);
    }

    // Base ambient darkness
    SDL_SetRenderDrawColor(m_renderer, 255 - darkness, 255 - darkness, std::max(20, 255 - darkness), 255);
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
        drawThickBeam(m_renderer, p1, p2, seg.width * 2.6f, wideMask);

        // Core bright illumination
        SDL_Color coreMask = { 255, 255, 255, 255 };
        drawThickBeam(m_renderer, p1, p2, seg.width * 1.3f, coreMask);
    }

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(m_renderer, nullptr);

    SDL_Rect screenRect{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderCopy(m_renderer, m_lightTexture, nullptr, &screenRect);
}

void Game::renderHUD() {
    // 1. Top Status Bar Container
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 15, 20, 30, 225);
    SDL_Rect topBar{ 15, 12, WINDOW_WIDTH - 30, 68 };
    SDL_RenderFillRect(m_renderer, &topBar);
    SDL_SetRenderDrawColor(m_renderer, 70, 85, 110, 255);
    SDL_RenderDrawRect(m_renderer, &topBar);

    // Player HP Bar
    float phpRatio = m_player.getHealth() / m_player.getMaxHealth();
    std::string keeperLabel = Localization::instance().get("KEEPER_HP");
    drawText(keeperLabel, 25, 20, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect phpBg{ 125, 24, 110, 14 };
    SDL_RenderFillRect(m_renderer, &phpBg);
    SDL_SetRenderDrawColor(m_renderer, 220, 60, 60, 255);
    SDL_Rect phpFg{ 125, 24, static_cast<int>(110 * phpRatio), 14 };
    SDL_RenderFillRect(m_renderer, &phpFg);

    // Lighthouse HP Bar
    float lhpRatio = m_lighthouse.getHealth() / m_lighthouse.getMaxHealth();
    std::string beaconLabel = Localization::instance().get("BEACON_HP");
    drawText(beaconLabel, 250, 20, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect lhpBg{ 350, 24, 110, 14 };
    SDL_RenderFillRect(m_renderer, &lhpBg);
    SDL_SetRenderDrawColor(m_renderer, 60, 190, 80, 255);
    SDL_Rect lhpFg{ 350, 24, static_cast<int>(110 * lhpRatio), 14 };
    SDL_RenderFillRect(m_renderer, &lhpFg);

    // Fuel Bar
    float fuelRatio = m_lighthouse.getFuel() / m_lighthouse.getMaxFuel();
    std::string fuelLabel = Localization::instance().get("FUEL");
    drawText(fuelLabel, 475, 20, { 230, 230, 230, 255 }, FontSize::Small);
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_Rect fuelBg{ 530, 24, 85, 14 };
    SDL_RenderFillRect(m_renderer, &fuelBg);
    SDL_SetRenderDrawColor(m_renderer, 240, 180, 40, 255);
    SDL_Rect fuelFg{ 530, 24, static_cast<int>(85 * fuelRatio), 14 };
    SDL_RenderFillRect(m_renderer, &fuelFg);

    // Day & Phase Clock
    float pDuration = m_dayDuration;
    if (m_phase == DayPhase::Dusk) pDuration = m_duskDuration;
    else if (m_phase == DayPhase::Night) pDuration = m_nightDuration;
    else if (m_phase == DayPhase::Dawn) pDuration = m_dawnDuration;

    int timeLeft = std::max(0, static_cast<int>(pDuration - m_phaseTimer));
    std::string clockStr = Localization::instance().getClockText(m_dayNumber, m_phase, timeLeft);
    SDL_Color phaseCol{ 255, 220, 90, 255 };
    if (m_phase == DayPhase::Dusk) phaseCol = { 255, 130, 50, 255 };
    else if (m_phase == DayPhase::Night) phaseCol = { 210, 90, 255, 255 };
    else if (m_phase == DayPhase::Dawn) phaseCol = { 100, 225, 255, 255 };

    drawText(clockStr, 630, 18, phaseCol, FontSize::Medium);

    // Altars Count
    int altarsLit = m_map.getIgnitedAltarsCount();
    std::string altarStr = Localization::instance().get("ALTARS") + " " + std::to_string(altarsLit) + "/3";
    drawText(altarStr, 1120, 20, { 240, 200, 80, 255 }, FontSize::Small);

    // Inventory Line (bottom row of top bar)
    std::string invStr = Localization::instance().getInventoryText(
        m_player.wood, m_player.crystals, m_player.oil,
        m_player.mirrorsInBag, m_player.relicCores, m_player.salves
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

void Game::renderWorkshop() {
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
        bool isSel = (m_workshopSelected == static_cast<int>(i));

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

void Game::renderJournal() {
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

void Game::renderSettings() {
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
    bool sel0 = (m_settingsSelected == 0);
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
    bool sel1 = (m_settingsSelected == 1);
    if (sel1) {
        SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
        SDL_Rect r{ WINDOW_WIDTH / 2 - 270, y1 - 6, 540, 42 };
        SDL_RenderFillRect(m_renderer, &r);
        SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
        SDL_RenderDrawRect(m_renderer, &r);
    }
    std::string fsState = m_fullscreen ? Localization::instance().get("SETTINGS_FS_ON") : Localization::instance().get("SETTINGS_FS_OFF");
    std::string fsLabel = Localization::instance().get("SETTINGS_FULLSCREEN") + " < " + fsState + " > [F11]";
    drawText(fsLabel, WINDOW_WIDTH / 2 - 240, y1 + 3, sel1 ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 220, 255 }, FontSize::Medium);

    // Option 2: Volume
    int y2 = WINDOW_HEIGHT / 2;
    bool sel2 = (m_settingsSelected == 2);
    if (sel2) {
        SDL_SetRenderDrawColor(m_renderer, 50, 75, 110, 255);
        SDL_Rect r{ WINDOW_WIDTH / 2 - 270, y2 - 6, 540, 42 };
        SDL_RenderFillRect(m_renderer, &r);
        SDL_SetRenderDrawColor(m_renderer, 255, 220, 90, 255);
        SDL_RenderDrawRect(m_renderer, &r);
    }
    std::string volLabel = Localization::instance().get("SETTINGS_VOLUME") + " < " + std::to_string(m_soundVolumePercent) + "% >";
    drawText(volLabel, WINDOW_WIDTH / 2 - 240, y2 + 3, sel2 ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 200, 205, 220, 255 }, FontSize::Medium);

    // Option 3: Back
    int y3 = WINDOW_HEIGHT / 2 + 65;
    bool sel3 = (m_settingsSelected == 3);
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

void Game::renderPaused() {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, 10, 15, 25, 230);
    SDL_Rect overlay{ 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(m_renderer, &overlay);

    std::string title = Localization::instance().get("PAUSE_TITLE");
    std::string resume = Localization::instance().get("PAUSE_RESUME");
    drawText(title, WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 60, { 255, 220, 90, 255 }, FontSize::Large);
    drawText(resume, WINDOW_WIDTH / 2 - 220, WINDOW_HEIGHT / 2 + 20, { 210, 220, 235, 255 }, FontSize::Medium);
}

void Game::renderGameOver() {
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

void Game::renderVictory() {
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
    renderHUD();

    if (m_state == GameState::Workshop) {
        renderWorkshop();
    } else if (m_state == GameState::Journal) {
        renderJournal();
    } else if (m_state == GameState::Settings) {
        renderSettings();
    } else if (m_state == GameState::Paused) {
        renderPaused();
    } else if (m_state == GameState::GameOver) {
        renderGameOver();
    } else if (m_state == GameState::Victory) {
        renderVictory();
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

        processEvents();
        update(dt);
        render();

        SDL_Delay(1);
    }
}
