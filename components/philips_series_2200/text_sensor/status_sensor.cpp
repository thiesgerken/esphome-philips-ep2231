#include "status_sensor.h"
#include "esphome/core/log.h"
#include <sstream>

namespace esphome {
namespace philips_series_2200 {
namespace philips_status_sensor {
static const char *TAG = "philips_status_sensor";

std::string format_beverage_status(BeverageLedStatus status) {
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

std::string format_binary_status(bool status) {
  if (status)
    return "An";

  return "Aus";
}

std::string format_setting_status(SettingLedStatus status) {
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

  switch (led_size_) {
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

  switch (led_beans_) {
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
  if (led_espresso_ == BeverageLedStatus::FULL_BRIGHTNESS &&
      led_hot_water_ == BeverageLedStatus::FULL_BRIGHTNESS &&
      led_coffee_ == BeverageLedStatus::FULL_BRIGHTNESS &&
      led_cappuccino_ == BeverageLedStatus::FULL_BRIGHTNESS) {
    // Check for idle state (selection led on)

    // selecting a beverage can result in a short "busy" period since the
    // play/pause button has not been blinking This can be circumvented: if the
    // user is on the selection screen/idle we can reset the timer
    start_stop_last_change_ = millis();

    return "Bereit";
  }
  if (led_espresso_ == BeverageLedStatus::HALF_BRIGHTNESS ||
      led_hot_water_ == BeverageLedStatus::HALF_BRIGHTNESS ||
      led_coffee_ == BeverageLedStatus::HALF_BRIGHTNESS ||
      led_cappuccino_ == BeverageLedStatus::HALF_BRIGHTNESS) {
    // Check for rotating icons - pre heating

    if (led_start_stop_)
      return "Spült";
    return "Vorbereitung";
  }
  if (led_water_empty_)
    return "Wasser leer";
  if (led_waste_full_)
    return "Trester voll";
  if (led_error_)
    return "Fehler";
  if (led_espresso_ == BeverageLedStatus::OFF &&
      led_hot_water_ == BeverageLedStatus::OFF &&
      led_coffee_ == BeverageLedStatus::FULL_BRIGHTNESS &&
      led_cappuccino_ == BeverageLedStatus::OFF) {
    if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
      return format_beverage_selection("Kaffee");
    return "Busy";
  }
  if (led_espresso_ == BeverageLedStatus::OFF &&
      led_hot_water_ == BeverageLedStatus::OFF &&
      led_coffee_ == BeverageLedStatus::OFF &&
      led_cappuccino_ == BeverageLedStatus::FULL_BRIGHTNESS) {
    if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
      return format_beverage_selection("Cappuccino");
    return "Busy";
  }
  if (led_espresso_ == BeverageLedStatus::OFF &&
      led_hot_water_ == BeverageLedStatus::FULL_BRIGHTNESS &&
      led_coffee_ == BeverageLedStatus::OFF &&
      led_cappuccino_ == BeverageLedStatus::OFF) {
    if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
      return format_beverage_selection("Heißes Wasser");
    return "Busy";
  }
  if (led_espresso_ == BeverageLedStatus::FULL_BRIGHTNESS &&
      led_hot_water_ == BeverageLedStatus::OFF &&
      led_coffee_ == BeverageLedStatus::OFF &&
      led_cappuccino_ == BeverageLedStatus::OFF) {
    if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
      return format_beverage_selection("Espresso");
    return "Busy";
  }

  return "Unbekannt";
}

void StatusSensor::setup() {}

void StatusSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Philips Status Text Sensor");
}

void StatusSensor::update_status(uint8_t *data, size_t len) {
  // reject invalid messages
  if (len != 19 || data[0] != 0xD5 || data[1] != 0x55)
    return;

  // TODO: figure out how the checksum is calculated and only parse valid
  // messages

  bool prev_led_start_stop = led_start_stop_;
  led_start_stop_ = data[16] == 0x07;

  // Check if the play/pause button is on/off/blinking
  if (prev_led_start_stop != led_start_stop_)
    start_stop_last_change_ = millis();

  if (data[3] == 0x03)
    led_espresso_ = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[3] == 0x07)
    led_espresso_ = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[3] == 0x38)
    led_espresso_ = BeverageLedStatus::TWO_DRINKS;
  else
    led_espresso_ = BeverageLedStatus::OFF;

  if (data[4] == 0x03)
    led_hot_water_ = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[4] == 0x07)
    led_hot_water_ = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[4] == 0x38)
    led_hot_water_ = BeverageLedStatus::TWO_DRINKS;
  else
    led_hot_water_ = BeverageLedStatus::OFF;

