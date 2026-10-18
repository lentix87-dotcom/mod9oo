#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
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

// Кнопка на главном экране
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

// Кнопка в паузе: чтобы не выходить из уровня
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

class $modify(MMPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* overlay = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(150);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({6.f, CCDirector::get()->getWinSize().height - 4.f});
        this->addChild(label, 1000);
        m_fields->overlay = label;

        mm::applySpeed(true);
        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto label = m_fields->overlay;
        if (!label) return;

        auto& s = mm::state();
        label->setVisible(s.overlay);
        if (!s.overlay) return;

        std::string text = fmt::format("{:.1f}%", this->getCurrentPercent());
        if (s.noclip) text += "  NOCLIP";
        if (s.speedIdx != 3) text += fmt::format("  x{}", s.speed());
        if (s.cheating()) text += "  [CHEATS: no records]";
        label->setString(text.c_str());
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (mm::state().noclip) return;
        PlayLayer::destroyPlayer(player, object);
    }

    void onQuit() {
        mm::applySpeed(false);
        PlayLayer::onQuit();
    }
};
