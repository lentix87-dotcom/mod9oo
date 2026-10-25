#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <cmath>

namespace {

constexpr float PI = 3.14159265f;
constexpr float WIN_W = 460.f;
constexpr float WIN_H = 290.f;
constexpr float DOCK_Y = 23.f; // Высота нижней панели вкладок

constexpr char const* TAB_TITLES[5] = {"Player", "Level", "Visuals", "Themes", "Client"};
constexpr char const* TAB_ICON_FRAMES[5] = {
    "GJ_playBtn2_001.png",
    "GJ_practiceBtn_001.png",
    "GJ_infoIcon_001.png",
    "GJ_paintBtn_001.png",
    "GJ_optionsBtn_001.png"
};

const ccColor3B CHEAT_RED = {255, 88, 98};
const ccColor3B CHEAT_RED_ON = {255, 120, 130};
const ccColor3B MUTED_TXT = {148, 155, 174};

// Правильный premultiplied цвет для CCDrawNode (чтобы Opacity работала на 100% везде!)
ccColor4F P(ccColor3B c, float a) {
    return { (c.r / 255.f) * a, (c.g / 255.f) * a, (c.b / 255.f) * a, a };
}

ccColor3B mixCol(ccColor3B a, ccColor3B b, float t) {
    return {
        static_cast<GLubyte>(a.r + (b.r - a.r) * t),
        static_cast<GLubyte>(a.g + (b.g - a.g) * t),
        static_cast<GLubyte>(a.b + (b.b - a.b) * t)
    };
}

float tabX(int idx) {
    float startX = 66.f;
    float stepX = 82.f;
    return startX + idx * stepX;
}

void drawRoundRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill,
                   float bw = 0.f, ccColor4F bc = {0.f, 0.f, 0.f, 0.f}) {
    if (s.width < 1.f || s.height < 1.f) return;
    r = std::clamp(r, 0.f, std::min(s.width, s.height) * 0.5f - 0.1f);

    std::vector<CCPoint> pts;
    pts.reserve(40);
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
                float a = startAngle + (PI * 0.5f) * (static_cast<float>(i) / SEG);
                pushUnique(cx + r * std::cos(a), cy + r * std::sin(a));
            }
        };
        addCorner(o.x + s.width - r, o.y + s.height - r, 0.f);
        addCorner(o.x + r,           o.y + s.height - r, PI * 0.5f);
        addCorner(o.x + r,           o.y + r,           PI);
        addCorner(o.x + s.width - r, o.y + r,           PI * 1.5f);
    }
    while (pts.size() > 3 && std::abs(pts.front().x - pts.back().x) < 0.05f && std::abs(pts.front().y - pts.back().y) < 0.05f) {
        pts.pop_back();
    }
    if (pts.size() >= 3) {
        n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), fill, bw, bc);
    }
}

void drawPolyCircle(CCDrawNode* n, CCPoint c, float r, ccColor4F col, int segs = 14) {
    std::vector<CCPoint> pts(segs);
    for (int i = 0; i < segs; i++) {
        float a = 2.f * PI * i / segs;
        pts[i] = ccp(c.x + r * std::cos(a), c.y + r * std::sin(a));
    }
    n->drawPolygon(pts.data(), segs, col, 0.f, {0.f, 0.f, 0.f, 0.f});
}

void drawSoftWhiteGlow(CCDrawNode* n, CCPoint o, CCSize s, float r, float intensity, ccColor3B tint = {255, 255, 255}) {
    if (!mm::on(mm::F::Glow) || intensity <= 0.01f) return;
    constexpr float EXPANDS[5] = {1.5f, 3.4f, 5.8f, 8.8f, 12.5f};
    constexpr float ALPHAS[5]  = {0.10f, 0.065f, 0.04f, 0.022f, 0.01f};
    for (int i = 0; i < 5; i++) {
        float ex = EXPANDS[i];
        float a = ALPHAS[i] * intensity;
        drawRoundRect(n, ccp(o.x - ex, o.y - ex),
                      CCSize(s.width + ex * 2.f, s.height + ex * 2.f),
                      r + ex, P(tint, a));
    }
}

void drawLoonGlass(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor3B col, float alpha,
                   ccColor3B borderCol = {255, 255, 255}, float borderAlpha = 0.12f) {
    bool glassOn = mm::on(mm::F::LiquidGlass);
    float glassK = glassOn ? mm::state().glassAlpha : 0.98f;
    float finalAlpha = std::clamp(alpha * glassK, 0.35f, 0.98f);

    drawRoundRect(n, o, s, r, P(col, finalAlpha), 0.85f, P(borderCol, borderAlpha));

    if (glassOn) {
        float hazeIntensity = (1.0f - mm::state().glassAlpha) * 0.18f + 0.03f;
        drawRoundRect(n, ccp(o.x + 1.f, o.y + 1.f), CCSize(s.width - 2.f, s.height - 2.f),
                      std::max(0.f, r - 1.f), P({235, 242, 255}, hazeIntensity));
    }
}

} // namespace

