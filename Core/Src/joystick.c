#include "joystick.h"

#include <stdint.h>

/* Set these to 1 if the physical joystick is mounted in another orientation. */
#define JOY_SWAP_XY 0
#define JOY_INVERT_X 0
#define JOY_INVERT_Y 0

#define JOY_SAMPLE_PERIOD_MS 10U
#define JOY_ADC_TIMEOUT_MS 5U
#define JOY_MIN_TRIGGER_DELTA 450
#define JOY_MAX_TRIGGER_DELTA 900
#define JOY_MIN_RELEASE_DELTA 180
#define JOY_DEBOUNCE_MS 25U
#define JOY_LONG_PRESS_MS 800U
#define JOY_REPEAT_DELAY_MS 350U
#define JOY_REPEAT_PERIOD_MS 160U
#define JOY_DISCONNECT_MS 500U
#define JOY_RAIL_LOW 16U
#define JOY_RAIL_HIGH 4079U

typedef struct {
  int32_t center_x;
  int32_t center_y;
  int32_t filtered_x;
  int32_t filtered_y;
  int32_t trigger_x;
  int32_t trigger_y;
  int32_t release_x;
  int32_t release_y;
  bool direction_armed;
  InputEvent held_direction;
  uint32_t next_repeat;
  bool switch_candidate;
  bool switch_stable;
  bool long_press_sent;
  bool connected;
  bool invalid_tracking;
  uint32_t candidate_since;
  uint32_t press_started;
  uint32_t last_axis_sample;
  uint32_t invalid_since;
} JoystickState;

static ADC_HandleTypeDef *joystick_adc;
static JoystickState joystick;
static volatile uint16_t adc_samples[2] __attribute__((aligned(4)));

static bool adc_read_channel(uint32_t channel, uint16_t *value)
{
  ADC_ChannelConfTypeDef config = {0};
  config.Channel = channel;
  config.Rank = 1U;
  config.SamplingTime = ADC_SAMPLETIME_144CYCLES;

  if ((HAL_ADC_ConfigChannel(joystick_adc, &config) != HAL_OK) ||
      (HAL_ADC_Start(joystick_adc) != HAL_OK)) {
    return false;
  }
  if (HAL_ADC_PollForConversion(joystick_adc, JOY_ADC_TIMEOUT_MS) != HAL_OK) {
    (void)HAL_ADC_Stop(joystick_adc);
    return false;
  }

  *value = (uint16_t)HAL_ADC_GetValue(joystick_adc);
  (void)HAL_ADC_Stop(joystick_adc);
  return true;
}

static bool adc_refresh(void)
{
  uint16_t x;
  uint16_t y;
  if (!adc_read_channel(ADC_CHANNEL_5, &x) ||
      !adc_read_channel(ADC_CHANNEL_13, &y)) {
    return false;
  }
  adc_samples[0] = x;
  adc_samples[1] = y;
  return true;
}

