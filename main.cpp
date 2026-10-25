#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
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

// Правильный premultiplied цвет для CCDrawNode (чтобы Opacity работала на 100%!)
ccColor4F PM(ccColor3B c, float a) {
    return { (c.r / 255.f) * a, (c.g / 255.f) * a, (c.b / 255.f) * a, a };
}

void drawBtnRoundRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill, float bw, ccColor4F bc) {
    if (s.width < 1.f || s.height < 1.f) return;
    r = std::clamp(r, 0.f, std::min(s.width, s.height) * 0.5f - 0.1f);

    std::vector<CCPoint> pts;
    auto pushUnique = [&](float x, float y) {
        CCPoint p = ccp(x, y);
        if (!pts.empty() && std::abs(pts.back().x - p.x) < 0.05f && std::abs(pts.back().y - p.y) < 0.05f) return;
        pts.push_back(p);
    };

    if (r < 0.5f) {
        pushUnique(o.x, o.y);
        pushUnique(o.x + s.width, o.y);
        pushUnique(o.x + s.width, o.y + s.height);
        pushUnique(o.x, o.y + s.height);
    } else {
        constexpr int SEG = 8;
        auto addCorner = [&](float cx, float cy, float startAngle) {
            for (int i = 0; i <= SEG; i++) {
                float a = startAngle + (PI_F * 0.5f) * (static_cast<float>(i) / SEG);
                pushUnique(cx + r * std::cos(a), cy + r * std::sin(a));
            }
        };
        addCorner(o.x + s.width - r, o.y + s.height - r, 0.f);
        addCorner(o.x + r,           o.y + s.height - r, PI_F * 0.5f);
        addCorner(o.x + r,           o.y + r,           PI_F);
        addCorner(o.x + s.width - r, o.y + r,           PI_F * 1.5f);
    }
    while (pts.size() > 3 && std::abs(pts.front().x - pts.back().x) < 0.05f && std::abs(pts.front().y - pts.back().y) < 0.05f) {
        pts.pop_back();
    }
    if (pts.size() >= 3) {
        n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), fill, bw, bc);
    }
}

// ----- Глобальная кнопка "L" с прямым перехватом касаний -----
class FloatingLButton : public CCNode {
    CCDrawNode* m_bg = nullptr;
    CCLabelBMFont* m_lbl = nullptr;
    bool m_dragging = false;
    bool m_moved = false;
    int m_activeTouchId = -1;
    CCPoint m_touchStart;
    CCPoint m_nodeStart;
    CCPoint m_targetPos;
    bool m_inPause = false;

public:
    static FloatingLButton* s_instance;

    static FloatingLButton* get() {
        if (!s_instance) {
            s_instance = new FloatingLButton();
            s_instance->init();
            s_instance->retain();
        }
        return s_instance;
    }

    void setPausedState(bool paused) { m_inPause = paused; }

    bool init() override {
        if (!CCNode::init()) return false;

        auto ws = CCDirector::get()->getWinSize();
        if (mm::state().btnX < 15.f || mm::state().btnY < 15.f ||
            mm::state().btnX > ws.width - 15.f || mm::state().btnY > ws.height - 15.f) {
            mm::state().btnX = 42.f;
            mm::state().btnY = ws.height - 64.f;
        }
        m_targetPos = ccp(mm::state().btnX, mm::state().btnY);

        this->ignoreAnchorPointForPosition(false);
        this->setAnchorPoint({0.5f, 0.5f});
        this->setContentSize({38.f, 38.f});
        this->setPosition(m_targetPos);

        m_bg = CCDrawNode::create();
        this->addChild(m_bg, 1);

        m_lbl = CCLabelBMFont::create("L", "bigFont.fnt");
        m_lbl->setScale(0.50f);
        m_lbl->setPosition({19.f, 20.f});
        this->addChild(m_lbl, 2);

        return true;
    }

    bool shouldBeVisible() const {
        if (mm::state().menuOpen) return false;
        auto scene = CCDirector::get()->getRunningScene();
        if (scene && scene->getChildByType<ModMenuPopup>(0)) return false;

        auto pl = PlayLayer::get();
        if (!pl) return true; // В любых меню видна всегда!

        bool paused = m_inPause || pl->m_isPaused || (scene && scene->getChildByType<PauseLayer>(0) != nullptr);
        if (paused) return mm::on(mm::F::BtnShowInPause);
        return !mm::on(mm::F::BtnHideInGame);
    }

