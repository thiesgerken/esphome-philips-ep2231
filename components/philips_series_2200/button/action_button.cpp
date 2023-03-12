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

void ActionButton::press_action() {
  switch (action_) {
  case COFFEE:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, 0x00, 0x00,
                 0x39, 0x1C});
    return;
  case ESPRESSO:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x02, 0x00, 0x00,
                 0x09, 0x2D});
    return;
  case HOT_WATER:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x04, 0x00, 0x00,
                 0x21, 0x01});
    return;
  case CAPPUCCINO:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x10, 0x00, 0x00,
                 0x09, 0x26});
    return;
  case START_STOP:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x00, 0x01,
                 0x19, 0x32});
    return;
  case BEANS:
    if (status_.led_beans == SettingLedStatus::LEVEL_0) {
      ESP_LOGW(TAG, "Refusing to press bean button as it is not lit up");
      return;
    }
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
                 0x09, 0x2F});
    return;
  case SIZE:
    if (status_.led_size == SettingLedStatus::LEVEL_0) {
      ESP_LOGW(TAG, "Refusing to press size button as it is not lit up");
      return;
    }
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x00,
                 0x20, 0x05});
    return;
  case AQUA_CLEAN:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x10, 0x00,
                 0x0D, 0x36});
    return;
  case CALC_CLEAN:
    write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x20, 0x00,
                 0x28, 0x37});
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