static int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum)
{
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

static int32_t adaptive_trigger_delta(int32_t center, int32_t noise)
{
  int32_t negative_range = center;
  int32_t positive_range = 4095 - center;
  int32_t usable_range = (negative_range < positive_range)
                             ? negative_range
                             : positive_range;
  int32_t range_threshold = (usable_range * 30) / 100;
  int32_t noise_threshold = noise * 8 + 120;
  int32_t threshold = (range_threshold > noise_threshold)
                          ? range_threshold
                          : noise_threshold;
  return clamp_i32(threshold, JOY_MIN_TRIGGER_DELTA, JOY_MAX_TRIGGER_DELTA);
}

static void update_connection(uint32_t now)
{
  bool both_low = (adc_samples[0] <= JOY_RAIL_LOW) &&
                  (adc_samples[1] <= JOY_RAIL_LOW);
  bool both_high = (adc_samples[0] >= JOY_RAIL_HIGH) &&
                   (adc_samples[1] >= JOY_RAIL_HIGH);
  bool invalid = both_low || both_high;

  if (!invalid) {
    joystick.invalid_tracking = false;
    joystick.connected = true;
    return;
  }
  if (!joystick.invalid_tracking) {
    joystick.invalid_tracking = true;
    joystick.invalid_since = now;
  } else if ((now - joystick.invalid_since) >= JOY_DISCONNECT_MS) {
    joystick.connected = false;
    joystick.direction_armed = true;
    joystick.held_direction = INPUT_NONE;
  }
}

bool Joystick_Init(ADC_HandleTypeDef *adc)
{
  joystick_adc = adc;
  return (joystick_adc != NULL) && adc_refresh();
}

bool Joystick_Calibrate(void)
{
  uint32_t sum_x = 0U;
  uint32_t sum_y = 0U;
  uint16_t minimum_x = 4095U;
  uint16_t maximum_x = 0U;
  uint16_t minimum_y = 4095U;
  uint16_t maximum_y = 0U;

  HAL_Delay(300U);
  for (uint32_t i = 0U; i < 64U; ++i) {
    if (!adc_refresh()) {
      return false;
    }
    sum_x += adc_samples[0];
    sum_y += adc_samples[1];
    if (adc_samples[0] < minimum_x) minimum_x = adc_samples[0];
    if (adc_samples[0] > maximum_x) maximum_x = adc_samples[0];
    if (adc_samples[1] < minimum_y) minimum_y = adc_samples[1];
    if (adc_samples[1] > maximum_y) maximum_y = adc_samples[1];
    HAL_Delay(5U);
  }

  joystick.center_x = (int32_t)(sum_x / 64U);
  joystick.center_y = (int32_t)(sum_y / 64U);
  joystick.filtered_x = joystick.center_x;
  joystick.filtered_y = joystick.center_y;
  joystick.trigger_x = adaptive_trigger_delta(
      joystick.center_x, (int32_t)(maximum_x - minimum_x));
  joystick.trigger_y = adaptive_trigger_delta(
      joystick.center_y, (int32_t)(maximum_y - minimum_y));
  joystick.release_x = clamp_i32(joystick.trigger_x / 2,
                                 JOY_MIN_RELEASE_DELTA,
                                 JOY_MAX_TRIGGER_DELTA / 2);
  joystick.release_y = clamp_i32(joystick.trigger_y / 2,
                                 JOY_MIN_RELEASE_DELTA,
                                 JOY_MAX_TRIGGER_DELTA / 2);
  joystick.direction_armed = true;
  joystick.held_direction = INPUT_NONE;
  joystick.connected = true;
  joystick.invalid_tracking = false;

  bool pressed =
      (HAL_GPIO_ReadPin(JOY_SW_GPIO_Port, JOY_SW_Pin) == GPIO_PIN_RESET);
  joystick.switch_candidate = pressed;
  joystick.switch_stable = pressed;
  joystick.candidate_since = HAL_GetTick();
  joystick.press_started = HAL_GetTick();
  joystick.long_press_sent = false;
  joystick.last_axis_sample = HAL_GetTick();
  return true;
}

InputEvent Joystick_Poll(void)
{
  uint32_t now = HAL_GetTick();
  bool raw_pressed =
      (HAL_GPIO_ReadPin(JOY_SW_GPIO_Port, JOY_SW_Pin) == GPIO_PIN_RESET);

  if (raw_pressed != joystick.switch_candidate) {
    joystick.switch_candidate = raw_pressed;
    joystick.candidate_since = now;
  }
  if ((joystick.switch_stable != joystick.switch_candidate) &&
      ((now - joystick.candidate_since) >= JOY_DEBOUNCE_MS)) {
    joystick.switch_stable = joystick.switch_candidate;
    if (joystick.switch_stable) {
      joystick.press_started = now;
      joystick.long_press_sent = false;
    } else if (!joystick.long_press_sent) {
      return INPUT_SHORT_PRESS;
    }
  }
  if (joystick.switch_stable && !joystick.long_press_sent &&
      ((now - joystick.press_started) >= JOY_LONG_PRESS_MS)) {
    joystick.long_press_sent = true;
    return INPUT_LONG_PRESS;
  }
  if ((now - joystick.last_axis_sample) < JOY_SAMPLE_PERIOD_MS) {
    return INPUT_NONE;
  }

  joystick.last_axis_sample = now;
  if (!adc_refresh()) {
    joystick.connected = false;
    return INPUT_NONE;
  }
  update_connection(now);
  if (!joystick.connected) {
    return INPUT_NONE;
  }

  joystick.filtered_x += ((int32_t)adc_samples[0] - joystick.filtered_x) / 4;
  joystick.filtered_y += ((int32_t)adc_samples[1] - joystick.filtered_y) / 4;
  int32_t dx = joystick.filtered_x - joystick.center_x;
  int32_t dy = joystick.filtered_y - joystick.center_y;
  int32_t physical_absolute_x = (dx < 0) ? -dx : dx;
  int32_t physical_absolute_y = (dy < 0) ? -dy : dy;
  if ((physical_absolute_x < (joystick.release_x / 2)) &&
      (physical_absolute_y < (joystick.release_y / 2))) {
    joystick.center_x += (joystick.filtered_x - joystick.center_x) / 64;
    joystick.center_y += (joystick.filtered_y - joystick.center_y) / 64;
    dx = joystick.filtered_x - joystick.center_x;
    dy = joystick.filtered_y - joystick.center_y;
  }

  int32_t trigger_x = joystick.trigger_x;
  int32_t trigger_y = joystick.trigger_y;
  int32_t release_x = joystick.release_x;
  int32_t release_y = joystick.release_y;
#if JOY_SWAP_XY
  int32_t temporary = dx;
  dx = dy;
  dy = temporary;
  temporary = trigger_x;
  trigger_x = trigger_y;
  trigger_y = temporary;
  temporary = release_x;
  release_x = release_y;
  release_y = temporary;
#endif
#if JOY_INVERT_X
  dx = -dx;
#endif
#if JOY_INVERT_Y
  dy = -dy;
#endif

  int32_t absolute_x = (dx < 0) ? -dx : dx;
  int32_t absolute_y = (dy < 0) ? -dy : dy;
  if ((absolute_x < release_x) && (absolute_y < release_y)) {
    joystick.direction_armed = true;
    joystick.held_direction = INPUT_NONE;
    return INPUT_NONE;
  }

  InputEvent direction = INPUT_NONE;
  if ((absolute_x >= trigger_x) || (absolute_y >= trigger_y)) {
    if ((absolute_x * trigger_y) > (absolute_y * trigger_x)) {
      direction = (dx < 0) ? INPUT_LEFT : INPUT_RIGHT;
    } else {
      direction = (dy < 0) ? INPUT_UP : INPUT_DOWN;
    }
  }
  if (direction == INPUT_NONE) {
    return INPUT_NONE;
  }
  if (joystick.direction_armed || (direction != joystick.held_direction)) {
    joystick.direction_armed = false;
    joystick.held_direction = direction;
    joystick.next_repeat = now + JOY_REPEAT_DELAY_MS;
    return direction;
  }
  if ((int32_t)(now - joystick.next_repeat) >= 0) {
    joystick.next_repeat = now + JOY_REPEAT_PERIOD_MS;
    return direction;
  }
  return INPUT_NONE;
}
