#include <Geode/Geode.hpp>
#include <Geode/ui/SceneManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <chrono>
#include <deque>
#include "ModMenuPopup.hpp"
#include "Settings.hpp"

using namespace geode::prelude;

namespace {

// ----- Плавающая чёрная кнопка с буквой "L" -----
class FloatingLButton : public CCLayer {
    CCDrawNode* m_bg = nullptr;
    CCLabelBMFont* m_lbl = nullptr;
    bool m_dragging = false;
    bool m_moved = false;
    CCPoint m_touchStart;
    CCPoint m_nodeStart;
    bool m_inPause = false;

public:
    static FloatingLButton* s_instance;

    static void ensureOnScene() {
        if (!s_instance) {
            s_instance = new FloatingLButton();
            if (s_instance->init()) {
                s_instance->autorelease();
                geode::SceneManager::get()->keepAcrossScenes(s_instance);
            } else {
                delete s_instance;
                s_instance = nullptr;
            }
        }
    }

    void setPausedState(bool paused) { m_inPause = paused; }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto ws = CCDirector::get()->getWinSize();
        if (mm::state().btnX < 0.f || mm::state().btnY < 0.f) {
            mm::state().btnX = 32.f;
            mm::state().btnY = ws.height - 64.f;
        }

        this->ignoreAnchorPointForPosition(false);
        this->setAnchorPoint({0.5f, 0.5f});
        this->setContentSize({36.f, 36.f});
        this->setPosition({mm::state().btnX, mm::state().btnY});
        this->setTouchEnabled(true);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-500); // Выше обычных слоёв

        m_bg = CCDrawNode::create();
        this->addChild(m_bg, 1);

        m_lbl = CCLabelBMFont::create("L", "bigFont.fnt");
        m_lbl->setScale(0.52f);
        m_lbl->setPosition({18.f, 19.f});
        this->addChild(m_lbl, 2);

        this->redraw();
        this->scheduleUpdate();
        return true;
    }

    void redraw() {
        m_bg->clear();
        float a = mm::state().btnOpacity;
        // Полностью чёрная кнопка с аккуратной рамкой
        CCPoint pts[4] = {ccp(2, 2), ccp(34, 2), ccp(34, 34), ccp(2, 34)};
        m_bg->drawPolygon(pts, 4, {0.03f * a, 0.03f * a, 0.04f * a, a}, 1.2f,
                          {mm::accent().r / 255.f * a, mm::accent().g / 255.f * a, mm::accent().b / 255.f * a, a});
        m_lbl->setOpacity(static_cast<GLubyte>(255.f * a));
        this->setScale(mm::state().btnScale);
    }

    void update(float) override {
        this->redraw();
        auto pl = PlayLayer::get();
        if (!pl) {
            this->setVisible(true);
            return;
        }
        if (m_inPause || pl->m_isPaused) {
            this->setVisible(mm::on(mm::F::BtnShowInPause));
        } else {
            this->setVisible(!mm::on(mm::F::BtnHideInGame));
        }
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!this->isVisible()) return false;
        CCPoint lp = this->convertToNodeSpace(touch->getLocation());
        if (lp.x < 0.f || lp.x > 36.f || lp.y < 0.f || lp.y > 36.f) return false;

        m_dragging = true;
        m_moved = false;
        m_touchStart = touch->getLocation();
        m_nodeStart = this->getPosition();
        this->stopAllActions();
        this->runAction(CCScaleTo::create(0.08f, mm::state().btnScale * 0.9f));
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) return;
        CCPoint cur = touch->getLocation();
        if (ccpDistance(cur, m_touchStart) > 5.f) m_moved = true;

        auto ws = CCDirector::get()->getWinSize();
        CCPoint target = ccp(
            std::clamp(m_nodeStart.x + (cur.x - m_touchStart.x), 20.f, ws.width - 20.f),
            std::clamp(m_nodeStart.y + (cur.y - m_touchStart.y), 20.f, ws.height - 20.f)
        );
        // Плавная анимация перемещения кнопки мод-меню
        this->stopAllActions();
        this->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.12f, target)));
        mm::state().btnX = target.x;
        mm::state().btnY = target.y;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_dragging) return;
        m_dragging = false;
        this->runAction(CCEaseBackOut::create(CCScaleTo::create(0.14f, mm::state().btnScale)));
        mm::saveAll();

        if (!m_moved) {
            if (auto p = ModMenuPopup::create()) p->show();
        }
    }
};

