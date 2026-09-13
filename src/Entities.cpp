#include "Entities.h"
#include "Map.h"
#include "Audio.h"
#include "Particles.h"
#include <cmath>
#include <cstdlib>

// ==========================================
// PLAYER IMPLEMENTATION
// ==========================================

Player::Player() = default;

void Player::handleInput(const Uint8* keystate, const Vec2& mouseWorld) {
    Vec2 moveDir(0.0f, 0.0f);
    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) moveDir.y -= 1.0f;
    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) moveDir.y += 1.0f;
    if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) moveDir.x -= 1.0f;
    if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) moveDir.x += 1.0f;

    if (moveDir.lengthSq() > 0.001f) {
        moveDir = moveDir.normalized();
        m_dashDir = moveDir;
    }

    if (m_dashTimer <= 0.0f) {
        m_vel = moveDir * m_speed;
    }

    // Aim angle towards mouse
    Vec2 diff = mouseWorld - m_pos;
    if (diff.lengthSq() > 1.0f) {
        m_aimAngle = diff.angle();
    }
}

void Player::tryDash() {
    if (m_dashCooldown <= 0.0f && m_dashTimer <= 0.0f) {
        m_dashTimer = 0.22f;
        m_dashCooldown = 0.9f;
        m_vel = m_dashDir * (m_speed * 2.8f);
        m_invulnTimer = 0.25f;
        AudioManager::instance().playSound(SoundID::PlayerDash, 0.6f);

        // Spawn dust
        for (int i = 0; i < 6; ++i) {
            Vec2 p = m_pos + Vec2((float)(rand() % 16 - 8), (float)(rand() % 16 - 8));
            ColorRGBA col{ 180, 160, 130, 180 };
            ParticleSystem::instance().spawn(p, -m_dashDir * 40.0f, col, 0.3f, 3.0f);
        }
    }
}

void Player::swingTool(IslandMap& map, std::vector<Enemy>& enemies) {
    if (m_swingTimer > 0.0f) return;

    m_swingTimer = 0.25f;
    m_swingAngle = m_aimAngle;
    AudioManager::instance().playSound(SoundID::Swing, 0.5f);

    Vec2 hitCenter = m_pos + Vec2::fromAngle(m_aimAngle, 38.0f);
    float hitRadius = 45.0f;

    // 1. Check hitting resource nodes
    ResourceNode* node = map.getNearbyNode(hitCenter, hitRadius);
    if (node && node->active) {
        node->health -= 35.0f;
        if (node->type == NodeType::Tree) {
            AudioManager::instance().playSound(SoundID::HarvestTree, 0.7f);
            ParticleSystem::instance().spawnSparks(node->pos, 8, ColorRGBA{ 130, 85, 45, 255 });
            wood += 2;
        } else if (node->type == NodeType::Crystal) {
            AudioManager::instance().playSound(SoundID::HarvestCrystal, 0.8f);
            ParticleSystem::instance().spawnSparks(node->pos, 8, ColorRGBA{ 80, 220, 255, 255 });
            crystals += 2;
        } else if (node->type == NodeType::OilBarrel) {
            AudioManager::instance().playSound(SoundID::HarvestOil, 0.7f);
            ParticleSystem::instance().spawnSparks(node->pos, 6, ColorRGBA{ 220, 180, 50, 255 });
            oil += 3;
        }

        if (node->health <= 0.0f) {
            node->active = false;
            node->respawnTimer = 60.0f; // respawn in 60s
            // Bonus drops on harvest destruction
            if (node->type == NodeType::Tree) wood += 3;
            if (node->type == NodeType::Crystal) crystals += 3;
            if (node->type == NodeType::OilBarrel) oil += 5;
        }
    }

    // 2. Check hitting enemies
    for (auto& e : enemies) {
        if (e.isDead()) continue;
        if ((e.getPos() - hitCenter).length() <= (hitRadius + e.getRadius())) {
            e.takeDamage(28.0f, false);
            Vec2 knockbackDir = (e.getPos() - m_pos).normalized();
            if (knockbackDir.lengthSq() < 0.001f) {
                knockbackDir = Vec2::fromAngle(m_aimAngle);
            }
            e.applyHitReaction(knockbackDir);
            AudioManager::instance().playSound(SoundID::EnemyHurt, 0.6f);
            ParticleSystem::instance().spawnShadowBurst(e.getPos(), 10);
            ParticleSystem::instance().spawnSparks(e.getPos(), 6, ColorRGBA{ 255, 230, 150, 255 });
        }
    }
}

