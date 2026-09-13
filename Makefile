CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra $(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_image SDL2_mixer fribidi)
LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_ttf SDL2_image SDL2_mixer fribidi) -lpthread -lm

SRC = src/main.cpp \
      src/Localization.cpp \
      src/Audio.cpp \
      src/Particles.cpp \
      src/Map.cpp \
      src/Entities.cpp \
      src/Lighthouse.cpp \
      src/UIManager.cpp \
      src/InputHandler.cpp \
      src/Game.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = beacon_keeper

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
