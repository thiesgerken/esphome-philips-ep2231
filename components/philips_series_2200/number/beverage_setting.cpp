#include "beverage_setting.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips_series_2200 {
namespace philips_beverage_setting {

static const char *const TAG = "philips_beverage_setting";

void BeverageSetting::setup() {}

void BeverageSetting::dump_config() {
  LOG_NUMBER("", "Philips Beverage Setting", this);
}

void BeverageSetting::control(float value) {
  // Without a current reading there is no selection screen to press against,
  // so there is nothing sensible to count towards.
  target_amount_ = (std::isnan(value) || std::isnan(state)) ? -1 : value;
}

void BeverageSetting::update_status(uint8_t *data, size_t len) {
  status_.update_status(data, len);

  SettingLedStatus level =
      type_ == Type::BEAN ? status_.led_beans : status_.led_size;

  if (level == SettingLedStatus::LEVEL_0) {
    // led is dark: the machine is not on a screen this setting applies to
    target_amount_ = -1;
    update_state(NAN);
    return;
  }

  update_state(level);

  if (target_amount_ == -1)
    return;

  if (state == target_amount_) {
    // hand control back so the user can keep using the physical button
    target_amount_ = -1;
    return;
  }

  if (millis() - last_transmission_ <= SETTINGS_BUTTON_SEQUENCE_DELAY)
    return;

  // the button cycles through the levels, so press until the target comes up
  for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
    mainboard_uart_->write_array(type_ == Type::BEAN ? command_press_beans
                                                     : command_press_size);
  mainboard_uart_->flush();
  last_transmission_ = millis();
}

} // namespace philips_beverage_setting
} // namespace philips_series_2200
} // namespace esphome
