#include "game2048.h"

#include <string.h>

static void spawn_tile(Game2048 *game, GameRandomFunction random_function)
{
  uint8_t rows[GAME2048_SIZE * GAME2048_SIZE];
  uint8_t cols[GAME2048_SIZE * GAME2048_SIZE];
  uint8_t count = 0U;

  for (uint8_t row = 0U; row < GAME2048_SIZE; ++row) {
    for (uint8_t col = 0U; col < GAME2048_SIZE; ++col) {
      if (game->cells[row][col] == 0U) {
        rows[count] = row;
        cols[count] = col;
        ++count;
      }
    }
  }

  if (count == 0U) {
    return;
  }

  uint8_t selected = (uint8_t)(random_function() % count);
  uint8_t exponent = ((random_function() % 10U) == 0U) ? 2U : 1U;
  uint8_t row = rows[selected];
  uint8_t col = cols[selected];
  game->cells[row][col] = exponent;

}

void Game2048_New(Game2048 *game, GameRandomFunction random_function)
{
  memset(game, 0, sizeof(*game));
  spawn_tile(game, random_function);
  spawn_tile(game, random_function);
}