ModMenuPopup* ModMenuPopup::create() {
    auto ret = new ModMenuPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

// Настоящий многослойный Screen Blur без бага с зумом!
void ModMenuPopup::createScreenBlur() {
    auto ws = CCDirector::get()->getWinSize();
    m_screenBlurHost = CCNode::create();
    m_screenBlurHost->setPosition({0.f, 0.f});
    this->addChild(m_screenBlurHost, -50);
    m_blurPasses.clear();

    auto scene = CCDirector::get()->getRunningScene();
    if (scene) {
        // Создаём CCRenderTexture ровно в размер виртуального окна Cocos2d (без уменьшения viewport, чтобы НЕ было зума!)
        int w = static_cast<int>(ws.width);
        int h = static_cast<int>(ws.height);
        auto rt = CCRenderTexture::create(w, h);
        if (rt) {
            bool wasVis = this->isVisible();
            this->setVisible(false);
            rt->begin();
            scene->visit();
            rt->end();
            this->setVisible(wasVis);

            if (auto spr = rt->getSprite()) {
                if (auto tex = spr->getTexture()) {
                    ccTexParams params = {GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
                    tex->setTexParameters(&params);

                    // Накладываем 9 смещённых полупрозрачных слоёв (Kawase Box Blur) для настоящего мягкого размытия экрана!
                    constexpr float OFFSETS[9][2] = {
                        {0.f, 0.f}, {-2.8f, -2.8f}, {2.8f, -2.8f}, {-2.8f, 2.8f}, {2.8f, 2.8f},
                        {-5.5f, 0.f}, {5.5f, 0.f}, {0.f, -5.5f}, {0.f, 5.5f}
                    };
                    for (int i = 0; i < 9; i++) {
                        auto pass = CCSprite::createWithTexture(tex);
                        pass->setFlipY(true);
                        pass->setPosition(ccp(ws.width * 0.5f + OFFSETS[i][0], ws.height * 0.5f + OFFSETS[i][1]));
                        m_screenBlurHost->addChild(pass, i);
                        m_blurPasses.push_back(pass);
                    }
                }
            }
        }
    }

    m_blurFrostOverlay = CCDrawNode::create();
    m_screenBlurHost->addChild(m_blurFrostOverlay, 20);
    this->updateScreenBlurOpacity();
}

void ModMenuPopup::updateScreenBlurOpacity() {
    auto ws = CCDirector::get()->getWinSize();
    bool blurOn = mm::on(mm::F::BgBlur);
    float strength = blurOn ? mm::state().bgBlurStrength : 0.f;

    this->setOpacity(0);

    for (size_t i = 0; i < m_blurPasses.size(); i++) {
        auto pass = m_blurPasses[i];
        if (!pass) continue;
        pass->setVisible(blurOn && strength > 0.02f);
        // Первый слой — базовый, остальные 8 — размывающие ореолы, чья сила растёт со ползунком Screen Blur
        float alpha = (i == 0) ? std::min(1.f, strength * 1.2f) : (strength * 0.36f);
        pass->setOpacity(static_cast<GLubyte>(255.f * alpha));
    }

    if (m_blurFrostOverlay) {
        m_blurFrostOverlay->clear();
        if (blurOn && strength > 0.01f) {
            drawRoundRect(m_blurFrostOverlay, ccp(0.f, 0.f), ws, 0.f, P({10, 12, 18}, 0.58f * strength));
            drawRoundRect(m_blurFrostOverlay, ccp(0.f, 0.f), ws, 0.f, P({215, 228, 255}, 0.07f * strength));
        }
    }
}

bool ModMenuPopup::init() {
    if (!Popup::init(WIN_W, WIN_H)) return false;

    mm::state().menuOpen = true;

    m_bgSprite->setVisible(false);
    if (m_title) m_title->setVisible(false);
    if (m_closeBtn) {
        m_closeBtn->setVisible(false);
        m_closeBtn->setEnabled(false);
    }

    this->createScreenBlur();

    m_buttonMenu->setContentSize(m_size);
    m_buttonMenu->setAnchorPoint({0.f, 0.f});
    m_buttonMenu->setPosition({0.f, 0.f});

    m_mainLayer->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->ignoreAnchorPointForPosition(false);
    m_mainLayer->setPosition(CCDirector::get()->getWinSize() * 0.5f);
    m_mainLayer->setScale(0.88f);
    m_mainLayer->runAction(CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.0f)));

    m_glowNode = CCDrawNode::create();
    m_mainLayer->addChild(m_glowNode, -15);

    m_bgNode = CCDrawNode::create();
    m_mainLayer->addChild(m_bgNode, -10);

    m_particleNode = CCDrawNode::create();
    m_mainLayer->addChild(m_particleNode, -5);

    // ----- Верхняя шапка Loon Client: Логотип слева + Профиль игрока по центру + Закрытие справа -----
    auto logoL = CCLabelBMFont::create("L", "bigFont.fnt");
    logoL->setScale(0.46f);
    logoL->setPosition({24.f, WIN_H - 21.f});
    m_mainLayer->addChild(logoL, 5);

    auto brandLbl = CCLabelBMFont::create("LOON CLIENT", "chatFont.fnt");
    brandLbl->setScale(0.78f);
    brandLbl->setAnchorPoint({0.f, 0.5f});
    brandLbl->setPosition({38.f, WIN_H - 21.f});
    m_mainLayer->addChild(brandLbl, 5);

    // Профиль игрока и кнопка "?" описания в верхней шапке
    {
        std::string playerName = "Player";
        if (auto gm = GameManager::get()) {
            if (!gm->m_playerName.empty()) playerName = gm->m_playerName;
        }
        auto pNode = CCNode::create();
        pNode->setContentSize({130.f, 24.f});
        auto pDraw = CCDrawNode::create();
        drawPolyCircle(pDraw, ccp(12.f, 12.f), 8.5f, P({45, 50, 68}, 0.95f), 14);
        pNode->addChild(pDraw);

        auto qSym = CCLabelBMFont::create("?", "chatFont.fnt");
        qSym->setScale(0.70f);
        qSym->setPosition({12.f, 12.f});
        pNode->addChild(qSym);

        auto uName = CCLabelBMFont::create(playerName.c_str(), "chatFont.fnt");
        uName->setScale(0.65f);
        uName->setAnchorPoint({0.f, 0.5f});
        uName->setPosition({25.f, 12.f});
        uName->setColor({235, 240, 252});
        pNode->addChild(uName);

        auto qBtn = CCMenuItemSpriteExtra::create(pNode, this, menu_selector(ModMenuPopup::onModInfo));
        qBtn->setPosition({205.f, WIN_H - 21.f});
        m_buttonMenu->addChild(qBtn, 15);
    }

    // Кнопка закрытия "X" справа вверху
    {
        auto cNode = CCNode::create();
        cNode->setContentSize({24.f, 24.f});
        auto cDraw = CCDrawNode::create();
        drawLoonGlass(cDraw, ccp(2.f, 2.f), CCSize(20.f, 20.f), 7.f, mm::themeCard(), 0.95f, {255, 255, 255}, 0.16f);
        cDraw->drawSegment(ccp(8.5f, 8.5f), ccp(15.5f, 15.5f), 0.9f, P({225, 230, 245}, 1.f));
        cDraw->drawSegment(ccp(8.5f, 15.5f), ccp(15.5f, 8.5f), 0.9f, P({225, 230, 245}, 1.f));
        cNode->addChild(cDraw);

        auto cBtn = CCMenuItemSpriteExtra::create(cNode, this, menu_selector(ModMenuPopup::onClosePopup));
        cBtn->setPosition({WIN_W - 20.f, WIN_H - 21.f});
        m_buttonMenu->addChild(cBtn, 15);
    }

    // ----- НИЖНЯЯ ДОК-ПАНЕЛЬ ВКЛАДОК (Bottom Navigation Bar) со значками GD в родных цветах! -----
    m_tabSlider = CCDrawNode::create();
    m_tabSlider->setPosition({tabX(0), DOCK_Y});
    m_mainLayer->addChild(m_tabSlider, 4);

    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({0.f, 0.f});
    for (int i = 0; i < TAB_COUNT; i++) {
        auto bNode = CCNode::create();
        bNode->setContentSize({76.f, 28.f});

        // Иконка GD слева от названия вкладки — БЕЗ перекрашивания в цвет темы!
        auto icon = CCSprite::createWithSpriteFrameName(TAB_ICON_FRAMES[i]);
        if (!icon) icon = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
        if (icon) {
            float maxDim = std::max(icon->getContentSize().width, icon->getContentSize().height);
            icon->setScale(14.f / std::max(1.f, maxDim));
            icon->setPosition({15.f, 14.f});
            bNode->addChild(icon, 2);
            m_tabIcons[i] = icon;
        }

        auto bLbl = CCLabelBMFont::create(TAB_TITLES[i], "chatFont.fnt");
        bLbl->setScale(0.68f);
        bLbl->setAnchorPoint({0.f, 0.5f});
        bLbl->setPosition({27.f, 14.f});
        bNode->addChild(bLbl, 2);
        m_tabLabels[i] = bLbl;

        auto btn = CCMenuItemSpriteExtra::create(bNode, this, menu_selector(ModMenuPopup::onTab));
        btn->setTag(i);
        btn->setPosition({tabX(i), DOCK_Y});
        bottomMenu->addChild(btn);
    }
    m_mainLayer->addChild(bottomMenu, 12);

    m_contentHost = CCNode::create();
    m_contentHost->setContentSize(m_size);
    m_mainLayer->addChild(m_contentHost, 6);

    m_subPopupHost = CCNode::create();
    m_subPopupHost->setContentSize(m_size);
    m_mainLayer->addChild(m_subPopupHost, 50);

    this->initParticles();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildContent(false);
    this->scheduleUpdate();
    return true;
}

