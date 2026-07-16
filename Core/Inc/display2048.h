#ifndef DISPLAY2048_H
#define DISPLAY2048_H

#include "game2048.h"

#include <stdbool.h>
#include <stdint.h>

bool Display2048_Init(void);
void Display2048_ShowSplash(void);
void Display2048_DrawBoard(const Game2048 *game, uint32_t high_score);
void Display2048_DrawMoveAnimation(const Game2048 *game,
                                   const GameMoveResult *move,
                                   uint32_t high_score, uint8_t progress);
void Display2048_DrawSpawnAnimation(const Game2048 *game,
                                    const GameMoveResult *move,
                                    uint32_t high_score, uint8_t progress);
void Display2048_DrawOverlay(const Game2048 *game, uint32_t high_score,
                             const char *title, const char *line1,
                             const char *line2);

#endif
