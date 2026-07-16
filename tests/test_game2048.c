#include "game2048.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t random_call;

static uint32_t deterministic_random(void)
{
  return ((random_call++ & 1U) == 0U) ? 0U : 1U;
}

static void clear_game(Game2048 *game)
{
  memset(game, 0, sizeof(*game));
  random_call = 0U;
}

static void test_double_pair(void)
{
  Game2048 game;
  GameMoveResult result;
  clear_game(&game);
  game.cells[0][0] = 1U;
  game.cells[0][1] = 1U;
  game.cells[0][2] = 1U;
  game.cells[0][3] = 1U;

  assert(Game2048_Move(&game, GAME_DIR_LEFT, deterministic_random, &result));
  assert(game.cells[0][0] == 2U);
  assert(game.cells[0][1] == 2U);
  assert(game.score == 8U);
  assert(result.score_delta == 8U);
}

static void test_one_merge_per_tile(void)
{
  Game2048 game;
  GameMoveResult result;
  clear_game(&game);
  game.cells[0][0] = 1U;
  game.cells[0][1] = 1U;
  game.cells[0][2] = 1U;

  assert(Game2048_Move(&game, GAME_DIR_LEFT, deterministic_random, &result));
  assert(game.cells[0][0] == 2U);
  assert(game.cells[0][1] == 1U);
  assert(game.score == 4U);
}

static void test_all_directions(void)
{
  Game2048 game;
  GameMoveResult result;

  clear_game(&game);
  game.cells[1][0] = 1U;
  game.cells[1][1] = 1U;
  assert(Game2048_Move(&game, GAME_DIR_LEFT, deterministic_random, &result));
  assert(game.cells[1][0] == 2U);

  clear_game(&game);
  game.cells[1][2] = 1U;
  game.cells[1][3] = 1U;
  assert(Game2048_Move(&game, GAME_DIR_RIGHT, deterministic_random, &result));
  assert(game.cells[1][3] == 2U);

  clear_game(&game);
  game.cells[0][2] = 1U;
  game.cells[1][2] = 1U;
  assert(Game2048_Move(&game, GAME_DIR_UP, deterministic_random, &result));
  assert(game.cells[0][2] == 2U);

  clear_game(&game);
  game.cells[2][2] = 1U;
  game.cells[3][2] = 1U;
  assert(Game2048_Move(&game, GAME_DIR_DOWN, deterministic_random, &result));
  assert(game.cells[3][2] == 2U);
}

static void test_noop_does_not_spawn(void)
{
  Game2048 game;
  GameMoveResult result;
  clear_game(&game);
  game.cells[0][0] = 1U;

  assert(!Game2048_Move(&game, GAME_DIR_LEFT, deterministic_random, &result));
  assert(game.cells[0][0] == 1U);
  assert(!result.spawned);
  assert(result.score_delta == 0U);
}

static void test_blocked_board(void)
{
  Game2048 game;
  GameMoveResult result;
  clear_game(&game);
  for (uint8_t row = 0U; row < GAME2048_SIZE; ++row) {
    for (uint8_t col = 0U; col < GAME2048_SIZE; ++col) {
      game.cells[row][col] = (uint8_t)(1U + ((row + col) & 1U));
    }
  }

  assert(Game2048_GetStatus(&game) == GAME_STATUS_OVER);
  assert(!Game2048_Move(&game, GAME_DIR_LEFT, deterministic_random, &result));
  assert(!result.spawned);
}

static void test_win(void)
{
  Game2048 game;
  GameMoveResult result;
  clear_game(&game);
  game.cells[0][0] = 10U;
  game.cells[0][1] = 10U;

  assert(Game2048_Move(&game, GAME_DIR_LEFT, deterministic_random, &result));
  assert(game.cells[0][0] == 11U);
  assert(Game2048_GetStatus(&game) == GAME_STATUS_WON);
  Game2048_ContinueAfterWin(&game);
  assert(Game2048_GetStatus(&game) == GAME_STATUS_PLAYING);
}

int main(void)
{
  test_double_pair();
  test_one_merge_per_tile();
  test_all_directions();
  test_noop_does_not_spawn();
  test_blocked_board();
  test_win();
  puts("All game2048 tests passed.");
  return 0;
}
