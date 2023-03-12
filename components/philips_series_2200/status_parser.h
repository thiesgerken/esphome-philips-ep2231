#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace philips_series_2200 {
enum BeverageLedStatus {
  OFF = 0,
  HALF_BRIGHTNESS,
  FULL_BRIGHTNESS,
  TWO_DRINKS,
};

enum SettingLedStatus {
  LEVEL_0 = 0,
  LEVEL_1,
  LEVEL_2,
  LEVEL_3,
};

enum StatusType {
  OVERALL = 0,
  LED_ESPRESSO,
  LED_HOT_WATER,
  LED_COFFEE,
  LED_CAPPUCCINO,
  LED_BEANS,
  LED_SIZE,
  LED_POWDER,
  LED_WATER_EMPTY,
  LED_WASTE_FULL,
  LED_START_STOP,
  LED_ERROR,
};

/**
 * @brief Reports status of the coffee machine
 */
class StatusParser {
public:
  /**
   * @brief Updates the status of this sensor based on the messages sent by the
   * mainboard
   */
  void update_status(uint8_t *data, size_t len);

  bool led_start_stop = false;
  bool led_waste_full = false;
  bool led_powder = false;
  bool led_water_empty = false;
  bool led_error = false;

  // TODO: add aqua clean & calcnclean

  BeverageLedStatus led_espresso = BeverageLedStatus::OFF;
  BeverageLedStatus led_hot_water = BeverageLedStatus::OFF;
  BeverageLedStatus led_coffee = BeverageLedStatus::OFF;
  BeverageLedStatus led_cappuccino = BeverageLedStatus::OFF;

  SettingLedStatus led_beans = SettingLedStatus::LEVEL_0;
  SettingLedStatus led_size = SettingLedStatus::LEVEL_0;
};
} // namespace philips_series_2200
} // namespace esphome