void ModMenuPopup::onExit() {
    mm::state().menuOpen = false;
    mm::saveAll();
    Popup::onExit();
}

void ModMenuPopup::initParticles() {
    m_particles.clear();
    m_particles.resize(24);
    for (int i = 0; i < 24; i++) {
        m_particles[i] = {
            ccp(std::rand() % static_cast<int>(WIN_W), std::rand() % static_cast<int>(WIN_H)),
            ccp(((std::rand() % 100) / 100.f - 0.5f) * 16.f, -16.f - (std::rand() % 20)),
            2.0f + (std::rand() % 26) / 10.f,
            static_cast<float>(std::rand() % 360),
            ((std::rand() % 100) / 100.f - 0.5f) * 2.8f,
            0.35f + (std::rand() % 40) / 100.f
        };
    }
}

void ModMenuPopup::update(float dt) {
    m_particleNode->clear();
    int pType = mm::state().particleType;
    if (pType <= 0) return;

    float realDt = std::clamp(dt / std::max(mm::speed(), 0.25f), 0.002f, 0.05f);

    for (auto& p : m_particles) {
        p.pos.x += p.vel.x * realDt;
        p.pos.y += p.vel.y * realDt;
        p.angle += p.spin * realDt;

        if (p.pos.y < 12.f) {
            p.pos.y = WIN_H - 12.f;
            p.pos.x = 12.f + (std::rand() % static_cast<int>(WIN_W - 24.f));
        }
        if (p.pos.x < 12.f) p.pos.x = WIN_W - 14.f;
        if (p.pos.x > WIN_W - 12.f) p.pos.x = 14.f;

        if (pType == 1) {
            drawPolyCircle(m_particleNode, p.pos, p.size * 0.75f, P({240, 248, 255}, p.alpha), 8);
        } else if (pType == 2) {
            ccColor3B leafCol = (static_cast<int>(p.size * 10) % 2 == 0) ? ccColor3B{240, 130, 45} : ccColor3B{220, 75, 40};
            float c = std::cos(p.angle), s = std::sin(p.angle);
            float rx = p.size * 1.3f, ry = p.size * 0.7f;
            CCPoint pts[4] = {
                ccp(p.pos.x + c * rx, p.pos.y + s * rx),
                ccp(p.pos.x - s * ry, p.pos.y + c * ry),
                ccp(p.pos.x - c * rx, p.pos.y - s * rx),
                ccp(p.pos.x + s * ry, p.pos.y - c * ry)
            };
            m_particleNode->drawPolygon(pts, 4, P(leafCol, p.alpha), 0.f, {0, 0, 0, 0});
        } else if (pType == 3) {
            ccColor3B sakCol = {255, 168, 208};
            float c = std::cos(p.angle), s = std::sin(p.angle);
            float rx = p.size * 1.2f, ry = p.size * 0.65f;
            CCPoint pts[4] = {
                ccp(p.pos.x + c * rx, p.pos.y + s * rx),
                ccp(p.pos.x - s * ry, p.pos.y + c * ry),
                ccp(p.pos.x - c * rx, p.pos.y - s * rx),
                ccp(p.pos.x + s * ry, p.pos.y - c * ry)
            };
            m_particleNode->drawPolygon(pts, 4, P(sakCol, p.alpha), 0.f, {0, 0, 0, 0});
        }
    }
}

void ModMenuPopup::redrawFrame() {
    m_bgNode->clear();
    m_glowNode->clear();

    auto bg = mm::themeBg();
    auto card = mm::themeCard();
    float glow = mm::state().glowIntensity;
    float menuRad = std::clamp(mm::state().menuRadius, 0.f, 28.f);

    drawSoftWhiteGlow(m_glowNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), menuRad, glow);
    drawLoonGlass(m_bgNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), menuRad, bg, 0.94f, {255, 255, 255}, 0.16f);

    // Верхний разделитель под шапкой
    m_bgNode->drawSegment(ccp(14.f, WIN_H - 38.f), ccp(WIN_W - 14.f, WIN_H - 38.f), 0.5f, P({255, 255, 255}, 0.08f));

    // Парящая нижняя док-панель для вкладок (Bottom Dock)
    float dockRad = std::min(12.f, menuRad * 0.6f + 2.f);
    drawLoonGlass(m_bgNode, ccp(20.f, 7.f), CCSize(WIN_W - 40.f, 32.f), dockRad, card, 0.92f, {255, 255, 255}, 0.12f);
}

void ModMenuPopup::redrawTabSlider() {
    m_tabSlider->clear();
    auto slCol = mm::sliderColor();
    float w = 76.f, h = 26.f;
    CCPoint o = ccp(-w * 0.5f, -h * 0.5f);

    drawSoftWhiteGlow(m_tabSlider, o, CCSize(w, h), 8.f, mm::state().glowIntensity * 0.45f);
    drawLoonGlass(m_tabSlider, o, CCSize(w, h), 8.f, mixCol(mm::themeCard(), slCol, 0.32f), 0.96f, mm::accent(), 0.65f);

    for (int i = 0; i < TAB_COUNT; i++) {
        bool act = (i == m_currentTab);
        if (m_tabLabels[i]) {
            m_tabLabels[i]->setColor(act ? ccColor3B{255, 255, 255} : MUTED_TXT);
        }
        // Иконки GD НЕ меняют свой цвет от темы (сохраняют оригинальные цвета GD!)
        if (m_tabIcons[i]) {
            m_tabIcons[i]->setOpacity(act ? 255 : 195);
        }
    }
}

void ModMenuPopup::redrawLButtonPreview() {
    if (!m_previewBtnRoot || !m_previewBtnBg || !m_previewBtnLbl) return;
    m_previewBtnBg->clear();

    float a = std::clamp(mm::state().btnOpacity, 0.20f, 1.0f);
    float rad = std::clamp(mm::state().btnRadius, 0.f, 17.5f);
    auto acc = mm::accent();

    drawRoundRect(m_previewBtnBg, ccp(1.f, 1.f), CCSize(36.f, 36.f), rad,
                  P({12, 12, 18}, a), 1.2f, P(acc, a));
    m_previewBtnLbl->setOpacity(static_cast<GLubyte>(255.f * a));
    m_previewBtnRoot->setScale(mm::state().btnScale);
}

void ModMenuPopup::drawSectionBox(CCNode* parent, float x, float yTop, float w, int rowCount, char const* sectionTitle) {
    float hdrH = 23.f;
    float rowH = 26.f;
    float totalH = hdrH + rowCount * rowH + 4.f;
    float yBot = yTop - totalH;
    float boxRad = std::clamp(mm::state().menuRadius * 0.45f, 2.f, 11.f);

    auto d = CCDrawNode::create();
    drawLoonGlass(d, ccp(x, yBot), CCSize(w, totalH), boxRad, mm::themeCard(), 0.90f, {255, 255, 255}, 0.10f);
    d->drawSegment(ccp(x + 8.f, yTop - hdrH), ccp(x + w - 8.f, yTop - hdrH), 0.5f, P({255, 255, 255}, 0.07f));
    parent->addChild(d, 1);

    auto lbl = CCLabelBMFont::create(sectionTitle, "chatFont.fnt");
    lbl->setScale(0.72f);
    lbl->setAnchorPoint({0.f, 0.5f});
    lbl->setPosition({x + 10.f, yTop - 11.5f});
    lbl->setColor(mm::accent());
    parent->addChild(lbl, 2);
}

