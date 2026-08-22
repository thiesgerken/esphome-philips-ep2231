#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace philips_series_2200 {
namespace checksum {

// CRC-16/CCITT over the whole message, header included. The 16 bit result is
// sent as two 6 bit values, low byte first, each carrying the top six bits of
// its byte, so 4 bits never make it onto the wire. One set of parameters covers
// both directions and every machine; see protocol.md.
constexpr uint16_t POLYNOMIAL = 0x1021;
constexpr uint16_t INITIAL_VALUE = 0xAAAA;

/**
 * @brief Computes the checksum of a message.
 *
 * @param message Message without its two checksum bytes
 * @param length Length of that part
 */
constexpr uint16_t compute(const uint8_t *message, size_t length) {
  uint16_t crc = INITIAL_VALUE;

  for (size_t i = 0; i < length; i++) {
    crc ^= (uint16_t)(message[i] << 8);

    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ POLYNOMIAL)
                           : (uint16_t)(crc << 1);
  }

  return crc;
}

/**
 * @brief True if a message carries the checksum it should.
 *
 * @param message Message including header and checksum
 * @param length Full length of the message
 */
constexpr bool valid(const uint8_t *message, size_t length) {
  const uint16_t crc = compute(message, length - 2);

  return message[length - 2] == (uint8_t)((crc & 0xFF) >> 2) &&
         message[length - 1] == (uint8_t)((crc >> 8) >> 2);
}

namespace {
// Captured messages, checked at compile time. FRAME_IDLE and COMMAND_EP3241 are
// from other machines, so they also pin down that the parameters do not vary
// per machine.
constexpr uint8_t FRAME_OFF[19] = {0xD5, 0x55, 0, 0, 0, 0, 0, 0,    0,   0,
                                   0,    0,    0, 0, 0, 0, 0, 0x39, 0x0D};
constexpr uint8_t FRAME_IDLE[19] = {0xD5, 0x55, 0x00, 0x07, 0x07, 0x07, 0x07,
                                    0,    0,    0,    0,    0,    0,    0,
                                    0,    0,    0,    0x07, 0x2B};
constexpr uint8_t FRAME_BREWING[19] = {0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00,
                                       0x00, 0x3F, 0x07, 0x38, 0x07, 0x00, 0x00,
                                       0x00, 0x00, 0x07, 0x19, 0x39};
constexpr uint8_t FRAME_TWO_COFFEES[19] = {
    0xD5, 0x55, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x38, 0x07,
    0x38, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x1B};
constexpr uint8_t FRAME_CORRUPT[19] = {0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00,
                                       0x00, 0x3F, 0x07, 0x38, 0x07, 0x00, 0x00,
                                       0x00, 0x00, 0x07, 0x19, 0x38};
constexpr uint8_t COMMAND_ESPRESSO[12] = {0xD5, 0x55, 0x00, 0x01, 0x02, 0x00,
                                          0x03, 0x02, 0x00, 0x00, 0x24, 0x30};
constexpr uint8_t COMMAND_EP3241[12] = {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00,
                                        0x12, 0x08, 0x00, 0x00, 0x38, 0x0B};

static_assert(valid(FRAME_OFF, 19), "rejects a captured frame");
static_assert(valid(FRAME_IDLE, 19), "rejects a captured frame");
static_assert(valid(FRAME_BREWING, 19), "rejects a captured frame");
static_assert(valid(FRAME_TWO_COFFEES, 19), "rejects a captured frame");
static_assert(!valid(FRAME_CORRUPT, 19), "accepts a damaged frame");
static_assert(valid(COMMAND_ESPRESSO, 12), "rejects a captured command");
static_assert(valid(COMMAND_EP3241, 12), "rejects a captured command");
} // namespace

} // namespace checksum
} // namespace philips_series_2200
} // namespace esphome
