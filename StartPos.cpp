// Полностью исправленный StartPos Switcher + Smart StartPos без вылетов куба
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/StartPosObject.hpp>
#include <Geode/binding/LevelSettingsObject.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <algorithm>
#include <cmath>
#include <vector>
#include "Settings.hpp"

using namespace geode::prelude;

namespace {

struct Portal {
    float x;
    float y;
    int id;
};

struct OrigStartPosCfg {
    int mode = 0;
    int speed = 0;
    bool mini = false;
    bool dual = false;
    bool flipped = false;
    bool wasCustomConfigured = false;
};

bool isSmartPortal(int id) {
    switch (id) {
        case 12: case 13: case 47: case 111: case 660: case 745: case 1331: case 1933:
        case 200: case 201: case 202: case 203: case 1334:
        case 10: case 11:
        case 99: case 101:
        case 286: case 287:
            return true;
        default:
            return false;
    }
}

void applySmart(PlayLayer* pl, StartPosObject* sp, OrigStartPosCfg const& orig, std::vector<Portal> const& portals) {
    if (!pl || !sp || !sp->m_startSettings) return;
    auto st = sp->m_startSettings;

    // Если автор уровня САМ настроил этот StartPos, сохраняем точные настройки автора!
    if (orig.wasCustomConfigured) {
        st->m_startMode = static_cast<decltype(st->m_startMode)>(orig.mode);
        st->m_startSpeed = static_cast<decltype(st->m_startSpeed)>(orig.speed);
        st->m_startMini = orig.mini;
        st->m_startDual = orig.dual;
        st->m_isFlipped = orig.flipped;
        st->m_disableStartPos = false;
        return;
    }

    int mode = 0, speed = 0;
    bool flipped = false, mini = false, dual = false;
    if (pl->m_levelSettings) {
        mode = static_cast<int>(pl->m_levelSettings->m_startMode);
        speed = static_cast<int>(pl->m_levelSettings->m_startSpeed);
        mini = pl->m_levelSettings->m_startMini;
        dual = pl->m_levelSettings->m_startDual;
        flipped = pl->m_levelSettings->m_isFlipped;
    }

    float spX = sp->getPositionX();
    float spY = sp->getPositionY();

    for (auto const& p : portals) {
        if (p.x > spX + 4.f) break;
        if (std::abs(p.x - spX) < 180.f && std::abs(p.y - spY) > 210.f) continue;

        switch (p.id) {
            case 12: mode = 0; break;
            case 13: mode = 1; break;
            case 47: mode = 2; break;
            case 111: mode = 3; break;
            case 660: mode = 4; break;
            case 745: mode = 5; break;
            case 1331: mode = 6; break;
            case 1933: mode = 7; break;
            case 200: speed = 1; break;
            case 201: speed = 0; break;
            case 202: speed = 2; break;
            case 203: speed = 3; break;
            case 1334: speed = 4; break;
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
    st->m_disableStartPos = false;
}

} // namespace

class $modify(MMStartPosPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> startPos;
        std::vector<OrigStartPosCfg> origConfigs;
        std::vector<Portal> portals;
        int index = -1;
        CCMenu* menu = nullptr;
        CCLabelBMFont* label = nullptr;
        CCSprite* leftSpr = nullptr;
        CCSprite* rightSpr = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto& f = m_fields;
        f->startPos.clear();
        f->origConfigs.clear();
        f->portals.clear();

        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            int id = obj->m_objectID;
            if (id == 31) {
                f->startPos.push_back(static_cast<StartPosObject*>(obj));
            } else if (isSmartPortal(id)) {
                f->portals.push_back({obj->getPositionX(), obj->getPositionY(), id});
            }
        }

        std::stable_sort(f->portals.begin(), f->portals.end(),
                         [](Portal const& a, Portal const& b) { return a.x < b.x; });
        std::stable_sort(f->startPos.begin(), f->startPos.end(),
                         [](StartPosObject* a, StartPosObject* b) { return a->getPositionX() < b->getPositionX(); });

        if (f->startPos.empty()) return;

        int lvlMode = m_levelSettings ? static_cast<int>(m_levelSettings->m_startMode) : 0;
        int lvlSpeed = m_levelSettings ? static_cast<int>(m_levelSettings->m_startSpeed) : 0;

        f->index = -1;
        for (size_t i = 0; i < f->startPos.size(); i++) {
            auto sp = f->startPos[i];
            OrigStartPosCfg cfg{};
            if (sp && sp->m_startSettings) {
                auto st = sp->m_startSettings;
                cfg.mode = static_cast<int>(st->m_startMode);
                cfg.speed = static_cast<int>(st->m_startSpeed);
                cfg.mini = st->m_startMini;
                cfg.dual = st->m_startDual;
                cfg.flipped = st->m_isFlipped;
                cfg.wasCustomConfigured = (cfg.mode != lvlMode || cfg.speed != lvlSpeed ||
                                           cfg.mini || cfg.dual || cfg.flipped);
            }
            f->origConfigs.push_back(cfg);

            if (m_startPosObject == sp) {
                f->index = static_cast<int>(i);
            }
        }

        this->buildSwitcherUi();
        this->flashSwitcherUi();
        this->schedule(schedule_selector(MMStartPosPlayLayer::tickStartPos), 0.2f);
    }

