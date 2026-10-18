#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include "ModMenuPopup.hpp"
#include "Settings.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    mm::load();
}

static CCMenuItemSpriteExtra* makeMenuButton(CCObject* target, SEL_MenuHandler handler) {
    auto spr = CircleButtonSprite::createWithSpriteFrameName(
        "GJ_optionsBtn_001.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Medium);
    auto btn = CCMenuItemSpriteExtra::create(spr, target, handler);
    btn->setID("modmenu-button"_spr);
    return btn;
}

// ---------------- кнопки открытия меню ----------------
class $modify(MMMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(makeMenuButton(this, menu_selector(MMMenuLayer::onOpenModMenu)));
            menu->updateLayout();
        }
        return true;
    }
    void onOpenModMenu(CCObject*) {
        if (auto p = ModMenuPopup::create()) p->show();
    }
};

class $modify(MMPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(makeMenuButton(this, menu_selector(MMPauseLayer::onOpenModMenu)));
            menu->updateLayout();
        }
    }
    void onOpenModMenu(CCObject*) {
        if (auto p = ModMenuPopup::create()) p->show();
    }
};

// ---------------- игровой процесс ----------------
class $modify(MMPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* overlay = nullptr;
        float hue = 0.f;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        mm::tainted() = false; // новая сессия: чистая репутация
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setScale(0.9f);
        label->setOpacity(190);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({6.f, CCDirector::get()->getWinSize().height - 4.f});
        this->addChild(label, 1000);
        m_fields->overlay = label;

        mm::applySpeed(true);
        mm::applyHitboxes();
        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& fields = m_fields;

        // Если читы использовались хоть раз за уровень, сессия помечается навсегда
        if (mm::cheating()) mm::tainted() = true;

        // --- косметика (дёшево: пара присваиваний, и только если включено) ---
        bool rIcon = mm::on(mm::F::RainbowIcon);
        bool rBG = mm::on(mm::F::RainbowBG);
        if (rIcon || rBG) {
            fields->hue += dt * 0.25f;
            if (fields->hue > 1.f) fields->hue -= 1.f;
        }
        if (rIcon) {
            auto c1 = mm::rainbow(fields->hue);
            auto c2 = mm::rainbow(fields->hue + 0.5f);
            if (m_player1) { m_player1->setColor(c1); m_player1->setSecondColor(c2); }
            if (m_player2) { m_player2->setColor(c2); m_player2->setSecondColor(c1); }
        }
        if (rBG && m_background) {
            m_background->setColor(mm::rainbow(fields->hue + 0.3f));
        }
        if (mm::on(mm::F::HidePlayer)) {
            if (m_player1) m_player1->setVisible(false);
            if (m_player2) m_player2->setVisible(false);
        }

        // --- оверлей ---
        auto label = fields->overlay;
        if (!label) return;
        bool show = mm::on(mm::F::Overlay);
        label->setVisible(show);
        if (!show) return;

        std::string text = fmt::format("{:.1f}%", this->getCurrentPercent());
        if (mm::on(mm::F::ShowFPS)) {
            float spf = std::max(CCDirector::get()->getSecondsPerFrame(), 0.0001f);
            text += fmt::format("   {:.0f} FPS", 1.f / spf);
        }
        if (mm::on(mm::F::Noclip)) text += "   NOCLIP";
        if (mm::on(mm::F::Jump)) text += "   JUMP";
        if (mm::state().speedIdx != 3) text += fmt::format("   x{}", mm::speed());
        if (mm::safeActive()) text += "   SAFE";
        else if (mm::cheating()) text += "   UNSAFE";
        label->setColor(mm::safeActive() ? ccColor3B{96, 222, 144} : mm::accent());
        if (!mm::safeActive() && mm::cheating()) label->setColor({255, 170, 60});
        label->setString(text.c_str());
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (mm::on(mm::F::Noclip)) return;
        PlayLayer::destroyPlayer(player, object);
    }

    void onQuit() {
        mm::applySpeed(false);
        mm::tainted() = false;
        PlayLayer::onQuit();
    }
};

// Эффект смерти
class $modify(MMPlayerObject, PlayerObject) {
    void playDeathEffect() {
        if (mm::on(mm::F::NoDeath)) return;
        PlayerObject::playDeathEffect();
    }
};

// Safe mode (часть 1): процент прохождения не сохраняется
class $modify(MMGJGameLevel, GJGameLevel) {
    void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid) {
        if (mm::safeActive()) return;
        GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);
    }
};

// Косметика: все иконки и цвета доступны
class $modify(MMGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (mm::on(mm::F::UnlockAll)) return true;
        return GameManager::isIconUnlocked(id, type);
    }
    bool isColorUnlocked(int id, UnlockType type) {
        if (mm::on(mm::F::UnlockAll)) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};
