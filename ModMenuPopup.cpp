#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>
#include <cmath>

namespace {

constexpr float PI = 3.14159265f;
constexpr float WIN_W = 468.f;
constexpr float WIN_H = 288.f;
constexpr float SIDE_W = 110.f;

constexpr char const* TAB_TITLES[5] = {"Player", "Level", "Visuals", "Themes", "Client"};
const ccColor3B CHEAT_RED = {255, 82, 92};
const ccColor3B CHEAT_RED_ON = {255, 115, 125};

ccColor4F P(ccColor3B c, float a) {
    return {c.r / 255.f * a, c.g / 255.f * a, c.b / 255.f * a, a};
}

ccColor3B mixCol(ccColor3B a, ccColor3B b, float t) {
    return {
        static_cast<GLubyte>(a.r + (b.r - a.r) * t),
        static_cast<GLubyte>(a.g + (b.g - a.g) * t),
        static_cast<GLubyte>(a.b + (b.b - a.b) * t)
    };
}

bool nearPt(CCPoint a, CCPoint b) {
    return std::abs(a.x - b.x) < 0.02f && std::abs(a.y - b.y) < 0.02f;
}

void drawRoundRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill,
                   float bw = 0.f, ccColor4F bc = {0.f, 0.f, 0.f, 0.f}) {
    if (s.width < 1.f || s.height < 1.f) return;
    r = std::clamp(r, 0.f, std::min(s.width, s.height) * 0.5f);

    std::vector<CCPoint> pts;
    pts.reserve(32);
    auto push = [&](float x, float y) {
        CCPoint p = ccp(x, y);
        if (!pts.empty() && nearPt(pts.back(), p)) return;
        pts.push_back(p);
    };

    if (r < 0.8f) {
        push(o.x + s.width, o.y + s.height);
        push(o.x, o.y + s.height);
        push(o.x, o.y);
        push(o.x + s.width, o.y);
    } else {
        constexpr int SEG = 6;
        auto arc = [&](float cx, float cy, float a0) {
            for (int i = 0; i <= SEG; i++) {
                float a = a0 + (PI * 0.5f) * i / SEG;
                push(cx + r * std::cos(a), cy + r * std::sin(a));
            }
        };
        arc(o.x + s.width - r, o.y + s.height - r, 0.f);
        arc(o.x + r, o.y + s.height - r, PI * 0.5f);
        arc(o.x + r, o.y + r, PI);
        arc(o.x + s.width - r, o.y + r, 1.5f * PI);
    }
    while (pts.size() > 3 && nearPt(pts.front(), pts.back())) pts.pop_back();
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

// Красивое 6-слойное мягкое неоновое белое свечение (Soft White Bloom)
void drawSoftWhiteGlow(CCDrawNode* n, CCPoint o, CCSize s, float r, float intensity, ccColor3B tint = {255, 255, 255}) {
    if (!mm::on(mm::F::Glow) || intensity <= 0.01f) return;
    constexpr float EXPANDS[6] = {1.2f, 2.6f, 4.4f, 6.8f, 9.8f, 13.5f};
    constexpr float ALPHAS[6]  = {0.11f, 0.075f, 0.05f, 0.032f, 0.018f, 0.009f};
    for (int i = 0; i < 6; i++) {
        float ex = EXPANDS[i];
        float a = ALPHAS[i] * intensity;
        drawRoundRect(n, ccp(o.x - ex, o.y - ex),
                      CCSize(s.width + ex * 2.f, s.height + ex * 2.f),
                      r + ex, P(tint, a));
    }
}

// Отрисовка карточки в стиле Liquid Glass (с верхним бликом и стеклянной кромкой)
void drawGlassBox(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor3B baseCol, float alpha,
                  ccColor3B borderCol, float borderAlpha) {
    bool glass = mm::on(mm::F::LiquidGlass);
    float useAlpha = glass ? alpha * mm::state().glassAlpha : 0.96f;

    // 1. Основное тело стекла
    drawRoundRect(n, o, s, r, P(baseCol, useAlpha), 0.9f, P(borderCol, borderAlpha));

    if (glass) {
        // 2. Внутренний мягкий стеклянный градиент-блик в верхней половине
        float hTop = s.height * 0.48f;
        drawRoundRect(n, ccp(o.x + 1.5f, o.y + s.height - hTop - 1.2f),
                      CCSize(s.width - 3.f, hTop),
                      std::max(r - 1.5f, 2.f), P({255, 255, 255}, 0.055f));
        // 3. Зеркальная верхняя кромка света (Liquid Specular Rim)
        n->drawSegment(ccp(o.x + r * 0.8f, o.y + s.height - 1.f),
                       ccp(o.x + s.width - r * 0.8f, o.y + s.height - 1.f),
                       0.65f, P({255, 255, 255}, 0.28f));
    }
}

float tabY(int idx) {
    return WIN_H - 72.f - idx * 36.f;
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

bool ModMenuPopup::init() {
    if (!Popup::init(WIN_W, WIN_H)) return false;

    m_bgSprite->setVisible(false);
    if (m_title) m_title->setVisible(false);
    if (m_closeBtn) {
        m_closeBtn->setVisible(false);
        m_closeBtn->setEnabled(false);
    }

    m_buttonMenu->setContentSize(m_size);
    m_buttonMenu->setAnchorPoint({0.f, 0.f});
    m_buttonMenu->setPosition({0.f, 0.f});

    // Плавная анимация появления самого окна Liquid Glass
    m_mainLayer->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->ignoreAnchorPointForPosition(false);
    m_mainLayer->setPosition(CCDirector::get()->getWinSize() * 0.5f);
    m_mainLayer->setScale(0.86f);
    m_mainLayer->runAction(CCEaseBackOut::create(CCScaleTo::create(0.24f, 1.0f)));

    m_glowNode = CCDrawNode::create();
    m_mainLayer->addChild(m_glowNode, -15);

    m_bgNode = CCDrawNode::create();
    m_mainLayer->addChild(m_bgNode, -10);

    m_particleNode = CCDrawNode::create();
    m_mainLayer->addChild(m_particleNode, -5);

    // Плавно скользящий индикатор выбранной вкладки в сайдбаре
    m_tabSlider = CCDrawNode::create();
    m_tabSlider->setPosition({SIDE_W * 0.5f, tabY(0)});
    m_mainLayer->addChild(m_tabSlider, 4);

    // Статичные кнопки вкладок сайдбара (создаются один раз, чтобы индикатор скользил плавно!)
    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.f, 0.f});
    for (int i = 0; i < TAB_COUNT; i++) {
        auto bNode = CCNode::create();
        bNode->setContentSize({SIDE_W - 16.f, 28.f});

        auto bLbl = CCLabelBMFont::create(TAB_TITLES[i], "chatFont.fnt");
        bLbl->setScale(0.82f);
        bLbl->setAnchorPoint({0.f, 0.5f});
        bLbl->setPosition({12.f, 14.f});
        bNode->addChild(bLbl);
        m_tabLabels[i] = bLbl;

        auto btn = CCMenuItemSpriteExtra::create(bNode, this, menu_selector(ModMenuPopup::onTab));
        btn->setTag(i);
        btn->setPosition({SIDE_W * 0.5f, tabY(i)});
        sideMenu->addChild(btn);
    }
    m_mainLayer->addChild(sideMenu, 12);

    m_contentHost = CCNode::create();
    m_contentHost->setContentSize(m_size);
    m_mainLayer->addChild(m_contentHost, 6);

    m_subPopupHost = CCNode::create();
    m_subPopupHost->setContentSize(m_size);
    m_mainLayer->addChild(m_subPopupHost, 50);

    // ----- Верхняя левая шапка: иконка L + Название + аккуратная кнопка "?" на расстоянии -----
    auto logoBadge = CCDrawNode::create();
    drawGlassBox(logoBadge, ccp(10.f, WIN_H - 36.f), CCSize(22.f, 22.f), 6.f, {10, 10, 14}, 0.95f, mm::accent(), 0.85f);
    m_mainLayer->addChild(logoBadge, 3);

    auto logoL = CCLabelBMFont::create("L", "bigFont.fnt");
    logoL->setScale(0.36f);
    logoL->setPosition({21.f, WIN_H - 25.f});
    m_mainLayer->addChild(logoL, 4);

    auto brandLbl = CCLabelBMFont::create("L-CLIENT", "chatFont.fnt");
    brandLbl->setScale(0.74f);
    brandLbl->setAnchorPoint({0.f, 0.5f});
    brandLbl->setPosition({36.f, WIN_H - 25.f});
    m_mainLayer->addChild(brandLbl, 4);

    // Кнопка "?" с отступом справа от названия (больше не налезает на текст!)
    {
        auto qNode = CCNode::create();
        qNode->setContentSize({18.f, 18.f});
        auto qDraw = CCDrawNode::create();
        drawGlassBox(qDraw, ccp(1.f, 1.f), CCSize(16.f, 16.f), 8.f, mm::themeCard(), 0.9f, {255, 255, 255}, 0.25f);
        qNode->addChild(qDraw);
        auto qLbl = CCLabelBMFont::create("?", "chatFont.fnt");
        qLbl->setScale(0.70f);
        qLbl->setColor(mm::accent());
        qLbl->setPosition({9.f, 9.f});
        qNode->addChild(qLbl);

        auto qBtn = CCMenuItemSpriteExtra::create(qNode, this, menu_selector(ModMenuPopup::onModInfo));
        qBtn->setPosition({96.f, WIN_H - 25.f});
        m_buttonMenu->addChild(qBtn, 15);
    }

    // Кнопка закрытия "X" справа вверху
    {
        auto cNode = CCNode::create();
        cNode->setContentSize({24.f, 24.f});
        auto cDraw = CCDrawNode::create();
        drawGlassBox(cDraw, ccp(2.f, 2.f), CCSize(20.f, 20.f), 7.f, mm::themeCard(), 0.9f, {255, 255, 255}, 0.2f);
        cDraw->drawSegment(ccp(8.5f, 8.5f), ccp(15.5f, 15.5f), 0.95f, P({235, 235, 245}, 1.f));
        cDraw->drawSegment(ccp(8.5f, 15.5f), ccp(15.5f, 8.5f), 0.95f, P({235, 235, 245}, 1.f));
        cNode->addChild(cDraw);

        auto cBtn = CCMenuItemSpriteExtra::create(cNode, this, menu_selector(ModMenuPopup::onClosePopup));
        cBtn->setPosition({WIN_W - 19.f, WIN_H - 21.f});
        m_buttonMenu->addChild(cBtn, 15);
    }

    this->initParticles();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildContent(false);
    this->scheduleUpdate();
    return true;
}

