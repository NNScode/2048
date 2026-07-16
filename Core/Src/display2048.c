#include "display2048.h"

#include "main.h"
#include "stm32f429i_discovery_lcd.h"

#include <stdio.h>
#include <string.h>

extern LTDC_HandleTypeDef LtdcHandler;

#define LCD_LAYER_INDEX 1U
#define LCD_FRONT_BUFFER LCD_FRAME_BUFFER
#define LCD_BACK_BUFFER (LCD_FRAME_BUFFER + BUFFER_OFFSET)
#define LCD_SWAP_TIMEOUT_MS 50U
#define LCD_BACKGROUND_COLOR 0xFFFAF8EFUL
#define GRID_PANEL_X 8U
#define GRID_PANEL_Y 78U
#define GRID_PANEL_SIZE 224U
#define GRID_X 12U
#define GRID_Y 82U
#define TILE_SIZE 50U
#define TILE_GAP 4U

static uint32_t lcd_draw_buffer;

static void present(void)
{
  LtdcHandler.Instance->SRCR = LTDC_SRCR_VBR;
  uint32_t started = HAL_GetTick();
  while ((LtdcHandler.Instance->SRCR & LTDC_SRCR_VBR) != 0U) {
    if ((HAL_GetTick() - started) >= LCD_SWAP_TIMEOUT_MS) {
      Error_Handler();
    }
  }
  lcd_draw_buffer = (lcd_draw_buffer == LCD_FRONT_BUFFER)
                        ? LCD_BACK_BUFFER
                        : LCD_FRONT_BUFFER;
  BSP_LCD_SetLayerAddress_NoReload(LCD_LAYER_INDEX, lcd_draw_buffer);
}

static uint32_t tile_color(uint8_t exponent)
{
  static const uint32_t colors[] = {
      0xFFCDC1B4UL, 0xFFEEE4DAUL, 0xFFEDE0C8UL, 0xFFF2B179UL,
      0xFFF59563UL, 0xFFF67C5FUL, 0xFFF65E3BUL, 0xFFEDCF72UL,
      0xFFEDCC61UL, 0xFFEDC850UL, 0xFFEDC53FUL, 0xFFEDC22EUL};
  if (exponent < (sizeof(colors) / sizeof(colors[0]))) {
    return colors[exponent];
  }
  return 0xFF3C3A32UL;
}

static uint16_t tile_x(uint8_t col)
{
  return (uint16_t)(GRID_X + col * (TILE_SIZE + TILE_GAP));
}

static uint16_t tile_y(uint8_t row)
{
  return (uint16_t)(GRID_Y + row * (TILE_SIZE + TILE_GAP));
}

static void draw_header(const Game2048 *game, uint32_t high_score)
{
  char text[40];
  BSP_LCD_SetTextColor(LCD_BACKGROUND_COLOR);
  BSP_LCD_FillRect(0U, 0U, 240U, 74U);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_SetBackColor(LCD_BACKGROUND_COLOR);
  BSP_LCD_SetTextColor(0xFF776E65UL);
  BSP_LCD_DisplayStringAt(8U, 8U, (uint8_t *)"2048", LEFT_MODE);
  BSP_LCD_SetFont(&Font16);
  (void)snprintf(text, sizeof(text), "SCORE %lu", (unsigned long)game->score);
  BSP_LCD_DisplayStringAt(90U, 8U, (uint8_t *)text, LEFT_MODE);
  (void)snprintf(text, sizeof(text), "BEST  %lu", (unsigned long)high_score);
  BSP_LCD_DisplayStringAt(90U, 30U, (uint8_t *)text, LEFT_MODE);
}

static void draw_empty_grid(void)
{
  BSP_LCD_SetTextColor(0xFFBBADA0UL);
  BSP_LCD_FillRect(GRID_PANEL_X, GRID_PANEL_Y, GRID_PANEL_SIZE, GRID_PANEL_SIZE);
  BSP_LCD_SetTextColor(tile_color(0U));
  for (uint8_t row = 0U; row < GAME2048_SIZE; ++row) {
    for (uint8_t col = 0U; col < GAME2048_SIZE; ++col) {
      BSP_LCD_FillRect(tile_x(col), tile_y(row), TILE_SIZE, TILE_SIZE);
    }
  }
}

static void draw_tile(uint16_t x, uint16_t y, uint8_t exponent,
                      uint8_t scale_percent)
{
  if ((exponent == 0U) || (scale_percent == 0U)) {
    return;
  }
  uint16_t size = (uint16_t)((TILE_SIZE * scale_percent) / 100U);
  if (size < 4U) {
    size = 4U;
  }
  uint16_t offset = (uint16_t)((TILE_SIZE - size) / 2U);
  x = (uint16_t)(x + offset);
  y = (uint16_t)(y + offset);

  uint32_t background = tile_color(exponent);
  BSP_LCD_SetTextColor(background);
  BSP_LCD_FillRect(x, y, size, size);
  if (scale_percent < 75U) {
    return;
  }

  uint32_t value = (exponent < 31U) ? (1UL << exponent) : 0U;
  char text[12];
  (void)snprintf(text, sizeof(text), "%lu", (unsigned long)value);
  size_t digits = strlen(text);
  if (digits <= 2U) {
    BSP_LCD_SetFont(&Font24);
  } else if (digits <= 3U) {
    BSP_LCD_SetFont(&Font20);
  } else if (digits <= 4U) {
    BSP_LCD_SetFont(&Font16);
  } else {
    BSP_LCD_SetFont(&Font12);
  }
  BSP_LCD_SetBackColor(background);
  BSP_LCD_SetTextColor((exponent <= 2U) ? 0xFF776E65UL : LCD_COLOR_WHITE);
  uint16_t text_width = (uint16_t)(digits * BSP_LCD_GetFont()->Width);
  uint16_t text_x = (uint16_t)(x + (size - text_width) / 2U);
  uint16_t text_y =
      (uint16_t)(y + (size - BSP_LCD_GetFont()->Height) / 2U);
  BSP_LCD_DisplayStringAt(text_x, text_y, (uint8_t *)text, LEFT_MODE);
}

