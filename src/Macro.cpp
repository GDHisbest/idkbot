#include "Macro.hpp"

#include <algorithm>

namespace idk {

namespace {

void putVarint(std::vector<uint8_t>& out, uint64_t value) {
    while (value >= 0x80) {
        out.push_back(static_cast<uint8_t>(value | 0x80));
        value >>= 7;
    }
    out.push_back(static_cast<uint8_t>(value));
}

// Reads one varint. Returns false if the data ends early or the number is too long.
bool getVarint(uint8_t const* data, size_t size, size_t& pos, uint64_t& value) {
    value = 0;
    for (int shift = 0; shift <= 63; shift += 7) {
        if (pos >= size) return false;
        uint8_t byte = data[pos++];
        value |= static_cast<uint64_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) return true;
    }
    return false;
}

}

bool Macro::isValid() const {
    uint32_t previous = 0;
    for (auto const& input : inputs) {
        if (input.button > 3) return false;
        if (input.step < previous) return false;
        previous = input.step;
    }
    return true;
}

std::vector<uint8_t> Macro::encode() const {
    if (!isValid()) return {};

    std::vector<uint8_t> out;
    out.reserve(8 + inputs.size() * 2);

    out.push_back('I');
    out.push_back('D');
    out.push_back('K');
    out.push_back(kFormatVersion);
    putVarint(out, tps);
    putVarint(out, inputs.size());

    uint32_t previous = 0;
    for (auto const& input : inputs) {
        uint64_t flags = (input.down ? 1u : 0u)
            | (static_cast<uint64_t>(input.button) << 1)
            | (input.isPlayer1 ? 8u : 0u);
        uint64_t delta = input.step - previous;
        putVarint(out, (delta << 4) | flags);
        previous = input.step;
    }

    return out;
}

bool Macro::decode(uint8_t const* data, size_t size, Macro& out, std::string& error) {
    if (size < 4 || data[0] != 'I' || data[1] != 'D' || data[2] != 'K') {
        error = "not an idkbot macro (bad signature)";
        return false;
    }
    if (data[3] != kFormatVersion) {
        error = "unsupported macro format version " + std::to_string(data[3]);
        return false;
    }

    size_t pos = 4;
    uint64_t tps = 0, count = 0;
    if (!getVarint(data, size, pos, tps) || tps == 0 || tps > 0xFFFF) {
        error = "invalid tps in header";
        return false;
    }
    if (!getVarint(data, size, pos, count)) {
        error = "invalid input count in header";
        return false;
    }
    // every input needs at least one byte, so a bigger count than the remaining data is corrupt
    if (count > size - pos) {
        error = "input count is larger than the file";
        return false;
    }

    Macro macro;
    macro.tps = static_cast<uint16_t>(tps);
    macro.inputs.reserve(static_cast<size_t>(count));

    uint64_t step = 0;
    for (uint64_t i = 0; i < count; i++) {
        uint64_t value = 0;
        if (!getVarint(data, size, pos, value)) {
            error = "file ends in the middle of the input list";
            return false;
        }
        step += value >> 4;
        if (step > 0xFFFFFFFFull) {
            error = "step number is too large";
            return false;
        }

        Input input;
        input.step = static_cast<uint32_t>(step);
        input.down = (value & 1) != 0;
        input.button = static_cast<uint8_t>((value >> 1) & 3);
        input.isPlayer1 = (value & 8) != 0;
        macro.inputs.push_back(input);
    }

    if (pos != size) {
        error = "unexpected data after the input list";
        return false;
    }

    out = std::move(macro);
    return true;
}

void Macro::truncateAfter(uint32_t step) {
    auto it = std::find_if(inputs.begin(), inputs.end(), [step](Input const& input) {
        return input.step > step;
    });
    inputs.erase(it, inputs.end());
}

}
