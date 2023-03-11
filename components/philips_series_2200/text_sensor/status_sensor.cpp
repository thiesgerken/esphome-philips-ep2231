#include "esphome/core/log.h"
#include "status_sensor.h"

namespace esphome
{
    namespace philips_series_2200
    {
        namespace philips_status_sensor
        {
            static const char *TAG = "philips_status_sensor";

            std::string format_beverage_status(BeverageLedStatus status) {
                switch (status) {
                    case BeverageLedStatus::OFF : return "Aus";
                    case BeverageLedStatus::HALF_BRIGHTNESS : return "Gedimmt";
                    case BeverageLedStatus::FULL_BRIGHTNESS : return "An";
                    case BeverageLedStatus::TWO_DRINKS : return "Zwei Getränke";
                }

                return std::string();
            }

            void StatusSensor::setup()
            {
            }

            void StatusSensor::dump_config()
            {
                ESP_LOGCONFIG(TAG, "Philips Status Text Sensor");
            }

            void StatusSensor::update_status(uint8_t *data, size_t len)
            {
                // reject invalid messages
                if (len != 19 || data[0] != 0xD5 || data[1] != 0x55)
                    return;

                // TODO: figure out how the checksum is calculated and only parse valid messages

                // Check if the play/pause button is on/off/blinking
                if ((data[16] == 0x07) != led_start_stop_)
                {
                    start_stop_last_change_ = millis();
                }

                led_start_stop_ = data[16] == 0x07;

                if (data[3] == 0x03)
                    led_espresso_ = BeverageLedStatus::HALF_BRIGHTNESS;
                else if (data[3] == 0x07)
                    led_espresso_ = BeverageLedStatus::FULL_BRIGHTNESS;
                else if (data[3] == 0x38)
                    led_espresso_ = BeverageLedStatus::TWO_DRINKS;
                else
                    led_espresso_ = BeverageLedStatus::OFF;

                if (data[4] == 0x03)
                    led_hot_water_ = BeverageLedStatus::HALF_BRIGHTNESS;
                else if (data[4] == 0x07)
                    led_hot_water_ = BeverageLedStatus::FULL_BRIGHTNESS;
                else if (data[4] == 0x38)
                    led_hot_water_ = BeverageLedStatus::TWO_DRINKS;
                else
                    led_hot_water_ = BeverageLedStatus::OFF;

                if (data[5] == 0x03)
                    led_coffee_ = BeverageLedStatus::HALF_BRIGHTNESS;
                else if (data[5] == 0x07)
                    led_coffee_ = BeverageLedStatus::FULL_BRIGHTNESS;
                else if (data[5] == 0x38)
                    led_coffee_ = BeverageLedStatus::TWO_DRINKS;
                else
                    led_coffee_ = BeverageLedStatus::OFF;

                if (data[6] == 0x03)
                    led_cappuccino_ = BeverageLedStatus::HALF_BRIGHTNESS;
                else if (data[6] == 0x07)
                    led_cappuccino_ = BeverageLedStatus::FULL_BRIGHTNESS;
                else if (data[6] == 0x38)
                    led_cappuccino_ = BeverageLedStatus::TWO_DRINKS;
                else
                    led_cappuccino_ = BeverageLedStatus::OFF;

                if (data[9] == 0x07) {
                    led_powder_ = false;

                    if (data[8] == 0x00)
                        led_beans_ = SettingLedStatus::LEVEL_1;
                    else if (data[8] == 0x38)
                        led_beans_ = SettingLedStatus::LEVEL_2;
                    else if (data[8] == 0x3F)
                        led_beans_ = SettingLedStatus::LEVEL_3;
                } else if (data[9] == 0x38) {
                    led_powder_ = true;
                    led_beans_ = SettingLedStatus::LEVEL_0;
                } else {
                    led_powder_ = false;
                    led_beans_ = SettingLedStatus::LEVEL_0;
                }

                if (data[11] == 0x07) {
                    if (data[10] == 0x00)
                        led_size_ = SettingLedStatus::LEVEL_1;
                    else if (data[10] == 0x38)
                        led_size_ = SettingLedStatus::LEVEL_2;
                    else if (data[10] == 0x3F)
                        led_size_ = SettingLedStatus::LEVEL_3;
                } else {
                    led_size_ = SettingLedStatus::LEVEL_0;
                }

                led_water_empty_ = data[14] == 0x38;
                led_error_ = data[15] == 0x38;
                led_waste_full_ = data[15] == 0x07;

                if (status_type_ == StatusType::OVERALL) {
                    if (led_espresso_ == BeverageLedStatus::FULL_BRIGHTNESS && led_hot_water_ == BeverageLedStatus::FULL_BRIGHTNESS && led_coffee_ == BeverageLedStatus::FULL_BRIGHTNESS && led_cappuccino_ == BeverageLedStatus::FULL_BRIGHTNESS)
                    {
                    // Check for idle state (selection led on)

                        // selecting a beverage can result in a short "busy" period since the play/pause button has not been blinking
                        // This can be circumvented: if the user is on the selection screen/idle we can reset the timer
                        start_stop_last_change_ = millis();

                        update_state("Idle");
                    } else if (led_espresso_ == BeverageLedStatus::HALF_BRIGHTNESS || led_hot_water_ == BeverageLedStatus::HALF_BRIGHTNESS || led_coffee_ == BeverageLedStatus::HALF_BRIGHTNESS || led_cappuccino_ == BeverageLedStatus::HALF_BRIGHTNESS)
                    {
                    // Check for rotating icons - pre heating

                        if (led_start_stop_)
                            update_state("Cleaning");
                        else
                            update_state("Preparing");
                        return;
                    } else if (led_water_empty_)
                    {
                        update_state("Water empty");
                    } else if (led_waste_full_)
                    {
                        update_state("Waste container warning");
                    } else if (led_error_)
                    {
                        update_state("Error");
                    } else if (led_espresso_ == BeverageLedStatus::OFF && led_hot_water_ == BeverageLedStatus::OFF && led_coffee_ == BeverageLedStatus::FULL_BRIGHTNESS && led_cappuccino_ == BeverageLedStatus::OFF)
                    {
                        if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
                            update_state("Coffee selected");
                        else
                            update_state("Busy");
                    } else if (led_espresso_ == BeverageLedStatus::OFF && led_hot_water_ == BeverageLedStatus::OFF && led_coffee_ == BeverageLedStatus::OFF && led_cappuccino_ == BeverageLedStatus::FULL_BRIGHTNESS)
                    {
                        if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
                            update_state("Cappuccino selected");
                        else
                            update_state("Busy");
                    } else if (led_espresso_ == BeverageLedStatus::OFF && led_hot_water_ == BeverageLedStatus::FULL_BRIGHTNESS && led_coffee_ == BeverageLedStatus::OFF && led_cappuccino_ == BeverageLedStatus::OFF)
                    {
                        if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
                            update_state("Hot water selected");
                        else
                            update_state("Busy");
                    } else if (led_espresso_ == BeverageLedStatus::FULL_BRIGHTNESS && led_hot_water_ == BeverageLedStatus::OFF && led_coffee_ == BeverageLedStatus::OFF && led_cappuccino_ == BeverageLedStatus::OFF)
                    {
                        if (millis() - start_stop_last_change_ < BLINK_THRESHOLD)
                            update_state("Espresso selected");
                        else
                            update_state("Busy");
                    }
                } else if (status_type_ == StatusType::LED_ESPRESSO) {
                    update_state(format_beverage_status(led_espresso_));
                } else if (status_type_ == StatusType::LED_COFFEE) {
                    update_state(format_beverage_status(led_coffee_));
                } else if (status_type_ == StatusType::LED_CAPPUCCINO) {
                    update_state(format_beverage_status(led_cappuccino_));
                } else if (status_type_ == StatusType::LED_HOT_WATER) {
                    update_state(format_beverage_status(led_hot_water_));
                }
                // TODO: this
            }

        } // namespace philips_status_sensor
    }     // namespace philips_series_2200
} // namespace esphome
