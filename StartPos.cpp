// Полностью исправленный StartPos Switcher + Smart StartPos для 2.2081
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/StartPosObject.hpp>
#include <Geode/binding/LevelSettingsObject.hpp>
#include <algorithm>
#include <vector>
#include "Settings.hpp"

using namespace geode::prelude;

namespace {

struct Portal {
    float x;
    float y;
    int id;
};

bool isSmartPortal(int id) {
    switch (id) {
        case 12: case 13: case 47: case 111: case 660: case 745: case 1331: case 1933: // режимы
        case 200: case 201: case 202: case 203: case 1334:                              // скорость
        case 10: case 11: case 2926:                                                    // гравитация
        case 99: case 101:                                                              // размер (Normal / Mini)
        case 286: case 287:                                                             // Dual
            return true;
        default:
            return false;
    }
}

void applySmart(PlayLayer* pl, StartPosObject* sp, std::vector<Portal> const& portals) {
    if (!pl || !sp || !sp->m_startSettings) return;
    auto st = sp->m_startSettings;

    // Берём стартовые настройки самого уровня (а не нули!), чтобы не ломать уровни, начинающиеся с корабля/волны/2x
    int mode = 0, speed = 0;
    bool flipped = false, mini = false, dual = false;
    if (pl->m_levelSettings) {
        mode = static_cast<int>(pl->m_levelSettings->m_startMode);
        speed = static_cast<int>(pl->m_levelSettings->m_startSpeed);
        mini = pl->m_levelSettings->m_startMini;
        dual = pl->m_levelSettings->m_startDual;
        flipped = pl->m_levelSettings->m_isFlipped;
    }

    float x = sp->getPositionX();
    for (auto const& p : portals) {
        if (p.x > x + 2.f) break;
        switch (p.id) {
            case 12: mode = 0; break;     // Cube
            case 13: mode = 1; break;     // Ship
            case 47: mode = 2; break;     // Ball
            case 111: mode = 3; break;    // UFO
            case 660: mode = 4; break;    // Wave
            case 745: mode = 5; break;    // Robot
            case 1331: mode = 6; break;   // Spider
            case 1933: mode = 7; break;   // Swing
            case 200: speed = 1; break;   // 0.5x
            case 201: speed = 0; break;   // 1x
            case 202: speed = 2; break;   // 2x
            case 203: speed = 3; break;   // 3x
            case 1334: speed = 4; break;  // 4x
            case 10: flipped = false; break;
            case 11: flipped = true; break;
            case 2926: flipped = !flipped; break; // Зелёный портал гравитации 2.2
            case 99: mini = false; break;
            case 101: mini = true; break;
            case 286: dual = true; break;
            case 287: dual = false; break;
            default: break;
        }
    }

    st->m_startMode = static_cast<decltype(st->m_startMode)>(mode);
    st->m_startSpeed = static_cast<decltype(st->m_startSpeed)>(speed);
    st->m_startMini = mini;
    st->m_startDual = dual;
    st->m_isFlipped = flipped;
    st->m_disableStartPos = false;
}

} // namespace

