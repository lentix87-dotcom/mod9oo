#include "ModMenuPopup.hpp"
#include "Settings.hpp"
#include <Geode/binding/PlayLayer.hpp>

using namespace geode::prelude;

namespace {
    enum ToggleId { TOGGLE_NOCLIP = 1, TOGGLE_OVERLAY = 2 };
    constexpr char const* TAB_NAMES[3] = {"Player", "Level", "Visual"};
}

ModMenuPopup* ModMenuPopup::create() {
    auto ret = new ModMenuPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ModMenuPopup::init() {
    if (!Popup::init(340.f, 230.f)) return false;
    this->setTitle("Mod Menu");

    // Делаем координаты предсказуемыми: (0,0) = левый нижний угол окна.
    // Если кнопка закрытия съехала, это место нужно подправить под актуальный Popup.
    m_buttonMenu->setContentSize(m_size);
    m_buttonMenu->setAnchorPoint({0.f, 0.f});
    m_buttonMenu->setPosition({0.f, 0.f});

    // ---- вкладки ----
    for (int i = 0; i < 3; i++) {
        auto spr = ButtonSprite::create(TAB_NAMES[i], 80, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onTab));
        btn->setTag(i);
        btn->setPosition({70.f + i * 100.f, 182.f});
        m_buttonMenu->addChild(btn);
        m_tabBtns[i] = btn;
    }

    auto& s = mm::state();

    // ---- вкладка Player ----
    addToggle(0, TOGGLE_NOCLIP, "Noclip", s.noclip, 140.f);

    m_speedLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_speedLabel->setScale(0.5f);
    m_speedLabel->setAnchorPoint({0.f, 0.5f});
    m_speedLabel->setPosition({30.f, 104.f});
    m_mainLayer->addChild(m_speedLabel);
    m_tabNodes[0].push_back(m_speedLabel);

    for (int i = 0; i < (int)mm::SPEEDS.size(); i++) {
        auto spr = ButtonSprite::create(fmt::format("x{}", mm::SPEEDS[i]).c_str(), 40, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onSpeed));
        btn->setTag(i);
        btn->setPosition({45.f + i * 50.f, 72.f});
        m_buttonMenu->addChild(btn);
        m_tabNodes[0].push_back(btn);
    }
    this->refreshSpeedLabel();

    // ---- вкладка Level ----
    {
        auto spr = ButtonSprite::create("Restart level", 140, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onRestart));
        btn->setPosition({m_size.width / 2, 130.f});
        m_buttonMenu->addChild(btn);
        m_tabNodes[1].push_back(btn);

        auto hint = CCLabelBMFont::create("Works only inside a level", "chatFont.fnt");
        hint->setScale(0.7f);
        hint->setOpacity(160);
        hint->setPosition({m_size.width / 2, 100.f});
        m_mainLayer->addChild(hint);
        m_tabNodes[1].push_back(hint);
    }

    // ---- вкладка Visual ----
    addToggle(2, TOGGLE_OVERLAY, "Info overlay", s.overlay, 140.f);

    this->switchTab(0);
    return true;
}

void ModMenuPopup::addToggle(int tab, int id, char const* label, bool value, float y) {
    auto toggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ModMenuPopup::onToggle), 0.7f);
    toggler->setTag(id);
    toggler->toggle(value);
    toggler->setPosition({40.f, y});
    m_buttonMenu->addChild(toggler);

    auto text = CCLabelBMFont::create(label, "bigFont.fnt");
    text->setScale(0.55f);
    text->setAnchorPoint({0.f, 0.5f});
    text->setPosition({66.f, y});
    m_mainLayer->addChild(text);

    m_tabNodes[tab].push_back(toggler);
    m_tabNodes[tab].push_back(text);
}

void ModMenuPopup::switchTab(int index) {
    for (int t = 0; t < 3; t++) {
        bool on = (t == index);
        for (auto node : m_tabNodes[t]) {
            node->setVisible(on);
            if (auto item = typeinfo_cast<CCMenuItem*>(node)) item->setEnabled(on);
        }
        if (auto spr = typeinfo_cast<ButtonSprite*>(m_tabBtns[t]->getNormalImage())) {
            spr->updateBGImage(on ? "GJ_button_01.png" : "GJ_button_04.png");
        }
    }
}

void ModMenuPopup::onTab(CCObject* sender) {
    this->switchTab(static_cast<CCNode*>(sender)->getTag());
}

void ModMenuPopup::onToggle(CCObject* sender) {
    auto t = static_cast<CCMenuItemToggler*>(sender);
    // в колбэке isToggled() ещё хранит СТАРОЕ состояние
    bool now = !t->isToggled();
    switch (t->getTag()) {
        case TOGGLE_NOCLIP: mm::state().noclip = now; break;
        case TOGGLE_OVERLAY:
            mm::state().overlay = now;
            mm::saveOverlay();
            break;
    }
}

void ModMenuPopup::onSpeed(CCObject* sender) {
    mm::state().speedIdx = static_cast<CCNode*>(sender)->getTag();
    mm::applySpeed(PlayLayer::get() != nullptr);
    this->refreshSpeedLabel();
}

void ModMenuPopup::refreshSpeedLabel() {
    m_speedLabel->setString(fmt::format("Speed: x{}", mm::state().speed()).c_str());
}

void ModMenuPopup::onRestart(CCObject*) {
    if (auto pl = PlayLayer::get()) {
        this->onClose(nullptr);
        pl->resetLevel();
    } else {
        FLAlertLayer::create("Mod Menu", "Open a level first.", "OK")->show();
    }
}
