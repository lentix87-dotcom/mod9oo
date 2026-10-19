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
    LiquidGlass,                          // Theme: режим Liquid Glass
    BtnHideInGame,                        // Client: скрывать кнопку L во время игры
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
        "glow", "liquidglass", "btnhideingame", "btnshowinpause",
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
        case F::LiquidGlass: return "Liquid Glass";
        case F::BtnHideInGame: return "Hide L-Btn in Game";
        case F::BtnShowInPause: return "Show L-Btn in Pause";
        default: return "";
    }
}

// Чистый английский текст, чтобы шрифт GD chatFont.fnt отображал 100% символов без пропусков
inline char const* desc(F x) {
    switch (x) {
        case F::Noclip: return "Ignore all obstacles and spikes";
        case F::Jump: return "Infinite mid-air jumps anywhere";
        case F::Safe: return "Block level progress & leaderboards";
        case F::AutoSafe: return "Auto-enable Safe Mode when cheating";
        case F::Hitboxes: return "Show collision boxes in all modes";
        case F::NoDeath: return "Disable death explosion & particles";
        case F::StartPos: return "Switch StartPos with arrow keys";
        case F::SmartStart: return "Auto-detect mode, speed & gravity";
        case F::FrameSmooth: return "Zero-drift micro-stutter remover";
        case F::TpsBypass: return "Custom physics ticks per second";
        case F::Overlay: return "Customizable in-game status bar";
        case F::ShowFPS: return "100% accurate stable FPS counter";
        case F::HidePlayer: return "Make player icons invisible";
        case F::HideAttempts: return "Hide attempt counter label";
        case F::NoShake: return "Disable camera shake triggers";
        case F::NoMirror: return "Block screen flip mirror portals";
        case F::RainbowIcon: return "Smooth rainbow player colors";
        case F::RainbowBG: return "Smooth rainbow background color";
        case F::UnlockAll: return "Unlock all icons & colors in garage";
        case F::Glow: return "Soft white bloom glow on UI elements";
        case F::LiquidGlass: return "Translucent frosted glass UI effect";
        case F::BtnHideInGame: return "Hide floating L button while playing";
        case F::BtnShowInPause: return "Show floating L button on pause";
        default: return "";
    }
}

// Читерные функции (помечаются красным цветом и включают Auto Safe)
// NoShake НЕ считается читом!
inline bool isRisky(F x) {
    switch (x) {
        case F::Noclip: case F::Jump: case F::Hitboxes: case F::NoMirror:
        case F::StartPos: case F::SmartStart: case F::TpsBypass:
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

// ---------- Темы в стиле Nursultan / WildClient ----------
struct ThemePreset {
    char const* name;
    cocos2d::ccColor3B bg;
    cocos2d::ccColor3B card;
    cocos2d::ccColor3B accent;
    cocos2d::ccColor3B text;
};

inline constexpr std::array<ThemePreset, 8> THEMES{{
    {"Nursultan Dark", {16, 15, 24}, {26, 25, 38}, {180, 145, 255}, {240, 240, 248}},
    {"Monochrome B&W", {10, 10, 13}, {22, 22, 27}, {245, 245, 250}, {250, 250, 255}},
    {"Sakura Breeze",  {23, 14, 23}, {36, 22, 36}, {255, 130, 185}, {250, 238, 245}},
    {"Midnight Azure", {11, 17, 28}, {19, 29, 44}, {75, 170, 255},  {235, 244, 255}},
    {"Crimson Blood",  {23, 12, 15}, {36, 18, 23}, {255, 75, 95},   {250, 235, 238}},
    {"Emerald Mint",   {11, 22, 18}, {18, 34, 28}, {75, 235, 165},  {235, 250, 242}},
    {"Sunset Gold",    {23, 17, 12}, {36, 27, 20}, {255, 178, 70},  {252, 244, 235}},
    {"Custom Palette", {16, 15, 24}, {26, 25, 38}, {180, 145, 255}, {240, 240, 248}}
}};

inline constexpr std::array<float, 6> SPEEDS{0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f};

struct State {
    std::array<bool, FCOUNT> f{};
    int speedIdx = 3;          // 3 = 1.0x
    int customTps = 240;       // Пользовательское значение TPS (60..2400)
    float rainbowSpeed = 0.25f;

    // Косметика и меню
    int particleType = 3;        // 0 = Off, 1 = Winter, 2 = Autumn, 3 = Sakura
    int themeIdx = 0;            // Индекс в THEMES (7 = Custom)
    float glowIntensity = 0.65f; // Сила белого свечения (0.0 .. 1.0)
    float glassAlpha = 0.76f;    // Прозрачность Liquid Glass (0.45 .. 0.96)
    int pressAnimType = 1;       // 0 = Off, 1 = Bounce, 2 = Smooth Shrink
    float pressAnimPower = 0.86f;

    // Кастомная палитра (HSV для 3 слотов: 0 = Accent, 1 = Background, 2 = Card)
    int editSlot = 0;
    float customH[3] = {0.73f, 0.70f, 0.70f};
    float customS[3] = {0.45f, 0.30f, 0.30f};
    float customV[3] = {1.00f, 0.09f, 0.14f};

    // Плавающая кнопка L
    float btnX = -1.f;
    float btnY = -1.f;
    float btnOpacity = 0.90f;
    float btnScale = 1.0f;

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
    if (s.themeIdx == 7) return {240, 240, 246};
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
    m->setSavedValue<int>("pressAnimType", s.pressAnimType);
    m->setSavedValue<double>("pressAnimPower", s.pressAnimPower);
    m->setSavedValue<double>("btnX", s.btnX);
    m->setSavedValue<double>("btnY", s.btnY);
    m->setSavedValue<double>("btnOpacity", s.btnOpacity);
    m->setSavedValue<double>("btnScale", s.btnScale);
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
    s.glowIntensity = std::clamp(static_cast<float>(m->getSavedValue<double>("glowIntensity", 0.65)), 0.f, 1.f);
    s.glassAlpha = std::clamp(static_cast<float>(m->getSavedValue<double>("glassAlpha", 0.76)), 0.45f, 0.96f);
    s.pressAnimType = std::clamp(m->getSavedValue<int>("pressAnimType", 1), 0, 2);
    s.pressAnimPower = std::clamp(static_cast<float>(m->getSavedValue<double>("pressAnimPower", 0.86)), 0.72f, 0.96f);
    s.btnX = static_cast<float>(m->getSavedValue<double>("btnX", -1.0));
    s.btnY = static_cast<float>(m->getSavedValue<double>("btnY", -1.0));
    s.btnOpacity = std::clamp(static_cast<float>(m->getSavedValue<double>("btnOpacity", 0.90)), 0.2f, 1.f);
    s.btnScale = std::clamp(static_cast<float>(m->getSavedValue<double>("btnScale", 1.0)), 0.6f, 1.5f);
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

inline std::string cheatList() {
    std::string s;
    auto add = [&](char const* n) { if (!s.empty()) s += ", "; s += n; };
    if (on(F::Noclip)) add("Noclip");
    if (on(F::Jump)) add("Jump");
    if (on(F::Hitboxes)) add("Hitboxes");
    if (on(F::NoMirror)) add("NoMirror");
    if (on(F::StartPos)) add("StartPos");
    if (on(F::SmartStart)) add("SmartSP");
    if (on(F::TpsBypass)) add("TPS");
    if (state().speedIdx != 3) add("Speed");
    if (s.empty()) s = "Tainted";
    return s;
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
