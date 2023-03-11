#include "esphome/core/log.h"
#include "action_button.h"

namespace esphome
{
    namespace philips_series_2200
    {
        namespace philips_action_button
        {

            static const char *const TAG = "philips-action-button";

            void ActionButton::dump_config()
            {
                LOG_BUTTON("", "Philips Action Button", this);
            }

            void ActionButton::write_array(const std::vector<uint8_t> &data)
            {
                for (unsigned int i = 0; i <= MESSAGE_REPETITIONS; i++)
                    mainboard_uart_->write_array(data);
                mainboard_uart_->flush();
            }

            void ActionButton::press_action()
            {
                switch (action_){
                    case SELECT_COFFEE:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, 0x00, 0x00, 0x39, 0x1C});
                        break;
                    case SELECT_ESPRESSO:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x02, 0x00, 0x00, 0x09, 0x2D});
                        break;
                    case SELECT_HOT_WATER:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x04, 0x00, 0x00, 0x21, 0x01});
                        break;
                    case SELECT_STEAM:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x10, 0x00, 0x00, 0x09, 0x26});
                        break;
                    case PLAY_PAUSE:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x00, 0x01, 0x19, 0x32});
                        break;
                    case SELECT_BEAN:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x09, 0x2F});
                        break;
                    case SELECT_SIZE:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x04, 0x00, 0x20, 0x05});
                        break;
                    case SELECT_AQUA_CLEAN:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x10, 0x00, 0x0D, 0x36});
                        break;
                    case SELECT_CALC_CLEAN:
                        write_array({0xD5, 0x55, 0x00, 0x01, 0x02, 0x00, 0x02, 0x00, 0x20, 0x00, 0x28, 0x37});
                        break;
                    default:
                        ESP_LOGE(TAG, "Invalid Action provided!");
                }
            }
        } // namespace philips_action_button
    }     // namespace philips_series_2200
} // namespace esphome
