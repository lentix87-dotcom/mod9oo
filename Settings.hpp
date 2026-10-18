#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace mm {

// ---------- список функций (флагов) ----------
enum class F : int {
    Noclip, Jump,                       // читы (ломают рекорды)
    Safe, AutoSafe,                     // защита
    Hitboxes, NoDeath,                  // уровень
    Overlay, ShowFPS, HidePlayer, HideAttempts, NoShake, NoMirror,
    RainbowIcon, RainbowBG, UnlockAll,  // косметика
    Glass,                              // тема
    COUNT
};
inline constexpr size_t FCOUNT = static_cast<size_t>(F::COUNT);

inline char const* key(F x) {
    static constexpr char const* K[] = {
        "noclip", "jump", "safe_manual", "autosafe",
        "hitboxes", "nodeath",
        "overlay", "showfps", "hideplayer", "hideattempts", "noshake", "nomirror",
        "rainbowicon", "rainbowbg", "unlockall",
        "glass"};
    return K[static_cast<int>(x)];
}
// Читерные функции: из-за них результаты нельзя отправлять в глобальный топ
inline bool isCheat(F x) { return x == F::Noclip || x == F::Jump; }
inline bool defaultValue(F x) { return x == F::AutoSafe || x == F::Overlay || x == F::Glass; }
// Читы намеренно не сохраняются: после перезапуска они выключены
inline bool persists(F x) { return !isCheat(x); }

inline constexpr std::array<float, 6> SPEEDS{0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f};
inline constexpr std::array<float, 3> GLASS_OPACITY{0.72f, 0.84f, 0.94f}; // Low / Mid / High
inline constexpr float VMIN = 0.35f; // минимальная яркость акцента, чтобы UI не пропадал

inline std::array<cocos2d::ccColor3B, 8> const& presets() {
    static std::array<cocos2d::ccColor3B, 8> const P{{
        {0, 200, 255}, {70, 130, 255}, {150, 100, 255}, {235, 90, 255},
        {255, 80, 150}, {255, 90, 70}, {255, 170, 50}, {90, 220, 140}}};
    return P;
}

struct State {
    std::array<bool, FCOUNT> f{};
    int speedIdx = 3;          // 3 = x1.0 (не сохраняется)
    float hue = 0.54f;         // акцентный цвет в HSV
    float sat = 1.f;
    float val = 1.f;
    int glassOpacity = 1;
};

inline State& state() {
    static State s;
    return s;
}

// "Заражённая" сессия: читы использовались в этом уровне, даже если их потом выключили
inline bool& tainted() {
    static bool t = false;
    return t;
}

inline bool on(F x) { return state().f[static_cast<size_t>(x)]; }
inline void set(F x, bool v) { state().f[static_cast<size_t>(x)] = v; }

// ---------- цвет ----------
inline cocos2d::ccColor3B hsv(float h, float s, float v) {
    h = h - std::floor(h);
    float x = h * 6.f;
    int i = static_cast<int>(x);
    float f = x - i;
    float p = v * (1.f - s), q = v * (1.f - s * f), t = v * (1.f - s * (1.f - f));
    float r, g, b;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return {static_cast<GLubyte>(std::lround(r * 255.f)),
            static_cast<GLubyte>(std::lround(g * 255.f)),
            static_cast<GLubyte>(std::lround(b * 255.f))};
}
inline cocos2d::ccColor3B rainbow(float h) { return hsv(h, 1.f, 1.f); }

inline void rgb2hsv(cocos2d::ccColor3B c, float& h, float& s, float& v) {
    float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
    float mx = std::max({r, g, b}), mn = std::min({r, g, b});
    float d = mx - mn;
    v = mx;
    s = mx <= 0.f ? 0.f : d / mx;
    if (d <= 0.f) { h = 0.f; return; }
    if (mx == r) h = std::fmod((g - b) / d, 6.f);
    else if (mx == g) h = (b - r) / d + 2.f;
    else h = (r - g) / d + 4.f;
    h /= 6.f;
    if (h < 0.f) h += 1.f;
}

inline cocos2d::ccColor3B accent() {
    auto& s = state();
    return hsv(s.hue, s.sat, std::max(s.val, VMIN));
}

// ---------- сохранение ----------
inline void save(F x) {
    if (persists(x)) geode::Mod::get()->setSavedValue<bool>(key(x), on(x));
}
inline void saveTheme() {
    auto m = geode::Mod::get();
    auto& s = state();
    m->setSavedValue<double>("accH", s.hue);
    m->setSavedValue<double>("accS", s.sat);
    m->setSavedValue<double>("accV", s.val);
    m->setSavedValue<int>("glassOpacity", s.glassOpacity);
}
inline void load() {
    auto m = geode::Mod::get();
    auto& s = state();
    for (size_t i = 0; i < FCOUNT; i++) {
        F x = static_cast<F>(i);
        s.f[i] = persists(x) ? m->getSavedValue<bool>(key(x), defaultValue(x)) : false;
    }
    s.hue = std::clamp(static_cast<float>(m->getSavedValue<double>("accH", 0.54)), 0.f, 1.f);
    s.sat = std::clamp(static_cast<float>(m->getSavedValue<double>("accS", 1.0)), 0.f, 1.f);
    s.val = std::clamp(static_cast<float>(m->getSavedValue<double>("accV", 1.0)), VMIN, 1.f);
    s.glassOpacity = std::clamp(m->getSavedValue<int>("glassOpacity", 1), 0, 2);
}

// ---------- читы и Safe mode ----------
inline float speed() { return SPEEDS[state().speedIdx]; }
inline bool cheating() { return on(F::Noclip) || on(F::Jump) || state().speedIdx != 3; }

// Safe mode активен, если он включён вручную, ИЛИ включён Auto safe и были читы
// (в том числе использованные ранее в этом уровне)
inline bool safeActive() {
    return on(F::Safe) || (on(F::AutoSafe) && (cheating() || tainted()));
}

inline std::string cheatList() {
    std::string s;
    auto add = [&](char const* n) { if (!s.empty()) s += ", "; s += n; };
    if (on(F::Noclip)) add("noclip");
    if (on(F::Jump)) add("jump hack");
    if (state().speedIdx != 3) add("speed");
    if (s.empty()) s = "earlier cheat";
    return s;
}

// ---------- применение ----------
// Скорость применяем только внутри уровня, чтобы не ломать меню
inline void applySpeed(bool inLevel) {
    cocos2d::CCDirector::get()->getScheduler()->setTimeScale(inLevel ? speed() : 1.f);
}

// Встроенные игровые переменные хитбоксов (остальное форсируется хуком в PerFrame.cpp)
inline void applyHitboxes() {
    auto gm = GameManager::get();
    gm->setGameVariable("0166", on(F::Hitboxes));
    gm->setGameVariable("0171", on(F::Hitboxes));
}

}
