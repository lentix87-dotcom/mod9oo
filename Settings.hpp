#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace mm {

// ---------- Список функций ----------
enum class F : int {
    Noclip, Jump,                         // Player (читы)
    Safe, AutoSafe,                       // Player (защита)
    Hitboxes, NoDeath,                    // Level
    StartPos, SmartStart, FrameSmooth,    // Level
    TpsBypass,                            // Level (TPS Bypass)
    Overlay, ShowFPS, HidePlayer, HideAttempts, NoShake, NoMirror,
    RainbowIcon, RainbowBG, UnlockAll,    // Visual
    Glow,                                 // Theme: белое свечение в меню
    LiquidGlass,                          // Theme: мыльное стекло HyperOS 4.0
    BgBlur,                               // Theme: размытие экрана позади открытого меню
    BtnShowInMenu,                        // Client: показывать кнопку L во всех меню
    BtnShowInGame,                        // Client: показывать кнопку L во время самой игры
    BtnShowInPause,                       // Client: показывать кнопку L в паузе
    OvPercent, OvCPS, OvTps, OvAttempts, OvStatus, OvBg, // Под-настройки Overlay
    COUNT
};
inline constexpr size_t FCOUNT = static_cast<size_t>(F::COUNT);

inline char const* key(F x) {
    static constexpr char const* K[] = {
        "noclip", "jump", "safe_manual", "autosafe",
        "hitboxes", "nodeath",
        "startpos", "smartstart", "framesmooth",
        "tpsbypass",
        "overlay", "showfps", "hideplayer", "hideattempts", "noshake", "nomirror",
        "rainbowicon", "rainbowbg", "unlockall",
        "glow", "liquidglass", "bgblur",
        "btnshowinmenu", "btnshowingame", "btnshowinpause",
        "ov_percent", "ov_cps", "ov_tps", "ov_attempts", "ov_status", "ov_bg"
    };
    return K[static_cast<int>(x)];
}

inline char const* title(F x) {
    switch (x) {
        case F::Noclip: return "Noclip";
        case F::Jump: return "Jump Hack";
        case F::Safe: return "Safe Mode";
        case F::AutoSafe: return "Auto Safe Mode";
        case F::Hitboxes: return "Show Hitboxes";
        case F::NoDeath: return "No Death Effect";
        case F::StartPos: return "StartPos Switcher";
        case F::SmartStart: return "Smart StartPos";
        case F::FrameSmooth: return "Frame Smoothing";
        case F::TpsBypass: return "TPS Bypass";
        case F::Overlay: return "Overlay";
        case F::ShowFPS: return "Show FPS";
        case F::HidePlayer: return "Hide Player";
        case F::HideAttempts: return "Hide Attempts";
        case F::NoShake: return "No Shake";
        case F::NoMirror: return "No Mirror";
        case F::RainbowIcon: return "Rainbow Icon";
        case F::RainbowBG: return "Rainbow BG";
        case F::UnlockAll: return "Unlock All Icons";
        case F::Glow: return "White Glow";
        case F::LiquidGlass: return "HyperOS Glass";
        case F::BgBlur: return "Screen Blur";
        case F::BtnShowInMenu: return "Show in Menus";
        case F::BtnShowInGame: return "Show in Gameplay";
        case F::BtnShowInPause: return "Show in Pause";
        default: return "";
    }
}

// StartPos и SmartStart НЕ считаются читами и НЕ горят красным!
inline bool isRisky(F x) {
    switch (x) {
        case F::Noclip: case F::Jump: case F::Hitboxes:
        case F::NoMirror: case F::TpsBypass:
            return true;
        default:
            return false;
    }
}

inline bool defaultValue(F x) {
    switch (x) {
        case F::AutoSafe:
        case F::Overlay:
        case F::ShowFPS:
        case F::Glow:
        case F::LiquidGlass:
        case F::BgBlur:
        case F::BtnShowInMenu:
        case F::BtnShowInPause:
        case F::OvPercent:
        case F::OvCPS:
        case F::OvStatus:
        case F::OvBg:
            return true;
        default:
            return false;
    }
}

inline bool persists(F x) { return !isRisky(x); }

// ---------- Темы в стиле HyperOS 4.0 Liquid Glass ----------
struct ThemePreset {
    char const* name;
    cocos2d::ccColor3B bg;
    cocos2d::ccColor3B card;
    cocos2d::ccColor3B accent;
    cocos2d::ccColor3B text;
};