void Player::takeDamage(float dmg) {
    if (m_invulnTimer > 0.0f || m_health <= 0.0f) return;
    m_health -= dmg;
    m_invulnTimer = 0.5f;
    AudioManager::instance().playSound(SoundID::PlayerHurt, 0.8f);
    ParticleSystem::instance().spawnSparks(m_pos, 10, ColorRGBA{ 255, 60, 60, 255 });
}

void Player::heal(float amount) {
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Player::update(float dt, const IslandMap& map) {
    if (m_dashTimer > 0.0f) {
        m_dashTimer -= dt;
    }
    if (m_dashCooldown > 0.0f) {
        m_dashCooldown -= dt;
    }
    if (m_invulnTimer > 0.0f) {
        m_invulnTimer -= dt;
    }
    if (m_swingTimer > 0.0f) {
        m_swingTimer -= dt;
    }

    // Movement collision with map
    Vec2 newPos = m_pos + m_vel * dt;

    if (map.isWalkable(Vec2(newPos.x, m_pos.y))) {
        m_pos.x = newPos.x;
    }
    if (map.isWalkable(Vec2(m_pos.x, newPos.y))) {
        m_pos.y = newPos.y;
    }

    // Footstep audio
    if (m_vel.lengthSq() > 10.0f && m_dashTimer <= 0.0f) {
        m_stepSoundTimer += dt;
        if (m_stepSoundTimer >= 0.32f) {
            m_stepSoundTimer = 0.0f;
            AudioManager::instance().playSound(SoundID::Step, 0.4f);
        }
    } else {
        m_stepSoundTimer = 0.25f;
    }
}

void Player::render(SDL_Renderer* ren, const Vec2& cameraOffset) {
    float sx = m_pos.x - cameraOffset.x;
    float sy = m_pos.y - cameraOffset.y;

    // Flicker if invulnerable
    if (m_invulnTimer > 0.0f && (static_cast<int>(m_invulnTimer * 20.0f) % 2 == 0)) {
        return;
    }

    // Shadow
    SDL_SetRenderDrawColor(ren, 15, 20, 25, 120);
    SDL_Rect shadow{ (int)sx - 12, (int)sy + 10, 24, 10 };
    SDL_RenderFillRect(ren, &shadow);

    // Body (Keeper Coat / Clothes)
    SDL_SetRenderDrawColor(ren, 45, 65, 80, 255); // navy coat
    SDL_Rect body{ (int)sx - 8, (int)sy - 10, 16, 20 };
    SDL_RenderFillRect(ren, &body);

    // Belt & Buckle
    SDL_SetRenderDrawColor(ren, 190, 140, 60, 255);
    SDL_Rect belt{ (int)sx - 8, (int)sy + 2, 16, 4 };
    SDL_RenderFillRect(ren, &belt);

    // Head / Hat
    SDL_SetRenderDrawColor(ren, 220, 175, 130, 255); // face
    SDL_Rect face{ (int)sx - 6, (int)sy - 22, 12, 12 };
    SDL_RenderFillRect(ren, &face);

    SDL_SetRenderDrawColor(ren, 30, 35, 45, 255); // Keeper hat
    SDL_Rect hatBrim{ (int)sx - 10, (int)sy - 24, 20, 4 };
    SDL_RenderFillRect(ren, &hatBrim);
    SDL_Rect hatCrown{ (int)sx - 7, (int)sy - 30, 14, 7 };
    SDL_RenderFillRect(ren, &hatCrown);

    // Handheld Lantern
    Vec2 lanternPos = m_pos + Vec2::fromAngle(m_aimAngle, 18.0f);
    float lx = lanternPos.x - cameraOffset.x;
    float ly = lanternPos.y - cameraOffset.y;

    // Lantern brass frame
    SDL_SetRenderDrawColor(ren, 170, 120, 40, 255);
    SDL_Rect lFrame{ (int)lx - 4, (int)ly - 5, 8, 10 };
    SDL_RenderFillRect(ren, &lFrame);

    // Lantern warm core
    SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
    SDL_Rect lGlass{ (int)lx - 2, (int)ly - 3, 4, 6 };
    SDL_RenderFillRect(ren, &lGlass);

    // Swing arc visual
    if (m_swingTimer > 0.0f) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        SDL_SetRenderDrawColor(ren, 255, 220, 100, 200);
        Vec2 swingDir = Vec2::fromAngle(m_swingAngle, 30.0f);
        SDL_RenderDrawLine(ren, (int)sx, (int)sy, (int)(sx + swingDir.x), (int)(sy + swingDir.y));
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    }
}