void ModMenuPopup::initParticles() {
    m_particles.clear();
    m_particles.resize(26);
    for (int i = 0; i < 26; i++) {
        m_particles[i] = {
            ccp(std::rand() % static_cast<int>(WIN_W), std::rand() % static_cast<int>(WIN_H)),
            ccp(((std::rand() % 100) / 100.f - 0.5f) * 18.f, -18.f - (std::rand() % 22)),
            2.2f + (std::rand() % 30) / 10.f,
            static_cast<float>(std::rand() % 360),
            ((std::rand() % 100) / 100.f - 0.5f) * 3.f,
            0.38f + (std::rand() % 42) / 100.f
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

        if (p.pos.y < 10.f) {
            p.pos.y = WIN_H - 10.f;
            p.pos.x = 10.f + (std::rand() % static_cast<int>(WIN_W - 20.f));
        }
        if (p.pos.x < 10.f) p.pos.x = WIN_W - 12.f;
        if (p.pos.x > WIN_W - 10.f) p.pos.x = 12.f;

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
    auto acc = mm::accent();
    float glow = mm::state().glowIntensity;

    // 1. Красивое мягкое 6-слойное белое свечение вокруг главного окна
    drawSoftWhiteGlow(m_glowNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), 16.f, glow);

    // 2. Главное окно Liquid Glass
    drawGlassBox(m_bgNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), 16.f,
                 mixCol(bg, acc, 0.05f), 0.94f, {255, 255, 255}, 0.22f);

    // 3. Стеклянная левая панель (Sidebar)
    drawGlassBox(m_bgNode, ccp(5.f, 5.f), CCSize(SIDE_W - 5.f, WIN_H - 10.f), 13.f,
                 card, 0.78f, {255, 255, 255}, 0.12f);

    // 4. Верхняя стеклянная капсула навигации
    drawGlassBox(m_bgNode, ccp(SIDE_W + 8.f, WIN_H - 33.f), CCSize(WIN_W - SIDE_W - 42.f, 24.f), 8.f,
                 card, 0.84f, {255, 255, 255}, 0.14f);
}