void ModMenuPopup::addModuleRow(float x, float yRowTop, float w, mm::F flag, bool hasSubOptions, bool drawBottomLine) {
    float rowH = 26.f;
    float cy = yRowTop - rowH * 0.5f;

    bool enabled = mm::on(flag);
    bool risky = mm::isRisky(flag);
    auto slCol = mm::sliderColor();
    auto txtCol = mm::themeText();

    if (drawBottomLine) {
        auto line = CCDrawNode::create();
        line->drawSegment(ccp(x + 8.f, yRowTop - rowH), ccp(x + w - 8.f, yRowTop - rowH), 0.5f, P({255, 255, 255}, 0.045f));
        m_contentHost->addChild(line, 2);
    }

    auto tLbl = CCLabelBMFont::create(mm::title(flag), "chatFont.fnt");
    tLbl->setScale(0.70f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x + 10.f, cy});
    if (risky) {
        tLbl->setColor(enabled ? CHEAT_RED_ON : CHEAT_RED);
    } else {
        tLbl->setColor(enabled ? ccColor3B{255, 255, 255} : mixCol(txtCol, MUTED_TXT, 0.30f));
    }
    m_contentHost->addChild(tLbl, 3);

    auto swNode = CCNode::create();
    swNode->setContentSize({26.f, 14.f});
    auto swDraw = CCDrawNode::create();
    if (enabled) {
        drawSoftWhiteGlow(swDraw, ccp(2.f, 2.f), CCSize(22.f, 10.f), 5.f, mm::state().glowIntensity * 0.65f);
    }
    // В Amoled Dark White тумблер использует контрастный тёмно-серый цвет slCol, чтобы белая точка не сливалась!
    ccColor3B swCol = enabled ? (risky ? CHEAT_RED : slCol) : ccColor3B{40, 43, 55};
    drawRoundRect(swDraw, ccp(2.f, 2.f), CCSize(22.f, 10.f), 5.f, P(swCol, 1.f));
    float knobX = enabled ? 19.f : 7.f;
    drawPolyCircle(swDraw, ccp(knobX, 7.f), 3.6f, P(enabled ? ccColor3B{255, 255, 255} : ccColor3B{130, 136, 152}, 1.f), 12);
    swNode->addChild(swDraw);

    auto swBtn = CCMenuItemSpriteExtra::create(swNode, this, menu_selector(ModMenuPopup::onToggle));
    swBtn->setTag(static_cast<int>(flag));
    swBtn->setPosition({x + w - 18.f, cy});
    m_mainCardsMenu->addChild(swBtn);

    if (hasSubOptions) {
        auto dotsNode = CCNode::create();
        dotsNode->setContentSize({18.f, 16.f});
        auto dotsDraw = CCDrawNode::create();
        for (int i = 0; i < 3; i++) {
            drawPolyCircle(dotsDraw, ccp(4.f + i * 4.5f, 8.f), 1.15f, P({200, 206, 225}, 0.95f), 8);
        }
        dotsNode->addChild(dotsDraw);

        auto dotsBtn = CCMenuItemSpriteExtra::create(dotsNode, this, menu_selector(ModMenuPopup::onOpenSub));
        dotsBtn->setTag(static_cast<int>(flag));
        dotsBtn->setPosition({x + w - 42.f, cy});
        m_mainCardsMenu->addChild(dotsBtn);
    }
}

void ModMenuPopup::addSliderControl(CCNode* host, float x, float y, float w, char const* title, char const* suffix,
                                    float minV, float maxV, bool isInt,
                                    std::function<float()> getter, std::function<void(float)> setter) {
    auto d = CCDrawNode::create();
    host->addChild(d, 3);

    auto tLbl = CCLabelBMFont::create(title, "chatFont.fnt");
    tLbl->setScale(0.66f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x, y + 10.f});
    tLbl->setColor(mm::themeText());
    host->addChild(tLbl, 4);

    auto vLbl = CCLabelBMFont::create("", "chatFont.fnt");
    vLbl->setScale(0.66f);
    vLbl->setAnchorPoint({1.f, 0.5f});
    vLbl->setPosition({x + w, y + 10.f});
    vLbl->setColor(mm::accent());
    host->addChild(vLbl, 4);

    SliderTrack tr{
        CCRect(x, y - 6.f, w, 14.f),
        minV, maxV, getter, setter, d, vLbl, title, suffix, isInt
    };
    this->refreshSliderVisual(tr);
    m_sliders.push_back(tr);
}

void ModMenuPopup::refreshSliderVisual(SliderTrack& sl) {
    if (!sl.drawNode || !sl.valLabel) return;
    sl.drawNode->clear();

    float val = std::clamp(sl.getter(), sl.minVal, sl.maxVal);
    float t = (val - sl.minVal) / std::max(0.0001f, sl.maxVal - sl.minVal);
    float x = sl.rect.origin.x;
    float y = sl.rect.origin.y + 6.f;
    float w = sl.rect.size.width;

    // В Amoled Dark White полоска ползунка использует контрастный тёмно-серый цвет, чтобы не сливаться с белым кружком!
    auto barCol = mm::sliderColor();
    drawRoundRect(sl.drawNode, ccp(x, y - 2.5f), CCSize(w, 5.f), 2.5f, P({36, 40, 52}, 1.f));
    if (t > 0.01f) {
        drawRoundRect(sl.drawNode, ccp(x, y - 2.5f), CCSize(std::max(5.f, w * t), 5.f), 2.5f, P(barCol, 1.f));
    }
    float kx = x + w * t;
    if (mm::on(mm::F::Glow)) {
        drawPolyCircle(sl.drawNode, ccp(kx, y), 7.5f, P({255, 255, 255}, 0.24f * mm::state().glowIntensity), 12);
    }
    drawPolyCircle(sl.drawNode, ccp(kx, y), 4.8f, P({255, 255, 255}, 1.f), 14);
    drawPolyCircle(sl.drawNode, ccp(kx, y), 2.8f, P(barCol, 1.f), 12);

    std::string txt = fmt::format("{:.0f}{}", val, sl.suffix);
    sl.valLabel->setString(txt.c_str());
}

