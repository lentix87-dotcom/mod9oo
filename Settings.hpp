#pragma once
#include <Geode/Geode.hpp>
#include <array>
#include <cmath>

namespace mm {

// ---------- список функций (флагов) ----------
enum class F : int {
    Noclip, Safe, Hitboxes, NoDeath,        // Player / Level
    Overlay, HidePlayer, RainbowIcon, RainbowBG, UnlockAll, // Visual
    Glass,                                   // Theme
    COUNT
};
inline constexpr size_t FCOUNT = static_cast<size_t>(F::COUNT);

inline char const* key(F x) {
    static constexpr char const* K[] = {
        "noclip", "safe", "hitboxes", "nodeath",
        "overlay", "hideplayer", "rainbowicon", "rainbowbg", "unlockall",
        "glass"};
    return K[static_cast<int>(x)];
}
inline bool defaultValue(F x) {
    return x == F::Safe || x == F::Overlay || x == F::Glass;
}
// Noclip намеренно не сохраняется: после перезапуска читы выключены
inline bool persists(F x) { return x != F::Noclip; }

inline constexpr std::array<float, 6> SPEEDS{0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f};
inline constexpr std::array<float, 3> GLASS_OPACITY{90.f, 130.f, 175.f}; // Low / Mid / High

inline std::array<cocos2d::ccColor3B, 12> const& accents() {
    static std::array<cocos2d::ccColor3B, 12> const A{{
        {0, 200, 255}, {60, 140, 255}, {140, 100, 255}, {220, 90, 255},
        {255, 80, 160}, {255, 70, 70}, {255, 150, 40}, {255, 220, 50},
        {120, 230, 70}, {40, 220, 140}, {235, 235, 235}, {120, 130, 150}}};
    return A;
}

struct State {
    std::array<bool, FCOUNT> f{};
    int speedIdx = 3;      // 3 = x1.0 (не сохраняется)
    int accent = 0;        // индекс в accents()
    int glassOpacity = 1;  // индекс в GLASS_OPACITY
};

inline State& state() {
    static State s;
    return s;
}

inline bool on(F x) { return state().f[static_cast<size_t>(x)]; }
inline void set(F x, bool v) { state().f[static_cast<size_t>(x)] = v; }

inline void save(F x) {
    if (persists(x)) geode::Mod::get()->setSavedValue<bool>(key(x), on(x));
}
inline void saveTheme() {
    auto m = geode::Mod::get();
    m->setSavedValue<int>("accent", state().accent);
    m->setSavedValue<int>("glassOpacity", state().glassOpacity);
}

inline void load() {
    auto m = geode::Mod::get();
    auto& s = state();
    for (size_t i = 0; i < FCOUNT; i++) {
        F x = static_cast<F>(i);
        s.f[i] = persists(x) ? m->getSavedValue<bool>(key(x), defaultValue(x)) : false;
    }
    s.accent = std::clamp(m->getSavedValue<int>("accent", 0), 0, 11);
    s.glassOpacity = std::clamp(m->getSavedValue<int>("glassOpacity", 1), 0, 2);
}

// ---------- производные значения ----------
inline float speed() { return SPEEDS[state().speedIdx]; }
inline bool cheating() { return on(F::Noclip) || state().speedIdx != 3; }
// Safe mode: если включён и есть читы, результат прохождения не сохраняется
inline bool safeActive() { return cheating() && on(F::Safe); }
inline cocos2d::ccColor3B accent() { return accents()[state().accent]; }

inline cocos2d::ccColor3B hsv(float h) { // h в [0,1)
    h = h - std::floor(h);
    float r, g, b;
    float x = h * 6.f;
    int i = static_cast<int>(x);
    float fr = x - i;
    float q = 1.f - fr;
    switch (i % 6) {
        case 0: r = 1; g = fr; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = fr; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = fr; g = 0; b = 1; break;
        default: r = 1; g = 0; b = q; break;
    }
    return {static_cast<GLubyte>(r * 255), static_cast<GLubyte>(g * 255), static_cast<GLubyte>(b * 255)};
}

// ---------- применение ----------
// Скорость применяем только внутри уровня, чтобы не ломать меню
inline void applySpeed(bool inLevel) {
    cocos2d::CCDirector::get()->getScheduler()->setTimeScale(inLevel ? speed() : 1.f);
}

// Хитбоксы: включаем встроенные игровые переменные
inline void applyHitboxes() {
    auto gm = GameManager::get();
    gm->setGameVariable("0166", on(F::Hitboxes));
    gm->setGameVariable("0171", on(F::Hitboxes));
}

}
