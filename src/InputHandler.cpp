#include "InputHandler.h"
#include "Game.h"

void InputHandler::processEvents(Game& game) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            game.quit();
        } else if (e.type == SDL_KEYDOWN) {
            handleKeyDown(game, e.key);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            handleMouseButtonDown(game, e.button);
        }
    }
}

void InputHandler::handleKeyDown(Game& game, const SDL_KeyboardEvent& key) {
    GameState state = game.getState();

    if (state == GameState::Playing) {
        switch (key.keysym.sym) {
        case SDLK_ESCAPE:
            game.pauseGame();
            break;
        case SDLK_o:
            game.openSettings();
            break;
        case SDLK_l:
            game.toggleLanguage();
            break;
        case SDLK_TAB:
        case SDLK_m:
            game.openJournal();
            break;
        case SDLK_e:
            game.interactNearby();
            break;
        case SDLK_f:
        case SDLK_c:
            game.placeMirror();
            break;
        case SDLK_r:
            game.rotateNearbyMirror();
            break;
        case SDLK_SPACE:
            game.playerDash();
            break;
        case SDLK_h:
            game.useHealingSalve();
            break;
        case SDLK_1:
            game.selectLens(LensType::Focused);
            break;
        case SDLK_2:
            game.selectLens(LensType::WideAmber);
            break;
        case SDLK_3:
            game.selectLens(LensType::UVPulse);
            break;
        case SDLK_t:
            game.toggleMannedLighthouse();
            break;
        case SDLK_F11:
            game.toggleFullscreen();
            break;
        case SDLK_RETURN:
            if (key.keysym.mod & KMOD_ALT) {
                game.toggleFullscreen();
            }
            break;
        case SDLK_F12:
            game.takeScreenshot();
            break;
        }
    } else if (state == GameState::Settings) {
        switch (key.keysym.sym) {
        case SDLK_ESCAPE:
            game.closeSettings();
            break;
        case SDLK_F11:
            game.toggleFullscreen();
            break;
        case SDLK_w:
        case SDLK_UP:
            game.settingsNavigateUp();
            break;
        case SDLK_s:
        case SDLK_DOWN:
            game.settingsNavigateDown();
            break;
        case SDLK_a:
        case SDLK_LEFT:
            game.settingsAdjustLeft();
            break;
        case SDLK_d:
        case SDLK_RIGHT:
        case SDLK_RETURN:
        case SDLK_SPACE:
            game.settingsConfirmOrRight();
            break;
        }
    } else if (state == GameState::Workshop) {
        switch (key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_e:
            game.closeWorkshop();
            break;
        case SDLK_w:
        case SDLK_UP:
            game.workshopNavigateUp();
            break;
        case SDLK_s:
        case SDLK_DOWN:
            game.workshopNavigateDown();
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            game.workshopConfirm();
            break;
        }
    } else if (state == GameState::Paused) {
        if (key.keysym.sym == SDLK_ESCAPE || key.keysym.sym == SDLK_RETURN) {
            game.resumeGame();
        } else if (key.keysym.sym == SDLK_o) {
            game.openSettings();
        }
    } else if (state == GameState::Journal) {
        if (key.keysym.sym == SDLK_ESCAPE || key.keysym.sym == SDLK_TAB || key.keysym.sym == SDLK_RETURN) {
            game.closeJournal();
        }
    } else if (state == GameState::GameOver || state == GameState::Victory) {
        if (key.keysym.sym == SDLK_RETURN || key.keysym.sym == SDLK_r) {
            game.restartGame();
        }
    }
}

void InputHandler::handleMouseButtonDown(Game& game, const SDL_MouseButtonEvent& button) {
    GameState state = game.getState();

    if (state == GameState::Playing) {
        if (button.button == SDL_BUTTON_LEFT) {
            game.playerAttack();
        } else if (button.button == SDL_BUTTON_RIGHT) {
            game.rotateNearbyMirror();
        }
    } else if (state == GameState::Workshop) {
        if (button.button == SDL_BUTTON_LEFT) {
            game.workshopConfirm();
        }
    } else if (state == GameState::Settings) {
        if (button.button == SDL_BUTTON_LEFT) {
            game.settingsClick();
        }
    }
}
