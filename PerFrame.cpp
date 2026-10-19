// Точный TPS Bypass (Physics Bypass) + Frame Smoothing без ускорения + Хитбоксы.
// ВАЖНО: Удали старый файл FpsBypass.cpp, чтобы не было конфликта хуков!
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMFrameLayer, GJBaseGameLayer) {
    struct Fields {
        std::array<float, 12> history{};
        int histIdx = 0;
        bool histFilled = false;
        double smoothCarry = 0.0;
        double tpsAccum = 0.0;
        bool inSubStep = false;
    };

    void update(float dt) {
        auto pl = PlayLayer::get();
        auto& f = m_fields;

        // Если это рекурсивный подшаг TPS или мы не в активной игре — вызываем оригинал
        if (f->inSubStep || !pl || pl->m_isPaused) {
            GJBaseGameLayer::update(dt);
            return;
        }

        float useDt = dt;

        // 1. Улучшенный Frame Smoothing (Zero-Drift Median + EMA фильтр без ускорения игры)
        if (mm::on(mm::F::FrameSmooth) && dt > 0.0005f && dt < 0.25f) {
            f->history[f->histIdx] = dt;
            f->histIdx = (f->histIdx + 1) % static_cast<int>(f->history.size());
            if (f->histIdx == 0) f->histFilled = true;

            int count = f->histFilled ? static_cast<int>(f->history.size()) : std::max(1, f->histIdx);
            std::array<float, 12> sorted = f->history;
            std::sort(sorted.begin(), sorted.begin() + count);

            // Медиана отсекает резкие микрофризы (спайки кадра)
            float median = sorted[count / 2];
            double wanted = static_cast<double>(dt) + f->smoothCarry;

            // Мягко притягиваем шаг к медиане в пределах ±12%, но весь остаток сохраняем в smoothCarry
            double clamped = std::clamp(wanted, static_cast<double>(median) * 0.88, static_cast<double>(median) * 1.12);
            f->smoothCarry = std::clamp(wanted - clamped, -0.025, 0.025);
            useDt = static_cast<float>(clamped);
        } else {
            f->smoothCarry = 0.0;
        }

        // 2. Настоящий TPS Bypass (дробление физического тика GJBaseGameLayer)
        if (mm::on(mm::F::TpsBypass) && mm::state().customTps > 60) {
            float speedMult = mm::speed();
            double targetTps = static_cast<double>(std::clamp(mm::state().customTps, 60, 2400));
            double stepSize = (1.0 / targetTps) * static_cast<double>(speedMult);

            f->tpsAccum += static_cast<double>(useDt);
            // Защита от "спирали смерти" на слабых телефонах: максимум 20 подшагов за 1 кадр рендера
            int steps = static_cast<int>( std::floor(f->tpsAccum / stepSize) );
            if (steps > 20) {
                steps = 20;
                f->tpsAccum = 0.0;
            } else if (steps > 0) {
                f->tpsAccum -= steps * stepSize;
            }

            if (steps > 0) {
                f->inSubStep = true;
                float subDt = static_cast<float>(stepSize);
                for (int i = 0; i < steps; i++) {
                    GJBaseGameLayer::update(subDt);
                }
                f->inSubStep = false;
            }
        } else {
            f->tpsAccum = 0.0;
            GJBaseGameLayer::update(useDt);
        }

        // 3. Скрытие попыток и отрисовка хитбоксов
        if (pl->m_attemptLabel) {
            pl->m_attemptLabel->setVisible(!mm::on(mm::F::HideAttempts));
        }
        if (mm::on(mm::F::Hitboxes)) {
            this->updateDebugDraw();
        }
    }

    void updateDebugDraw() {
        if (!mm::on(mm::F::Hitboxes)) {
            GJBaseGameLayer::updateDebugDraw();
            return;
        }

        auto pl = PlayLayer::get();
        static unsigned int lastFrame = 0xFFFFFFFFu;
        unsigned int frame = CCDirector::get()->getTotalFrames();
        if (pl && frame == lastFrame) return;
        lastFrame = frame;

        bool wasPractice = pl ? pl->m_isPracticeMode : false;
        if (pl) pl->m_isPracticeMode = true;
        GJBaseGameLayer::updateDebugDraw();
        if (pl) pl->m_isPracticeMode = wasPractice;

        if (m_debugDrawNode) m_debugDrawNode->setVisible(true);
    }
};
