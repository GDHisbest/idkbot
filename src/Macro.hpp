#pragma once

// Macro data + the ".idk" file format. Plain C++ (no Geode types) so it can be
// unit-tested on a normal computer, see tests/macro_test.cpp.
//
// FORMAT (version 1), all numbers are unsigned LEB128 varints unless noted:
//
//   bytes 0..2   magic 'I' 'D' 'K'
//   byte  3      format version (1)
//   varint       tps          (physics steps per second the macro was recorded at, e.g. 240)
//   varint       input count
//   then, per input, ONE varint:
//
//        value = (stepDelta << 4) | flags
//
//        stepDelta = step - previousInputStep   (previousInputStep starts at 0)
//        flags bit 0     : 1 = button pressed, 0 = released
//        flags bits 1..2 : button (0..3, GD's PlayerButton: 1 jump, 2 left, 3 right)
//        flags bit 3     : the third argument of GJBaseGameLayer::handleButton, stored
//                          exactly as the game passed it and handed back unchanged
//
// A typical input is 2 bytes (3 when 1024+ steps pass without any input).

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace idk {

struct Input {
    uint32_t step = 0;      // physics step (GJGameState::m_currentProgress) the input belongs to
    uint8_t button = 1;     // 0..3
    bool down = false;
    bool isPlayer1 = true;  // raw 3rd argument of handleButton, not interpreted

    bool operator==(Input const&) const = default;
};

struct Macro {
    static constexpr uint8_t kFormatVersion = 1;
    static constexpr uint16_t kDefaultTps = 240;

    uint16_t tps = kDefaultTps;
    std::vector<Input> inputs;

    // Inputs must be in non-decreasing step order and button <= 3.
    bool isValid() const;

    // Serialises the macro. Returns an empty vector if the macro is not valid.
    std::vector<uint8_t> encode() const;

    // Parses a macro. On failure returns false and fills `error`; `out` is left untouched.
    static bool decode(uint8_t const* data, size_t size, Macro& out, std::string& error);
    static bool decode(std::vector<uint8_t> const& bytes, Macro& out, std::string& error) {
        return decode(bytes.data(), bytes.size(), out, error);
    }

    // Removes every input recorded after `step` (used when a run restarts from an earlier step).
    void truncateAfter(uint32_t step);
};

}