static void draw_board_without_present(const Game2048 *game,
                                       const GameMoveResult *move,
                                       uint32_t high_score, bool skip_spawn)
{
  BSP_LCD_Clear(LCD_BACKGROUND_COLOR);
  draw_header(game, high_score);
  draw_empty_grid();
  for (uint8_t row = 0U; row < GAME2048_SIZE; ++row) {
    for (uint8_t col = 0U; col < GAME2048_SIZE; ++col) {
      if (skip_spawn && (move != NULL) && move->spawned &&
          (row == move->spawn_row) && (col == move->spawn_col)) {
        continue;
      }
      draw_tile(tile_x(col), tile_y(row), game->cells[row][col], 100U);
    }
  }
}

bool Display2048_Init(void)
{
  if (BSP_LCD_Init() != LCD_OK) {
    return false;
  }
  BSP_LCD_LayerDefaultInit(LCD_LAYER_INDEX, LCD_FRONT_BUFFER);
  BSP_LCD_SelectLayer(LCD_LAYER_INDEX);
  BSP_LCD_Clear(LCD_BACKGROUND_COLOR);
  BSP_LCD_DisplayOn();

  lcd_draw_buffer = LCD_BACK_BUFFER;
  BSP_LCD_SetLayerAddress_NoReload(LCD_LAYER_INDEX, lcd_draw_buffer);
  BSP_LCD_Clear(LCD_BACKGROUND_COLOR);
  present();
  return true;
}

void Display2048_ShowSplash(void)
{
  BSP_LCD_Clear(LCD_BACKGROUND_COLOR);
  BSP_LCD_SetBackColor(LCD_BACKGROUND_COLOR);
  BSP_LCD_SetTextColor(0xFF776E65UL);
  BSP_LCD_SetFont(&Font24);
  BSP_LCD_DisplayStringAt(0U, 126U, (uint8_t *)"2048", CENTER_MODE);
  present();
}

void Display2048_DrawBoard(const Game2048 *game, uint32_t high_score)
{
  draw_board_without_present(game, NULL, high_score, false);
  present();
}

void Display2048_DrawMoveAnimation(const Game2048 *game,
                                   const GameMoveResult *move,
                                   uint32_t high_score, uint8_t progress)
{
  BSP_LCD_Clear(LCD_BACKGROUND_COLOR);
  draw_header(game, high_score);
  draw_empty_grid();
  for (uint8_t i = 0U; i < move->motion_count; ++i) {
    const GameMotion *motion = &move->motions[i];
    int32_t from_x = tile_x(motion->from_col);
    int32_t from_y = tile_y(motion->from_row);
    int32_t to_x = tile_x(motion->to_col);
    int32_t to_y = tile_y(motion->to_row);
    uint16_t x = (uint16_t)(from_x + ((to_x - from_x) * progress) / 255);
    uint16_t y = (uint16_t)(from_y + ((to_y - from_y) * progress) / 255);
    draw_tile(x, y, motion->exponent, 100U);
  }
  present();
}

void Display2048_DrawSpawnAnimation(const Game2048 *game,
                                    const GameMoveResult *move,
                                    uint32_t high_score, uint8_t progress)
{
  draw_board_without_present(game, move, high_score, true);
  if (move->spawned) {
    uint8_t scale = (uint8_t)(45U + ((uint16_t)progress * 55U) / 255U);
    draw_tile(tile_x(move->spawn_col), tile_y(move->spawn_row),
              move->spawn_exponent, scale);
  }
  present();
}

void Display2048_DrawOverlay(const Game2048 *game, uint32_t high_score,
                             const char *title, const char *line1,
                             const char *line2)
{
  draw_board_without_present(game, NULL, high_score, false);
  BSP_LCD_SetTextColor(0xFFEEE4DAUL);
  BSP_LCD_FillRect(20U, 118U, 200U, 98U);
  BSP_LCD_SetBackColor(0xFFEEE4DAUL);
  BSP_LCD_SetTextColor(0xFF776E65UL);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(0U, 128U, (uint8_t *)title, CENTER_MODE);
  BSP_LCD_DisplayStringAt(0U, 166U, (uint8_t *)line1, CENTER_MODE);
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_DisplayStringAt(0U, 192U, (uint8_t *)line2, CENTER_MODE);
  present();
}