void ModMenuPopup::redrawTabSlider() {
    m_tabSlider->clear();
    auto acc = mm::accent();
    float w = SIDE_W - 16.f, h = 28.f;
    CCPoint o = ccp(-w * 0.5f, -h * 0.5f);

    drawSoftWhiteGlow(m_tabSlider, o, CCSize(w, h), 8.f, mm::state().glowIntensity * 0.7f);
    drawGlassBox(m_tabSlider, o, CCSize(w, h), 8.f, mixCol(mm::themeCard(), acc, 0.28f), 0.95f, acc, 0.9f);

    for (int i = 0; i < TAB_COUNT; i++) {
        if (m_tabLabels[i]) {
            m_tabLabels[i]->setColor(i == m_currentTab ? ccColor3B{255, 255, 255} : ccColor3B{160, 166, 182});
        }
    }
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
            CCEaseBackOut::create(CCScaleTo::create(0.16f, 1.0f)),
            nullptr
        ));
    } else {
        target->runAction(CCSequence::create(
            CCScaleTo::create(0.05f, pwr),
            CCEaseExponentialOut::create(CCScaleTo::create(0.14f, 1.0f)),
            nullptr
        ));
    }
}

void ModMenuPopup::addModuleCard(int col, int row, mm::F flag, bool hasSubOptions) {
    float cardW = 168.f;
    float cardH = 39.f;
    float x = SIDE_W + 10.f + col * (cardW + 8.f);
    float y = WIN_H - 80.f - row * (cardH + 6.f);

    bool enabled = mm::on(flag);
    bool risky = mm::isRisky(flag);
    auto cardCol = mm::themeCard();
    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    auto d = CCDrawNode::create();
    if (enabled) {
        drawSoftWhiteGlow(d, ccp(x, y), CCSize(cardW, cardH), 9.f,
                          mm::state().glowIntensity * 0.55f,
                          risky ? CHEAT_RED : ccColor3B{255, 255, 255});
    }
    ccColor3B borderC = enabled ? (risky ? CHEAT_RED : acc) : ccColor3B{255, 255, 255};
    float borderA = enabled ? 0.78f : 0.12f;
    drawGlassBox(d, ccp(x, y), CCSize(cardW, cardH), 9.f,
                 enabled ? mixCol(cardCol, borderC, 0.10f) : cardCol,
                 0.88f, borderC, borderA);
    m_contentHost->addChild(d, 1);

    // Заголовок функции: если читерная (isRisky) — КРАСНЫЙ цвет текста!
    auto tLbl = CCLabelBMFont::create(mm::title(flag), "chatFont.fnt");
    tLbl->setScale(0.76f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x + 9.f, y + cardH - 12.f});
    if (risky) {
        tLbl->setColor(enabled ? CHEAT_RED_ON : CHEAT_RED);
    } else {
        tLbl->setColor(enabled ? acc : txtCol);
    }
    m_contentHost->addChild(tLbl, 2);

    // Описание функции под названием (теперь на 100% отображается)
    auto dLbl = CCLabelBMFont::create(mm::desc(flag), "chatFont.fnt");
    float maxDescW = cardW - (hasSubOptions ? 66.f : 44.f);
    float sc = 0.54f;
    if (dLbl->getContentSize().width * sc > maxDescW) {
        sc = maxDescW / std::max(1.f, dLbl->getContentSize().width);
    }
    dLbl->setScale(sc);
    dLbl->setAnchorPoint({0.f, 0.5f});
    dLbl->setPosition({x + 9.f, y + 10.f});
    dLbl->setColor({155, 162, 178});
    m_contentHost->addChild(dLbl, 2);

    // Тумблер-переключатель Liquid Glass
    auto swNode = CCNode::create();
    swNode->setContentSize({30.f, 16.f});
    auto swDraw = CCDrawNode::create();
    if (enabled) {
        drawSoftWhiteGlow(swDraw, ccp(2.f, 2.f), CCSize(26.f, 12.f), 6.f, mm::state().glowIntensity * 0.75f);
    }
    ccColor3B swFill = enabled ? (risky ? CHEAT_RED : acc) : ccColor3B{48, 52, 66};
    drawGlassBox(swDraw, ccp(2.f, 2.f), CCSize(26.f, 12.f), 6.f, swFill, 0.95f, {255, 255, 255}, 0.22f);

    float knobX = enabled ? 21.f : 9.f;
    drawPolyCircle(swDraw, ccp(knobX, 8.f), 4.6f, P({255, 255, 255}, 1.f), 14);
    swNode->addChild(swDraw);

    auto swBtn = CCMenuItemSpriteExtra::create(swNode, this, menu_selector(ModMenuPopup::onToggle));
    swBtn->setTag(static_cast<int>(flag));
    swBtn->setPosition({x + cardW - 18.f, y + cardH * 0.5f});
    m_mainCardsMenu->addChild(swBtn);

    // Кнопка "..." (три точки) без дублирования
    if (hasSubOptions) {
        auto dotsNode = CCNode::create();
        dotsNode->setContentSize({22.f, 18.f});
        auto dotsDraw = CCDrawNode::create();
        drawGlassBox(dotsDraw, ccp(1.f, 2.f), CCSize(20.f, 14.f), 5.f, cardCol, 0.9f, {255, 255, 255}, 0.18f);
        for (int i = 0; i < 3; i++) {
            drawPolyCircle(dotsDraw, ccp(6.f + i * 5.f, 9.f), 1.3f, P({225, 230, 242}, 0.95f), 8);
        }
        dotsNode->addChild(dotsDraw);

        auto dotsBtn = CCMenuItemSpriteExtra::create(dotsNode, this, menu_selector(ModMenuPopup::onOpenSub));
        dotsBtn->setTag(static_cast<int>(flag));
        dotsBtn->setPosition({x + cardW - 45.f, y + cardH * 0.5f});
        m_mainCardsMenu->addChild(dotsBtn);
    }
}