    void buildSwitcherUi() {
        if (!m_uiLayer) return;
        auto& f = m_fields;
        auto ws = CCDirector::get()->getWinSize();

        f->menu = CCMenu::create();
        f->menu->setPosition({0.f, 0.f});
        f->menu->setVisible(mm::on(mm::F::StartPos));

        f->leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        f->leftSpr->setScale(0.52f);
        auto left = CCMenuItemSpriteExtra::create(f->leftSpr, this, menu_selector(MMStartPosPlayLayer::onPrevStartPos));
        left->setPosition({ws.width / 2.f - 76.f, 22.f});
        f->menu->addChild(left);

        f->rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        f->rightSpr->setScale(0.52f);
        f->rightSpr->setFlipX(true);
        auto right = CCMenuItemSpriteExtra::create(f->rightSpr, this, menu_selector(MMStartPosPlayLayer::onNextStartPos));
        right->setPosition({ws.width / 2.f + 76.f, 22.f});
        f->menu->addChild(right);

        f->label = CCLabelBMFont::create("", "bigFont.fnt");
        f->label->setScale(0.30f);
        f->label->setPosition({ws.width / 2.f, 22.f});
        f->menu->addChild(f->label);

        m_uiLayer->addChild(f->menu, 100);
    }

    void flashSwitcherUi() {
        auto& f = m_fields;
        if (!f->label || !f->leftSpr || !f->rightSpr) return;
        int n = static_cast<int>(f->startPos.size());
        std::string text = f->index < 0 ? fmt::format("0/{}", n)
                                        : fmt::format("{}/{}", f->index + 1, n);
        f->label->setString(text.c_str());

        f->label->stopAllActions();
        f->leftSpr->stopAllActions();
        f->rightSpr->stopAllActions();

        f->label->setOpacity(215);
        f->leftSpr->setOpacity(175);
        f->rightSpr->setOpacity(175);

        f->label->runAction(CCSequence::create(CCDelayTime::create(1.1f), CCFadeTo::create(0.45f, 0), nullptr));
        f->leftSpr->runAction(CCSequence::create(CCDelayTime::create(1.1f), CCFadeTo::create(0.45f, 40), nullptr));
        f->rightSpr->runAction(CCSequence::create(CCDelayTime::create(1.1f), CCFadeTo::create(0.45f, 40), nullptr));
    }

    void tickStartPos(float) {
        auto& f = m_fields;
        bool show = mm::on(mm::F::StartPos) && !f->startPos.empty();
        if (f->menu) f->menu->setVisible(show);
    }

    void switchStartPos(int newIdx) {
        auto& f = m_fields;
        int n = static_cast<int>(f->startPos.size());
        if (n == 0) return;

        int total = n + 1;
        newIdx = ((newIdx + 1) % total + total) % total - 1;
        f->index = newIdx;

        for (int i = 0; i < n; i++) {
            if (f->startPos[i] && f->startPos[i]->m_startSettings) {
                f->startPos[i]->m_startSettings->m_disableStartPos = (i != newIdx);
            }
        }

        StartPosObject* sp = (newIdx < 0) ? nullptr : f->startPos[newIdx];
        if (sp) {
            if (mm::on(mm::F::SmartStart)) {
                applySmart(this, sp, f->origConfigs[newIdx], f->portals);
            } else {
                auto const& orig = f->origConfigs[newIdx];
                auto st = sp->m_startSettings;
                if (st) {
                    st->m_startMode = static_cast<decltype(st->m_startMode)>(orig.mode);
                    st->m_startSpeed = static_cast<decltype(st->m_startSpeed)>(orig.speed);
                    st->m_startMini = orig.mini;
                    st->m_startDual = orig.dual;
                    st->m_isFlipped = orig.flipped;
                }
            }
        }

        m_currentCheckpoint = nullptr;
        this->setStartPosObject(sp);
        if (m_isPracticeMode) this->removeAllCheckpoints();

        this->resetLevel();

        // Сбрасываем вертикальное ускорение и отпускаем кнопку прыжка, чтобы куб НЕ вылетал вверх!
        if (m_player1) {
            m_player1->m_yVelocity = 0.0;
            m_player1->releaseButton(PlayerButton::Jump);
        }
        if (m_player2) {
            m_player2->m_yVelocity = 0.0;
            m_player2->releaseButton(PlayerButton::Jump);
        }

        this->flashSwitcherUi();
    }

    void onPrevStartPos(CCObject*) { this->switchStartPos(m_fields->index - 1); }
    void onNextStartPos(CCObject*) { this->switchStartPos(m_fields->index + 1); }
};