class $modify(MMStartPosPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> startPos;
        std::vector<Portal> portals;
        int index = -1;
        CCMenu* menu = nullptr;
        CCLabelBMFont* label = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto& f = m_fields;
        f->startPos.clear();
        f->portals.clear();

        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            int id = obj->m_objectID;
            if (id == 31) {
                auto sp = static_cast<StartPosObject*>(obj);
                f->startPos.push_back(sp);
            } else if (isSmartPortal(id)) {
                f->portals.push_back({obj->getPositionX(), obj->getPositionY(), id});
            }
        }

        std::stable_sort(f->portals.begin(), f->portals.end(),
                         [](Portal const& a, Portal const& b) { return a.x < b.x; });
        std::stable_sort(f->startPos.begin(), f->startPos.end(),
                         [](StartPosObject* a, StartPosObject* b) { return a->getPositionX() < b->getPositionX(); });

        if (f->startPos.empty()) return;

        // Определяем, какой StartPos сейчас реально выбран игрой
        f->index = -1;
        for (size_t i = 0; i < f->startPos.size(); i++) {
            if (m_startPosObject == f->startPos[i]) {
                f->index = static_cast<int>(i);
            }
            if (mm::on(mm::F::SmartStart)) {
                applySmart(this, f->startPos[i], f->portals);
            }
        }

        this->buildSwitcherUi();
        this->updateSwitcherLabel();
        this->schedule(schedule_selector(MMStartPosPlayLayer::tickStartPos), 0.15f);
    }

    void buildSwitcherUi() {
        if (!m_uiLayer) return;
        auto& f = m_fields;
        auto ws = CCDirector::get()->getWinSize();

        f->menu = CCMenu::create();
        f->menu->setPosition({0.f, 0.f});
        f->menu->setVisible(mm::on(mm::F::StartPos));

        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        leftSpr->setScale(0.55f);
        leftSpr->setOpacity(190);
        auto left = CCMenuItemSpriteExtra::create(leftSpr, this, menu_selector(MMStartPosPlayLayer::onPrevStartPos));
        left->setPosition({ws.width / 2.f - 82.f, 26.f});
        f->menu->addChild(left);

        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        rightSpr->setScale(0.55f);
        rightSpr->setOpacity(190);
        rightSpr->setFlipX(true);
        auto right = CCMenuItemSpriteExtra::create(rightSpr, this, menu_selector(MMStartPosPlayLayer::onNextStartPos));
        right->setPosition({ws.width / 2.f + 82.f, 26.f});
        f->menu->addChild(right);

        f->label = CCLabelBMFont::create("", "bigFont.fnt");
        f->label->setScale(0.32f);
        f->label->setOpacity(210);
        f->label->setPosition({ws.width / 2.f, 26.f});
        f->menu->addChild(f->label);

        m_uiLayer->addChild(f->menu, 100);
    }

    void updateSwitcherLabel() {
        auto& f = m_fields;
        if (!f->label) return;
        int n = static_cast<int>(f->startPos.size());
        std::string text = f->index < 0 ? fmt::format("StartPos: 0/{}", n)
                                        : fmt::format("StartPos {}/{}", f->index + 1, n);
        f->label->setString(text.c_str());
    }

    void tickStartPos(float) {
        auto& f = m_fields;
        bool show = mm::on(mm::F::StartPos) && !f->startPos.empty();
        if (f->menu) f->menu->setVisible(show);
        if (show) this->updateSwitcherLabel();
    }

    void switchStartPos(int newIdx) {
        auto& f = m_fields;
        int n = static_cast<int>(f->startPos.size());
        if (n == 0) return;

        int total = n + 1; // -1 (0%) .. n-1
        newIdx = ((newIdx + 1) % total + total) % total - 1;
        f->index = newIdx;

        // Включаем выбранный StartPos и выключаем остальные, чтобы сама игра не путала их при ресете
        for (int i = 0; i < n; i++) {
            if (f->startPos[i] && f->startPos[i]->m_startSettings) {
                f->startPos[i]->m_startSettings->m_disableStartPos = (i != newIdx);
            }
        }

        StartPosObject* sp = (newIdx < 0) ? nullptr : f->startPos[newIdx];
        if (sp && mm::on(mm::F::SmartStart)) {
            applySmart(this, sp, f->portals);
        }

        m_currentCheckpoint = nullptr;
        this->setStartPosObject(sp);
        if (m_isPracticeMode) this->removeAllCheckpoints();

        this->resetLevel();
        this->startMusic();
        this->updateSwitcherLabel();
    }

    void onPrevStartPos(CCObject*) { this->switchStartPos(m_fields->index - 1); }
    void onNextStartPos(CCObject*) { this->switchStartPos(m_fields->index + 1); }
};