void ModMenuPopup::addActionCard(int col, int row, char const* titleText, char const* subText, SEL_MenuHandler cb) {
    float cardW = 168.f;
    float cardH = 39.f;
    float x = SIDE_W + 10.f + col * (cardW + 8.f);
    float y = WIN_H - 80.f - row * (cardH + 6.f);

    auto node = CCNode::create();
    node->setContentSize({cardW, cardH});
    auto d = CCDrawNode::create();
    drawGlassBox(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 9.f, mm::themeCard(), 0.9f, mm::accent(), 0.6f);
    node->addChild(d);

    auto tLbl = CCLabelBMFont::create(titleText, "chatFont.fnt");
    tLbl->setScale(0.78f);
    tLbl->setColor(mm::accent());
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({10.f, cardH - 12.f});
    node->addChild(tLbl);

    auto sLbl = CCLabelBMFont::create(subText, "chatFont.fnt");
    sLbl->setScale(0.54f);
    sLbl->setColor({155, 162, 178});
    sLbl->setAnchorPoint({0.f, 0.5f});
    sLbl->setPosition({10.f, 10.f});
    node->addChild(sLbl);

    auto btn = CCMenuItemSpriteExtra::create(node, this, cb);
    btn->setPosition({x + cardW * 0.5f, y + cardH * 0.5f});
    m_mainCardsMenu->addChild(btn);
}