void ModMenuPopup::buildVisualPalette(CCNode* host, float x, float y) {
    m_hasPalette = true;
    float svW = 240.f, svH = 62.f;
    float hueH = 10.f;

    m_svRect = CCRect(x, y + 16.f, svW, svH);
    m_hueRect = CCRect(x, y, svW, hueH);

    int sl = mm::state().editSlot;
    auto baseHueCol = mm::hsv(mm::state().customH[sl], 1.f, 1.f);

    m_svBase = CCLayerColor::create(ccc4(baseHueCol.r, baseHueCol.g, baseHueCol.b, 255), svW, svH);
    m_svBase->setPosition(m_svRect.origin);
    host->addChild(m_svBase, 2);

    auto satGrad = CCLayerGradient::create(ccc4(255, 255, 255, 255), ccc4(255, 255, 255, 0), ccp(1.f, 0.f));
    satGrad->setContentSize({svW, svH});
    satGrad->setPosition(m_svRect.origin);
    host->addChild(satGrad, 3);

    auto valGrad = CCLayerGradient::create(ccc4(0, 0, 0, 0), ccc4(0, 0, 0, 245), ccp(0.f, -1.f));
    valGrad->setContentSize({svW, svH});
    valGrad->setPosition(m_svRect.origin);
    host->addChild(valGrad, 4);

    float segW = svW / 6.f;
    for (int i = 0; i < 6; i++) {
        auto c1 = mm::hsv(i / 6.f, 1.f, 1.f);
        auto c2 = mm::hsv((i + 1) / 6.f, 1.f, 1.f);
        auto g = CCLayerGradient::create(ccc4(c1.r, c1.g, c1.b, 255), ccc4(c2.r, c2.g, c2.b, 255), ccp(1.f, 0.f));
        g->setContentSize({segW + 0.5f, hueH});
        g->setPosition({x + i * segW, y});
        host->addChild(g, 3);
    }

    m_svCursor = CCDrawNode::create();
    host->addChild(m_svCursor, 6);
    m_hueCursor = CCDrawNode::create();
    host->addChild(m_hueCursor, 6);

    this->refreshPaletteVisual();
}

void ModMenuPopup::refreshPaletteVisual() {
    if (!m_hasPalette || !m_svBase || !m_svCursor || !m_hueCursor) return;
    int sl = mm::state().editSlot;
    float h = mm::state().customH[sl];
    float s = mm::state().customS[sl];
    float v = mm::state().customV[sl];

    auto pureHue = mm::hsv(h, 1.f, 1.f);
    auto picked = mm::hsv(h, s, v);
    m_svBase->setColor(pureHue);

    m_svCursor->clear();
    CCPoint svPt = ccp(m_svRect.origin.x + s * m_svRect.size.width,
                       m_svRect.origin.y + v * m_svRect.size.height);
    drawPolyCircle(m_svCursor, svPt, 5.5f, P({255, 255, 255}, 1.f), 12);
    drawPolyCircle(m_svCursor, svPt, 4.0f, P(picked, 1.f), 12);

    m_hueCursor->clear();
    CCPoint hPt = ccp(m_hueRect.origin.x + h * m_hueRect.size.width,
                      m_hueRect.origin.y + m_hueRect.size.height * 0.5f);
    drawPolyCircle(m_hueCursor, hPt, 6.0f, P({255, 255, 255}, 1.f), 12);
    drawPolyCircle(m_hueCursor, hPt, 4.4f, P(pureHue, 1.f), 12);
}

void ModMenuPopup::animatePress(CCNode* target) {
    if (!target) return;
    int type = mm::state().pressAnimType;
    if (type == 0) return;

    float pwr = mm::state().pressAnimPower;
    target->stopAllActions();
    target->setScale(1.f);
    if (type == 1) {
        target->runAction(CCSequence::create(
            CCEaseSineOut::create(CCScaleTo::create(0.06f, pwr)),
            CCEaseBackOut::create(CCScaleTo::create(0.15f, 1.0f)),
            nullptr
        ));
    } else {
        target->runAction(CCSequence::create(
            CCScaleTo::create(0.05f, pwr),
            CCEaseExponentialOut::create(CCScaleTo::create(0.13f, 1.0f)),
            nullptr
        ));
    }
}

