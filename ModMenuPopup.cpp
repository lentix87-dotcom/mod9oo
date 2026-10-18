#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>

using namespace geode::prelude;

namespace {
    constexpr char const* TAB_NAMES[4] = {"Player", "Level", "Visual", "Theme"};
    constexpr float W = 380.f;
    constexpr float H = 260.f;
    constexpr float COL1 = 34.f;   // x тогглера в 1-й колонке
    constexpr float COL2 = 206.f;  // x тогглера во 2-й колонке
    constexpr float ROW1 = 160.f;
    constexpr float ROW_STEP = 34.f;

    ccColor3B scaled(ccColor3B c, float k) {
        return {static_cast<GLubyte>(c.r * k), static_cast<GLubyte>(c.g * k), static_cast<GLubyte>(c.b * k)};
    }
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
    if (!Popup::init(W, H)) return false;
    this->setTitle("Mod Menu");

    // Делаем координаты предсказуемыми: (0,0) = левый нижний угол окна
    m_buttonMenu->setContentSize(m_size);
    m_buttonMenu->setAnchorPoint({0.f, 0.f});
    m_buttonMenu->setPosition({0.f, 0.f});

    // ---- декор "жидкого стекла": 3 статичных слоя, без шейдеров и без обновлений каждый кадр ----
    m_bgSprite->setZOrder(-3);

    m_gloss = CCLayerGradient::create(ccc4(255, 255, 255, 70), ccc4(255, 255, 255, 0));
    m_gloss->setContentSize({W - 14.f, H * 0.5f});
    m_gloss->setPosition({7.f, H - 7.f - H * 0.5f});
    m_mainLayer->addChild(m_gloss, -2);

    m_edge = CCLayerColor::create(ccc4(255, 255, 255, 120), W - 24.f, 1.5f);
    m_edge->setPosition({12.f, H - 8.f});
    m_mainLayer->addChild(m_edge, -1);

    m_accentLine = CCLayerColor::create(ccc4(255, 255, 255, 220), W - 40.f, 2.f);
    m_accentLine->setPosition({20.f, 188.f});
    m_mainLayer->addChild(m_accentLine, -1);

    // ---- вкладки ----
    for (int i = 0; i < TAB_COUNT; i++) {
        auto spr = ButtonSprite::create(TAB_NAMES[i], 72, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onTab));
        btn->setTag(i);
        btn->setPosition({58.f + i * 88.f, 208.f});
        m_buttonMenu->addChild(btn);
        m_tabBtns[i] = btn;
    }

    auto& s = mm::state();

    // ======== Player ========
    addToggle(0, mm::F::Noclip, "Noclip", COL1, ROW1);
    addToggle(0, mm::F::Safe, "Safe mode", COL2, ROW1);

    m_speedLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_speedLabel->setScale(0.5f);
    m_speedLabel->setAnchorPoint({0.f, 0.5f});
    m_speedLabel->setPosition({COL1, ROW1 - ROW_STEP});
    m_mainLayer->addChild(m_speedLabel);
    m_tabNodes[0].push_back(m_speedLabel);

    for (int i = 0; i < (int)mm::SPEEDS.size(); i++) {
        auto spr = ButtonSprite::create(fmt::format("x{}", mm::SPEEDS[i]).c_str(), 46, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onSpeed));
        btn->setTag(i);
        btn->setPosition({55.f + i * 54.f, ROW1 - 2 * ROW_STEP - 4.f});
        m_buttonMenu->addChild(btn);
        m_tabNodes[0].push_back(btn);
    }
    this->refreshSpeedLabel();
    addLabel(0, "Safe mode: no records saved while cheats are on", COL1, 34.f, 0.4f);

    // ======== Level ========
    {
        auto spr = ButtonSprite::create("Restart level", 140, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onRestart));
        btn->setPosition({W / 2, ROW1});
        m_buttonMenu->addChild(btn);
        m_tabNodes[1].push_back(btn);
    }
    addToggle(1, mm::F::Hitboxes, "Show hitboxes", COL1, ROW1 - ROW_STEP);
    addToggle(1, mm::F::NoDeath, "No death effect", COL1, ROW1 - 2 * ROW_STEP);
    addLabel(1, "Restart works only inside a level", COL1, 34.f, 0.4f);

    // ======== Visual (косметика) ========
    addToggle(2, mm::F::Overlay, "Info overlay", COL1, ROW1);
    addToggle(2, mm::F::HidePlayer, "Hide player", COL1, ROW1 - ROW_STEP);
    addToggle(2, mm::F::UnlockAll, "Unlock icons", COL1, ROW1 - 2 * ROW_STEP);
    addToggle(2, mm::F::RainbowIcon, "Rainbow icon", COL2, ROW1);
    addToggle(2, mm::F::RainbowBG, "Rainbow BG", COL2, ROW1 - ROW_STEP);

    // ======== Theme ========
    addLabel(3, "Accent color", COL1, ROW1 + 6.f, 0.45f);
    for (int i = 0; i < 12; i++) {
        auto sw = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
        sw->setColor(mm::accents()[i]);
        sw->setScale(0.5f);
        auto btn = CCMenuItemSpriteExtra::create(sw, this, menu_selector(ModMenuPopup::onAccent));
        btn->setTag(i);
        float x = 40.f + (i % 6) * 60.f;
        float y = (i < 6) ? ROW1 - 22.f : ROW1 - 56.f;
        btn->setPosition({x, y});
        m_buttonMenu->addChild(btn);
        m_tabNodes[3].push_back(btn);
        m_swatches.push_back(sw);
    }
    addToggle(3, mm::F::Glass, "Liquid glass", COL1, ROW1 - 2 * ROW_STEP - 6.f);

    char const* opNames[3] = {"Low", "Mid", "High"};
    for (int i = 0; i < 3; i++) {
        auto spr = ButtonSprite::create(opNames[i], 50, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModMenuPopup::onGlassOpacity));
        btn->setTag(i);
        btn->setPosition({226.f + i * 56.f, ROW1 - 2 * ROW_STEP - 6.f});
        m_buttonMenu->addChild(btn);
        m_tabNodes[3].push_back(btn);
        m_opacityBtns.push_back(btn);
    }
    addLabel(3, "Glass opacity", 200.f, ROW1 - 2 * ROW_STEP + 14.f, 0.4f);

    this->switchTab(0);
    this->applyTheme();
    return true;
}

