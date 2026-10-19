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
    BtnHideInGame,                        // Настройка кнопки L: скрывать во время игры
    BtnShowInPause,                       // Настройка кнопки L: показывать в паузе
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
        "glow", "btnhideingame", "btnshowinpause",
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
        default: return "";
    }
}

inline char const* desc(F x) {
    switch (x) {
        case F::Noclip: return "Игрок проходит сквозь шипы и стены без смерти";
        case F::Jump: return "Позволяет прыгать прямо в воздухе бесконечно";
        case F::Safe: return "Блокирует сохранение прогресса и отправку рекордов";
        case F::AutoSafe: return "Автоматически включает Safe Mode при активации читов";
        case F::Hitboxes: return "Отображает коллизии объектов и игрока во всех режимах";
        case F::NoDeath: return "Отключает взрыв и эффекты частиц при смерти";
        case F::StartPos: return "Позволяет переключаться между StartPos стрелками внизу экрана";
        case F::SmartStart: return "Автоматически подбирает режим, скорость и гравитацию для StartPos";
        case F::FrameSmooth: return "Убирает микрофризы и выравнивает время кадра без ускорения игры";
        case F::TpsBypass: return "Изменяет частоту просчёта физики (Ticks Per Second) свыше 240 Гц";
        case F::Overlay: return "Компактная информационная панель в углу экрана во время игры";
        case F::ShowFPS: return "Точный счётчик реального FPS в панели Overlay";
        case F::HidePlayer: return "Делает иконку игрока полностью невидимой в уровне";
        case F::HideAttempts: return "Скрывает надпись Attempt в уровне";
        case F::NoShake: return "Отключает тряску камеры триггерами (включается в настройках игры)";
        case F::NoMirror: return "Блокирует порталы разворота экрана (Mirror Portals)";
        case F::RainbowIcon: return "Плавное переливание цветов иконки игрока";
        case F::RainbowBG: return "Плавное переливание цвета фона уровня";
        case F::UnlockAll: return "Открывает все иконки, цвета и эффекты в гараже";
        case F::Glow: return "Добавляет мягкое белое свечение элементам мод-меню";
        default: return "";
    }
}

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
    {"Nursultan Dark", {16, 15, 22}, {24, 23, 33}, {180, 145, 255}, {238, 238, 245}},
    {"Monochrome B&W", {10, 10, 12}, {20, 20, 24}, {245, 245, 250}, {250, 250, 255}},
    {"Sakura Breeze",  {22, 15, 22}, {33, 22, 33}, {255, 130, 185}, {248, 235, 242}},
    {"Midnight Azure", {12, 17, 26}, {19, 27, 40}, {75, 165, 255},  {232, 242, 255}},
    {"Crimson Blood",  {22, 12, 15}, {34, 18, 22}, {255, 75, 95},   {248, 232, 235}},
    {"Emerald Mint",   {12, 21, 18}, {18, 32, 27}, {75, 230, 160},  {232, 248, 240}},
    {"Sunset Gold",    {22, 17, 13}, {34, 26, 20}, {255, 175, 70},  {250, 242, 232}},
    {"Custom Palette", {16, 15, 22}, {24, 23, 33}, {180, 145, 255}, {238, 238, 245}}
}};

inline constexpr std::array<float, 6> SPEEDS{0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f};

struct State {
    std::array<bool, FCOUNT> f{};
    int speedIdx = 3;          // 3 = 1.0x
    float customSpeed = 1.0f;  // Точная скорость
    int customTps = 240;       // Пользовательское значение TPS (60..2400)
    float rainbowSpeed = 0.25f;

    // Косметика и меню
    int particleType = 1;      // 0 = Off, 1 = Winter (Зима), 2 = Autumn (Осень), 3 = Sakura (Сакура)
    int themeIdx = 0;          // Индекс в THEMES (7 = Custom)
    float glowIntensity = 0.65f; // Сила белого свечения (0.0 .. 1.0)
    int pressAnimType = 1;     // 0 = Off, 1 = Bounce, 2 = Smooth Shrink
    float pressAnimPower = 0.85f; // Масштаб при нажатии (0.75 .. 0.96)

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
    int ovCorner = 0;          // 0 = Top-Left, 1 = Top-Right, 2 = Bottom-Left, 3 = Bottom-Right
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
    s.particleType = std::clamp(m->getSavedValue<int>("particleType", 1), 0, 3);
    s.themeIdx = std::clamp(m->getSavedValue<int>("themeIdx", 0), 0, 7);
    s.glowIntensity = std::clamp(static_cast<float>(m->getSavedValue<double>("glowIntensity", 0.65)), 0.f, 1.f);
    s.pressAnimType = std::clamp(m->getSavedValue<int>("pressAnimType", 1), 0, 2);
    s.pressAnimPower = std::clamp(static_cast<float>(m->getSavedValue<double>("pressAnimPower", 0.85)), 0.72f, 0.96f);
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
    // Включаем официальный Disable Shake ("0081") в настройках GD перед уровнем
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
