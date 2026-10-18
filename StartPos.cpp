// StartPos Switcher + Smart StartPos.
// Если при сборке ошибка в этом файле: удали его, остальной мод соберётся.
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
    int id;
};

bool isSmartPortal(int id) {
    switch (id) {
        case 12: case 13: case 47: case 111: case 660: case 745: case 1331: case 1933: // режимы
        case 200: case 201: case 202: case 203: case 1334:                              // скорость
        case 10: case 11:                                                               // гравитация
        case 99: case 101:                                                              // размер
        case 286: case 287:                                                             // dual
            return true;
        default:
            return false;
    }
}

// Определяет, в каком состоянии игрок окажется в точке StartPos, и записывает это в её настройки
void applySmart(StartPosObject* sp, std::vector<Portal> const& portals) {
    if (!sp) return;
    auto st = sp->m_startSettings;
    if (!st) return;

    float x = sp->getPositionX();
    int mode = 0, speed = 0;
    bool flipped = false, mini = false, dual = false;

    for (auto const& p : portals) {
        if (p.x > x) break;
        switch (p.id) {
            case 12: mode = 0; break;     // cube
            case 13: mode = 1; break;     // ship
            case 47: mode = 2; break;     // ball
            case 111: mode = 3; break;    // ufo
            case 660: mode = 4; break;    // wave
            case 745: mode = 5; break;    // robot
            case 1331: mode = 6; break;   // spider
            case 1933: mode = 7; break;   // swing
            case 200: speed = 1; break;   // 0.5x
            case 201: speed = 0; break;   // 1x
            case 202: speed = 2; break;   // 2x
            case 203: speed = 3; break;   // 3x
            case 1334: speed = 4; break;  // 4x
            case 10: flipped = false; break;
            case 11: flipped = true; break;
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
}

}

class $modify(MMStartPosPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> startPos;
        std::vector<Portal> portals;
        int index = -1; // -1 = обычный старт
        CCMenu* menu = nullptr;
        CCLabelBMFont* label = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto& f = *m_fields;

        CCObject* o = nullptr;
        CCARRAY_FOREACH(m_objects, o) {
            auto obj = static_cast<GameObject*>(o);
            int id = obj->m_objectID;
            if (id == 31) {
                f.startPos.push_back(static_cast<StartPosObject*>(obj));
            } else if (isSmartPortal(id)) {
                f.portals.push_back({obj->getPositionX(), id});
            }
        }
        std::stable_sort(f.portals.begin(), f.portals.end(),
                         [](Portal const& a, Portal const& b) { return a.x < b.x; });
        std::stable_sort(f.startPos.begin(), f.startPos.end(),
                         [](StartPosObject* a, StartPosObject* b) { return a->getPositionX() < b->getPositionX(); });

        if (f.startPos.empty()) return;

        if (mm::on(mm::F::SmartStart)) {
            for (auto sp : f.startPos) applySmart(sp, f.portals);
        }
        this->buildSwitcherUi();
        this->schedule(schedule_selector(MMStartPosPlayLayer::tickStartPos), 0.2f);
    }

    void buildSwitcherUi() {
        if (!m_uiLayer) return;
        auto& f = *m_fields;
        auto ws = CCDirector::get()->getWinSize();

        f.menu = CCMenu::create();
        f.menu->setPosition({0.f, 0.f});
        f.menu->setVisible(false);

        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        leftSpr->setScale(0.55f);
        leftSpr->setOpacity(170);
        auto left = CCMenuItemSpriteExtra::create(leftSpr, this, menu_selector(MMStartPosPlayLayer::onPrevStartPos));
        left->setPosition({ws.width / 2.f - 80.f, 36.f});
        f.menu->addChild(left);

        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        rightSpr->setScale(0.55f);
        rightSpr->setOpacity(170);
        rightSpr->setFlipX(true);
        auto right = CCMenuItemSpriteExtra::create(rightSpr, this, menu_selector(MMStartPosPlayLayer::onNextStartPos));
        right->setPosition({ws.width / 2.f + 80.f, 36.f});
        f.menu->addChild(right);

        f.label = CCLabelBMFont::create("", "bigFont.fnt");
        f.label->setScale(0.32f);
        f.label->setOpacity(190);
        f.label->setPosition({ws.width / 2.f, 36.f});
        f.menu->addChild(f.label);

        m_uiLayer->addChild(f.menu, 100);
    }

    void tickStartPos(float) {
        auto& f = *m_fields;
        bool show = mm::on(mm::F::StartPos) && !f.startPos.empty();
        if (f.menu) f.menu->setVisible(show);
        if (show && f.label) {
            int n = static_cast<int>(f.startPos.size());
            std::string text = f.index < 0 ? fmt::format("StartPos: off ({})", n)
                                           : fmt::format("StartPos {}/{}", f.index + 1, n);
            f.label->setString(text.c_str());
        }
    }

    void switchStartPos(int idx) {
        auto& f = *m_fields;
        int n = static_cast<int>(f.startPos.size());
        if (n == 0) return;

        // позиции: -1 (обычный старт), 0 .. n-1
        int total = n + 1;
        idx = ((idx + 1) % total + total) % total - 1;
        f.index = idx;

        StartPosObject* sp = idx < 0 ? nullptr : f.startPos[idx];
        if (sp && mm::on(mm::F::SmartStart)) applySmart(sp, f.portals);

        this->setStartPosObject(sp);
        if (m_isPracticeMode) this->removeAllCheckpoints();
        this->resetLevel();
    }

    void onPrevStartPos(CCObject*) { this->switchStartPos(m_fields->index - 1); }
    void onNextStartPos(CCObject*) { this->switchStartPos(m_fields->index + 1); }
};
