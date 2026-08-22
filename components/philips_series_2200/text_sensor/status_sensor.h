#pragma once

#include "../status_parser.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#define BLINK_THRESHOLD 1500

// The mainboard sends roughly 12 frames per second, so this is a fifth of a
// second of settling time. It debounces the LED animations the machine plays
// while powering up and down, whose intermediate states are genuine and pass
// the checksum. It is not there to catch damaged frames; those are rejected in
// the bridge before they reach any sensor.
#define STATE_REPEAT_REQUIREMENT 3

namespace esphome {
namespace philips_series_2200 {
namespace philips_status_sensor {

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
class StatusSensor : public text_sensor::TextSensor, public Component {
public:
  void setup() override;
  void dump_config() override;

  std::string format_beverage_status(BeverageLedStatus status);
  std::string format_setting_status(SettingLedStatus status);
  std::string format_binary_status(bool status);
  std::string beverage_selection();
  std::string format_beverage_selection(const std::string &beverage);
  std::string format_overall_status();

  /**
   * @brief Updates the status of this sensor based on the messages sent by the
   * mainboard
   */
  void update_status(uint8_t *data, size_t len);

  /**
   * @brief Sets the status to OFF
   */
  void set_state_off() {
    selection_ = "";
    if (state != "Aus")
      publish_state("Aus");
  };

  /**
   * @brief Set the type of stuff that is reported by this sensor.
   *
   * @param tp
   */
  void set_type(StatusType status_type) { status_type_ = status_type; };

  /**
   * @brief Published the state if it's different form the currently published
   * state.
   *
   */
  void update_state(const std::string &state) {
    if (state == new_state_) {
      if (new_state_counter_ >= STATE_REPEAT_REQUIREMENT) {
        if (this->state != state)
          publish_state(state);
      } else {
        new_state_counter_++;
      }
    } else {
      new_state_counter_ = 0;
      new_state_ = state;
    }
  }

private:
  /// @brief counter which count how often a message has been seen
  int new_state_counter_ = 0;

  /// @brief cache for counting new messages
  std::string new_state_ = "";

  /// @brief time of play/pause change
  long start_stop_last_change_ = 0;

  /// @brief beverage the user selected, empty while no selection is active
  std::string selection_ = "";

  StatusType status_type_ = StatusType::OVERALL;

  StatusParser status_;
};
} // namespace philips_status_sensor
} // namespace philips_series_2200
} // namespace esphome
