#pragma once
#include <Geode/Geode.hpp>
#include <array>

namespace mm {

inline constexpr std::array<float, 6> SPEEDS{0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f};

struct State {
    bool noclip = false;     // намеренно НЕ сохраняется: после перезапуска читы выключены
    bool overlay = true;     // инфо-оверлей в уровне
    int speedIdx = 3;        // индекс в SPEEDS (3 = x1.0), тоже не сохраняется

    float speed() const { return SPEEDS[speedIdx]; }
    bool cheating() const { return noclip || speedIdx != 3; }
};

inline State& state() {
    static State s;
    return s;
}

inline void load() {
    state().overlay = geode::Mod::get()->getSavedValue<bool>("overlay", true);
}

inline void saveOverlay() {
    geode::Mod::get()->setSavedValue<bool>("overlay", state().overlay);
}

// Скорость применяем только внутри уровня, чтобы не ломать меню
inline void applySpeed(bool inLevel) {
    cocos2d::CCDirector::get()->getScheduler()->setTimeScale(inLevel ? state().speed() : 1.f);
}

}