void ModMenuPopup::rebuildContent(bool animateTransition) {
    m_contentHost->removeAllChildren();
    m_sliders.clear();
    m_hasPalette = false;
    m_previewBtnRoot = nullptr;
    m_previewBtnBg = nullptr;
    m_previewBtnLbl = nullptr;

    this->redrawFrame();
    this->redrawTabSlider();

    m_mainCardsMenu = CCMenu::create();
    m_mainCardsMenu->setPosition({0.f, 0.f});
    m_contentHost->addChild(m_mainCardsMenu, 10);

    auto acc = mm::accent();
    auto slCol = mm::sliderColor();
    auto txtCol = mm::themeText();

    auto safeLbl = CCLabelBMFont::create(mm::safeActive() ? "SAFE MODE: ACTIVE" : "UNSAFE MODE", "chatFont.fnt");
    safeLbl->setScale(0.62f);
    safeLbl->setAnchorPoint({1.f, 0.5f});
    safeLbl->setPosition({WIN_W - 42.f, WIN_H - 21.f});
    safeLbl->setColor(mm::safeActive() ? ccColor3B{95, 235, 150} : CHEAT_RED);
    m_contentHost->addChild(safeLbl, 3);

    // Новая просторная 2-колоночная сетка во всю ширину окна (над нижней док-панелью!)
    float col0X = 16.f;
    float col1X = 236.f;
    float boxW = 208.f;
    float topY = WIN_H - 46.f;

    if (m_currentTab == 0) {
        // PLAYER
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "Movement");
        this->addModuleRow(col0X, topY - 23.f, boxW, mm::F::Noclip, false, true);
        this->addModuleRow(col0X, topY - 49.f, boxW, mm::F::Jump, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 2, "Protection");
        this->addModuleRow(col1X, topY - 23.f, boxW, mm::F::Safe, false, true);
        this->addModuleRow(col1X, topY - 49.f, boxW, mm::F::AutoSafe, false, false);

        float spTop = topY - 92.f;
        this->drawSectionBox(m_contentHost, col0X, spTop, WIN_W - 32.f, 2, "Speedhack Multiplier");
        for (int i = 0; i < static_cast<int>(mm::SPEEDS.size()); i++) {
            bool sel = (mm::state().speedIdx == i);
            auto pNode = CCNode::create();
            pNode->setContentSize({58.f, 24.f});
            auto pDraw = CCDrawNode::create();
            drawLoonGlass(pDraw, ccp(0.f, 0.f), CCSize(58.f, 24.f), 6.f,
                          sel ? (i != 3 ? CHEAT_RED : slCol) : ccColor3B{36, 40, 54}, 0.96f,
                          {255, 255, 255}, sel ? 0.5f : 0.08f);
            pNode->addChild(pDraw);
            auto pLbl = CCLabelBMFont::create(fmt::format("x{}", mm::SPEEDS[i]).c_str(), "chatFont.fnt");
            pLbl->setScale(0.68f);
            pLbl->setPosition({29.f, 12.f});
            pLbl->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            pNode->addChild(pLbl);

            auto btn = CCMenuItemSpriteExtra::create(pNode, this, menu_selector(ModMenuPopup::onSpeedPill));
            btn->setTag(i);
            btn->setPosition({col0X + 42.f + i * 68.f, spTop - 48.f});
            m_mainCardsMenu->addChild(btn);
        }
    }
    else if (m_currentTab == 1) {
        // LEVEL (без StartPos, без Bismuth, без FrameSmooth, а TPS настраивается ТОЛЬКО вводом с клавиатуры!)
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 3, "Level & Bypass");
        this->addModuleRow(col0X, topY - 23.f, boxW, mm::F::TpsBypass, false, true);
        this->addModuleRow(col0X, topY - 49.f, boxW, mm::F::Hitboxes, false, true);
        this->addModuleRow(col0X, topY - 75.f, boxW, mm::F::NoDeath, false, false);

        // Справа блок ручного ввода TPS с клавиатуры (TextInput)
        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 3, "Type Custom TPS");
        auto infoLbl = CCLabelBMFont::create("Enter Physics Ticks (60 - 2400):", "chatFont.fnt");
        infoLbl->setScale(0.62f);
        infoLbl->setColor(MUTED_TXT);
        infoLbl->setPosition({col1X + boxW * 0.5f, topY - 38.f});
        m_contentHost->addChild(infoLbl, 4);

        auto input = geode::TextInput::create(140.f, "e.g. 240", "chatFont.fnt");
        input->setCommonFilter(geode::CommonFilter::Uint);
        input->setMaxCharCount(4);
        input->setString(fmt::format("{}", mm::state().customTps));
        input->setPosition({col1X + boxW * 0.5f, topY - 70.f});
        input->setCallback([](std::string const& str) {
            if (str.empty()) return;
            if (auto res = geode::utils::numFromString<int>(str)) {
                mm::state().customTps = std::clamp(res.unwrap(), 60, 2400);
                mm::saveAll();
            }
        });
        m_contentHost->addChild(input, 15);
    }
    else if (m_currentTab == 2) {
        // VISUALS
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 4, "Interface");
        this->addModuleRow(col0X, topY - 23.f, boxW, mm::F::Overlay, true, true);
        this->addModuleRow(col0X, topY - 49.f, boxW, mm::F::ShowFPS, false, true);
        this->addModuleRow(col0X, topY - 75.f, boxW, mm::F::HideAttempts, false, true);
        this->addModuleRow(col0X, topY - 101.f, boxW, mm::F::UnlockAll, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 5, "World & FX");
        this->addModuleRow(col1X, topY - 23.f, boxW, mm::F::NoShake, false, true);
        this->addModuleRow(col1X, topY - 49.f, boxW, mm::F::NoMirror, false, true);
        this->addModuleRow(col1X, topY - 75.f, boxW, mm::F::HidePlayer, false, true);
        this->addModuleRow(col1X, topY - 101.f, boxW, mm::F::RainbowIcon, false, true);
        this->addModuleRow(col1X, topY - 127.f, boxW, mm::F::RainbowBG, false, false);
    }
    else if (m_currentTab == 3) {
        // THEMES (Loon Effects + Amoled Dark White + Скругление меню)
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "Loon Effects");
        this->addModuleRow(col0X, topY - 23.f, boxW, mm::F::LiquidGlass, true, true);
        this->addModuleRow(col0X, topY - 49.f, boxW, mm::F::BgBlur, true, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 2, "Bloom & Rounding");
        this->addModuleRow(col1X, topY - 23.f, boxW, mm::F::Glow, true, true);
        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 66.f, boxW - 20.f,
                               "Menu Corners", " px", 0.f, 28.f, true,
                               [] { return mm::state().menuRadius; },
                               [this](float v) {
                                   mm::state().menuRadius = v;
                                   this->redrawFrame();
                               });

        for (int i = 0; i < 8; i++) {
            int col = i % 2;
            int row = i / 2;
            float cardW = boxW, cardH = 24.f;
            float x = (col == 0 ? col0X : col1X);
            float y = topY - 110.f - row * (cardH + 4.f);

            bool sel = (mm::state().themeIdx == i);
            auto node = CCNode::create();
            node->setContentSize({cardW, cardH});
            auto d = CCDrawNode::create();
            drawLoonGlass(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 6.f,
                          mm::THEMES[i].card, 0.94f,
                          sel ? acc : ccColor3B{255, 255, 255}, sel ? 0.90f : 0.10f);

            ccColor3B swCol = (i == 7) ? mm::accent() : mm::THEMES[i].accent;
            drawRoundRect(d, ccp(cardW - 30.f, 3.f), CCSize(22.f, 18.f), 4.f, P(swCol, 1.f));
            node->addChild(d);

            auto lbl = CCLabelBMFont::create(mm::THEMES[i].name, "chatFont.fnt");
            lbl->setScale(0.65f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({10.f, cardH * 0.5f});
            lbl->setColor(sel ? acc : txtCol);
            node->addChild(lbl);

            auto btn = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onThemeSelect));
            btn->setTag(i);
            btn->setPosition({x + cardW * 0.5f, y + cardH * 0.5f});
            m_mainCardsMenu->addChild(btn);
        }
    }
    else if (m_currentTab == 4) {
        // CLIENT
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "L-Button Visibility");
        this->addModuleRow(col0X, topY - 23.f, boxW, mm::F::BtnHideInGame, false, true);
        this->addModuleRow(col0X, topY - 49.f, boxW, mm::F::BtnShowInPause, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 4, "L-Button Sliders");
        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 42.f, boxW - 20.f,
                               "Button Size", "%", 55.f, 150.f, true,
                               [] { return mm::state().btnScale * 100.f; },
                               [this](float v) {
                                   mm::state().btnScale = v / 100.f;
                                   this->redrawLButtonPreview();
                               });

        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 72.f, boxW - 20.f,
                               "Opacity", "%", 20.f, 100.f, true,
                               [] { return mm::state().btnOpacity * 100.f; },
                               [this](float v) {
                                   mm::state().btnOpacity = v / 100.f;
                                   this->redrawLButtonPreview();
                               });

        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 102.f, boxW - 20.f,
                               "Corner Radius", " px", 0.f, 17.f, true,
                               [] { return mm::state().btnRadius; },
                               [this](float v) {
                                   mm::state().btnRadius = v;
                                   this->redrawLButtonPreview();
                               });

        float botTop = topY - 94.f;
        this->drawSectionBox(m_contentHost, col0X, botTop, boxW, 2, "Menu Particles");
        char const* pNames[4] = {"Off", "Win", "Aut", "Sak"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().particleType == i);
            auto n = CCNode::create();
            n->setContentSize({42.f, 20.f});
            auto d = CCDrawNode::create();
            drawLoonGlass(d, ccp(0.f, 0.f), CCSize(42.f, 20.f), 5.f, sel ? slCol : ccColor3B{38, 42, 56}, 0.96f);
            n->addChild(d);
            auto l = CCLabelBMFont::create(pNames[i], "chatFont.fnt");
            l->setScale(0.60f);
            l->setPosition({21.f, 10.f});
            l->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onParticleSelect));
            b->setTag(i);
            b->setPosition({col0X + 30.f + i * 48.f, botTop - 46.f});
            m_mainCardsMenu->addChild(b);
        }

        this->drawSectionBox(m_contentHost, col1X, topY - 134.f, boxW, 2, "Live Button Preview");
        m_previewBtnRoot = CCNode::create();
        m_previewBtnRoot->ignoreAnchorPointForPosition(false);
        m_previewBtnRoot->setAnchorPoint({0.5f, 0.5f});
        m_previewBtnRoot->setContentSize({38.f, 38.f});
        m_previewBtnRoot->setPosition({col1X + boxW * 0.5f, topY - 184.f});

        m_previewBtnBg = CCDrawNode::create();
        m_previewBtnRoot->addChild(m_previewBtnBg, 1);

        m_previewBtnLbl = CCLabelBMFont::create("L", "bigFont.fnt");
        m_previewBtnLbl->setScale(0.50f);
        m_previewBtnLbl->setPosition({19.f, 20.f});
        m_previewBtnRoot->addChild(m_previewBtnLbl, 2);

        m_contentHost->addChild(m_previewBtnRoot, 8);
        this->redrawLButtonPreview();
    }

    if (animateTransition) {
        m_contentHost->stopAllActions();
        m_contentHost->setPosition({0.f, 5.f});
        m_contentHost->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.18f, {0.f, 0.f})));
    }

    this->rebuildSubPopup(false);
}

