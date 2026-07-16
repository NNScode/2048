#include "game2048.h"

#include <string.h>

static void coordinate(GameDirection direction, uint8_t line, uint8_t index,
                       uint8_t *row, uint8_t *col)
{
  switch (direction) {
  case GAME_DIR_LEFT:
    *row = line;
    *col = index;
    break;
  case GAME_DIR_RIGHT:
    *row = line;
    *col = (uint8_t)(GAME2048_SIZE - 1U - index);
    break;
  case GAME_DIR_UP:
    *row = index;
    *col = line;
    break;
  default:
    *row = (uint8_t)(GAME2048_SIZE - 1U - index);
    *col = line;
    break;
  }
}

static void add_motion(GameMoveResult *result, GameDirection direction,
                       uint8_t line, uint8_t source_index,
                       uint8_t destination_index, uint8_t exponent)
{
  if (result->motion_count >= (GAME2048_SIZE * GAME2048_SIZE)) {
    return;
  }

  GameMotion *motion = &result->motions[result->motion_count++];
  coordinate(direction, line, source_index, &motion->from_row, &motion->from_col);
  coordinate(direction, line, destination_index, &motion->to_row, &motion->to_col);
  motion->exponent = exponent;
}

static bool spawn_tile(Game2048 *game, GameRandomFunction random_function,
                       GameMoveResult *result)
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
    return false;
  }

  uint8_t selected = (uint8_t)(random_function() % count);
  uint8_t exponent = ((random_function() % 10U) == 0U) ? 2U : 1U;
  uint8_t row = rows[selected];
  uint8_t col = cols[selected];
  game->cells[row][col] = exponent;

  if (result != NULL) {
    result->spawned = true;
    result->spawn_row = row;
    result->spawn_col = col;
    result->spawn_exponent = exponent;
  }
  return true;
}

void Game2048_New(Game2048 *game, GameRandomFunction random_function)
{
  memset(game, 0, sizeof(*game));
  (void)spawn_tile(game, random_function, NULL);
  (void)spawn_tile(game, random_function, NULL);
}

bool Game2048_Move(Game2048 *game, GameDirection direction,
                   GameRandomFunction random_function, GameMoveResult *result)
{
  uint8_t original[GAME2048_SIZE][GAME2048_SIZE];
  memcpy(original, game->cells, sizeof(original));
  memset(result, 0, sizeof(*result));

  for (uint8_t line = 0U; line < GAME2048_SIZE; ++line) {
    uint8_t values[GAME2048_SIZE] = {0U};
    uint8_t sources[GAME2048_SIZE] = {0U};
    uint8_t count = 0U;

    for (uint8_t index = 0U; index < GAME2048_SIZE; ++index) {
      uint8_t row;
      uint8_t col;
      coordinate(direction, line, index, &row, &col);
      if (game->cells[row][col] != 0U) {
        values[count] = game->cells[row][col];
        sources[count] = index;
        ++count;
      }
      game->cells[row][col] = 0U;
    }

    uint8_t source = 0U;
    uint8_t destination = 0U;
    while (source < count) {
      uint8_t row;
      uint8_t col;
      coordinate(direction, line, destination, &row, &col);

      if ((source + 1U < count) && (values[source] == values[source + 1U])) {
        uint8_t merged = (uint8_t)(values[source] + 1U);
        game->cells[row][col] = merged;
        add_motion(result, direction, line, sources[source], destination,
                   values[source]);
        add_motion(result, direction, line, sources[source + 1U], destination,
                   values[source + 1U]);
        if (merged < 32U) {
          result->score_delta += (1UL << merged);
        }
        source = (uint8_t)(source + 2U);
      } else {
        game->cells[row][col] = values[source];
        add_motion(result, direction, line, sources[source], destination,
                   values[source]);
        ++source;
      }
      ++destination;
    }
  }

  result->changed = (memcmp(original, game->cells, sizeof(original)) != 0);
  if (!result->changed) {
    result->motion_count = 0U;
    result->score_delta = 0U;
    return false;
  }

  game->score += result->score_delta;
  (void)spawn_tile(game, random_function, result);
  return true;
}
