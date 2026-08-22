#pragma once

#include <cstdint>

namespace esphome {
namespace philips_series_2200 {
namespace checksum {

/// @brief marks a weight that has never been observed and cannot be computed
constexpr uint16_t UNKNOWN = 0xFFFF;

/// @brief checksum of a frame whose content bytes are all zero
constexpr uint16_t FRAME_CONSTANT = 0xE4D;

// The checksum is linear: the XOR of FRAME_CONSTANT and one fixed 12-bit weight
// per content byte, where the weight depends on the byte's distance from the
// last content byte rather than on its offset from the start of the frame. The
// weights below cover every value the display is known to show; see protocol.md
// for how they were obtained and why this is not a CRC.
//
// Columns are the values 0x01, 0x03, 0x07 and 0x38. Rows are the distance.
constexpr uint16_t WEIGHTS[15][4] = {
    /*  0 */ {0x204, UNKNOWN, 0xE5C, UNKNOWN},
    /*  1 */ {0x30C, 0x515, 0x966, 0x3E3},
    /*  2 */ {0x30D, 0x516, 0x921, 0x05B},
    /*  3 */ {0xB5D, 0xDE6, UNKNOWN, UNKNOWN},
    /*  4 */ {UNKNOWN, 0x3B7, 0xD59, UNKNOWN},
    /*  5 */ {UNKNOWN, UNKNOWN, 0x432, 0xCC8},
    /*  6 */ {0x8A2, UNKNOWN, 0x446, 0x232},
    /*  7 */ {0xD11, 0x772, 0x1B1, 0x056},
    /*  8 */ {UNKNOWN, UNKNOWN, 0xAAC, 0xFF7},
    /*  9 */ {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN},
    /* 10 */ {UNKNOWN, 0xF9E, 0xBCA, UNKNOWN},
    /* 11 */ {UNKNOWN, 0x9C1, 0x482, 0x590},
    /* 12 */ {UNKNOWN, 0xA73, 0x0B2, UNKNOWN},
    /* 13 */ {UNKNOWN, 0xA23, 0x05C, 0x52C},
    /* 14 */ {0xFEB, UNKNOWN, UNKNOWN, UNKNOWN},
};

/**
 * @brief Looks up the checksum weight of a single content byte.
 *
 * @param distance Distance from the last content byte
 * @param value Byte value
 * @param weight Receives the weight
 * @return false if this value has never been observed at this distance, which
 * for a frame from the mainboard means it is damaged
 */
constexpr bool lookup(uint8_t distance, uint8_t value, uint16_t *weight) {
  const uint16_t *row = WEIGHTS[distance];

  switch (value) {
  case 0x00:
    *weight = 0;
    return true;
  case 0x01:
    *weight = row[0];
    break;
  case 0x03:
    *weight = row[1];
    break;
  case 0x07:
    *weight = row[2];
    break;
  case 0x38:
    *weight = row[3];
    break;
  case 0x3F:
    // The display never sends 0x3F where 0x07 and 0x38 are not both known, but
    // the table is written by hand, so do not trust that.
    if (row[2] == UNKNOWN || row[3] == UNKNOWN)
      return false;
    *weight = row[2] ^ row[3];
    return true;
  default:
    return false;
  }

  return *weight != UNKNOWN;
}

/// @brief True for the byte values the display is known to use
constexpr bool known_value(uint8_t value) {
  return value == 0x00 || value == 0x01 || value == 0x03 || value == 0x07 ||
         value == 0x38 || value == 0x3F;
}

enum FrameCheck {
  /// @brief checksum recomputed and matches
  FRAME_VALID,
  /// @brief carries a value the display uses whose weight was never measured
  FRAME_UNVERIFIABLE,
  /// @brief checksum mismatch, or a byte the display would never send
  FRAME_DAMAGED,
};

/**
 * @brief Checks the checksum of a 19 byte frame from the mainboard.
 *
 * A missing weight must not count as damage. The table is built from captures,
 * so a state nobody has recorded yet would otherwise make every frame fail for
 * as long as the machine stays in it, freezing every sensor. Those frames are
 * reported separately so they can be used anyway and logged.
 *
 * @param frame Frame including header and checksum
 */
constexpr FrameCheck check_frame(const uint8_t *frame) {
  uint16_t sum = FRAME_CONSTANT;
  bool complete = true;

  for (uint8_t i = 2; i <= 16; i++) {
    if (!known_value(frame[i]))
      return FRAME_DAMAGED;

    uint16_t weight = 0;
    if (lookup(16 - i, frame[i], &weight))
      sum ^= weight;
    else
      complete = false;
  }

  if (!complete)
    return FRAME_UNVERIFIABLE;

  return sum == (uint16_t)((frame[17] << 6) | frame[18]) ? FRAME_VALID
                                                         : FRAME_DAMAGED;
}

namespace {
// Captured frames, verified at compile time so a typo in the table cannot ship.
// FRAME_IDLE is upstream's EP2220 capture, which doubles as a check that the
// weights really are machine independent.
constexpr uint8_t FRAME_OFF[19] = {0xD5, 0x55, 0, 0, 0, 0, 0, 0,    0,   0,
                                   0,    0,    0, 0, 0, 0, 0, 0x39, 0x0D};
constexpr uint8_t FRAME_IDLE[19] = {0xD5, 0x55, 0x00, 0x07, 0x07, 0x07, 0x07,
                                    0,    0,    0,    0,    0,    0,    0,
                                    0,    0,    0,    0x07, 0x2B};
constexpr uint8_t FRAME_BREWING[19] = {0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00,
                                       0x00, 0x3F, 0x07, 0x38, 0x07, 0x00, 0x00,
                                       0x00, 0x00, 0x07, 0x19, 0x39};
constexpr uint8_t FRAME_POWDER[19] = {0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00,
                                      0x00, 0x00, 0x38, 0x38, 0x07, 0x00, 0x00,
                                      0x00, 0x00, 0x07, 0x0B, 0x05};
constexpr uint8_t FRAME_CORRUPT[19] = {0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00,
                                       0x00, 0x00, 0x38, 0x38, 0x07, 0x00, 0x00,
                                       0x00, 0x00, 0x07, 0x0B, 0x04};
// byte 7 has never been seen lit, so its weights are unmeasured
constexpr uint8_t FRAME_UNMEASURED[19] = {
    0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x38,
    0x38, 0x07, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0B, 0x05};
// 0x04 is not a value the display uses
constexpr uint8_t FRAME_GARBAGE[19] = {0xD5, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00,
                                       0x00, 0x00, 0x38, 0x04, 0x07, 0x00, 0x00,
                                       0x00, 0x00, 0x07, 0x0B, 0x05};

static_assert(check_frame(FRAME_OFF) == FRAME_VALID, "rejects a valid frame");
static_assert(check_frame(FRAME_IDLE) == FRAME_VALID, "rejects a valid frame");
static_assert(check_frame(FRAME_BREWING) == FRAME_VALID,
              "rejects a valid frame");
static_assert(check_frame(FRAME_POWDER) == FRAME_VALID,
              "rejects a valid frame");
static_assert(check_frame(FRAME_CORRUPT) == FRAME_DAMAGED,
              "accepts a damaged frame");
static_assert(check_frame(FRAME_UNMEASURED) == FRAME_UNVERIFIABLE,
              "an unmeasured weight must not count as damage");
static_assert(check_frame(FRAME_GARBAGE) == FRAME_DAMAGED,
              "accepts a value the display never sends");
} // namespace

} // namespace checksum
} // namespace philips_series_2200
} // namespace esphome