// ----- Всплывающее окно "..." -----
void ModMenuPopup::rebuildSubPopup(bool animateOpen) {
    m_subPopupHost->removeAllChildren();
    if (m_mainCardsMenu) {
        m_mainCardsMenu->setEnabled(!m_hasSubSettings);
    }
    if (!m_hasSubSettings || m_activeSubFeature == mm::F::COUNT) return;

    m_sliders.clear();
    m_hasPalette = false;

    auto acc = mm::accent();
    auto slCol = mm::sliderColor();
    auto txtCol = mm::themeText();

    auto d = CCDrawNode::create();
    drawLoonGlass(d, ccp(14.f, 44.f), CCSize(WIN_W - 28.f, WIN_H - 86.f), 12.f,
                  mm::themeBg(), 0.97f, acc, 0.65f);
    m_subPopupHost->addChild(d, 1);

    auto subMenu = CCMenu::create();
    subMenu->setPosition({0.f, 0.f});

    auto backNode = CCNode::create();
    backNode->setContentSize({56.f, 22.f});
    auto backDraw = CCDrawNode::create();
    drawLoonGlass(backDraw, ccp(0.f, 0.f), CCSize(56.f, 22.f), 6.f, mm::themeCard(), 0.96f, acc, 0.7f);
    backNode->addChild(backDraw);
    auto backLbl = CCLabelBMFont::create("< Back", "chatFont.fnt");
    backLbl->setScale(0.68f);
    backLbl->setPosition({28.f, 11.f});
    backNode->addChild(backLbl);
    auto backBtn = CCMenuItemSpriteExtra::create(backNode, this, menu_selector(ModMenuPopup::onCloseSub));
    backBtn->setPosition({52.f, WIN_H - 58.f});
    subMenu->addChild(backBtn);

    if (m_activeSubFeature == mm::F::Overlay) {
        mm::F ovFlags[6] = {mm::F::OvPercent, mm::F::OvCPS, mm::F::OvTps, mm::F::OvAttempts, mm::F::OvStatus, mm::F::OvBg};
        char const* ovNames[6] = {"Percent %", "CPS", "TPS", "Attempts", "Safe Status", "Glass Pill"};

        for (int i = 0; i < 6; i++) {
            int c = i % 3, r = i / 3;
            float bx = 34.f + c * 134.f;
            float by = 182.f - r * 30.f;
            bool onFlag = mm::on(ovFlags[i]);

            auto n = CCNode::create();
            n->setContentSize({122.f, 24.f});
            auto dr = CCDrawNode::create();
            drawLoonGlass(dr, ccp(0.f, 0.f), CCSize(122.f, 24.f), 6.f,
                          onFlag ? mixCol(mm::themeCard(), slCol, 0.28f) : mm::themeCard(), 0.96f,
                          onFlag ? acc : ccColor3B{255, 255, 255}, onFlag ? 0.8f : 0.1f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(ovNames[i], "chatFont.fnt");
            lb->setScale(0.64f);
            lb->setPosition({61.f, 12.f});
            lb->setColor(onFlag ? acc : txtCol);
            n->addChild(lb);

            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onToggle));
            b->setTag(static_cast<int>(ovFlags[i]));
            b->setPosition({bx + 61.f, by + 12.f});
            subMenu->addChild(b);
        }

        this->addSliderControl(m_subPopupHost, 34.f, 114.f, 182.f,
                               "Overlay Scale", "%", 50.f, 130.f, true,
                               [] { return mm::state().ovScale * 100.f; },
                               [](float v) { mm::state().ovScale = v / 100.f; });

        this->addSliderControl(m_subPopupHost, 244.f, 114.f, 182.f,
                               "Overlay Alpha", "%", 20.f, 100.f, true,
                               [] { return mm::state().ovOpacity * 100.f; },
                               [](float v) { mm::state().ovOpacity = v / 100.f; });

        char const* corners[4] = {"Top-L", "Top-R", "Bot-L", "Bot-R"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().ovCorner == i);
            auto n = CCNode::create();
            n->setContentSize({86.f, 22.f});
            auto dr = CCDrawNode::create();
            drawLoonGlass(dr, ccp(0.f, 0.f), CCSize(86.f, 22.f), 6.f, sel ? slCol : mm::themeCard(), 0.96f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(corners[i], "chatFont.fnt");
            lb->setScale(0.64f);
            lb->setPosition({43.f, 11.f});
            lb->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onOvCornerSelect));
            b->setTag(i);
            b->setPosition({78.f + i * 100.f, 66.f});
            subMenu->addChild(b);
        }
    }
    else if (m_activeSubFeature == mm::F::Glow || m_activeSubFeature == mm::F::LiquidGlass || m_activeSubFeature == mm::F::BgBlur) {
        this->addSliderControl(m_subPopupHost, 32.f, 200.f, 185.f,
                               "Screen Blur", "%", 0.f, 100.f, true,
                               [] { return mm::state().bgBlurStrength * 100.f; },
                               [this](float v) {
                                   mm::state().bgBlurStrength = v / 100.f;
                                   this->updateScreenBlurOpacity();
                               });

        this->addSliderControl(m_subPopupHost, 242.f, 200.f, 185.f,
                               "Glass Opacity", "%", 40.f, 96.f, true,
                               [] { return mm::state().glassAlpha * 100.f; },
                               [this](float v) {
                                   mm::state().glassAlpha = v / 100.f;
                                   this->redrawFrame();
                               });

        this->addSliderControl(m_subPopupHost, 32.f, 166.f, 185.f,
                               "White Glow", "%", 0.f, 100.f, true,
                               [] { return mm::state().glowIntensity * 100.f; },
                               [this](float v) {
                                   mm::state().glowIntensity = v / 100.f;
                                   this->redrawFrame();
                                   this->redrawTabSlider();
                               });

        this->addSliderControl(m_subPopupHost, 242.f, 166.f, 185.f,
                               "Menu Corners", " px", 0.f, 28.f, true,
                               [] { return mm::state().menuRadius; },
                               [this](float v) {
                                   mm::state().menuRadius = v;
                                   this->redrawFrame();
                               });

        char const* slots[3] = {"Accent Color", "Background", "Containers"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().editSlot == i);
            auto n = CCNode::create();
            n->setContentSize({116.f, 22.f});
            auto dr = CCDrawNode::create();
            drawLoonGlass(dr, ccp(0.f, 0.f), CCSize(116.f, 22.f), 6.f, sel ? slCol : mm::themeCard(), 0.96f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(slots[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({58.f, 11.f});
            lb->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onColorSlotSelect));
            b->setTag(i);
            b->setPosition({358.f, 122.f - i * 28.f});
            subMenu->addChild(b);
        }

        this->buildVisualPalette(m_subPopupHost, 32.f, 56.f);
    }

    m_subPopupHost->addChild(subMenu, 10);

    if (animateOpen) {
        m_subPopupHost->stopAllActions();
        m_subPopupHost->setScale(0.94f);
        m_subPopupHost->runAction(CCEaseBackOut::create(CCScaleTo::create(0.16f, 1.0f)));
    }
}

