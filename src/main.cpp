#include "Game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "Starting The Last Beacon Keeper (حارس المنارة الأخير)..." << std::endl;

    Game game;
    if (!game.init()) {
        std::cerr << "Failed to initialize game engine." << std::endl;
        return 1;
    }

    game.run();

    return 0;
}
