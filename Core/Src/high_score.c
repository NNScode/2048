#include "high_score.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash_ex.h"

/* The linker script reserves STM32F429 flash sector 23 for these records. */
#define HIGHSCORE_ADDRESS 0x081E0000UL
#define HIGHSCORE_END 0x08200000UL
#define HIGHSCORE_MAGIC 0x32303438UL
#define HIGHSCORE_VERSION 1UL
#define HIGHSCORE_SAVE_DELAY_MS 5000U

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t sequence;
  uint32_t score;
  uint32_t crc;
} HighScoreRecord;

static uint32_t high_score;
static uint32_t high_score_sequence;
static uint32_t high_score_changed_at;
static bool high_score_pending;

static uint32_t crc32_bytes(const uint8_t *data, uint32_t length)
{
  uint32_t crc = 0xFFFFFFFFUL;
  for (uint32_t i = 0U; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
      uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
      crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
  }
  return ~crc;
}

static bool record_is_blank(const HighScoreRecord *record)
{
  const uint32_t *words = (const uint32_t *)record;
  for (uint32_t i = 0U; i < (sizeof(*record) / sizeof(uint32_t)); ++i) {
    if (words[i] != 0xFFFFFFFFUL) {
      return false;
    }
  }
  return true;
}

static bool record_is_valid(const HighScoreRecord *record)
{
  if ((record->magic != HIGHSCORE_MAGIC) ||
      (record->version != HIGHSCORE_VERSION)) {
    return false;
  }
  return crc32_bytes((const uint8_t *)record,
                     sizeof(*record) - sizeof(record->crc)) == record->crc;
}

static bool save_record(uint32_t score)
{
  uint32_t write_address = HIGHSCORE_END;
  for (uint32_t address = HIGHSCORE_ADDRESS;
       address <= (HIGHSCORE_END - sizeof(HighScoreRecord));
       address += sizeof(HighScoreRecord)) {
    if (record_is_blank((const HighScoreRecord *)address)) {
      write_address = address;
      break;
    }
  }

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return false;
  }
  if (write_address == HIGHSCORE_END) {
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sector_error = 0U;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_23;
    erase.NbSectors = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK) {
      (void)HAL_FLASH_Lock();
      return false;
    }
    write_address = HIGHSCORE_ADDRESS;
  }

  HighScoreRecord record = {
      .magic = HIGHSCORE_MAGIC,
      .version = HIGHSCORE_VERSION,
      .sequence = high_score_sequence + 1U,
      .score = score,
      .crc = 0U};
  record.crc = crc32_bytes((const uint8_t *)&record,
                           sizeof(record) - sizeof(record.crc));

  const uint32_t *words = (const uint32_t *)&record;
  bool success = true;
  for (uint32_t i = 0U; i < (sizeof(record) / sizeof(uint32_t)); ++i) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                          write_address + i * sizeof(uint32_t),
                          words[i]) != HAL_OK) {
      success = false;
      break;
    }
  }
  (void)HAL_FLASH_Lock();
  if (success) {
    high_score_sequence = record.sequence;
  }
  return success;
}

void HighScore_Init(void)
{
  high_score = 0U;
  high_score_sequence = 0U;
  high_score_pending = false;
  for (uint32_t address = HIGHSCORE_ADDRESS;
       address <= (HIGHSCORE_END - sizeof(HighScoreRecord));
       address += sizeof(HighScoreRecord)) {
    const HighScoreRecord *record = (const HighScoreRecord *)address;
    if (record_is_blank(record)) {
      break;
    }
    if (record_is_valid(record) && (record->sequence >= high_score_sequence)) {
      high_score_sequence = record->sequence;
      high_score = record->score;
    }
  }
}

uint32_t HighScore_Get(void)
{
  return high_score;
}

void HighScore_Update(uint32_t score, uint32_t now)
{
  if (score > high_score) {
    high_score = score;
    high_score_pending = true;
    high_score_changed_at = now;
  }
}

void HighScore_Process(uint32_t now, bool force)
{
  if (!high_score_pending ||
      (!force && ((now - high_score_changed_at) < HIGHSCORE_SAVE_DELAY_MS))) {
    return;
  }
  if (save_record(high_score)) {
    high_score_pending = false;
  } else {
    high_score_changed_at = now;
  }
}
