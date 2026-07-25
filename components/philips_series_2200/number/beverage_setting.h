#pragma once

#include "../commands.h"
#include "../status_parser.h"
#include "esphome/components/number/number.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

// The machine needs a moment to apply a press and report the new level back,
// so wait for that before pressing again.
#define SETTINGS_BUTTON_SEQUENCE_DELAY 500

namespace esphome {
namespace philips_series_2200 {
namespace philips_beverage_setting {

enum Type {
  BEAN = 0,
  SIZE,
};

/**
 * @brief Reports and sets the bean amount/cup size of the beverage that is
 * currently selected on the machine.
 */
class BeverageSetting : public number::Number, public Component {
public:
  void setup() override;
  void dump_config() override;

  void set_type(Type type) { type_ = type; };

  /**
   * @brief Reference to uart which is connected to the mainboard
   *
   * @param uart uart connected to mainboard
   */
  void set_uart_device(uart::UARTDevice *uart) { mainboard_uart_ = uart; };

  /**
   * @brief Updates the value and presses the button if a target is pending
   */
  void update_status(uint8_t *data, size_t len);

protected:
  void control(float value) override;

private:
  /**
   * @brief Publishes the state if it differs from the currently published one
   */
  void update_state(float value) {
    if (std::isnan(value) && std::isnan(state))
      return;
    if (value != state)
      publish_state(value);
  };

  Type type_ = Type::BEAN;

  /// @brief reference to uart connected to mainboard
  uart::UARTDevice *mainboard_uart_;

  /// @brief level the user asked for, -1 if there is nothing to do
  int8_t target_amount_ = -1;

  /// @brief time at which the last button press was sent
  uint32_t last_transmission_ = 0;

  StatusParser status_;
};

} // namespace philips_beverage_setting
} // namespace philips_series_2200
} // namespace esphome
