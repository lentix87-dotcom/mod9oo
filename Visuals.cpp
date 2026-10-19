// No shake / No mirror.
// Если при сборке ошибка в этом файле: удали его, остальной мод соберётся.
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMVisualsLayer, GJBaseGameLayer) {
    void shakeCamera(float duration, float strength, float interval) {
        if (mm::on(mm::F::NoShake)) return;
        GJBaseGameLayer::shakeCamera(duration, strength, interval);
    }

    void toggleFlipped(bool flipped, bool instant) {
        if (mm::on(mm::F::NoMirror)) return;
        GJBaseGameLayer::toggleFlipped(flipped, instant);
    }
};
