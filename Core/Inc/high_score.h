#ifndef HIGH_SCORE_H
#define HIGH_SCORE_H

#include <stdbool.h>
#include <stdint.h>

void HighScore_Init(void);
uint32_t HighScore_Get(void);
void HighScore_Update(uint32_t score, uint32_t now);
void HighScore_Process(uint32_t now, bool force);

#endif
