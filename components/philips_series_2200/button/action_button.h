#pragma once

#include "../commands.h"
#include "../status_parser.h"
#include "esphome/components/button/button.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#define MESSAGE_REPETITIONS 5
#define BUTTON_SEQUENCE_DELAY 100
#define LONG_PRESS_REPETITION_DELAY 50
#define LONG_PRESS_DURATION 3500

namespace esphome {
namespace philips_series_2200 {
namespace philips_action_button {
/**
 * @brief Executable actions. Select actions only select the type.
 * Make actions select the type and press play.
 *
 */
enum Action {
  COFFEE = 0,
  ESPRESSO,
  HOT_WATER,
  CAPPUCCINO,
  BEANS,
  SIZE,
  AQUA_CLEAN,
  CALC_CLEAN,
  START_STOP,
};

/**
 * @brief Emulates (a) button press(es) using the mainboard uart.
 *
 */
class ActionButton : public button::Button, public Component {
public:
  void dump_config() override;
  void loop() override;

  /**
   * @brief Set the action used by this ActionButton.
   *
   * @param action Action to use
   */
  void set_action(Action action) { action_ = action; };

  /**
   * @brief Reference to uart which is connected to the mainboard
   *
   * @param uart uart connected to mainboard
   */
  void set_uart_device(uart::UARTDevice *uart) { mainboard_uart_ = uart; };

  /**
   * @brief Updates the status of this sensor based on the messages sent by the
   * mainboard
   */
  void update_status(uint8_t *data, size_t len);

  /**
   * @brief Sets whether pressing this button holds the machine's button down
   * instead of tapping it.
   */
  void set_long_press(bool long_press) { should_long_press_ = long_press; };

  /**
   * @brief True while this button is holding down the machine's button
   */
  bool is_long_pressing() { return is_long_pressing_; };

private:
  /**
   * @brief Writes data MESSAGE_REPETITIONS times to the mainboard uart
   *
   * @param data Data to send
   */
  void write_array(const std::vector<uint8_t> &data);

  /**
   * @brief Executes button press
   *
   */
  void press_action() override;

  /**
   * @brief Writes the command for this button's action to the mainboard uart
   *
   */
  void perform_action();

  /// @brief Action used by this Button
  Action action_;
  /// @brief reference to uart connected to mainboard
  uart::UARTDevice *mainboard_uart_;
  /// @brief true if a press should hold the button instead of tapping it
  bool should_long_press_ = false;
  /// @brief true while a long press is in progress
  bool is_long_pressing_ = false;
  /// @brief time at which the current long press was started. Initialized so
  /// that the long press has already expired on boot.
  uint32_t press_start_ = -(LONG_PRESS_DURATION + 1);
  /// @brief time at which the last long press message was sent
  uint32_t last_message_sent_ = 0;

  StatusParser status_;
};
} // namespace philips_action_button
} // namespace philips_series_2200
} // namespace esphome