// ==========================================
// ENEMY IMPLEMENTATION
// ==========================================

Enemy::Enemy(EnemyType type, const Vec2& pos) : m_type(type), m_pos(pos) {
    switch (type) {
    case EnemyType::Crawler:
        m_health = 35.0f;
        m_maxHealth = 35.0f;
        m_speed = 115.0f;
        m_radius = 14.0f;
        m_attackDmg = 12.0f;
        break;
    case EnemyType::Eater:
        m_health = 75.0f;
        m_maxHealth = 75.0f;
        m_speed = 80.0f;
        m_radius = 18.0f;
        m_attackDmg = 18.0f;
        break;
    case EnemyType::Brute:
        m_health = 280.0f;
        m_maxHealth = 280.0f;
        m_speed = 50.0f;
        m_radius = 28.0f;
        m_attackDmg = 35.0f;
        break;
    case EnemyType::Leviathan:
        m_health = 1200.0f;
        m_maxHealth = 1200.0f;
        m_speed = 40.0f;
        m_radius = 50.0f;
        m_attackDmg = 50.0f;
        break;
    }
}

void Enemy::takeDamage(float dmg, bool isLight) {
    m_health -= dmg;
    if (isLight) {
        m_burnTimer = 0.5f;
    }
    if (m_health <= 0.0f) {
        AudioManager::instance().playSound(SoundID::EnemyDie, 0.7f);
        ParticleSystem::instance().spawnShadowBurst(m_pos, 16);
    } else if (isLight) {
        AudioManager::instance().playSound(SoundID::BeamSizzle, 0.3f);
    }
}

void Enemy::applyHitReaction(const Vec2& knockbackDir) {
    if (m_health <= 0.0f) return;

    m_hitFlashTimer = 0.12f;
    m_stunTimer = 0.14f;

    float knockbackForce = 160.0f;
    switch (m_type) {
    case EnemyType::Crawler:
        knockbackForce = 180.0f;
        break;
    case EnemyType::Eater:
        knockbackForce = 130.0f;
        break;
    case EnemyType::Brute:
        knockbackForce = 65.0f;
        break;
    case EnemyType::Leviathan:
        knockbackForce = 20.0f;
        break;
    }

    m_vel = knockbackDir * knockbackForce;
}