// ============================ Обработка касаний ============================

bool ModMenuPopup::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    CCPoint lp = m_mainLayer->convertToNodeSpace(touch->getLocation());

    if (m_hasPalette) {
        if (m_svRect.containsPoint(lp)) {
            m_touchMode = TouchMode::PaletteSV;
            this->ccTouchMoved(touch, event);
            return true;
        }
        if (CCRect(m_hueRect.origin.x - 4.f, m_hueRect.origin.y - 4.f,
                   m_hueRect.size.width + 8.f, m_hueRect.size.height + 8.f).containsPoint(lp)) {
            m_touchMode = TouchMode::PaletteHue;
            this->ccTouchMoved(touch, event);
            return true;
        }
    }

    for (int i = 0; i < static_cast<int>(m_sliders.size()); i++) {
        CCRect hit = m_sliders[i].rect;
        hit.origin.y -= 6.f;
        hit.size.height += 12.f;
        if (hit.containsPoint(lp)) {
            m_touchMode = TouchMode::Slider;
            m_activeSliderIdx = i;
            this->ccTouchMoved(touch, event);
            return true;
        }
    }

    m_touchMode = TouchMode::None;
    return Popup::ccTouchBegan(touch, event);
}

void ModMenuPopup::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (m_touchMode == TouchMode::None) {
        Popup::ccTouchMoved(touch, event);
        return;
    }

    CCPoint lp = m_mainLayer->convertToNodeSpace(touch->getLocation());

    if (m_touchMode == TouchMode::Slider && m_activeSliderIdx >= 0 && m_activeSliderIdx < static_cast<int>(m_sliders.size())) {
        auto& sl = m_sliders[m_activeSliderIdx];
        float t = std::clamp((lp.x - sl.rect.origin.x) / std::max(1.f, sl.rect.size.width), 0.f, 1.f);
        float newVal = sl.minVal + t * (sl.maxVal - sl.minVal);
        if (sl.isInt) newVal = std::round(newVal);
        sl.setter(newVal);
        this->refreshSliderVisual(sl);
    }
    else if (m_touchMode == TouchMode::PaletteSV && m_hasPalette) {
        int slot = mm::state().editSlot;
        mm::state().themeIdx = 7;
        mm::state().customS[slot] = std::clamp((lp.x - m_svRect.origin.x) / m_svRect.size.width, 0.f, 1.f);
        mm::state().customV[slot] = std::clamp((lp.y - m_svRect.origin.y) / m_svRect.size.height, 0.04f, 1.f);
        this->refreshPaletteVisual();
        this->redrawFrame();
        this->redrawTabSlider();
    }
    else if (m_touchMode == TouchMode::PaletteHue && m_hasPalette) {
        int slot = mm::state().editSlot;
        mm::state().themeIdx = 7;
        mm::state().customH[slot] = std::clamp((lp.x - m_hueRect.origin.x) / m_hueRect.size.width, 0.f, 0.999f);
        this->refreshPaletteVisual();
        this->redrawFrame();
        this->redrawTabSlider();
    }
}

void ModMenuPopup::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    if (m_touchMode != TouchMode::None) {
        mm::saveAll();
        m_touchMode = TouchMode::None;
        m_activeSliderIdx = -1;
        return;
    }
    Popup::ccTouchEnded(touch, event);
}

void ModMenuPopup::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    if (m_touchMode != TouchMode::None) {
        mm::saveAll();
        m_touchMode = TouchMode::None;
        m_activeSliderIdx = -1;
        return;
    }
    Popup::ccTouchCancelled(touch, event);
}

// ============================ Колбэки ============================

void ModMenuPopup::onTab(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    int newTab = node->getTag();
    if (newTab == m_currentTab && !m_hasSubSettings) return;

    m_currentTab = newTab;
    m_hasSubSettings = false;

    m_tabSlider->stopAllActions();
    m_tabSlider->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.20f, {tabX(m_currentTab), DOCK_Y})));

    this->rebuildContent(true);
}

void ModMenuPopup::onToggle(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    auto flag = static_cast<mm::F>(node->getTag());
    mm::set(flag, !mm::on(flag));
    mm::save(flag);

    if (flag == mm::F::Hitboxes) mm::applyHitboxes();
    if (flag == mm::F::NoShake) mm::applyNoShake();
    if (flag == mm::F::BgBlur) this->updateScreenBlurOpacity();

    if (m_hasSubSettings) {
        this->rebuildSubPopup(false);
    } else {
        this->rebuildContent(false);
    }
}

void ModMenuPopup::onOpenSub(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    m_activeSubFeature = static_cast<mm::F>(node->getTag());
    m_hasSubSettings = true;
    this->rebuildSubPopup(true);
}

void ModMenuPopup::onCloseSub(CCObject*) {
    m_hasSubSettings = false;
    this->rebuildContent(false);
}

void ModMenuPopup::onModInfo(CCObject* sender) {
    this->animatePress(static_cast<CCNode*>(sender));
    FLAlertLayer::create(
        "About Loon Client",
        "<cy>Loon Client</c> — Modern Frosted Glass Menu for GD 2.2081.\n"
        "<cg>Bottom Dock UI:</c> Fast access to all categories at the bottom.\n"
        "<cg>Screen Blur:</c> Multi-pass frosted blur behind the menu.\n"
        "<cg>TPS Input:</c> Type any custom physics TPS directly in the Level tab!",
        "Got It"
    )->show();
}

void ModMenuPopup::onClosePopup(CCObject*) {
    mm::state().menuOpen = false;
    mm::saveAll();
    this->onClose(nullptr);
}

void ModMenuPopup::onSpeedPill(CCObject* sender) {
    mm::state().speedIdx = static_cast<CCNode*>(sender)->getTag();
    mm::applySpeed(PlayLayer::get() != nullptr);
    this->rebuildContent(false);
}

void ModMenuPopup::onParticleSelect(CCObject* sender) {
    mm::state().particleType = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
    this->rebuildContent(false);
}

void ModMenuPopup::onThemeSelect(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    mm::state().themeIdx = idx;
    mm::saveAll();
    if (idx == 7) {
        m_activeSubFeature = mm::F::LiquidGlass;
        m_hasSubSettings = true;
        this->rebuildContent(false);
        this->rebuildSubPopup(true);
        return;
    }
    this->rebuildContent(false);
}

void ModMenuPopup::onOvCornerSelect(CCObject* sender) {
    mm::state().ovCorner = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onColorSlotSelect(CCObject* sender) {
    mm::state().editSlot = static_cast<CCNode*>(sender)->getTag();
    mm::state().themeIdx = 7;
    this->rebuildSubPopup(false);
}
