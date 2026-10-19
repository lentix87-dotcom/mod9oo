// Safe mode (часть 2): прохождение уровня не засчитывается.
// Если при сборке ошибка в этом файле: удали его, остальной мод соберётся.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMSafePlayLayer, PlayLayer) {
    struct Fields {
        bool notified = false;
    };

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->notified = false;
    }

    void levelComplete() {
        // В практике рекорды и так не сохраняются, поэтому блокируем только обычное прохождение.
        if (mm::safeActive() && !m_isPracticeMode) {
            if (!m_fields->notified) {
                m_fields->notified = true;
                FLAlertLayer::create("Safe mode",
                                     "Level finished with Safe mode active.\n"
                                     "The completion was NOT saved or submitted.",
                                     "OK")->show();
                queueInMainThread([] {
                    if (auto pl = PlayLayer::get()) pl->resetLevel();
                });
            }
            return;
        }
        PlayLayer::levelComplete();
    }
};
