#include "status_sensor.h"
#include "esphome/core/log.h"
#include <sstream>

namespace esphome {
namespace philips_series_2200 {
namespace philips_status_sensor {
static const char *TAG = "philips_status_sensor";

std::string StatusSensor::format_beverage_status(BeverageLedStatus status) {
  switch (status) {
  case BeverageLedStatus::OFF:
    return "Aus";
  case BeverageLedStatus::HALF_BRIGHTNESS:
    return "Gedimmt";
  case BeverageLedStatus::FULL_BRIGHTNESS:
    return "An";
  case BeverageLedStatus::TWO_DRINKS:
    return "Zwei Getränke";
  }

  return "unbekannt";
}

std::string StatusSensor::format_binary_status(bool status) {
  if (status)
    return "An";

  return "Aus";
}

std::string StatusSensor::format_setting_status(SettingLedStatus status) {
  switch (status) {
  case SettingLedStatus::LEVEL_0:
    return "Aus";
  case SettingLedStatus::LEVEL_1:
    return "Stufe 1";
  case SettingLedStatus::LEVEL_2:
    return "Stufe 2";
  case SettingLedStatus::LEVEL_3:
    return "Stufe 3";
  }

  return "unbekannt";
}

std::string StatusSensor::format_beverage_selection(std::string beverage) {
  std::stringstream ss;
  ss << beverage << " ausgewählt (";

  switch (status_.led_size) {
  case SettingLedStatus::LEVEL_1:
    ss << "klein";
    break;
  case SettingLedStatus::LEVEL_2:
    ss << "mittelgroß";
    break;
  case SettingLedStatus::LEVEL_3:
    ss << "groß";
    break;
  default:
    ss << "Größe unbekannt";
  }

  switch (status_.led_beans) {
  case SettingLedStatus::LEVEL_1:
    ss << " & schwach";
    break;
  case SettingLedStatus::LEVEL_2:
    ss << " & mittelstark";
    break;
  case SettingLedStatus::LEVEL_3:
    ss << " & stark";
    break;
  default:
    break;
  }

  ss << ")";
  return ss.str();
}

std::string StatusSensor::format_overall_status() {

  if (status_.led_espresso == BeverageLedStatus::FULL_BRIGHTNESS &&
      status_.led_hot_water == BeverageLedStatus::FULL_BRIGHTNESS &&
      status_.led_coffee == BeverageLedStatus::FULL_BRIGHTNESS &&
      status_.led_cappuccino == BeverageLedStatus::FULL_BRIGHTNESS) {
    // Check for idle state (selection led on)

    // selecting a beverage can result in a short "busy" period since the
    // play/pause button has not been blinking This can be circumvented: if the
    // user is on the selection screen/idle we can reset the timer
    start_stop_last_change_ = millis();

    return "Bereit";
  }

  if (status_.led_start_stop &&
      millis() - start_stop_last_change_ >= BLINK_THRESHOLD)
    // start stop led is on + did not change for a while -> it is not blinking.
    // the machine is probably brewing a drink.
    return "Zubereitung";

  if (status_.led_espresso == BeverageLedStatus::HALF_BRIGHTNESS ||
      status_.led_hot_water == BeverageLedStatus::HALF_BRIGHTNESS ||
      status_.led_coffee == BeverageLedStatus::HALF_BRIGHTNESS ||
      status_.led_cappuccino == BeverageLedStatus::HALF_BRIGHTNESS) {
    // Check for rotating icons - pre heating

    if (status_.led_start_stop)
      return "Spült";
    return "Vorbereitung";
  }

  if (status_.led_water_empty)
    return "Wasser leer";
  if (status_.led_waste_full)
    return "Trester voll";
  if (status_.led_error)
    return "Fehler";

  if (status_.led_espresso == BeverageLedStatus::OFF &&
      status_.led_hot_water == BeverageLedStatus::OFF &&
      status_.led_coffee == BeverageLedStatus::FULL_BRIGHTNESS &&
      status_.led_cappuccino == BeverageLedStatus::OFF) {
    return format_beverage_selection("Kaffee");
  }
  if (status_.led_espresso == BeverageLedStatus::OFF &&
      status_.led_hot_water == BeverageLedStatus::OFF &&
      status_.led_coffee == BeverageLedStatus::TWO_DRINKS &&
      status_.led_cappuccino == BeverageLedStatus::OFF) {
    return format_beverage_selection("2x Kaffee");
  }
  if (status_.led_espresso == BeverageLedStatus::FULL_BRIGHTNESS &&
      status_.led_hot_water == BeverageLedStatus::OFF &&
      status_.led_coffee == BeverageLedStatus::OFF &&
      status_.led_cappuccino == BeverageLedStatus::OFF) {
    return format_beverage_selection("Espresso");
  }
  if (status_.led_espresso == BeverageLedStatus::TWO_DRINKS &&
      status_.led_hot_water == BeverageLedStatus::OFF &&
      status_.led_coffee == BeverageLedStatus::OFF &&
      status_.led_cappuccino == BeverageLedStatus::OFF) {
    return format_beverage_selection("2x Espresso");
  }
  if (status_.led_espresso == BeverageLedStatus::OFF &&
      status_.led_hot_water == BeverageLedStatus::OFF &&
      status_.led_coffee == BeverageLedStatus::OFF &&
      status_.led_cappuccino == BeverageLedStatus::FULL_BRIGHTNESS) {
    return format_beverage_selection("Cappuccino");
  }
  if (status_.led_espresso == BeverageLedStatus::OFF &&
      status_.led_hot_water == BeverageLedStatus::FULL_BRIGHTNESS &&
      status_.led_coffee == BeverageLedStatus::OFF &&
      status_.led_cappuccino == BeverageLedStatus::OFF) {
    return format_beverage_selection("Heißes Wasser");
  }
  return "Unbekannt";
}

void StatusSensor::setup() {}

void StatusSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Philips Status Text Sensor");
}

void StatusSensor::update_status(uint8_t *data, size_t len) {
  bool prev_led_start_stop = status_.led_start_stop;
  status_.update_status(data, len);

  // Check if the play/pause button is on/off/blinking
  if (prev_led_start_stop != status_.led_start_stop)
    start_stop_last_change_ = millis();

  if (status_type_ == StatusType::OVERALL) {
    update_state(format_overall_status());
  } else if (status_type_ == StatusType::LED_ESPRESSO) {
    update_state(format_beverage_status(status_.led_espresso));
  } else if (status_type_ == StatusType::LED_COFFEE) {
    update_state(format_beverage_status(status_.led_coffee));
  } else if (status_type_ == StatusType::LED_CAPPUCCINO) {
    update_state(format_beverage_status(status_.led_cappuccino));
  } else if (status_type_ == StatusType::LED_HOT_WATER) {
    update_state(format_beverage_status(status_.led_hot_water));
  } else if (status_type_ == StatusType::LED_BEANS) {
    update_state(format_setting_status(status_.led_beans));
  } else if (status_type_ == StatusType::LED_SIZE) {
    update_state(format_setting_status(status_.led_size));
  } else if (status_type_ == StatusType::LED_POWDER) {
    update_state(format_binary_status(status_.led_powder));
  } else if (status_type_ == StatusType::LED_WASTE_FULL) {
    update_state(format_binary_status(status_.led_waste_full));
  } else if (status_type_ == StatusType::LED_WATER_EMPTY) {
    update_state(format_binary_status(status_.led_water_empty));
  } else if (status_type_ == StatusType::LED_ERROR) {
    update_state(format_binary_status(status_.led_error));
  } else if (status_type_ == StatusType::LED_START_STOP) {
    if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
      update_state("Blinkt");
    else
      update_state(format_binary_status(status_.led_start_stop));
  }
}

} // namespace philips_status_sensor
} // namespace philips_series_2200
} // namespace esphome
