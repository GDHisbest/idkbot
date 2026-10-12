// Host-side test for the macro codec. Not part of the mod build.
//   g++ -std=c++23 -O1 -Wall -Wextra -I src tests/macro_test.cpp src/Macro.cpp -o macro_test && ./macro_test

#include "Macro.hpp"

#include <cstdio>
#include <cstdlib>
#include <random>

using namespace idk;

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);              \
            failures++;                                                      \
        }                                                                    \
    } while (0)

static Macro randomMacro(std::mt19937& rng, size_t count, uint32_t maxGap) {
    Macro m;
    m.tps = 240;
    uint32_t step = 0;
    for (size_t i = 0; i < count; i++) {
        step += rng() % (maxGap + 1);
        Input in;
        in.step = step;
        in.button = static_cast<uint8_t>(rng() % 4);
        in.down = rng() & 1;
        in.isPlayer1 = rng() & 1;
        m.inputs.push_back(in);
    }
    return m;
}

int main() {
    std::mt19937 rng(12345);

    // empty macro
    {
        Macro m;
        auto bytes = m.encode();
        CHECK(!bytes.empty());
        Macro back; std::string err;
        CHECK(Macro::decode(bytes, back, err));
        CHECK(back.inputs.empty());
        CHECK(back.tps == 240);
        std::printf("empty macro: %zu bytes\n", bytes.size());
    }

    // round trips of many random macros, including big gaps and same-step inputs
    for (int round = 0; round < 500; round++) {
        size_t count = rng() % 300;
        uint32_t gap = (round % 3 == 0) ? 3 : (round % 3 == 1 ? 400 : 100000);
        Macro m = randomMacro(rng, count, gap);
        m.tps = static_cast<uint16_t>(1 + rng() % 1000);
        auto bytes = m.encode();
        CHECK(!bytes.empty());
        Macro back; std::string err;
        bool ok = Macro::decode(bytes, back, err);
        CHECK(ok);
        if (ok) {
            CHECK(back.tps == m.tps);
            CHECK(back.inputs == m.inputs);
        }
    }

    // size on a realistic macro: 1000 inputs, a press/release every 5..200 steps
    {
        Macro m;
        uint32_t step = 0;
        for (int i = 0; i < 1000; i++) {
            step += 5 + rng() % 196;
            m.inputs.push_back({step, 1, (i % 2) == 0, true});
        }
        auto bytes = m.encode();
        std::printf("1000 inputs: %zu bytes (%.2f bytes/input)\n", bytes.size(), bytes.size() / 1000.0);
        CHECK(bytes.size() < 2200);
    }

    // largest legal step
    {
        Macro m;
        m.inputs.push_back({0xFFFFFFFFu, 3, true, false});
        auto bytes = m.encode();
        Macro back; std::string err;
        CHECK(Macro::decode(bytes, back, err));
        CHECK(back.inputs == m.inputs);
    }

    // invalid macros must not encode
    {
        Macro unsorted; unsorted.inputs = {{10, 1, true, true}, {5, 1, false, true}};
        CHECK(unsorted.encode().empty());
        Macro badButton; badButton.inputs = {{1, 7, true, true}};
        CHECK(badButton.encode().empty());
    }

    // corrupt / truncated files must fail cleanly, never crash
    {
        Macro m = randomMacro(rng, 50, 200);
        auto bytes = m.encode();
        for (size_t cut = 0; cut < bytes.size(); cut++) {
            Macro back; std::string err;
            CHECK(!Macro::decode(bytes.data(), cut, back, err));
        }
        auto withTail = bytes; withTail.push_back(0);
        Macro back; std::string err;
        CHECK(!Macro::decode(withTail, back, err));

        auto badMagic = bytes; badMagic[0] = 'X';
        CHECK(!Macro::decode(badMagic, back, err));

        auto badVersion = bytes; badVersion[3] = 99;
        CHECK(!Macro::decode(badVersion, back, err));

        // random garbage
        for (int i = 0; i < 2000; i++) {
            std::vector<uint8_t> junk(rng() % 40);
            for (auto& b : junk) b = static_cast<uint8_t>(rng());
            if (junk.size() >= 4 && (i % 2) == 0) { junk[0] = 'I'; junk[1] = 'D'; junk[2] = 'K'; junk[3] = 1; }
            Macro out; std::string e;
            Macro::decode(junk, out, e);   // only checking that it does not crash
        }
    }

    // a failed decode leaves the output untouched
    {
        Macro keep; keep.inputs = {{1, 1, true, true}};
        std::vector<uint8_t> junk = {'I', 'D', 'K', 1, 0xF0};
        std::string err;
        CHECK(!Macro::decode(junk, keep, err));
        CHECK(keep.inputs.size() == 1);
    }

    // truncateAfter
    {
        Macro m; m.inputs = {{1, 1, true, true}, {5, 1, false, true}, {5, 2, true, true}, {9, 1, true, true}};
        m.truncateAfter(5);
        CHECK(m.inputs.size() == 3);
        m.truncateAfter(0);
        CHECK(m.inputs.empty());
    }

    if (failures == 0) std::printf("ALL TESTS PASSED\n");
    else std::printf("%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