FloatingLButton* FloatingLButton::s_instance = nullptr;

// Глобальный счётчик кликов (CPS) для Overlay
std::deque<std::chrono::steady_clock::time_point> g_clicks;

} // namespace

$on_mod(Loaded) {
    mm::load();
}

// Статичные кнопки в MenuLayer и PauseLayer УДАЛЕНЫ.
// Оставлена только инициализация плавающей кнопки L и отслеживание состояния паузы.
class $modify(MMMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        FloatingLButton::ensureOnScene();
        if (FloatingLButton::s_instance) FloatingLButton::s_instance->setPausedState(false);
        return true;
    }
};

class $modify(MMPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (FloatingLButton::s_instance) FloatingLButton::s_instance->setPausedState(true);
    }
    void onResume(CCObject* sender) {
        if (FloatingLButton::s_instance) FloatingLButton::s_instance->setPausedState(false);
        PauseLayer::onResume(sender);
    }
    void onQuit(CCObject* sender) {
        if (FloatingLButton::s_instance) FloatingLButton::s_instance->setPausedState(false);
        PauseLayer::onQuit(sender);
    }
};

// ---------------- Игровой процесс + 100% точный FPS + красивый Overlay ----------------
class $modify(MMPlayLayer, PlayLayer) {
    struct Fields {
        CCNode* ovRoot = nullptr;
        CCDrawNode* ovBg = nullptr;
        CCLabelBMFont* ovLabel = nullptr;
        float hue = 0.f;

        // Для 100% точного замера реального FPS без скачков
        std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
        int frameCount = 0;
        int stableFps = 60;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        mm::tainted() = false;
        // Включаем No Shake в настройках перед запуском уровня
        mm::applyNoShake();

        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto root = CCNode::create();
        auto bg = CCDrawNode::create();
        auto label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setAnchorPoint({0.f, 0.5f});

        root->addChild(bg, 1);
        root->addChild(label, 2);
        this->addChild(root, 1000);

        m_fields->ovRoot = root;
        m_fields->ovBg = bg;
        m_fields->ovLabel = label;
        m_fields->lastTime = std::chrono::steady_clock::now();

        mm::applySpeed(true);
        mm::applyHitboxes();
        if (FloatingLButton::s_instance) FloatingLButton::s_instance->setPausedState(false);
        return true;
    }

    void resetLevel() {
        // Если читы выключены, при рестарте с 0% очищаем tainted
        if (!mm::cheating() && !m_startPosObject && (!m_isPracticeMode || !m_currentCheckpoint)) {
            mm::tainted() = false;
        }
        PlayLayer::resetLevel();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& f = m_fields;

        if (mm::cheating()) mm::tainted() = true;

        // 1. 100% точный расчёт FPS по реальным системным часам (обновляется раз в 0.4 сек)
        f->frameCount++;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - f->lastTime).count();
        if (elapsed >= 0.40) {
            f->stableFps = static_cast<int>(std::lround(f->frameCount / elapsed));
            f->frameCount = 0;
            f->lastTime = now;
        }

        // Очистка старых кликов старше 1 секунды для CPS
        while (!g_clicks.empty() && std::chrono::duration<double>(now - g_clicks.front()).count() > 1.0) {
            g_clicks.pop_front();
        }