inline constexpr std::array<ThemePreset, 8> THEMES{{
    {"HyperOS Frost",  {20, 22, 32}, {32, 35, 50}, {135, 165, 255}, {244, 246, 255}},
    {"Obsidian Glass", {13, 14, 20}, {22, 24, 34}, {160, 145, 255}, {240, 242, 250}},
    {"Monochrome B&W", {12, 12, 15}, {24, 24, 30}, {245, 245, 250}, {250, 250, 255}},
    {"Sakura Mist",    {24, 16, 26}, {38, 25, 42}, {255, 140, 195}, {252, 240, 248}},
    {"Azure Crystal",  {14, 20, 32}, {22, 32, 50}, {80, 180, 255},  {238, 246, 255}},
    {"Crimson Velvet", {24, 14, 18}, {38, 22, 28}, {255, 90, 110},  {252, 238, 240}},
    {"Emerald Dew",    {14, 23, 20}, {22, 36, 32}, {80, 235, 170},  {238, 252, 245}},
    {"Custom Palette", {20, 22, 32}, {32, 35, 50}, {135, 165, 255}, {244, 246, 255}}
}};

inline constexpr std::array<float, 6> SPEEDS{0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f};

struct State {
    std::array<bool, FCOUNT> f{};
    int speedIdx = 3;
    int customTps = 240;
    float rainbowSpeed = 0.25f;

    // Настройки HyperOS 4.0 Liquid Glass & Blur
    int particleType = 3;        // 0 = Off, 1 = Winter, 2 = Autumn, 3 = Sakura
    int themeIdx = 0;            // 0 = HyperOS Frost, 7 = Custom Palette
    float glowIntensity = 0.60f; // 0.0 .. 1.0
    float glassAlpha = 0.78f;    // Мыльность/прозрачность стекла меню (0.40 .. 0.96)
    float bgBlurStrength = 0.75f;// Сила размытия экрана позади меню (0.0 .. 1.0)
    float menuRadius = 18.0f;    // Скругление углов самого мод-меню (0.0 .. 28.0 px)
    int pressAnimType = 1;       // 0 = Off, 1 = Bounce, 2 = Shrink
    float pressAnimPower = 0.88f;

    // Кастомная палитра (0 = Accent, 1 = Background, 2 = Containers)
    int editSlot = 0;
    float customH[3] = {0.62f, 0.64f, 0.64f};
    float customS[3] = {0.48f, 0.35f, 0.32f};
    float customV[3] = {1.00f, 0.13f, 0.20f};

    // Плавающая кнопка L
    float btnX = -1.f;
    float btnY = -1.f;
    float btnOpacity = 0.92f;    // 0.15 .. 1.0
    float btnScale = 1.00f;      // 0.55 .. 1.60
    float btnRadius = 10.0f;     // 0.0 .. 18.0

    // Флаг: открыто ли сейчас само мод-меню (чтобы скрывать кнопку L)
    bool menuOpen = false;

    // Настройки Overlay
    int ovCorner = 0;
    float ovScale = 0.85f;
    float ovOpacity = 0.85f;
};

inline State& state() {
    static State s;
    return s;
}

inline bool& tainted() {
    static bool t = false;
    return t;
}

inline bool on(F x) { return state().f[static_cast<size_t>(x)]; }
inline void set(F x, bool v) { state().f[static_cast<size_t>(x)] = v; }

// ---------- Цветовые утилиты ----------
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
inline cocos2d::ccColor3B rainbow(float h) { return hsv(h, 0.85f, 1.f); }

inline cocos2d::ccColor3B themeBg() {
    auto& s = state();
    if (s.themeIdx == 7) return hsv(s.customH[1], s.customS[1], std::clamp(s.customV[1], 0.04f, 0.95f));
    return THEMES[std::clamp(s.themeIdx, 0, 6)].bg;
}
inline cocos2d::ccColor3B themeCard() {
    auto& s = state();
    if (s.themeIdx == 7) return hsv(s.customH[2], s.customS[2], std::clamp(s.customV[2], 0.06f, 0.98f));
    return THEMES[std::clamp(s.themeIdx, 0, 6)].card;
}
inline cocos2d::ccColor3B accent() {
    auto& s = state();
    if (s.themeIdx == 7) return hsv(s.customH[0], s.customS[0], std::clamp(s.customV[0], 0.35f, 1.0f));
    return THEMES[std::clamp(s.themeIdx, 0, 6)].accent;
}
inline cocos2d::ccColor3B themeText() {
    auto& s = state();
    if (s.themeIdx == 7) return {244, 246, 255};
    return THEMES[std::clamp(s.themeIdx, 0, 6)].text;
}

// ---------- Сохранение и загрузка ----------
inline void save(F x) {
    if (persists(x)) geode::Mod::get()->setSavedValue<bool>(key(x), on(x));
}

