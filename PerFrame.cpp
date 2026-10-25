// Чистый TPS Bypass + Хитбоксы + Скрытие попыток (без Frame Smoothing)
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <algorithm>
#include <cmath>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMFrameLayer, GJBaseGameLayer) {
    struct Fields {
        double tpsAccum = 0.0;
        bool inSubStep = false;
    };

    void update(float dt) {
        auto pl = PlayLayer::get();
        auto& f = m_fields;

        if (f->inSubStep || !pl || pl->m_isPaused) {
            GJBaseGameLayer::update(dt);
            return;
        }

        if (mm::on(mm::F::TpsBypass) && mm::state().customTps > 60) {
            float speedMult = mm::speed();
            double targetTps = static_cast<double>(std::clamp(mm::state().customTps, 60, 2400));
            double stepSize = (1.0 / targetTps) * static_cast<double>(speedMult);

            f->tpsAccum += static_cast<double>(dt);
            int steps = static_cast<int>(std::floor(f->tpsAccum / stepSize));
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
            GJBaseGameLayer::update(dt);
        }

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
