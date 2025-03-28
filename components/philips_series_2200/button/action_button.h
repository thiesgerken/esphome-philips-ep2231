#pragma once

#include "../status_parser.h"
#include "esphome/components/button/button.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#define MESSAGE_REPETITIONS 5
#define BUTTON_SEQUENCE_DELAY 100

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

  /// @brief Action used by this Button
  Action action_;
  /// @brief reference to uart connected to mainboard
  uart::UARTDevice *mainboard_uart_;

  StatusParser status_;
};
} // namespace philips_action_button
} // namespace philips_series_2200
} // namespace esphome
