#pragma once

#include "../commands.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#define POWER_TRIP_DELAY 500
// The display needs a moment to boot and resume polling after its power is
// restored; retrying earlier would cut it off mid-boot instead of noticing that
// the trip worked.
#define POWER_TRIP_RETRY_DELAY 2000
#define MAX_POWER_TRIPS 5

namespace esphome {
namespace philips_series_2200 {
namespace philips_power_switch {

/**
 * @brief Power Switch wich reflects the power state of the coffee machine.
 * On/Off will change the hardware state of the machine using uart and the power
 * tripping mechanism.
 *
 */
class Power : public switch_::Switch, public Component {
public:
  void setup() override;
  void loop() override;

  /**
   * @brief Write a boolean state to this entity which should be propagated to
   * hardware
   *
   * @param state new State the entity should write to hardware
   */
  void write_state(bool state);
  void dump_config() override;

  /**
   * @brief Sets the mainboard uart reference used by this power switch. The
   * uart is used to fake power on and power off messages.
   *
   * @param uart uart reference
   */
  void set_mainboard_uart(uart::UARTDevice *uart) { mainboard_uart_ = uart; }

  /**
   * @brief Sets the power pin reference which is used to trip the display power
   *
   * @param ping pin reference
   */
  void set_power_pin(GPIOPin *pin) { power_pin_ = pin; }

  /**
   * @brief Sets the cleaning status of this power switch.
   * If true the machine will clean during startup
   */
  void set_cleaning(bool cleaning) { cleaning_ = cleaning; }

  /**
   * @brief Publishes the power state and stops pending trip retries once the
   * display is back on the bus
   *
   * @param state new state of the machine
   */
  void update_state(bool state);

private:
  /// @brief Cuts the display power; restored by loop()
  void start_trip_();

  /// @brief Reference to uart which is connected to the mainboard
  uart::UARTDevice *mainboard_uart_;
  /// @brief power pin which is used for display power
  GPIOPin *power_pin_;
  /// @brief True if the coffee machine is supposed to clean
  bool cleaning_ = true;
  /// @brief true while the display is held without power
  bool tripping_ = false;
  /// @brief time at which the display power was cut
  uint32_t trip_start_ = 0;
  /// @brief true while the display has not answered a power on yet
  bool awaiting_display_ = false;
  /// @brief number of trips performed for the current power on
  uint8_t trip_count_ = 0;
  /// @brief time at which the next trip may be attempted
  uint32_t retry_at_ = 0;
};

} // namespace philips_power_switch
} // namespace philips_series_2200
} // namespace esphome