void ModMenuPopup::rebuildContent(bool animateTransition) {
    m_contentHost->removeAllChildren();
    this->redrawFrame();
    this->redrawTabSlider();

    m_mainCardsMenu = CCMenu::create();
    m_mainCardsMenu->setPosition({0.f, 0.f});
    m_contentHost->addChild(m_mainCardsMenu, 10);

    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    // Навигационный путь сверху
    auto pathLbl = CCLabelBMFont::create(fmt::format("l-client.gd  >  {}", TAB_TITLES[m_currentTab]).c_str(), "chatFont.fnt");
    pathLbl->setScale(0.75f);
    pathLbl->setAnchorPoint({0.f, 0.5f});
    pathLbl->setPosition({SIDE_W + 16.f, WIN_H - 21.f});
    pathLbl->setColor(txtCol);
    m_contentHost->addChild(pathLbl, 3);

    // Статус Safe Mode
    auto safeLbl = CCLabelBMFont::create(mm::safeActive() ? "SAFE MODE: ON" : "SAFE MODE: OFF", "chatFont.fnt");
    safeLbl->setScale(0.65f);
    safeLbl->setAnchorPoint({1.f, 0.5f});
    safeLbl->setPosition({WIN_W - 42.f, WIN_H - 21.f});
    safeLbl->setColor(mm::safeActive() ? ccColor3B{95, 235, 150} : CHEAT_RED);
    m_contentHost->addChild(safeLbl, 3);

    // ----- Содержимое вкладок -----
    if (m_currentTab == 0) {
        // PLAYER (убрано пустое меню "..." у Noclip!)
        this->addModuleCard(0, 0, mm::F::Noclip, false);
        this->addModuleCard(1, 0, mm::F::Safe, false);
        this->addModuleCard(0, 1, mm::F::Jump, false);
        this->addModuleCard(1, 1, mm::F::AutoSafe, false);

        // Стеклянная панель выбора скорости (Speedhack — помечена красным, если != x1)
        float sx = SIDE_W + 10.f, sy = WIN_H - 80.f - 2 * 45.f;
        bool speedCheating = (mm::state().speedIdx != 3);
        auto spDraw = CCDrawNode::create();
        drawGlassBox(spDraw, ccp(sx, sy), CCSize(344.f, 39.f), 9.f, mm::themeCard(), 0.88f,
                     speedCheating ? CHEAT_RED : ccColor3B{255, 255, 255}, speedCheating ? 0.7f : 0.14f);
        m_contentHost->addChild(spDraw, 1);

        auto spTitle = CCLabelBMFont::create(fmt::format("Speedhack: x{}", mm::speed()).c_str(), "chatFont.fnt");
        spTitle->setScale(0.76f);
        spTitle->setAnchorPoint({0.f, 0.5f});
        spTitle->setPosition({sx + 10.f, sy + 20.f});
        spTitle->setColor(speedCheating ? CHEAT_RED_ON : txtCol);
        m_contentHost->addChild(spTitle, 2);

        for (int i = 0; i < static_cast<int>(mm::SPEEDS.size()); i++) {
            bool sel = (mm::state().speedIdx == i);
            auto pNode = CCNode::create();
            pNode->setContentSize({34.f, 22.f});
            auto pDraw = CCDrawNode::create();
            drawGlassBox(pDraw, ccp(0.f, 0.f), CCSize(34.f, 22.f), 6.f,
                         sel ? (i != 3 ? CHEAT_RED : acc) : ccColor3B{44, 48, 60}, 0.95f,
                         {255, 255, 255}, sel ? 0.4f : 0.1f);
            pNode->addChild(pDraw);
            auto pLbl = CCLabelBMFont::create(fmt::format("x{}", mm::SPEEDS[i]).c_str(), "chatFont.fnt");
            pLbl->setScale(0.62f);
            pLbl->setPosition({17.f, 11.f});
            pLbl->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            pNode->addChild(pLbl);

            auto btn = CCMenuItemSpriteExtra::create(pNode, this, menu_selector(ModMenuPopup::onSpeedPill));
            btn->setTag(i);
            btn->setPosition({sx + 134.f + i * 36.f, sy + 20.f});
            m_mainCardsMenu->addChild(btn);
        }
    }
    else if (m_currentTab == 1) {
        // LEVEL
        this->addModuleCard(0, 0, mm::F::TpsBypass, true);  // "..." открывает настройку TPS (60..2400)
        this->addModuleCard(1, 0, mm::F::FrameSmooth, false);
        this->addModuleCard(0, 1, mm::F::StartPos, false);
        this->addModuleCard(1, 1, mm::F::SmartStart, false);
        this->addModuleCard(0, 2, mm::F::Hitboxes, false);
        this->addModuleCard(1, 2, mm::F::NoDeath, false);
        this->addActionCard(0, 3, "Restart Level", "Instant restart from 0%", menu_selector(ModMenuPopup::onRestartLevel));
    }
    else if (m_currentTab == 2) {
        // VISUALS
        this->addModuleCard(0, 0, mm::F::Overlay, true);    // "..." открывает настройку Overlay
        this->addModuleCard(1, 0, mm::F::ShowFPS, false);
        this->addModuleCard(0, 1, mm::F::NoShake, false);   // Не чит (белое название)
        this->addModuleCard(1, 1, mm::F::NoMirror, false);  // Чит (красное название)
        this->addModuleCard(0, 2, mm::F::HidePlayer, false);
        this->addModuleCard(1, 2, mm::F::HideAttempts, false);
        this->addModuleCard(0, 3, mm::F::RainbowIcon, false);
        this->addModuleCard(1, 3, mm::F::RainbowBG, false);
        this->addModuleCard(0, 4, mm::F::UnlockAll, false);
    }
    else if (m_currentTab == 3) {
        // THEMES (White Glow + Liquid Glass + Пресеты + Кастомная палитра)
        this->addModuleCard(0, 0, mm::F::Glow, true);        // "..." открывает настройку Glow & Palette
        this->addModuleCard(1, 0, mm::F::LiquidGlass, true); // "..." открывает настройку прозрачности стекла

        for (int i = 0; i < 8; i++) {
            int col = i % 2;
            int row = 1 + (i / 2);
            float cardW = 168.f, cardH = 31.f;
            float x = SIDE_W + 10.f + col * (cardW + 8.f);
            float y = WIN_H - 80.f - row * (cardH + 6.f);

            bool sel = (mm::state().themeIdx == i);
            auto node = CCNode::create();
            node->setContentSize({cardW, cardH});
            auto d = CCDrawNode::create();
            if (sel) {
                drawSoftWhiteGlow(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 8.f, mm::state().glowIntensity * 0.5f);
            }
            drawGlassBox(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 8.f,
                         mm::THEMES[i].card, 0.9f,
                         sel ? acc : ccColor3B{255, 255, 255}, sel ? 0.95f : 0.14f);

            ccColor3B swCol = (i == 7) ? mm::accent() : mm::THEMES[i].accent;
            drawRoundRect(d, ccp(cardW - 34.f, 6.5f), CCSize(26.f, 18.f), 5.f, P(swCol, 1.f));
            node->addChild(d);

            auto lbl = CCLabelBMFont::create(mm::THEMES[i].name, "chatFont.fnt");
            lbl->setScale(0.72f);
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
        // CLIENT (Партиклы меню, Анимация нажатий, Настройки кнопки L с подписями!)
        float sx = SIDE_W + 10.f;

        // 1. Выбор партиклов в меню
        float py = WIN_H - 80.f;
        auto pDraw = CCDrawNode::create();
        drawGlassBox(pDraw, ccp(sx, py), CCSize(344.f, 39.f), 9.f, mm::themeCard(), 0.88f, {255, 255, 255}, 0.14f);
        m_contentHost->addChild(pDraw, 1);

        auto pLbl = CCLabelBMFont::create("Menu Particles:", "chatFont.fnt");
        pLbl->setScale(0.75f);
        pLbl->setAnchorPoint({0.f, 0.5f});
        pLbl->setPosition({sx + 10.f, py + 19.5f});
        m_contentHost->addChild(pLbl, 2);

        char const* pNames[4] = {"Off", "Winter", "Autumn", "Sakura"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().particleType == i);
            auto n = CCNode::create();
            n->setContentSize({50.f, 22.f});
            auto d = CCDrawNode::create();
            drawGlassBox(d, ccp(0.f, 0.f), CCSize(50.f, 22.f), 6.f, sel ? acc : ccColor3B{46, 50, 62}, 0.95f, {255, 255, 255}, 0.2f);
            n->addChild(d);
            auto l = CCLabelBMFont::create(pNames[i], "chatFont.fnt");
            l->setScale(0.62f);
            l->setPosition({25.f, 11.f});
            l->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onParticleSelect));
            b->setTag(i);
            b->setPosition({sx + 146.f + i * 54.f, py + 19.5f});
            m_mainCardsMenu->addChild(b);
        }

        // 2. Анимация нажатия кнопок
        float ay = py - 45.f;
        auto aDraw = CCDrawNode::create();
        drawGlassBox(aDraw, ccp(sx, ay), CCSize(344.f, 39.f), 9.f, mm::themeCard(), 0.88f, {255, 255, 255}, 0.14f);
        m_contentHost->addChild(aDraw, 1);

        auto aLbl = CCLabelBMFont::create("Click Animation:", "chatFont.fnt");
        aLbl->setScale(0.75f);
        aLbl->setAnchorPoint({0.f, 0.5f});
        aLbl->setPosition({sx + 10.f, ay + 19.5f});
        m_contentHost->addChild(aLbl, 2);

        char const* aNames[3] = {"Off", "Bounce", "Shrink"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().pressAnimType == i);
            auto n = CCNode::create();
            n->setContentSize({58.f, 22.f});
            auto d = CCDrawNode::create();
            drawGlassBox(d, ccp(0.f, 0.f), CCSize(58.f, 22.f), 6.f, sel ? acc : ccColor3B{46, 50, 62}, 0.95f, {255, 255, 255}, 0.2f);
            n->addChild(d);
            auto l = CCLabelBMFont::create(aNames[i], "chatFont.fnt");
            l->setScale(0.62f);
            l->setPosition({29.f, 11.f});
            l->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onAnimTypeSelect));
            b->setTag(i);
            b->setPosition({sx + 165.f + i * 64.f, ay + 19.5f});
            m_mainCardsMenu->addChild(b);
        }

        // 3. Карточки управления плавающей кнопкой "L" (теперь с заголовками и описаниями!)
        this->addModuleCard(0, 2, mm::F::BtnHideInGame, true); // "..." открывает размер и прозрачность L-кнопки
        this->addModuleCard(1, 2, mm::F::BtnShowInPause, false);
    }

    // Плавный сдвиг контента при смене вкладки
    if (animateTransition) {
        m_contentHost->stopAllActions();
        m_contentHost->setPosition({8.f, 0.f});
        m_contentHost->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.20f, {0.f, 0.f})));
    }

    this->rebuildSubPopup(false);
}

