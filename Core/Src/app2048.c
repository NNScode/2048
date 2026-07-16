#include "app2048.h"

#include "display2048.h"
#include "game2048.h"
#include "high_score.h"
#include "joystick.h"
#include "main.h"

#include <stdbool.h>
#include <string.h>

extern ADC_HandleTypeDef hadc1;
extern RNG_HandleTypeDef hrng;

#define MOVE_ANIMATION_MS 100U
#define SPAWN_ANIMATION_MS 80U
#define FRAME_PERIOD_MS 16U

typedef enum {
  APP_STATE_PLAYING = 0,
  APP_STATE_MOVE_ANIMATION,
  APP_STATE_SPAWN_ANIMATION,
  APP_STATE_WIN,
  APP_STATE_GAME_OVER,
  APP_STATE_RESTART_CONFIRM
} AppState;

static Game2048 game;
static Game2048 undo_game;
static GameMoveResult active_move;
static AppState app_state;
static AppState state_before_confirm;
static uint32_t state_started;
static uint32_t last_frame;
static bool undo_available;
static uint32_t fallback_random = 0x6D2B79F5UL;

static uint32_t random_u32(void)
{
  uint32_t value;
  if (HAL_RNG_GenerateRandomNumber(&hrng, &value) == HAL_OK) {
    return value;
  }
  fallback_random ^= fallback_random << 13;
  fallback_random ^= fallback_random >> 17;
  fallback_random ^= fallback_random << 5;
  return fallback_random;
}

static bool event_to_direction(InputEvent event, GameDirection *direction)
{
  switch (event) {
  case INPUT_LEFT: *direction = GAME_DIR_LEFT; return true;
  case INPUT_RIGHT: *direction = GAME_DIR_RIGHT; return true;
  case INPUT_UP: *direction = GAME_DIR_UP; return true;
  case INPUT_DOWN: *direction = GAME_DIR_DOWN; return true;
  default: return false;
  }
}

static void draw_board(void)
{
  Display2048_DrawBoard(&game, HighScore_Get());
}

static void draw_overlay(const char *title, const char *line1,
                         const char *line2)
{
  Display2048_DrawOverlay(&game, HighScore_Get(), title, line1, line2);
}

static void start_new_game(void)
{
  uint32_t now = HAL_GetTick();
  HighScore_Process(now, true);
  Game2048_New(&game, random_u32);
  memset(&active_move, 0, sizeof(active_move));
  undo_available = false;
  app_state = APP_STATE_PLAYING;
  state_started = now;
  draw_board();
}

static void undo_last_move(void)
{
  if (!undo_available) {
    return;
  }
  game = undo_game;
  memset(&active_move, 0, sizeof(active_move));
  undo_available = false;
  app_state = APP_STATE_PLAYING;
  state_started = HAL_GetTick();
  draw_board();
}

static void finish_spawn_animation(void)
{
  GameStatus status = Game2048_GetStatus(&game);
  if (status == GAME_STATUS_WON) {
    app_state = APP_STATE_WIN;
    draw_overlay("You reached 2048!", "Click to continue",
                 "Hold click to restart");
  } else if (status == GAME_STATUS_OVER) {
    app_state = APP_STATE_GAME_OVER;
    HighScore_Process(HAL_GetTick(), true);
    draw_overlay("Game over", "Click to restart", "No moves remain");
  } else {
    app_state = APP_STATE_PLAYING;
    draw_board();
  }
}

void App2048_Init(void)
{
  if (!Joystick_Init(&hadc1) || !Display2048_Init()) {
    Error_Handler();
  }
  HighScore_Init();
  fallback_random ^= HAL_GetTick();
  Display2048_ShowSplash();
  if (!Joystick_Calibrate()) {
    Error_Handler();
  }
  start_new_game();
}

void App2048_Process(void)
{
  uint32_t now = HAL_GetTick();
  InputEvent event = Joystick_Poll();
  GameDirection direction;
  HighScore_Process(now, false);

  switch (app_state) {
  case APP_STATE_PLAYING:
    if (event_to_direction(event, &direction)) {
      Game2048 previous_game = game;
      if (Game2048_Move(&game, direction, random_u32, &active_move)) {
        undo_game = previous_game;
        undo_available = true;
        HighScore_Update(game.score, now);
        app_state = APP_STATE_MOVE_ANIMATION;
        state_started = now;
        last_frame = 0U;
      }
    } else if (event == INPUT_SHORT_PRESS) {
      undo_last_move();
    } else if (event == INPUT_LONG_PRESS) {
      state_before_confirm = app_state;
      app_state = APP_STATE_RESTART_CONFIRM;
      draw_overlay("Restart game?", "Click to confirm",
                   "Move stick to cancel");
    }
    break;

  case APP_STATE_MOVE_ANIMATION: {
    uint32_t elapsed = now - state_started;
    if ((last_frame == 0U) || ((now - last_frame) >= FRAME_PERIOD_MS)) {
      uint8_t progress = (elapsed >= MOVE_ANIMATION_MS)
                             ? 255U
                             : (uint8_t)((elapsed * 255U) / MOVE_ANIMATION_MS);
      Display2048_DrawMoveAnimation(&game, &active_move, HighScore_Get(),
                                    progress);
      last_frame = now;
    }
    if (elapsed >= MOVE_ANIMATION_MS) {
      app_state = APP_STATE_SPAWN_ANIMATION;
      state_started = now;
      last_frame = 0U;
    }
    break;
  }

  case APP_STATE_SPAWN_ANIMATION: {
    uint32_t elapsed = now - state_started;
    if ((last_frame == 0U) || ((now - last_frame) >= FRAME_PERIOD_MS)) {
      uint8_t progress = (elapsed >= SPAWN_ANIMATION_MS)
                             ? 255U
                             : (uint8_t)((elapsed * 255U) / SPAWN_ANIMATION_MS);
      Display2048_DrawSpawnAnimation(&game, &active_move, HighScore_Get(),
                                     progress);
      last_frame = now;
    }
    if (elapsed >= SPAWN_ANIMATION_MS) {
      finish_spawn_animation();
    }
    break;
  }

  case APP_STATE_WIN:
    if (event == INPUT_SHORT_PRESS) {
      Game2048_ContinueAfterWin(&game);
      if (Game2048_GetStatus(&game) == GAME_STATUS_OVER) {
        app_state = APP_STATE_GAME_OVER;
        HighScore_Process(now, true);
        draw_overlay("Game over", "Click to restart", "No moves remain");
      } else {
        app_state = APP_STATE_PLAYING;
        draw_board();
      }
    } else if (event == INPUT_LONG_PRESS) {
      state_before_confirm = app_state;
      app_state = APP_STATE_RESTART_CONFIRM;
      draw_overlay("Restart game?", "Click to confirm",
                   "Move stick to cancel");
    }
    break;

  case APP_STATE_GAME_OVER:
    if ((event == INPUT_SHORT_PRESS) || (event == INPUT_LONG_PRESS)) {
      start_new_game();
    }
    break;

  case APP_STATE_RESTART_CONFIRM:
    if (event == INPUT_SHORT_PRESS) {
      start_new_game();
    } else if (event_to_direction(event, &direction)) {
      app_state = state_before_confirm;
      if (app_state == APP_STATE_WIN) {
        draw_overlay("You reached 2048!", "Click to continue",
                     "Hold click to restart");
      } else {
        draw_board();
      }
    }
    break;

  default:
    app_state = APP_STATE_PLAYING;
    draw_board();
    break;
  }
}