  if (data[5] == 0x03)
    led_coffee_ = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[5] == 0x07)
    led_coffee_ = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[5] == 0x38)
    led_coffee_ = BeverageLedStatus::TWO_DRINKS;
  else
    led_coffee_ = BeverageLedStatus::OFF;

  if (data[6] == 0x03)
    led_cappuccino_ = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[6] == 0x07)
    led_cappuccino_ = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[6] == 0x38)
    led_cappuccino_ = BeverageLedStatus::TWO_DRINKS;
  else
    led_cappuccino_ = BeverageLedStatus::OFF;

  if (data[9] == 0x07) {
    led_powder_ = false;

    if (data[8] == 0x00)
      led_beans_ = SettingLedStatus::LEVEL_1;
    else if (data[8] == 0x38)
      led_beans_ = SettingLedStatus::LEVEL_2;
    else if (data[8] == 0x3F)
      led_beans_ = SettingLedStatus::LEVEL_3;
  } else if (data[9] == 0x38) {
    led_powder_ = true;
    led_beans_ = SettingLedStatus::LEVEL_0;
  } else {
    led_powder_ = false;
    led_beans_ = SettingLedStatus::LEVEL_0;
  }

  if (data[11] == 0x07) {
    if (data[10] == 0x00)
      led_size_ = SettingLedStatus::LEVEL_1;
    else if (data[10] == 0x38)
      led_size_ = SettingLedStatus::LEVEL_2;
    else if (data[10] == 0x3F)
      led_size_ = SettingLedStatus::LEVEL_3;
  } else {
    led_size_ = SettingLedStatus::LEVEL_0;
  }

  led_water_empty_ = data[14] == 0x38;
  led_error_ = data[15] == 0x38;
  led_waste_full_ = data[15] == 0x07;

  if (status_type_ == StatusType::OVERALL) {
    update_state(format_overall_status());
  } else if (status_type_ == StatusType::LED_ESPRESSO) {
    update_state(format_beverage_status(led_espresso_));
  } else if (status_type_ == StatusType::LED_COFFEE) {
    update_state(format_beverage_status(led_coffee_));
  } else if (status_type_ == StatusType::LED_CAPPUCCINO) {
    update_state(format_beverage_status(led_cappuccino_));
  } else if (status_type_ == StatusType::LED_HOT_WATER) {
    update_state(format_beverage_status(led_hot_water_));
  } else if (status_type_ == StatusType::LED_BEANS) {
    update_state(format_setting_status(led_beans_));
  } else if (status_type_ == StatusType::LED_SIZE) {
    update_state(format_setting_status(led_size_));
  } else if (status_type_ == StatusType::LED_POWDER) {
    update_state(format_binary_status(led_powder_));
  } else if (status_type_ == StatusType::LED_START_STOP) {
    update_state(format_binary_status(led_start_stop_));
  } else if (status_type_ == StatusType::LED_WASTE_FULL) {
    update_state(format_binary_status(led_waste_full_));
  } else if (status_type_ == StatusType::LED_WATER_EMPTY) {
    update_state(format_binary_status(led_water_empty_));
  } else if (status_type_ == StatusType::LED_ERROR) {
    update_state(format_binary_status(led_error_));
  }
}

} // namespace philips_status_sensor
} // namespace philips_series_2200
} // namespace esphome
