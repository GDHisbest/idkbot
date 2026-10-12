# idkbot

Macro bot for Geometry Dash 2.2081 (Android64), built with Geode 5.

Status: v0.1.0 skeleton. Hooks only LOG physics steps and inputs (see `src/main.cpp`);
the `.idk` macro format is implemented and tested (`src/Macro.*`, `tests/macro_test.cpp`).

Build: push to GitHub, the `Build idkbot (Android64)` workflow produces the `.geode` file as an artifact.

Format test (on a computer):
    g++ -std=c++23 -O1 -Wall -Wextra -I src tests/macro_test.cpp src/Macro.cpp -o macro_test && ./macro_test