        // 2. Косметика игрока
        bool rIcon = mm::on(mm::F::RainbowIcon);
        bool rBG = mm::on(mm::F::RainbowBG);
        if (rIcon || rBG) {
            f->hue += dt * mm::state().rainbowSpeed;
            if (f->hue > 1.f) f->hue -= 1.f;
        }
        if (rIcon) {
            auto c1 = mm::rainbow(f->hue);
            auto c2 = mm::rainbow(f->hue + 0.5f);
            if (m_player1) { m_player1->setColor(c1); m_player1->setSecondColor(c2); }
            if (m_player2) { m_player2->setColor(c2); m_player2->setSecondColor(c1); }
        }
        if (rBG && m_background) {
            m_background->setColor(mm::rainbow(f->hue + 0.3f));
        }
        if (m_player1) m_player1->setVisible(!mm::on(mm::F::HidePlayer));
        if (m_player2) m_player2->setVisible(!mm::on(mm::F::HidePlayer));

        // 3. Обновление кастомизируемого Overlay
        if (!f->ovRoot || !f->ovLabel || !f->ovBg) return;
        bool show = mm::on(mm::F::Overlay);
        f->ovRoot->setVisible(show);
        if (!show) return;

        std::string text;
        auto append = [&](std::string const& part) {
            if (!text.empty()) text += "  |  ";
            text += part;
        };

        if (mm::on(mm::F::OvPercent)) append(fmt::format("{:.1f}%", this->getCurrentPercent()));
        if (mm::on(mm::F::ShowFPS))   append(fmt::format("{} FPS", f->stableFps));
        if (mm::on(mm::F::OvTps))     append(fmt::format("{} TPS", mm::on(mm::F::TpsBypass) ? mm::state().customTps : 240));
        if (mm::on(mm::F::OvCPS))     append(fmt::format("{} CPS", g_clicks.size()));
        if (mm::on(mm::F::OvAttempts)) append(fmt::format("Att {}", m_attempts));
        if (mm::on(mm::F::OvStatus)) {
            if (mm::safeActive()) append("SAFE");
            else if (mm::cheating()) append("CHEAT");
        }
        if (text.empty()) text = "L-Client";

        f->ovLabel->setString(text.c_str());
        float sc = mm::state().ovScale;
        f->ovLabel->setScale(sc);
        f->ovLabel->setOpacity(static_cast<GLubyte>(255.f * mm::state().ovOpacity));
        f->ovLabel->setColor(mm::themeText());

        float tw = f->ovLabel->getContentSize().width * sc;
        float th = 18.f * sc;
        float padX = 8.f;

        auto ws = CCDirector::get()->getWinSize();
        int corner = mm::state().ovCorner;
        float rx = (corner == 1 || corner == 3) ? (ws.width - tw - padX * 2.f - 8.f) : 8.f;
        float ry = (corner == 2 || corner == 3) ? 10.f : (ws.height - th - 10.f);

        f->ovRoot->setPosition({rx, ry});
        f->ovLabel->setPosition({padX, th * 0.5f});

        f->ovBg->clear();
        if (mm::on(mm::F::OvBg)) {
            float a = 0.72f * mm::state().ovOpacity;
            CCPoint pts[4] = {ccp(0, 0), ccp(tw + padX * 2.f, 0), ccp(tw + padX * 2.f, th), ccp(0, th)};
            auto acc = mm::accent();
            f->ovBg->drawPolygon(pts, 4, {0.05f * a, 0.05f * a, 0.07f * a, a}, 0.8f,
                                 {acc.r / 255.f * a, acc.g / 255.f * a, acc.b / 255.f * a, a});
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (mm::on(mm::F::Noclip)) return;
        PlayLayer::destroyPlayer(player, object);
    }

    void onQuit() {
        mm::applySpeed(false);
        mm::tainted() = false;
        if (FloatingLButton::s_instance) FloatingLButton::s_instance->setPausedState(false);
        PlayLayer::onQuit();
    }
};

class $modify(MMPlayerObject, PlayerObject) {
    bool pushButton(PlayerButton btn) {
        g_clicks.push_back(std::chrono::steady_clock::now());
        return PlayerObject::pushButton(btn);
    }
    void playDeathEffect() {
        if (mm::on(mm::F::NoDeath)) return;
        PlayerObject::playDeathEffect();
    }
};

class $modify(MMGJGameLevel, GJGameLevel) {
    void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid) {
        if (mm::safeActive()) return;
        GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);
    }
};

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
