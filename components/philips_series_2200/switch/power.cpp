#include "power.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips_series_2200 {
namespace philips_power_switch {

static const char *TAG = "philips_power_switch";

void Power::setup() {}

void Power::loop() {
  // Restoring the pin from here rather than blocking in write_state: the uart
  // bridging runs in the same loop, so blocking dropped every mainboard message
  // for the duration of the trip.
  if (tripping_ && millis() - trip_start_ >= POWER_TRIP_DELAY) {
    power_pin_->digital_write(1);
    tripping_ = false;
  }
}

void Power::write_state(bool state) {
  if (state) {
    for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
      mainboard_uart_->write_array(command_pre_power_on);

    for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
      mainboard_uart_->write_array(cleaning_ ? command_power_with_cleaning
                                             : command_power_without_cleaning);

    mainboard_uart_->flush();

    // The display does not notice an injected power on, so reboot it by
    // cutting its power briefly. Completed in loop().
    power_pin_->digital_write(0);
    trip_start_ = millis();
    tripping_ = true;
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
