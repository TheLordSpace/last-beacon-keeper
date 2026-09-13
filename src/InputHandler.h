#pragma once

#include <SDL2/SDL.h>

class Game;

class InputHandler {
public:
    InputHandler() = default;
    ~InputHandler() = default;

    void processEvents(Game& game);

private:
    void handleKeyDown(Game& game, const SDL_KeyboardEvent& key);
    void handleMouseButtonDown(Game& game, const SDL_MouseButtonEvent& button);
};
