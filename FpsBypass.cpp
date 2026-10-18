// FPS bypass + Frame smoothing.
// FPS bypass: за один кадр экрана игровая логика прогоняется несколько раз с малым шагом,
// как будто игра идёт на более высоком FPS (например 240 на 60-герцовом экране).
// Frame smoothing: выравнивает длительность шагов, чтобы микрофризы были менее заметны.
// Если при сборке ошибка в этом файле: удали его, остальной мод соберётся.
#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <algorithm>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMScheduler, CCScheduler) {
    void update(float dt) {
        int target = mm::fpsTarget();
        bool smooth = mm::on(mm::F::FrameSmooth);

        // Работаем только внутри уровня
        if ((target <= 0 && !smooth) || !PlayLayer::get()) {
            CCScheduler::update(dt);
            return;
        }

        static float avg = 1.f / 60.f;
        static float carry = 0.f;

        float use = dt;
        if (smooth) {
            // Сглаживание: длительность шага ограничена вокруг среднего,
            // остаток переносится на следующий кадр (время не теряется)
            float total = dt + carry;
            avg += (std::clamp(total, 0.002f, 0.1f) - avg) * 0.08f;
            use = std::clamp(total, avg * 0.8f, avg * 1.2f);
            carry = std::clamp(total - use, -avg, avg);
        }

        int steps = 1;
        if (target > 0) {
            // Не растим число шагов при просадках, иначе будет ещё хуже
            float ref = std::min(use, 1.f / 55.f);
            steps = std::clamp(static_cast<int>(std::lround(target * ref)), 1, 8);
        }

        float sub = use / static_cast<float>(steps);
        for (int i = 0; i < steps; i++) {
            CCScheduler::update(sub);
        }
    }
};
