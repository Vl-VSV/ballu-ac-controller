#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace ballu_ac {

// Формат UART-пакетов (размер фрейма, контрольная сумма) изначально взят из
// проекта tclac (https://github.com/I-am-nightingale/tclac,
// components/tclac/tclac.h и tclac.cpp) — это reverse-engineered протокол,
// официальной документации от TCL/Ballu не существует. Байты 1-2 заголовка на
// нашем железе идут в другом порядке (BB 01 00, не BB 00 01, как у tclac) —
// подтверждено захватом реальных пакетов, поэтому проверяем только маркер
// начала пакета (первый байт), не весь заголовок целиком.
static const uint8_t FRAME_START = 0xBB;

static const size_t RX_FRAME_SIZE = 61;
static const size_t TX_FRAME_SIZE = 38;

// Позиции и маски байт статуса подтверждены сравнением реальных RX-пакетов
// в выключенном/включенном состоянии на живом кондиционере (см. project memory
// ballu-ac-protocol-findings): байт 7 меняется на константу режима tclac
// (например MODE_COOL = 0x31) точно в момент включения с пульта; байт 8 даёт
// ровно установленную температуру по формуле (byte & 0x0F) + 16.
static const uint8_t MODE_POS = 7;
static const uint8_t MODE_MASK = 0b00111111;
static const uint8_t MODE_COOL = 0b00110001;
static const uint8_t MODE_HEAT = 0b00110100;
static const uint8_t MODE_DRY = 0b00110011;
static const uint8_t MODE_FAN_ONLY = 0b00110010;
static const uint8_t MODE_AUTO = 0b00110101;

static const uint8_t TARGET_TEMP_POS = 8;
static const uint8_t TARGET_TEMP_MASK = 0b00001111;
static const uint8_t TARGET_TEMP_BASE = 16;

static const uint8_t CURRENT_TEMP_POS_HI = 17;
static const uint8_t CURRENT_TEMP_POS_LO = 18;

// Скорость вентилятора: high nibble байта 8 (ниже TARGET_TEMP_MASK).
// Подтверждено пошаговым переключением каждой скорости на реальном пульте
// (см. project memory ballu-ac-protocol-findings): значения не идут по
// возрастанию мощности — это порядок, в котором кнопка на пульте их
// перебирает, а не физическая шкала. "Турбо" — ОТДЕЛЬНЫЙ флаг (byte7 бит
// 0x80), а не отдельный код здесь; при включённом турбо этот нибл совпадает
// со значением обычной высокой скорости ("5"), поэтому turbo и "5"
// сознательно отображаются в HA одинаково, как CLIMATE_FAN_HIGH.
static const uint8_t FAN_SPEED_MASK = 0b11110000;
static const uint8_t FAN_SPEED_AUTO = 0b10000000;    // код 0
static const uint8_t FAN_SPEED_QUIET = 0b10010000;   // код 1 (пульт: "1")
static const uint8_t FAN_SPEED_LOW = 0b10100000;     // код 2 (пульт: "3")
static const uint8_t FAN_SPEED_HIGH = 0b10110000;    // код 3 (пульт: "5"/турбо)
static const uint8_t FAN_SPEED_MEDIUM = 0b11000000;  // код 4 (пульт: "2")
static const uint8_t FAN_SPEED_FOCUS = 0b11010000;   // код 5 (пульт: "4")

static const uint8_t TURBO_POS = MODE_POS;
static const uint8_t TURBO_BIT = 0b10000000;

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
  void parse_status_frame_(const uint8_t *data);

  uint8_t rx_buffer_[RX_FRAME_SIZE];
  size_t rx_pos_{0};
};

}  // namespace ballu_ac
}  // namespace esphome
