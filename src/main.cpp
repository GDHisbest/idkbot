// idkbot, step 1: hooks that only LOG. They show, on a real device, how physics steps
// and inputs line up in GD 2.2081, so recording and playback can be built on facts.
//
// Signatures below are taken from the Geode bindings for 2.2081:
//   GJBaseGameLayer::processCommands(float dt, bool isHalfTick, bool isLastTick)
//   GJBaseGameLayer::processQueuedButtons(float dt, bool clearInputQueue)
//   GJBaseGameLayer::handleButton(bool down, int button, bool isPlayer1)
//   PlayLayer::resetLevel()
// and the step counter is GJBaseGameLayer::m_gameState.m_currentProgress.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "Macro.hpp"

using namespace geode::prelude;

namespace {

// Per-attempt trace: only the first kMaxTraceEvents step events after a (re)start are logged,
// otherwise the log would get ~240 lines per second. Button events are always logged.
constexpr int kMaxTraceEvents = 200;
int g_traceEvents = 0;
int g_sequence = 0;

bool isPlaying(GJBaseGameLayer* layer) {
    auto pl = PlayLayer::get();
    return pl && static_cast<GJBaseGameLayer*>(pl) == layer;
}

}

class $modify(IdkBaseGameLayer, GJBaseGameLayer) {
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        bool trace = isPlaying(this) && g_traceEvents < kMaxTraceEvents;
        unsigned int before = m_gameState.m_currentProgress;

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (trace) {
            g_traceEvents++;
            log::info(
                "[idkbot] #{} processCommands dt={} half={} last={} step {} -> {}",
                g_sequence++, dt, isHalfTick, isLastTick, before, m_gameState.m_currentProgress
            );
        }
    }

    void processQueuedButtons(float dt, bool clearInputQueue) {
        bool trace = isPlaying(this) && g_traceEvents < kMaxTraceEvents;
        unsigned int before = m_gameState.m_currentProgress;

        GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);

        if (trace) {
            g_traceEvents++;
            log::info(
                "[idkbot] #{} processQueuedButtons dt={} clear={} step {} -> {}",
                g_sequence++, dt, clearInputQueue, before, m_gameState.m_currentProgress
            );
        }
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        if (isPlaying(this)) {
            log::info(
                "[idkbot] #{} handleButton down={} button={} isPlayer1={} step={}",
                g_sequence++, down, button, isPlayer1, m_gameState.m_currentProgress
            );
        }
    }
};

class $modify(IdkPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();

        g_traceEvents = 0;
        g_sequence = 0;
        log::info("[idkbot] resetLevel, step is now {}", m_gameState.m_currentProgress);
    }
};

$on_mod(Loaded) {
    log::info("[idkbot] loaded, macro format v{}", idk::Macro::kFormatVersion);
}