// ----- Всплывающее стеклянное окно по нажатию на "..." -----
void ModMenuPopup::rebuildSubPopup(bool animateOpen) {
    m_subPopupHost->removeAllChildren();

    // Блокируем карточки под окном "...", чтобы они НЕ перехватывали нажатия!
    if (m_mainCardsMenu) {
        m_mainCardsMenu->setEnabled(!m_hasSubSettings);
    }

    if (!m_hasSubSettings || m_activeSubFeature == mm::F::COUNT) return;

    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    auto d = CCDrawNode::create();
    drawSoftWhiteGlow(d, ccp(SIDE_W + 6.f, 8.f), CCSize(WIN_W - SIDE_W - 12.f, WIN_H - 46.f), 12.f, mm::state().glowIntensity * 0.7f);
    drawGlassBox(d, ccp(SIDE_W + 6.f, 8.f), CCSize(WIN_W - SIDE_W - 12.f, WIN_H - 46.f), 12.f,
                 mm::themeBg(), 0.96f, acc, 0.85f);
    m_subPopupHost->addChild(d, 1);

    auto subMenu = CCMenu::create();
    subMenu->setPosition({0.f, 0.f});

    // Кнопка "< Back"
    auto backNode = CCNode::create();
    backNode->setContentSize({58.f, 24.f});
    auto backDraw = CCDrawNode::create();
    drawGlassBox(backDraw, ccp(0.f, 0.f), CCSize(58.f, 24.f), 7.f, mm::themeCard(), 0.95f, acc, 0.9f);
    backNode->addChild(backDraw);
    auto backLbl = CCLabelBMFont::create("< Back", "chatFont.fnt");
    backLbl->setScale(0.72f);
    backLbl->setPosition({29.f, 12.f});
    backNode->addChild(backLbl);
    auto backBtn = CCMenuItemSpriteExtra::create(backNode, this, menu_selector(ModMenuPopup::onCloseSub));
    backBtn->setPosition({SIDE_W + 44.f, WIN_H - 56.f});
    subMenu->addChild(backBtn);

    auto makeStepBtn = [&](char const* txt, CCPoint pos, int tag, SEL_MenuHandler sel) {
        auto n = CCNode::create();
        n->setContentSize({46.f, 26.f});
        auto dr = CCDrawNode::create();
        drawGlassBox(dr, ccp(0.f, 0.f), CCSize(46.f, 26.f), 7.f, mm::themeCard(), 0.95f, acc, 0.65f);
        n->addChild(dr);
        auto lb = CCLabelBMFont::create(txt, "chatFont.fnt");
        lb->setScale(0.72f);
        lb->setPosition({23.f, 13.f});
        n->addChild(lb);
        auto b = CCMenuItemSpriteExtra::create(n, this, sel);
        b->setTag(tag);
        b->setPosition(pos);
        subMenu->addChild(b);
    };

    if (m_activeSubFeature == mm::F::TpsBypass) {
        auto t = CCLabelBMFont::create(fmt::format("Custom Physics TPS: {} Hz", mm::state().customTps).c_str(), "bigFont.fnt");
        t->setScale(0.38f);
        t->setPosition({SIDE_W + 178.f, 178.f});
        m_subPopupHost->addChild(t, 2);

        auto info = CCLabelBMFont::create("Set any custom TPS value from 60 to 2400 Hz", "chatFont.fnt");
        info->setScale(0.72f);
        info->setColor({165, 172, 188});
        info->setPosition({SIDE_W + 178.f, 145.f});
        m_subPopupHost->addChild(info, 2);

        makeStepBtn("-100", {SIDE_W + 52.f, 100.f}, -100, menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("-10",  {SIDE_W + 102.f, 100.f}, -10, menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("-1",   {SIDE_W + 152.f, 100.f}, -1,  menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("+1",   {SIDE_W + 202.f, 100.f}, 1,   menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("+10",  {SIDE_W + 252.f, 100.f}, 10,  menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("+100", {SIDE_W + 302.f, 100.f}, 100, menu_selector(ModMenuPopup::onTpsStep));
    }
    else if (m_activeSubFeature == mm::F::Overlay) {
        auto t = CCLabelBMFont::create("Overlay Customization", "chatFont.fnt");
        t->setScale(0.88f);
        t->setColor(acc);
        t->setPosition({SIDE_W + 185.f, WIN_H - 56.f});
        m_subPopupHost->addChild(t, 2);

        mm::F ovFlags[6] = {mm::F::OvPercent, mm::F::OvCPS, mm::F::OvTps, mm::F::OvAttempts, mm::F::OvStatus, mm::F::OvBg};
        char const* ovNames[6] = {"Show Percent %", "Show CPS", "Show TPS", "Show Attempts", "Show Cheat/Safe", "Glass Pill BG"};

        for (int i = 0; i < 6; i++) {
            int c = i % 2, r = i / 2;
            float bx = SIDE_W + 22.f + c * 162.f;
            float by = 165.f - r * 34.f;
            bool onFlag = mm::on(ovFlags[i]);

            auto n = CCNode::create();
            n->setContentSize({150.f, 26.f});
            auto dr = CCDrawNode::create();
            drawGlassBox(dr, ccp(0.f, 0.f), CCSize(150.f, 26.f), 7.f,
                         onFlag ? mixCol(mm::themeCard(), acc, 0.25f) : mm::themeCard(), 0.95f,
                         onFlag ? acc : ccColor3B{255, 255, 255}, onFlag ? 0.9f : 0.15f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(ovNames[i], "chatFont.fnt");
            lb->setScale(0.66f);
            lb->setPosition({75.f, 13.f});
            lb->setColor(onFlag ? acc : txtCol);
            n->addChild(lb);

            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onToggle));
            b->setTag(static_cast<int>(ovFlags[i]));
            b->setPosition({bx + 75.f, by + 13.f});
            subMenu->addChild(b);
        }

        char const* corners[4] = {"Top-L", "Top-R", "Bot-L", "Bot-R"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().ovCorner == i);
            auto n = CCNode::create();
            n->setContentSize({68.f, 24.f});
            auto dr = CCDrawNode::create();
            drawGlassBox(dr, ccp(0.f, 0.f), CCSize(68.f, 24.f), 6.f, sel ? acc : mm::themeCard(), 0.95f, {255, 255, 255}, 0.25f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(corners[i], "chatFont.fnt");
            lb->setScale(0.65f);
            lb->setPosition({34.f, 12.f});
            lb->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onOvCornerSelect));
            b->setTag(i);
            b->setPosition({SIDE_W + 58.f + i * 76.f, 44.f});
            subMenu->addChild(b);
        }
    }
    else if (m_activeSubFeature == mm::F::Glow || m_activeSubFeature == mm::F::LiquidGlass) {
        auto t = CCLabelBMFont::create(
            fmt::format("Glow: {:.0f}%   |   Glass Opacity: {:.0f}%",
                        mm::state().glowIntensity * 100.f, mm::state().glassAlpha * 100.f).c_str(),
            "chatFont.fnt");
        t->setScale(0.80f);
        t->setPosition({SIDE_W + 178.f, 198.f});
        m_subPopupHost->addChild(t, 2);

        makeStepBtn("Glow -",  {SIDE_W + 78.f, 168.f},  -15, menu_selector(ModMenuPopup::onGlowStep));
        makeStepBtn("Glow +",  {SIDE_W + 132.f, 168.f}, 15,  menu_selector(ModMenuPopup::onGlowStep));
        makeStepBtn("Glass -", {SIDE_W + 224.f, 168.f}, -8,  menu_selector(ModMenuPopup::onGlassStep));
        makeStepBtn("Glass +", {SIDE_W + 278.f, 168.f}, 8,   menu_selector(ModMenuPopup::onGlassStep));

        char const* slots[3] = {"Accent Color", "Background", "Cards"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().editSlot == i);
            auto n = CCNode::create();
            n->setContentSize({96.f, 24.f});
            auto dr = CCDrawNode::create();
            drawGlassBox(dr, ccp(0.f, 0.f), CCSize(96.f, 24.f), 6.f, sel ? acc : mm::themeCard(), 0.95f, {255, 255, 255}, 0.25f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(slots[i], "chatFont.fnt");
            lb->setScale(0.64f);
            lb->setPosition({48.f, 12.f});
            lb->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onColorSlotSelect));
            b->setTag(i);
            b->setPosition({SIDE_W + 72.f + i * 104.f, 122.f});
            subMenu->addChild(b);
        }

        makeStepBtn("Hue -", {SIDE_W + 68.f, 74.f},  -1, menu_selector(ModMenuPopup::onHueStep));
        makeStepBtn("Hue +", {SIDE_W + 118.f, 74.f}, 1,  menu_selector(ModMenuPopup::onHueStep));
        makeStepBtn("Sat -", {SIDE_W + 175.f, 74.f}, -1, menu_selector(ModMenuPopup::onSatStep));
        makeStepBtn("Sat +", {SIDE_W + 225.f, 74.f}, 1,  menu_selector(ModMenuPopup::onSatStep));
        makeStepBtn("Val -", {SIDE_W + 282.f, 74.f}, -1, menu_selector(ModMenuPopup::onValStep));
        makeStepBtn("Val +", {SIDE_W + 332.f, 74.f}, 1,  menu_selector(ModMenuPopup::onValStep));
    }
    else if (m_activeSubFeature == mm::F::BtnHideInGame) {
        auto t = CCLabelBMFont::create(
            fmt::format("L-Button Scale: {:.0f}%   |   Opacity: {:.0f}%",
                        mm::state().btnScale * 100.f, mm::state().btnOpacity * 100.f).c_str(),
            "chatFont.fnt");
        t->setScale(0.8f);
        t->setPosition({SIDE_W + 178.f, 165.f});
        m_subPopupHost->addChild(t, 2);

        makeStepBtn("Size -",  {SIDE_W + 95.f, 115.f},  -10, menu_selector(ModMenuPopup::onBtnScaleStep));
        makeStepBtn("Size +",  {SIDE_W + 150.f, 115.f}, 10,  menu_selector(ModMenuPopup::onBtnScaleStep));
        makeStepBtn("Alpha -", {SIDE_W + 215.f, 115.f}, -10, menu_selector(ModMenuPopup::onBtnOpacityStep));
        makeStepBtn("Alpha +", {SIDE_W + 270.f, 115.f}, 10,  menu_selector(ModMenuPopup::onBtnOpacityStep));
    }

    m_subPopupHost->addChild(subMenu, 10);

    if (animateOpen) {
        m_subPopupHost->stopAllActions();
        m_subPopupHost->setScale(0.92f);
        m_subPopupHost->runAction(CCEaseBackOut::create(CCScaleTo::create(0.18f, 1.0f)));
    }
}

