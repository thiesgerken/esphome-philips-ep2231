#include "action_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips_series_2200 {
namespace philips_action_button {

static const char *const TAG = "philips-action-button";

void ActionButton::dump_config() {
  LOG_BUTTON("", "Philips Action Button", this);
}

void ActionButton::write_array(const std::vector<uint8_t> &data) {
  for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
    mainboard_uart_->write_array(data);
  mainboard_uart_->flush();
}

void ActionButton::loop() {
  if (!should_long_press_ || millis() - press_start_ > LONG_PRESS_DURATION) {
    is_long_pressing_ = false;
    return;
  }

  is_long_pressing_ = true;
  if (millis() - last_message_sent_ > LONG_PRESS_REPETITION_DELAY) {
    last_message_sent_ = millis();
    perform_action();
  }
}

void ActionButton::press_action() {
  // The machine ignores these buttons while their led is dark. Checked once
  // per press, not per repetition: a long press on the bean button switches to
  // ground coffee, which turns that led off while the press is still running.
  if (action_ == BEANS && status_.led_beans == SettingLedStatus::LEVEL_0) {
    ESP_LOGW(TAG, "Refusing to press bean button as it is not lit up");
    return;
  }
  if (action_ == SIZE && status_.led_size == SettingLedStatus::LEVEL_0) {
    ESP_LOGW(TAG, "Refusing to press size button as it is not lit up");
    return;
  }

  if (should_long_press_) {
    press_start_ = millis();
    last_message_sent_ = 0;
    return;
  }

  perform_action();
}

void ActionButton::perform_action() {
  switch (action_) {
  case COFFEE:
    write_array(command_press_coffee);
    return;
  case ESPRESSO:
    write_array(command_press_espresso);
    return;
  case HOT_WATER:
    write_array(command_press_hot_water);
    return;
  case CAPPUCCINO:
    write_array(command_press_cappuccino);
    return;
  case START_STOP:
    write_array(command_press_start_stop);
    return;
  case BEANS:
    if (status_.led_beans == SettingLedStatus::LEVEL_0) {
      ESP_LOGW(TAG, "Refusing to press bean button as it is not lit up");
      return;
    }
    write_array(command_press_beans);
    return;
  case SIZE:
    if (status_.led_size == SettingLedStatus::LEVEL_0) {
      ESP_LOGW(TAG, "Refusing to press size button as it is not lit up");
      return;
    }
    write_array(command_press_size);
    return;
  case AQUA_CLEAN:
    write_array(command_press_aqua_clean);
    return;
  case CALC_CLEAN:
    write_array(command_press_calc_clean);
    return;
  default:
    ESP_LOGE(TAG, "Invalid Action provided!");
    return;
  }
}

void ActionButton::update_status(uint8_t *data, size_t len) {
  status_.update_status(data, len);
}

} // namespace philips_action_button
} // namespace philips_series_2200
} // namespace esphome
