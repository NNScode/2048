#ifndef GAME2048_H
#define GAME2048_H

#include <stdbool.h>
#include <stdint.h>

#define GAME2048_SIZE 4U
#define GAME2048_TARGET_EXPONENT 11U

typedef enum {
  GAME_DIR_LEFT = 0,
  GAME_DIR_RIGHT,
  GAME_DIR_UP,
  GAME_DIR_DOWN
} GameDirection;

typedef enum {
  GAME_STATUS_PLAYING = 0,
  GAME_STATUS_WON,
  GAME_STATUS_OVER
} GameStatus;

typedef struct {
  uint8_t from_row;
  uint8_t from_col;
  uint8_t to_row;
  uint8_t to_col;
  uint8_t exponent;
} GameMotion;

typedef struct {
  bool changed;
  uint32_t score_delta;
  uint8_t motion_count;
  GameMotion motions[GAME2048_SIZE * GAME2048_SIZE];
  bool spawned;
  uint8_t spawn_row;
  uint8_t spawn_col;
  uint8_t spawn_exponent;
} GameMoveResult;

typedef struct {
  uint8_t cells[GAME2048_SIZE][GAME2048_SIZE];
  uint32_t score;
  bool win_acknowledged;
} Game2048;

typedef uint32_t (*GameRandomFunction)(void);

void Game2048_New(Game2048 *game, GameRandomFunction random_function);
bool Game2048_Move(Game2048 *game, GameDirection direction,
                   GameRandomFunction random_function, GameMoveResult *result);
GameStatus Game2048_GetStatus(const Game2048 *game);
void Game2048_ContinueAfterWin(Game2048 *game);

#endif
