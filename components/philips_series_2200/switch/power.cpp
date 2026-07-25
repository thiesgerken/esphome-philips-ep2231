#include "power.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips_series_2200 {
namespace philips_power_switch {

static const char *TAG = "philips_power_switch";

void Power::setup() {}

void Power::loop() {}

void Power::write_state(bool state) {
  if (state) {
    for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
      mainboard_uart_->write_array(command_pre_power_on);

    for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
      mainboard_uart_->write_array(cleaning_ ? command_power_with_cleaning
                                             : command_power_without_cleaning);

    mainboard_uart_->flush();

    power_pin_->digital_write(0);
    delay(POWER_TRIP_DELAY);
    power_pin_->digital_write(1);
  } else {
    for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
      mainboard_uart_->write_array(command_power_off);
    mainboard_uart_->flush();
  }

  // The state will be published once the display starts sending messages
}

void Power::dump_config() {
  ESP_LOGCONFIG(TAG, "Philips Series 2200 Power Switch");
}

} // namespace philips_power_switch
} // namespace philips_series_2200
} // namespace esphome
