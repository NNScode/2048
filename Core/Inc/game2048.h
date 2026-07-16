#ifndef GAME2048_H
#define GAME2048_H

#include <stdint.h>

#define GAME2048_SIZE 4U

typedef struct {
  uint8_t cells[GAME2048_SIZE][GAME2048_SIZE];
  uint32_t score;
} Game2048;

typedef uint32_t (*GameRandomFunction)(void);

void Game2048_New(Game2048 *game, GameRandomFunction random_function);

#endif
