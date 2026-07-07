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
    ESP_LOGD(TAG, "RX byte: 0x%02X", byte);
  }
}

void BalluAcClimate::update() {
  ESP_LOGD(TAG, "poll tick");
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