    void tickAndDraw() {
        if (!this->shouldBeVisible()) return;

        auto dir = CCDirector::get();
        float dt = std::clamp(dir->getDeltaTime() / std::max(mm::speed(), 0.25f), 0.001f, 0.05f);
        CCPoint cur = this->getPosition();
        CCPoint next = ccp(
            cur.x + (m_targetPos.x - cur.x) * std::min(1.f, dt * 18.f),
            cur.y + (m_targetPos.y - cur.y) * std::min(1.f, dt * 18.f)
        );
        this->setPosition(next);

        float a = std::clamp(mm::state().btnOpacity, 0.20f, 1.0f);
        float rad = std::clamp(mm::state().btnRadius, 0.f, 17.5f);
        float sc = std::clamp(mm::state().btnScale, 0.55f, 1.60f);
        auto acc = mm::accent();

        m_bg->clear();
        drawBtnRoundRect(m_bg, ccp(1.f, 1.f), CCSize(36.f, 36.f), rad,
                         PM({12, 12, 18}, a), 1.2f, PM(acc, a));

        m_lbl->setOpacity(static_cast<GLubyte>(255.f * a));
        this->setScale(m_dragging ? sc * 0.88f : sc);

        this->visit();
    }

    // Прямая обработка нажатия из хука CCTouchDispatcher
    bool handleTouchBegan(CCTouch* touch) {
        if (!touch || !this->shouldBeVisible()) return false;

        CCPoint touchPos = touch->getLocation();
        CCPoint center = this->getPosition();
        float halfHit = 22.f * std::clamp(mm::state().btnScale, 0.55f, 1.60f);

        if (std::abs(touchPos.x - center.x) > halfHit ||
            std::abs(touchPos.y - center.y) > halfHit) {
            return false;
        }

        m_dragging = true;
        m_moved = false;
        m_activeTouchId = touch->getID();
        m_touchStart = touchPos;
        m_nodeStart = m_targetPos;
        return true;
    }

    bool handleTouchMoved(CCTouch* touch) {
        if (!m_dragging || !touch || touch->getID() != m_activeTouchId) return false;

        CCPoint cur = touch->getLocation();
        if (ccpDistance(cur, m_touchStart) > 5.f) {
            m_moved = true;
        }

        auto ws = CCDirector::get()->getWinSize();
        m_targetPos = ccp(
            std::clamp(m_nodeStart.x + (cur.x - m_touchStart.x), 22.f, ws.width - 22.f),
            std::clamp(m_nodeStart.y + (cur.y - m_touchStart.y), 22.f, ws.height - 22.f)
        );
        mm::state().btnX = m_targetPos.x;
        mm::state().btnY = m_targetPos.y;
        return true;
    }

    bool handleTouchEnded(CCTouch* touch) {
        if (!m_dragging || !touch || touch->getID() != m_activeTouchId) return false;

        m_dragging = false;
        m_activeTouchId = -1;
        mm::saveAll();

        if (!m_moved) {
            if (auto p = ModMenuPopup::create()) {
                p->show();
            }
        }
        return true;
    }
};

FloatingLButton* FloatingLButton::s_instance = nullptr;
std::deque<std::chrono::steady_clock::time_point> g_clicks;

} // namespace

$on_mod(Loaded) {
    mm::load();
}

// 1. Отрисовка кнопки L поверх всех сцен без морганий
class $modify(MMLoonDirector, CCDirector) {
    void drawScene() {
        CCDirector::drawScene();
        if (this->getRunningScene()) {
            FloatingLButton::get()->tickAndDraw();
        }
    }
};

// 2. Железобетонный перехват касаний напрямую в CCTouchDispatcher!
class $modify(MMLoonTouchDispatcher, CCTouchDispatcher) {
    void touches(CCSet* touches, CCEvent* event, unsigned int type) {
        if (touches && touches->count() > 0) {
            auto touch = static_cast<CCTouch*>(touches->anyObject());
            auto btn = FloatingLButton::get();

            if (type == CCTOUCHBEGAN) {
                if (btn->handleTouchBegan(touch)) return;
            } else if (type == CCTOUCHMOVED) {
                if (btn->handleTouchMoved(touch)) return;
            } else if (type == CCTOUCHENDED || type == CCTOUCHCANCELLED) {
                if (btn->handleTouchEnded(touch)) return;
            }
        }
        CCTouchDispatcher::touches(touches, event, type);
    }
};

class $modify(MMPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        FloatingLButton::get()->setPausedState(true);
    }
    void onResume(CCObject* sender) {
        FloatingLButton::get()->setPausedState(false);
        PauseLayer::onResume(sender);
    }
    void onQuit(CCObject* sender) {
        FloatingLButton::get()->setPausedState(false);
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
        FloatingLButton::get()->setPausedState(false);
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
        if (text.empty()) text = "Loon Client";

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
                             PM({12, 12, 18}, a), 0.8f, PM(acc, a * 0.7f));
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (mm::on(mm::F::Noclip)) return;
        PlayLayer::destroyPlayer(player, object);
    }

    void onQuit() {
        mm::applySpeed(false);
        mm::tainted() = false;
        FloatingLButton::get()->setPausedState(false);
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
