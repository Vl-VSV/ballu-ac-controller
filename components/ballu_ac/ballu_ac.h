#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace ballu_ac {

// Формат UART-пакетов (заголовок, размеры, контрольная сумма) взят из
// проекта tclac (https://github.com/I-am-nightingale/tclac,
// components/tclac/tclac.h и tclac.cpp) — это reverse-engineered протокол,
// официальной документации от TCL/Ballu не существует. Значения здесь —
// прямая копия констант оттуда, а не новое исследование протокола.
static const uint8_t FRAME_HEADER_0 = 0xBB;
static const uint8_t FRAME_HEADER_1 = 0x00;
static const uint8_t FRAME_HEADER_2 = 0x01;

static const size_t RX_FRAME_SIZE = 61;
static const size_t TX_FRAME_SIZE = 38;

class BalluAcClimate : public climate::Climate, public uart::UARTDevice, public PollingComponent {
 public:
  BalluAcClimate() : PollingComponent(5000) {}

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  uint8_t rx_buffer_[RX_FRAME_SIZE];
  size_t rx_pos_{0};
};

}  // namespace ballu_ac
}  // namespace esphome
