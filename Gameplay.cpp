// Jump hack (читерная функция: включает Safe mode, если включён Auto safe).
// Если при сборке ошибка в этом файле: удали его, остальной мод соберётся.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "Settings.hpp"

using namespace geode::prelude;

class $modify(MMJumpPlayer, PlayerObject) {
    bool pushButton(PlayerButton button) {
        if (mm::on(mm::F::Jump)) m_isOnGround = true;
        return PlayerObject::pushButton(button);
    }
};
