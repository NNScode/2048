#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "main.h"

#include <stdbool.h>

typedef enum {
  INPUT_NONE = 0,
  INPUT_LEFT,
  INPUT_RIGHT,
  INPUT_UP,
  INPUT_DOWN,
  INPUT_SHORT_PRESS,
  INPUT_LONG_PRESS
} InputEvent;

bool Joystick_Init(ADC_HandleTypeDef *adc);
bool Joystick_Calibrate(void);
InputEvent Joystick_Poll(void);

#endif
