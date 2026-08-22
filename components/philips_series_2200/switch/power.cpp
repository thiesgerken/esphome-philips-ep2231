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
  if (tripping_) {
    if (millis() - trip_start_ >= POWER_TRIP_DELAY) {
      power_pin_->digital_write(1);
      tripping_ = false;
      retry_at_ = millis() + POWER_TRIP_RETRY_DELAY;
    }
    return;
  }

  // One trip does not always bring the display back up, so keep trying until it
  // polls the mainboard again (update_state) or the machine is beyond help.
  if (awaiting_display_ && (int32_t)(millis() - retry_at_) >= 0) {
    if (trip_count_ >= MAX_POWER_TRIPS) {
      ESP_LOGE(TAG, "Display did not come up after %u power trips!",
               trip_count_);
      awaiting_display_ = false;
      return;
    }
    start_trip_();
  }
}

void Power::start_trip_() {
  power_pin_->digital_write(0);
  trip_start_ = millis();
  tripping_ = true;
  trip_count_++;
}

void Power::update_state(bool state) {
  if (state && awaiting_display_) {
    ESP_LOGD(TAG, "Display came up after %u power trip(s)", trip_count_);
    awaiting_display_ = false;
  }

  publish_state(state);
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
    awaiting_display_ = true;
    trip_count_ = 0;
    start_trip_();
  } else {
    for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
      mainboard_uart_->write_array(command_power_off);
    mainboard_uart_->flush();

    awaiting_display_ = false;
  }

  // The state will be published once the display starts sending messages
}

void Power::dump_config() {
  ESP_LOGCONFIG(TAG, "Philips Series 2200 Power Switch");
}

} // namespace philips_power_switch
} // namespace philips_series_2200
} // namespace esphome