void Enemy::update(float dt, const Vec2& playerPos, const Vec2& lighthousePos,
                   std::vector<PlacedMirror>& mirrors, const IslandMap& map) {
    (void)map;
    if (m_burnTimer > 0.0f) {
        m_burnTimer -= dt;
    }
    if (m_hitFlashTimer > 0.0f) {
        m_hitFlashTimer -= dt;
    }
    if (m_attackCooldown > 0.0f) {
        m_attackCooldown -= dt;
    }

    // Apply knockback movement with smooth friction dampening
    if (m_vel.lengthSq() > 1.0f) {
        m_pos += m_vel * dt;
        m_vel -= m_vel * (dt * 12.0f);
    } else {
        m_vel = Vec2(0.0f, 0.0f);
    }

    // Hit stun halts regular movement and animation briefly
    if (m_stunTimer > 0.0f) {
        m_stunTimer -= dt;
        return;
    }

    m_animTimer += dt * 4.0f;

    Vec2 target = lighthousePos;

    // AI targeting logic
    if (m_type == EnemyType::Eater) {
        // Find closest placed mirror
        float closestDistSq = 999999.0f;
        PlacedMirror* bestMirror = nullptr;
        for (auto& m : mirrors) {
            float dsq = (m.pos - m_pos).lengthSq();
            if (dsq < closestDistSq) {
                closestDistSq = dsq;
                bestMirror = &m;
            }
        }
        if (bestMirror && closestDistSq < 500.0f * 500.0f) {
            target = bestMirror->pos;
            if (closestDistSq < (m_radius + bestMirror->length * 0.5f) * (m_radius + bestMirror->length * 0.5f)) {
                if (m_attackCooldown <= 0.0f) {
                    m_attackCooldown = 1.0f;
                    bestMirror->health -= m_attackDmg;
                    AudioManager::instance().playSound(SoundID::EnemyHurt, 0.6f);
                    ParticleSystem::instance().spawnSparks(bestMirror->pos, 8, ColorRGBA{ 220, 220, 240, 255 });
                }
            }
        } else {
            // Target player
            target = playerPos;
        }
    } else if (m_type == EnemyType::Crawler) {
        // If player is close, hunt player, else lighthouse
        float distToPlayer = (playerPos - m_pos).length();
        if (distToPlayer < 320.0f) {
            target = playerPos;
        } else {
            target = lighthousePos;
        }
    } else {
        // Brute & Leviathan target Lighthouse
        target = lighthousePos;
    }

    // Move towards target
    Vec2 dir = (target - m_pos);
    if (dir.lengthSq() > 1.0f) {
        dir = dir.normalized();
        float currentSpeed = m_speed;
        if (m_burnTimer > 0.0f) {
            currentSpeed *= 0.55f; // slowed by light
        }
        m_pos += dir * currentSpeed * dt;
    }
}

void Enemy::render(SDL_Renderer* ren, const Vec2& cameraOffset) {
    float sx = m_pos.x - cameraOffset.x;
    float sy = m_pos.y - cameraOffset.y;

    if (sx < -60 || sx > WINDOW_WIDTH + 60 || sy < -60 || sy > WINDOW_HEIGHT + 60) return;

    // Shadow pulse
    float pulse = 1.0f + 0.1f * std::sin(m_animTimer);
    if (m_hitFlashTimer > 0.0f) {
        pulse *= 1.18f; // brief impact flinch expansion
    }

    // Color: Deep dark void black, bright flash on melee hit, burning orange if in beam
    uint8_t r = 18, g = 12, b = 28;
    if (m_hitFlashTimer > 0.0f) {
        r = 255; g = 225; b = 225;
    } else if (m_burnTimer > 0.0f) {
        r = 240; g = 110; b = 40;
    }

    // Draw creature body
    SDL_SetRenderDrawColor(ren, r, g, b, 240);
    int rad = static_cast<int>(m_radius * pulse);
    SDL_Rect bodyRect{ (int)sx - rad, (int)sy - rad, rad * 2, rad * 2 };
    SDL_RenderFillRect(ren, &bodyRect);

    // Glowing Eyes
    if (m_hitFlashTimer > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    } else {
        SDL_SetRenderDrawColor(ren, (m_burnTimer > 0.0f) ? 255 : 220, (m_burnTimer > 0.0f) ? 255 : 30, 40, 255);
    }
    int eyeOff = (int)(m_radius * 0.35f);
    SDL_Rect eyeL{ (int)sx - eyeOff - 3, (int)sy - eyeOff, 4, 4 };
    SDL_Rect eyeR{ (int)sx + eyeOff - 1, (int)sy - eyeOff, 4, 4 };
    SDL_RenderFillRect(ren, &eyeL);
    SDL_RenderFillRect(ren, &eyeR);

    // Health bar above enemy if damaged
    if (m_health < m_maxHealth) {
        float hpRatio = m_health / m_maxHealth;
        SDL_SetRenderDrawColor(ren, 40, 40, 40, 200);
        SDL_Rect barBg{ (int)sx - 16, (int)sy - rad - 8, 32, 4 };
        SDL_RenderFillRect(ren, &barBg);

        SDL_SetRenderDrawColor(ren, 220, 50, 50, 255);
        SDL_Rect barFg{ (int)sx - 16, (int)sy - rad - 8, static_cast<int>(32 * hpRatio), 4 };
        SDL_RenderFillRect(ren, &barFg);
    }
}
