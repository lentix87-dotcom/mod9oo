// Хитбоксы во всех режимах + скрытие счётчика попыток.
// Если при сборке ошибка в этом файле: удали его, остальной мод соберётся.
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMFrameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);

        auto pl = PlayLayer::get();
        if (!pl) return;

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
        if (pl && frame == lastFrame) return; // не чаще одного раза за кадр
        lastFrame = frame;

        // Игра рисует хитбоксы только в практике: на время вызова "включаем" практику
        bool wasPractice = pl ? pl->m_isPracticeMode : false;
        if (pl) pl->m_isPracticeMode = true;
        GJBaseGameLayer::updateDebugDraw();
        if (pl) pl->m_isPracticeMode = wasPractice;

        if (m_debugDrawNode) m_debugDrawNode->setVisible(true);
    }
};