inline void saveAll() {
    auto m = geode::Mod::get();
    auto& s = state();
    for (size_t i = 0; i < FCOUNT; i++) {
        F x = static_cast<F>(i);
        if (persists(x)) m->setSavedValue<bool>(key(x), s.f[i]);
    }
    m->setSavedValue<int>("customTps", s.customTps);
    m->setSavedValue<int>("particleType", s.particleType);
    m->setSavedValue<int>("themeIdx", s.themeIdx);
    m->setSavedValue<double>("glowIntensity", s.glowIntensity);
    m->setSavedValue<double>("glassAlpha", s.glassAlpha);
    m->setSavedValue<double>("bgBlurStrength", s.bgBlurStrength);
    m->setSavedValue<double>("menuRadius", s.menuRadius);
    m->setSavedValue<int>("pressAnimType", s.pressAnimType);
    m->setSavedValue<double>("btnX", s.btnX);
    m->setSavedValue<double>("btnY", s.btnY);
    m->setSavedValue<double>("btnOpacity", s.btnOpacity);
    m->setSavedValue<double>("btnScale", s.btnScale);
    m->setSavedValue<double>("btnRadius", s.btnRadius);
    m->setSavedValue<int>("ovCorner", s.ovCorner);
    m->setSavedValue<double>("ovScale", s.ovScale);
    m->setSavedValue<double>("ovOpacity", s.ovOpacity);
    for (int i = 0; i < 3; i++) {
        m->setSavedValue<double>(fmt::format("cH{}", i), s.customH[i]);
        m->setSavedValue<double>(fmt::format("cS{}", i), s.customS[i]);
        m->setSavedValue<double>(fmt::format("cV{}", i), s.customV[i]);
    }
}

inline void load() {
    auto m = geode::Mod::get();
    auto& s = state();
    for (size_t i = 0; i < FCOUNT; i++) {
        F x = static_cast<F>(i);
        s.f[i] = persists(x) ? m->getSavedValue<bool>(key(x), defaultValue(x)) : false;
    }
    s.customTps = std::clamp(m->getSavedValue<int>("customTps", 240), 60, 2400);
    s.particleType = std::clamp(m->getSavedValue<int>("particleType", 3), 0, 3);
    s.themeIdx = std::clamp(m->getSavedValue<int>("themeIdx", 0), 0, 7);
    s.glowIntensity = std::clamp(static_cast<float>(m->getSavedValue<double>("glowIntensity", 0.60)), 0.f, 1.f);
    s.glassAlpha = std::clamp(static_cast<float>(m->getSavedValue<double>("glassAlpha", 0.78)), 0.40f, 0.96f);
    s.bgBlurStrength = std::clamp(static_cast<float>(m->getSavedValue<double>("bgBlurStrength", 0.75)), 0.f, 1.f);
    s.menuRadius = std::clamp(static_cast<float>(m->getSavedValue<double>("menuRadius", 18.0)), 0.f, 28.f);
    s.pressAnimType = std::clamp(m->getSavedValue<int>("pressAnimType", 1), 0, 2);
    s.btnX = static_cast<float>(m->getSavedValue<double>("btnX", -1.0));
    s.btnY = static_cast<float>(m->getSavedValue<double>("btnY", -1.0));
    s.btnOpacity = std::clamp(static_cast<float>(m->getSavedValue<double>("btnOpacity", 0.92)), 0.25f, 1.f);
    s.btnScale = std::clamp(static_cast<float>(m->getSavedValue<double>("btnScale", 1.0)), 0.55f, 1.6f);
    s.btnRadius = std::clamp(static_cast<float>(m->getSavedValue<double>("btnRadius", 10.0)), 0.f, 17.f);
    s.ovCorner = std::clamp(m->getSavedValue<int>("ovCorner", 0), 0, 3);
    s.ovScale = std::clamp(static_cast<float>(m->getSavedValue<double>("ovScale", 0.85)), 0.5f, 1.3f);
    s.ovOpacity = std::clamp(static_cast<float>(m->getSavedValue<double>("ovOpacity", 0.85)), 0.2f, 1.f);
    for (int i = 0; i < 3; i++) {
        s.customH[i] = std::clamp(static_cast<float>(m->getSavedValue<double>(fmt::format("cH{}", i), s.customH[i])), 0.f, 1.f);
        s.customS[i] = std::clamp(static_cast<float>(m->getSavedValue<double>(fmt::format("cS{}", i), s.customS[i])), 0.f, 1.f);
        s.customV[i] = std::clamp(static_cast<float>(m->getSavedValue<double>(fmt::format("cV{}", i), s.customV[i])), 0.04f, 1.f);
    }
}

// ---------- Логика читов и применения ----------
inline float speed() { return SPEEDS[state().speedIdx]; }

inline bool cheating() {
    if (state().speedIdx != 3) return true;
    for (size_t i = 0; i < FCOUNT; i++) {
        F x = static_cast<F>(i);
        if (isRisky(x) && on(x)) return true;
    }
    return false;
}

inline bool safeActive() {
    return on(F::Safe) || (on(F::AutoSafe) && (cheating() || tainted()));
}

inline void applySpeed(bool inLevel) {
    cocos2d::CCDirector::get()->getScheduler()->setTimeScale(inLevel ? speed() : 1.f);
}

inline void applyNoShake() {
    if (on(F::NoShake)) {
        GameManager::get()->setGameVariable("0081", true);
    }
}

inline void applyHitboxes() {
    if (on(F::Hitboxes)) {
        auto gm = GameManager::get();
        gm->setGameVariable("0166", true);
        gm->setGameVariable("0171", true);
    }
}

} // namespace mm