// ============================ Колбэки ============================

void ModMenuPopup::onTab(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    int newTab = node->getTag();
    if (newTab == m_currentTab && !m_hasSubSettings) return;

    m_currentTab = newTab;
    m_hasSubSettings = false;

    // Плавное скольжение стеклянного ползунка в сайдбаре
    m_tabSlider->stopAllActions();
    m_tabSlider->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.22f, {SIDE_W * 0.5f, tabY(m_currentTab)})));

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
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onModInfo(CCObject* sender) {
    this->animatePress(static_cast<CCNode*>(sender));
    FLAlertLayer::create(
        "About L-Client Mod Menu",
        "<cy>L-Client</c> — Liquid Glass mod menu for Geometry Dash 2.2081.\n"
        "<cr>Red Modules</c> are considered cheats and trigger <cg>Auto Safe Mode</c>.\n"
        "<cg>Player:</c> Noclip, Jump Hack, Safe Mode & Speedhack.\n"
        "<cg>Level:</c> Custom TPS Bypass (60-2400 Hz), Zero-Drift Frame Smoothing & Smart StartPos.\n"
        "<cg>Visuals & Themes:</c> Custom Overlay, Liquid Glass, White Glow & Particles.",
        "Got It"
    )->show();
}

void ModMenuPopup::onRestartLevel(CCObject*) {
    if (auto pl = PlayLayer::get()) {
        this->onClose(nullptr);
        pl->resetLevel();
    } else {
        FLAlertLayer::create("L-Client", "Open a level first to restart!", "OK")->show();
    }
}

