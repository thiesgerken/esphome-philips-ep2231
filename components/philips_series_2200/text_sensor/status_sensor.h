#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#define BLINK_THRESHOLD 750

namespace esphome
{
    namespace philips_series_2200
    {
        namespace philips_status_sensor
        {
            enum BeverageLedStatus {
                OFF = 0,
                HALF_BRIGHTNESS,
                FULL_BRIGHTNESS,
                TWO_DRINKS,

            };

            std::string format_beverage_status(BeverageLedStatus status);

            enum SettingLedStatus {
                LEVEL_0 = 0,
                LEVEL_1,
                LEVEL_2,
                LEVEL_3,
            };

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
                LED_WASTE,
                LED_START_STOP
            };

            /**
             * @brief Reports status of the coffee machine
             */
            class StatusSensor : public text_sensor::TextSensor, public Component
            {
            public:
                void setup() override;
                void dump_config() override;

                /**
                 * @brief Updates the status of this sensor based on the messages sent by the mainboard
                 */
                void update_status(uint8_t *data, size_t len);

                /**
                 * @brief Sets the status to OFF
                 */
                void set_state_off()
                {
                    if (state != "OFF")
                        publish_state("OFF");
                };

                /**
                 * @brief Set the type of stuff that is reported by this sensor.
                 *
                 * @param tp
                 */
                void set_type(StatusType status_type) { status_type_ = status_type; };

                /**
                 * @brief Published the state if it's different form the currently published state.
                 *
                 */
                void update_state(const std::string &state)
                {
                    size_t repeat_requirement = status_type_ == StatusType::OVERALL ? 30 : 2;

                    if (state == new_state_)
                    {
                        if (new_state_counter_ >= repeat_requirement)
                        {
                            if (this->state != state)
                                publish_state(state);
                        }
                        else
                        {
                            new_state_counter_++;
                        }
                    }
                    else
                    {
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

                bool led_start_stop_ = false;
                bool led_waste_full_ = false;
                bool led_powder_ = false;
                bool led_water_empty_ = false;
                bool led_error_ = false;

                // TODO: add aqua clean & calcnclean

                BeverageLedStatus led_espresso_ = BeverageLedStatus::OFF;
                BeverageLedStatus led_hot_water_ = BeverageLedStatus::OFF;
                BeverageLedStatus led_coffee_ = BeverageLedStatus::OFF;
                BeverageLedStatus led_cappuccino_ = BeverageLedStatus::OFF;

                SettingLedStatus led_beans_ = SettingLedStatus::LEVEL_0;
                SettingLedStatus led_size_ = SettingLedStatus::LEVEL_0;

                StatusType status_type_ = StatusType::OVERALL;

            };
        } // namespace philips_status_sensor
    }     // namespace philips_series_2200
} // namespace esphome
