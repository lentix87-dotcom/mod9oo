#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <chrono>
#include <cmath>
#include <deque>
#include "ModMenuPopup.hpp"
#include "Settings.hpp"

using namespace geode::prelude;

namespace {

constexpr float PI_F = 3.14159265f;

void drawBtnRoundRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill, float bw, ccColor4F bc) {
    r = std::clamp(r, 0.f, std::min(s.width, s.height) * 0.5f);
    std::vector<CCPoint> pts;
    if (r < 0.6f) {
        pts = {ccp(o.x, o.y), ccp(o.x + s.width, o.y), ccp(o.x + s.width, o.y + s.height), ccp(o.x, o.y + s.height)};
    } else {
        constexpr int SEG = 6;
        auto arc = [&](float cx, float cy, float a0) {
            for (int i = 0; i <= SEG; i++) {
                float a = a0 + (PI_F * 0.5f) * i / SEG;
                pts.push_back(ccp(cx + r * std::cos(a), cy + r * std::sin(a)));
            }
        };
        arc(o.x + s.width - r, o.y + s.height - r, 0.f);
        arc(o.x + r, o.y + s.height - r, PI_F * 0.5f);
        arc(o.x + r, o.y + r, PI_F);
        arc(o.x + s.width - r, o.y + r, 1.5f * PI_F);
    }
    n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), fill, bw, bc);
}

// ----- Плавающая чёрная кнопка с буквой "L" (Видна везде + Плавный Lerp + Opacity + Скругление) -----
class FloatingLButton : public CCLayer {
    CCDrawNode* m_bg = nullptr;
    CCLabelBMFont* m_lbl = nullptr;
    bool m_dragging = false;
    bool m_moved = false;
    CCPoint m_touchStart;
    CCPoint m_nodeStart;
    CCPoint m_targetPos;
    bool m_inPause = false;

public:
    static FloatingLButton* s_instance;

    static void ensureOnRunningScene() {
        auto scene = CCDirector::get()->getRunningScene();
        if (!scene) return;

        if (!s_instance) {
            s_instance = new FloatingLButton();
            if (s_instance->init()) {
                s_instance->retain();
            } else {
                delete s_instance;
                s_instance = nullptr;
                return;
            }
        }

        if (s_instance->getParent() != scene) {
            s_instance->removeFromParentAndCleanup(false);
            scene->addChild(s_instance, 10005);
        }
    }

    void setPausedState(bool paused) { m_inPause = paused; }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto ws = CCDirector::get()->getWinSize();
        if (mm::state().btnX < 0.f || mm::state().btnY < 0.f) {
            mm::state().btnX = 36.f;
            mm::state().btnY = ws.height - 64.f;
        }
        m_targetPos = ccp(mm::state().btnX, mm::state().btnY);