void ModMenuPopup::onClosePopup(CCObject*) {
    mm::saveAll();
    this->onClose(nullptr);
}

void ModMenuPopup::onSpeedPill(CCObject* sender) {
    mm::state().speedIdx = static_cast<CCNode*>(sender)->getTag();
    mm::applySpeed(PlayLayer::get() != nullptr);
    this->rebuildContent(false);
}

void ModMenuPopup::onTpsStep(CCObject* sender) {
    int delta = static_cast<CCNode*>(sender)->getTag();
    mm::state().customTps = std::clamp(mm::state().customTps + delta, 60, 2400);
    mm::saveAll();
    this->rebuildSubPopup(false);
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
        m_activeSubFeature = mm::F::Glow;
        m_hasSubSettings = true;
        this->rebuildContent(false);
        this->rebuildSubPopup(true);
        return;
    }
    this->rebuildContent(false);
}

void ModMenuPopup::onAnimTypeSelect(CCObject* sender) {
    mm::state().pressAnimType = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
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

void ModMenuPopup::onHueStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    int sl = mm::state().editSlot;
    mm::state().themeIdx = 7;
    mm::state().customH[sl] = std::fmod(mm::state().customH[sl] + d * 0.05f + 1.f, 1.f);
    mm::saveAll();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onSatStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    int sl = mm::state().editSlot;
    mm::state().themeIdx = 7;
    mm::state().customS[sl] = std::clamp(mm::state().customS[sl] + d * 0.08f, 0.f, 1.f);
    mm::saveAll();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onValStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    int sl = mm::state().editSlot;
    mm::state().themeIdx = 7;
    mm::state().customV[sl] = std::clamp(mm::state().customV[sl] + d * 0.08f, 0.05f, 1.f);
    mm::saveAll();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onGlowStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().glowIntensity = std::clamp(mm::state().glowIntensity + d / 100.f, 0.f, 1.f);
    mm::saveAll();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onGlassStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().glassAlpha = std::clamp(mm::state().glassAlpha + d / 100.f, 0.45f, 0.96f);
    mm::saveAll();
    this->redrawFrame();
    this->redrawTabSlider();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onBtnScaleStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().btnScale = std::clamp(mm::state().btnScale + d / 100.f, 0.6f, 1.5f);
    mm::saveAll();
    this->rebuildSubPopup(false);
}

void ModMenuPopup::onBtnOpacityStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().btnOpacity = std::clamp(mm::state().btnOpacity + d / 100.f, 0.2f, 1.f);
    mm::saveAll();
    this->rebuildSubPopup(false);
}