void ModMenuPopup::addToggle(int tab, mm::F flag, char const* label, float x, float y) {
    auto toggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ModMenuPopup::onToggle), 0.7f);
    toggler->setTag(static_cast<int>(flag));
    toggler->toggle(mm::on(flag));
    toggler->setPosition({x, y});
    m_buttonMenu->addChild(toggler);

    auto text = CCLabelBMFont::create(label, "bigFont.fnt");
    text->setScale(0.45f);
    text->setAnchorPoint({0.f, 0.5f});
    text->setPosition({x + 24.f, y});
    m_mainLayer->addChild(text);

    m_tabNodes[tab].push_back(toggler);
    m_tabNodes[tab].push_back(text);
}

void ModMenuPopup::addLabel(int tab, char const* text, float x, float y, float scale) {
    auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
    lbl->setScale(scale);
    lbl->setOpacity(190);
    lbl->setAnchorPoint({0.f, 0.5f});
    lbl->setPosition({x, y});
    m_mainLayer->addChild(lbl);
    m_tabNodes[tab].push_back(lbl);
}

void ModMenuPopup::switchTab(int index) {
    m_currentTab = index;
    for (int t = 0; t < TAB_COUNT; t++) {
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

// Тема применяется только при открытии и при смене настроек: никаких per-frame обновлений
void ModMenuPopup::applyTheme() {
    auto& s = mm::state();
    auto acc = mm::accent();
    bool glass = mm::on(mm::F::Glass);

    if (glass) {
        m_bgSprite->setColor(scaled(acc, 0.28f));
        m_bgSprite->setOpacity(static_cast<GLubyte>(mm::GLASS_OPACITY[s.glassOpacity]));
    } else {
        m_bgSprite->setColor(scaled(acc, 0.35f));
        m_bgSprite->setOpacity(255);
    }
    m_gloss->setVisible(glass);
    m_edge->setVisible(glass);

    m_accentLine->setColor(acc);
    m_title->setColor(acc);

    for (int i = 0; i < (int)m_swatches.size(); i++) {
        m_swatches[i]->setScale(i == s.accent ? 0.68f : 0.5f);
    }
    for (int i = 0; i < (int)m_opacityBtns.size(); i++) {
        if (auto spr = typeinfo_cast<ButtonSprite*>(m_opacityBtns[i]->getNormalImage())) {
            spr->updateBGImage(i == s.glassOpacity ? "GJ_button_01.png" : "GJ_button_04.png");
        }
        m_opacityBtns[i]->setEnabled(glass && m_currentTab == 3);
    }
}

void ModMenuPopup::onTab(CCObject* sender) {
    this->switchTab(static_cast<CCNode*>(sender)->getTag());
    this->applyTheme();
}

void ModMenuPopup::onToggle(CCObject* sender) {
    auto t = static_cast<CCMenuItemToggler*>(sender);
    auto flag = static_cast<mm::F>(t->getTag());
    // в колбэке isToggled() ещё хранит СТАРОЕ состояние
    mm::set(flag, !t->isToggled());
    mm::save(flag);

    switch (flag) {
        case mm::F::Hitboxes: mm::applyHitboxes(); break;
        case mm::F::Glass: this->applyTheme(); break;
        default: break;
    }
}

void ModMenuPopup::onSpeed(CCObject* sender) {
    mm::state().speedIdx = static_cast<CCNode*>(sender)->getTag();
    mm::applySpeed(PlayLayer::get() != nullptr);
    this->refreshSpeedLabel();
}

void ModMenuPopup::onAccent(CCObject* sender) {
    mm::state().accent = static_cast<CCNode*>(sender)->getTag();
    mm::saveTheme();
    this->applyTheme();
}

void ModMenuPopup::onGlassOpacity(CCObject* sender) {
    mm::state().glassOpacity = static_cast<CCNode*>(sender)->getTag();
    mm::saveTheme();
    this->applyTheme();
}

void ModMenuPopup::refreshSpeedLabel() {
    m_speedLabel->setString(fmt::format("Speed: x{}", mm::speed()).c_str());
}

void ModMenuPopup::onRestart(CCObject*) {
    if (auto pl = PlayLayer::get()) {
        this->onClose(nullptr);
        pl->resetLevel();
    } else {
        FLAlertLayer::create("Mod Menu", "Open a level first.", "OK")->show();
    }
}
