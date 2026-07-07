#include "ballu_ac.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ballu_ac {

static const char *const TAG = "ballu_ac";

void BalluAcClimate::setup() {}

void BalluAcClimate::loop() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);

    if (this->rx_pos_ == 0 && byte != FRAME_START) {
      // Не начало пакета — игнорируем шум/рассинхронизацию.
      continue;
    }
    this->rx_buffer_[this->rx_pos_++] = byte;

    if (this->rx_pos_ == RX_FRAME_SIZE) {
      uint8_t checksum = 0;
      for (size_t i = 0; i < RX_FRAME_SIZE - 1; i++)
        checksum ^= this->rx_buffer_[i];

      if (checksum != this->rx_buffer_[RX_FRAME_SIZE - 1]) {
        ESP_LOGW(TAG, "RX checksum mismatch: computed 0x%02X, got 0x%02X", checksum,
                 this->rx_buffer_[RX_FRAME_SIZE - 1]);
      } else {
        this->parse_status_frame_(this->rx_buffer_);
      }
      this->rx_pos_ = 0;
    }
  }
}

void BalluAcClimate::parse_status_frame_(const uint8_t *data) {
  uint16_t raw_current_temp = (data[CURRENT_TEMP_POS_HI] << 8) | data[CURRENT_TEMP_POS_LO];
  this->current_temperature = ((raw_current_temp / 374.0f) - 32.0f) / 1.8f;

  this->target_temperature = (data[TARGET_TEMP_POS] & TARGET_TEMP_MASK) + TARGET_TEMP_BASE;

  switch (data[MODE_POS] & MODE_MASK) {
    case MODE_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      break;
    case MODE_HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      break;
    case MODE_DRY:
      this->mode = climate::CLIMATE_MODE_DRY;
      break;
    case MODE_FAN_ONLY:
      this->mode = climate::CLIMATE_MODE_FAN_ONLY;
      break;
    case MODE_AUTO:
      this->mode = climate::CLIMATE_MODE_AUTO;
      break;
    default:
      this->mode = climate::CLIMATE_MODE_OFF;
      break;
  }

  switch (data[TARGET_TEMP_POS] & FAN_SPEED_MASK) {
    case FAN_SPEED_QUIET:
      this->fan_mode = climate::CLIMATE_FAN_QUIET;
      break;
    case FAN_SPEED_LOW:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case FAN_SPEED_MEDIUM:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case FAN_SPEED_HIGH:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    case FAN_SPEED_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
    // Код 4 (пульт: "2", нибл 0xC) не отображается отдельным HA-режимом —
    // ближе всего к LOW, поэтому мапим сюда вместо AUTO.
    default:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
  }
  // Турбо — отдельный флаг поверх скорости "5" (тот же код, что и обычная
  // высокая скорость), сознательно отображаем турбо как ту же CLIMATE_FAN_HIGH.
  if (data[TURBO_POS] & TURBO_BIT)
    this->fan_mode = climate::CLIMATE_FAN_HIGH;

  char hex[RX_FRAME_SIZE * 3 + 1] = {0};
  for (size_t i = 0; i < RX_FRAME_SIZE; i++)
    sprintf(hex + i * 3, "%02X ", data[i]);
  ESP_LOGD(TAG, "status: mode_byte=0x%02X current=%.1f target=%.0f full=%s", data[MODE_POS],
           this->current_temperature, this->target_temperature, hex);
  this->publish_state();
}

void BalluAcClimate::update() {
  // Пакет опроса статуса — формат из tclac (components/tclac/tclac.h,
  // массив poll[]): BB 00 01, тип 04 (опрос), длина 02, данные 01 00,
  // контрольная сумма BD. Работает как есть: кондиционер отвечает валидным
  // RX-фреймом на этот TX-пакет несмотря на то, что RX-заголовок приходит в
  // другом порядке байт (BB 01 00) — TX и RX, видимо, не обязаны совпадать
  // побайтово в этой части, поэтому TX не менялся.
  static const uint8_t POLL[8] = {0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0xBD};
  ESP_LOGD(TAG, "sending poll request");
  this->write_array(POLL, sizeof(POLL));
}

void BalluAcClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "Ballu AC Climate");
}

climate::ClimateTraits BalluAcClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_AUTO,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_QUIET,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
  });
  traits.set_visual_min_temperature(16.0f);
  traits.set_visual_max_temperature(31.0f);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

void BalluAcClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value())
    this->mode = *call.get_mode();
  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();

  this->publish_state();
}

}  // namespace ballu_ac
}  // namespace esphome