        this->ignoreAnchorPointForPosition(false);
        this->setAnchorPoint({0.5f, 0.5f});
        this->setContentSize({36.f, 36.f});
        this->setPosition(m_targetPos);
        this->setTouchEnabled(true);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-510);

        m_bg = CCDrawNode::create();
        // Стандартный альфа-блендинг, чтобы Opacity работала на 100% правильно!
        m_bg->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        this->addChild(m_bg, 1);

        m_lbl = CCLabelBMFont::create("L", "bigFont.fnt");
        m_lbl->setScale(0.50f);
        m_lbl->setPosition({18.f, 19.f});
        this->addChild(m_lbl, 2);

        this->redraw();
        this->scheduleUpdate();
        return true;
    }

    void onEnter() override {
        CCLayer::onEnter();
        CCDirector::get()->getTouchDispatcher()->removeDelegate(this);
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -510, true);
    }

    void onExit() override {
        CCDirector::get()->getTouchDispatcher()->removeDelegate(this);
        CCLayer::onExit();
    }

    void redraw() {
        m_bg->clear();
        float a = std::clamp(mm::state().btnOpacity, 0.15f, 1.0f);
        float rad = std::clamp(mm::state().btnRadius, 0.f, 17.f);
        auto acc = mm::accent();

        // Тёмно-чёрная кнопка со скруглёнными углами и прозрачностью a
        drawBtnRoundRect(m_bg, ccp(1.f, 1.f), CCSize(34.f, 34.f), rad,
                         {0.04f, 0.04f, 0.06f, a}, 1.1f,
                         {acc.r / 255.f, acc.g / 255.f, acc.b / 255.f, a * 0.9f});

        m_lbl->setOpacity(static_cast<GLubyte>(255.f * a));
        if (!m_dragging) {
            this->setScale(mm::state().btnScale);
        }
    }

    void update(float dt) override {
        // Плавная интерполяция (Smooth Lerp) позиции кнопки к пальцу
        float realDt = std::clamp(dt / std::max(mm::speed(), 0.25f), 0.001f, 0.05f);
        CCPoint cur = this->getPosition();
        CCPoint next = ccp(
            cur.x + (m_targetPos.x - cur.x) * std::min(1.f, realDt * 18.f),
            cur.y + (m_targetPos.y - cur.y) * std::min(1.f, realDt * 18.f)
        );
        this->setPosition(next);

        this->redraw();

        // Видна ВЕЗДЕ, кроме активной игры в PlayLayer (если включён Hide L-Btn in Play)
        auto pl = PlayLayer::get();
        if (!pl) {
            m_inPause = false;
            this->setVisible(true);
            return;
        }
        bool paused = m_inPause || pl->m_isPaused || (CCDirector::get()->getRunningScene()->getChildByType<PauseLayer>(0) != nullptr);
        if (paused) {
            this->setVisible(mm::on(mm::F::BtnShowInPause));
        } else {
            this->setVisible(!mm::on(mm::F::BtnHideInGame));
        }
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!this->isVisible()) return false;
        // Если открыто само мод-меню, не перехватываем клики поверх его ползунков
        if (CCDirector::get()->getRunningScene()->getChildByType<ModMenuPopup>(0)) return false;

        CCPoint lp = this->convertToNodeSpace(touch->getLocation());
        if (lp.x < -2.f || lp.x > 38.f || lp.y < -2.f || lp.y > 38.f) return false;

        m_dragging = true;
        m_moved = false;
        m_touchStart = touch->getLocation();
        m_nodeStart = m_targetPos;
        this->stopAllActions();
        this->runAction(CCEaseSineOut::create(CCScaleTo::create(0.08f, mm::state().btnScale * 0.88f)));
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) return;
        CCPoint cur = touch->getLocation();
        if (ccpDistance(cur, m_touchStart) > 5.f) m_moved = true;

        auto ws = CCDirector::get()->getWinSize();
        m_targetPos = ccp(
            std::clamp(m_nodeStart.x + (cur.x - m_touchStart.x), 20.f, ws.width - 20.f),
            std::clamp(m_nodeStart.y + (cur.y - m_touchStart.y), 20.f, ws.height - 20.f)
        );
        mm::state().btnX = m_targetPos.x;
        mm::state().btnY = m_targetPos.y;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_dragging) return;
        m_dragging = false;
        this->runAction(CCEaseBackOut::create(CCScaleTo::create(0.16f, mm::state().btnScale)));
        mm::saveAll();

        if (!m_moved) {
            if (auto p = ModMenuPopup::create()) p->show();
        }
    }
};

FloatingLButton* FloatingLButton::s_instance = nullptr;
std::deque<std::chrono::steady_clock::time_point> g_clicks;

} // namespace

$on_mod(Loaded) {
    mm::load();
}

// Гарантирует, что кнопка L видна абсолютно на ВСЕХ экранах игры (LevelSelect, Garage, Editor, Menu)
class $modify(MMGlobalBtnScheduler, CCScheduler) {
    void update(float dt) {
        CCScheduler::update(dt);
        FloatingLButton::ensureOnRunningScene();
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

class $modify(MMPlayLayer, PlayLayer) {
    struct Fields {
        CCNode* ovRoot = nullptr;
        CCDrawNode* ovBg = nullptr;
        CCLabelBMFont* ovLabel = nullptr;
        float hue = 0.f;

        std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
        int frameCount = 0;
        int stableFps = 60;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        mm::tainted() = false;
        mm::applyNoShake();

        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto root = CCNode::create();
        auto bg = CCDrawNode::create();
        bg->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
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
        if (!mm::cheating() && !m_startPosObject && (!m_isPracticeMode || !m_currentCheckpoint)) {
            mm::tainted() = false;
        }
        PlayLayer::resetLevel();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& f = m_fields;

        if (mm::cheating()) mm::tainted() = true;

        f->frameCount++;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - f->lastTime).count();
        if (elapsed >= 0.40) {
            f->stableFps = static_cast<int>(std::lround(f->frameCount / elapsed));
            f->frameCount = 0;
            f->lastTime = now;
        }

        while (!g_clicks.empty() && std::chrono::duration<double>(now - g_clicks.front()).count() > 1.0) {
            g_clicks.pop_front();
        }

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
            float a = 0.75f * mm::state().ovOpacity;
            auto acc = mm::accent();
            drawBtnRoundRect(f->ovBg, ccp(0.f, 0.f), CCSize(tw + padX * 2.f, th), 5.f,
                             {0.05f, 0.05f, 0.08f, a}, 0.8f,
                             {acc.r / 255.f, acc.g / 255.f, acc.b / 255.f, a * 0.7f});
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
