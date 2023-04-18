#include "philips_series_2200.h"
#include "esphome/core/log.h"
#include "status_parser.h"

#define BUFFER_SIZE 32

namespace esphome {
namespace philips_series_2200 {

// NOTE: couldn't figure out how to convince esphome to link another compile
// unit, that's why this is here and not in status_parser.cpp
void StatusParser::update_status(uint8_t *data, size_t len) {
  led_start_stop = data[16] == 0x07;

  if (data[3] == 0x03)
    led_espresso = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[3] == 0x07)
    led_espresso = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[3] == 0x38)
    led_espresso = BeverageLedStatus::TWO_DRINKS;
  else
    led_espresso = BeverageLedStatus::OFF;

  if (data[4] == 0x03)
    led_hot_water = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[4] == 0x07)
    led_hot_water = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[4] == 0x38)
    led_hot_water = BeverageLedStatus::TWO_DRINKS;
  else
    led_hot_water = BeverageLedStatus::OFF;

  if (data[5] == 0x03)
    led_coffee = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[5] == 0x07)
    led_coffee = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[5] == 0x38)
    led_coffee = BeverageLedStatus::TWO_DRINKS;
  else
    led_coffee = BeverageLedStatus::OFF;

  if (data[6] == 0x03)
    led_cappuccino = BeverageLedStatus::HALF_BRIGHTNESS;
  else if (data[6] == 0x07)
    led_cappuccino = BeverageLedStatus::FULL_BRIGHTNESS;
  else if (data[6] == 0x38)
    led_cappuccino = BeverageLedStatus::TWO_DRINKS;
  else
    led_cappuccino = BeverageLedStatus::OFF;

  if (data[9] == 0x07) {
    led_powder = false;

    if (data[8] == 0x00)
      led_beans = SettingLedStatus::LEVEL_1;
    else if (data[8] == 0x38)
      led_beans = SettingLedStatus::LEVEL_2;
    else if (data[8] == 0x3F)
      led_beans = SettingLedStatus::LEVEL_3;
  } else if (data[9] == 0x38) {
    led_powder = true;
    led_beans = SettingLedStatus::LEVEL_0;
  } else {
    led_powder = false;
    led_beans = SettingLedStatus::LEVEL_0;
  }

  if (data[11] == 0x07) {
    if (data[10] == 0x00)
      led_size = SettingLedStatus::LEVEL_1;
    else if (data[10] == 0x38)
      led_size = SettingLedStatus::LEVEL_2;
    else if (data[10] == 0x3F)
      led_size = SettingLedStatus::LEVEL_3;
  } else {
    led_size = SettingLedStatus::LEVEL_0;
  }

  led_water_empty = data[14] == 0x38;
  led_error = data[15] == 0x38;
  led_waste_full = data[15] == 0x07;
}

static const char *TAG = "philips_series_2200";

void PhilipsSeries2200::setup() {
  power_pin_->setup();
  power_pin_->pin_mode(gpio::FLAG_OUTPUT);
  power_pin_->digital_write(true);
}

void PhilipsSeries2200::loop() {
  uint8_t buffer[BUFFER_SIZE];
  bool display_active = false;

  // Pipe display to mainboard
  if (display_uart_.available()) {
    uint8_t size = std::min(display_uart_.available(), BUFFER_SIZE);
    display_uart_.read_array(buffer, size);

    mainboard_uart_.write_array(buffer, size);
    last_message_from_display_time_ = millis();

    // if (size == 12 && buffer[0] == 0xD5 && buffer[1] == 0x55) {
    //   std::string res = "Display -> Mainboard: ";
    //   char buf[5];
    //   for (size_t i = 0; i < size; i++) {
    //     sprintf(buf, "%02X ", buffer[i]);
    //     res += buf;
    //   }
    //   ESP_LOGD(TAG, res.c_str());
    // }

    // don't block for too long
    mainboard_uart_.flush();
    return;
  }

  // Read from mainboard until start index
  uint8_t cnt = 0;
  while (mainboard_uart_.available()) {
    uint8_t buffer = mainboard_uart_.peek();
    if (buffer == 0xD5)
      break;

    display_uart_.write(mainboard_uart_.read());

    if (cnt++ >= 64) {
        // don't block for too long
        display_uart_.flush();
        return;
    }
  }

  // Pipe to display
  if (mainboard_uart_.available()) {
    uint8_t size = std::min(mainboard_uart_.available(), 19);

    mainboard_uart_.read_array(buffer, size);
    display_uart_.write_array(buffer, size);

    // if (size == 19 && buffer[0] == 0xD5 && buffer[1] == 0x55) {
    //   std::string res = "Mainboard -> Display: ";
    //   char buf[5];
    //   for (size_t i = 0; i < size; i++) {
    //     sprintf(buf, "%02X ", buffer[i]);
    //     res += buf;
    //   }
    //   ESP_LOGD(TAG, res.c_str());
    // }

    // NOTE: would be nice to figure out how the checksum works
    // in order to ignore invalid messages better
    if (size == 19 && buffer[0] == 0xD5 && buffer[1] == 0x55) {
      last_message_from_mainboard_time_ = millis();

      for (philips_status_sensor::StatusSensor *status_sensor : status_sensors_)
        status_sensor->update_status(buffer, size);
      for (philips_action_button::ActionButton *action_button : action_buttons_)
        action_button->update_status(buffer, size);
      for (philips_power_switch::Power *power_switch : power_switches_)
        power_switch->publish_state(true);
    }
  }

  if (millis() - last_message_from_mainboard_time_ > POWER_STATE_TIMEOUT) {
    // Update power switches
    for (philips_power_switch::Power *power_switch : power_switches_)
      power_switch->publish_state(false);

    // Update status sensors
    for (philips_status_sensor::StatusSensor *status_sensor : status_sensors_)
      status_sensor->set_state_off();
  }

  display_uart_.flush();
  mainboard_uart_.flush();
}

void PhilipsSeries2200::dump_config() {
  ESP_LOGCONFIG(TAG, "Philips Series 2200");
  display_uart_.check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_NONE,
                                    8);
  mainboard_uart_.check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_NONE,
                                      8);
}

} // namespace philips_series_2200
} // namespace esphome
