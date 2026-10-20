#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>
#include <cmath>

namespace {

constexpr float PI = 3.14159265f;
constexpr float WIN_W = 472.f;
constexpr float WIN_H = 292.f;
constexpr float SIDE_W = 114.f;

constexpr char const* TAB_TITLES[5] = {"Player", "Level", "Visuals", "Themes", "Client"};
// Значки GD возле вкладок сайдбара
constexpr char const* TAB_ICON_FRAMES[5] = {
    "GJ_playBtn2_001.png",
    "GJ_practiceBtn_001.png",
    "GJ_colorBtn_001.png",
    "GJ_paintBtn_001.png",
    "GJ_optionsBtn_001.png"
};

const ccColor3B CHEAT_RED = {255, 88, 98};
const ccColor3B CHEAT_RED_ON = {255, 120, 130};
const ccColor3B MUTED_TXT = {142, 148, 164};

ccColor4F P(ccColor3B c, float a) {
    return {c.r / 255.f, c.g / 255.f, c.b / 255.f, a};
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

// Мягкое белое свечение (Soft White Bloom)
void drawSoftWhiteGlow(CCDrawNode* n, CCPoint o, CCSize s, float r, float intensity, ccColor3B tint = {255, 255, 255}) {
    if (!mm::on(mm::F::Glow) || intensity <= 0.01f) return;
    constexpr float EXPANDS[5] = {1.5f, 3.2f, 5.5f, 8.5f, 12.0f};
    constexpr float ALPHAS[5]  = {0.10f, 0.065f, 0.04f, 0.022f, 0.01f};
    for (int i = 0; i < 5; i++) {
        float ex = EXPANDS[i];
        float a = ALPHAS[i] * intensity;
        drawRoundRect(n, ccp(o.x - ex, o.y - ex),
                      CCSize(s.width + ex * 2.f, s.height + ex * 2.f),
                      r + ex, P(tint, a));
    }
}

// Чистое матовое стекло как на скриншоте (source: 26) — без лишних белых полос посередине
void drawCleanGlass(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor3B col, float alpha,
                    ccColor3B borderCol = {255, 255, 255}, float borderAlpha = 0.06f) {
    float glassK = mm::on(mm::F::LiquidGlass) ? mm::state().glassAlpha : 0.98f;
    drawRoundRect(n, o, s, r, P(col, alpha * glassK), 0.75f, P(borderCol, borderAlpha));
}

float tabY(int idx) {
    return WIN_H - 78.f - idx * 32.f;
}

CCDrawNode* makeAlphaDrawNode() {
    auto d = CCDrawNode::create();
    d->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
    return d;
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

    m_mainLayer->setAnchorPoint({0.5f, 0.5f});
    m_mainLayer->ignoreAnchorPointForPosition(false);
    m_mainLayer->setPosition(CCDirector::get()->getWinSize() * 0.5f);
    m_mainLayer->setScale(0.88f);
    m_mainLayer->runAction(CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.0f)));

    m_glowNode = makeAlphaDrawNode();
    m_mainLayer->addChild(m_glowNode, -15);

    m_bgNode = makeAlphaDrawNode();
    m_mainLayer->addChild(m_bgNode, -10);

    m_particleNode = makeAlphaDrawNode();
    m_mainLayer->addChild(m_particleNode, -5);

    m_tabSlider = makeAlphaDrawNode();
    m_tabSlider->setPosition({SIDE_W * 0.5f, tabY(0)});
    m_mainLayer->addChild(m_tabSlider, 4);

    // Заголовок "FEATURES" над вкладками как на скриншоте (source: 26)
    auto featHdr = CCLabelBMFont::create("FEATURES", "chatFont.fnt");
    featHdr->setScale(0.52f);
    featHdr->setAnchorPoint({0.f, 0.5f});
    featHdr->setPosition({14.f, WIN_H - 54.f});
    featHdr->setColor({105, 110, 126});
    m_mainLayer->addChild(featHdr, 5);

    // Сайдбар со значками в стиле GD возле каждой вкладки
    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.f, 0.f});
    for (int i = 0; i < TAB_COUNT; i++) {
        auto bNode = CCNode::create();
        bNode->setContentSize({SIDE_W - 16.f, 26.f});

        // Иконка в стиле GD слева от названия вкладки
        auto icon = CCSprite::createWithSpriteFrameName(TAB_ICON_FRAMES[i]);
        if (!icon) icon = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
        if (icon) {
            float maxDim = std::max(icon->getContentSize().width, icon->getContentSize().height);
            icon->setScale(13.5f / std::max(1.f, maxDim));
            icon->setPosition({14.f, 13.f});
            bNode->addChild(icon, 2);
            m_tabIcons[i] = icon;
        }

        auto bLbl = CCLabelBMFont::create(TAB_TITLES[i], "chatFont.fnt");
        bLbl->setScale(0.76f);
        bLbl->setAnchorPoint({0.f, 0.5f});
        bLbl->setPosition({28.f, 13.f});
        bNode->addChild(bLbl, 2);
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

    // Стилизованный логотип "L" сверху в сайдбаре (как буква N на скрине source: 26)
    auto logoL = CCLabelBMFont::create("L", "bigFont.fnt");
    logoL->setScale(0.55f);
    logoL->setPosition({SIDE_W * 0.5f, WIN_H - 24.f});
    m_mainLayer->addChild(logoL, 5);

    // Внизу сайдбара (как кружок "?" с именем на скрине source: 26) — кнопка описания мода
    {
        auto qNode = CCNode::create();
        qNode->setContentSize({SIDE_W - 16.f, 28.f});
        auto qDraw = makeAlphaDrawNode();
        drawPolyCircle(qDraw, ccp(12.f, 14.f), 9.f, P({38, 40, 52}, 0.95f), 14);
        qNode->addChild(qDraw);

        auto qSym = CCLabelBMFont::create("?", "chatFont.fnt");
        qSym->setScale(0.75f);
        qSym->setPosition({12.f, 14.f});
        qSym->setColor({200, 205, 220});
        qNode->addChild(qSym);

        auto uName = CCLabelBMFont::create("L-Client", "chatFont.fnt");
        uName->setScale(0.66f);
        uName->setAnchorPoint({0.f, 0.5f});
        uName->setPosition({26.f, 18.f});
        uName->setColor({230, 232, 240});
        qNode->addChild(uName);

        auto uSub = CCLabelBMFont::create("GD 2.2081", "chatFont.fnt");
        uSub->setScale(0.48f);
        uSub->setAnchorPoint({0.f, 0.5f});
        uSub->setPosition({26.f, 9.f});
        uSub->setColor(MUTED_TXT);
        qNode->addChild(uSub);

        auto qBtn = CCMenuItemSpriteExtra::create(qNode, this, menu_selector(ModMenuPopup::onModInfo));
        qBtn->setPosition({SIDE_W * 0.5f, 22.f});
        m_buttonMenu->addChild(qBtn, 15);
    }

    // Кнопка закрытия справа вверху (в стиле кнопки меню на скрине source: 26)
    {
        auto cNode = CCNode::create();
        cNode->setContentSize({24.f, 24.f});
        auto cDraw = makeAlphaDrawNode();
        drawCleanGlass(cDraw, ccp(2.f, 2.f), CCSize(20.f, 20.f), 6.f, mm::themeCard(), 0.95f, {255, 255, 255}, 0.10f);
        cDraw->drawSegment(ccp(8.5f, 8.5f), ccp(15.5f, 15.5f), 0.9f, P({210, 215, 230}, 1.f));
        cDraw->drawSegment(ccp(8.5f, 15.5f), ccp(15.5f, 8.5f), 0.9f, P({210, 215, 230}, 1.f));
        cNode->addChild(cDraw);

        auto cBtn = CCMenuItemSpriteExtra::create(cNode, this, menu_selector(ModMenuPopup::onClosePopup));
        cBtn->setPosition({WIN_W - 20.f, WIN_H - 22.f});
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
    float glow = mm::state().glowIntensity;

    // 1. Внешнее мягкое белое свечение (Glow)
    drawSoftWhiteGlow(m_glowNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), 14.f, glow);

    // 2. Главный тёмный стеклянный корпус как на скрине (source: 26)
    drawCleanGlass(m_bgNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), 14.f, bg, 0.96f, {255, 255, 255}, 0.08f);

    // 3. Тонкая вертикальная линия, отделяющая левый сайдбар, и разделитель внизу сайдбара
    m_bgNode->drawSegment(ccp(SIDE_W, 8.f), ccp(SIDE_W, WIN_H - 8.f), 0.5f, P({255, 255, 255}, 0.05f));
    m_bgNode->drawSegment(ccp(10.f, 42.f), ccp(SIDE_W - 10.f, 42.f), 0.5f, P({255, 255, 255}, 0.05f));

    // 4. Верхняя поисковая/статусная строка как "Search something" на скрине (source: 26)
    drawCleanGlass(m_bgNode, ccp(SIDE_W + 10.f, WIN_H - 33.f), CCSize(WIN_W - SIDE_W - 46.f, 22.f), 6.f, card, 0.95f, {255, 255, 255}, 0.05f);
}

void ModMenuPopup::redrawTabSlider() {
    m_tabSlider->clear();
    auto acc = mm::accent();
    float w = SIDE_W - 16.f, h = 25.f;
    CCPoint o = ccp(-w * 0.5f, -h * 0.5f);

    drawSoftWhiteGlow(m_tabSlider, o, CCSize(w, h), 6.f, mm::state().glowIntensity * 0.45f);
    // Темно-акцентная капсула выбранной вкладки как "Combat" на скрине (source: 26)
    drawCleanGlass(m_tabSlider, o, CCSize(w, h), 6.f, mixCol(mm::themeCard(), acc, 0.22f), 0.98f, acc, 0.45f);

    for (int i = 0; i < TAB_COUNT; i++) {
        bool act = (i == m_currentTab);
        if (m_tabLabels[i]) {
            m_tabLabels[i]->setColor(act ? ccColor3B{255, 255, 255} : MUTED_TXT);
        }
        if (m_tabIcons[i]) {
            m_tabIcons[i]->setColor(act ? acc : ccColor3B{165, 170, 185});
            m_tabIcons[i]->setOpacity(act ? 255 : 175);
        }
    }
}

// Рисует контейнер-группу ("Fighting", "Tools", "Base") с двойной стрелочкой ↕ справа как на скрине (source: 26)
void ModMenuPopup::drawSectionBox(CCNode* parent, float x, float yTop, float w, int rowCount, char const* sectionTitle) {
    float hdrH = 24.f;
    float rowH = 26.f;
    float totalH = hdrH + rowCount * rowH + 4.f;
    float yBot = yTop - totalH;

    auto d = makeAlphaDrawNode();
    drawCleanGlass(d, ccp(x, yBot), CCSize(w, totalH), 8.f, mm::themeCard(), 0.94f, {255, 255, 255}, 0.05f);
    // Линия под шапкой группы
    d->drawSegment(ccp(x + 8.f, yTop - hdrH), ccp(x + w - 8.f, yTop - hdrH), 0.5f, P({255, 255, 255}, 0.05f));

    // Двойная стрелочка ↕ справа в шапке группы (акцентного цвета как на скрине source: 26)
    auto acc = mm::accent();
    float ax = x + w - 13.f, ay = yTop - 12.f;
    d->drawSegment(ccp(ax - 2.5f, ay + 1.5f), ccp(ax, ay + 4.0f), 0.85f, P(acc, 0.95f));
    d->drawSegment(ccp(ax + 2.5f, ay + 1.5f), ccp(ax, ay + 4.0f), 0.85f, P(acc, 0.95f));
    d->drawSegment(ccp(ax - 2.5f, ay - 1.5f), ccp(ax, ay - 4.0f), 0.85f, P(acc, 0.95f));
    d->drawSegment(ccp(ax + 2.5f, ay - 1.5f), ccp(ax, ay - 4.0f), 0.85f, P(acc, 0.95f));
    parent->addChild(d, 1);

    auto lbl = CCLabelBMFont::create(sectionTitle, "chatFont.fnt");
    lbl->setScale(0.74f);
    lbl->setAnchorPoint({0.f, 0.5f});
    lbl->setPosition({x + 10.f, yTop - 12.f});
    lbl->setColor(mm::themeText());
    parent->addChild(lbl, 2);
}

// Добавляет строку функции внутри контейнера-группы (точь-в-точь как "Attack Aura ... (o)" на скрине source: 26)
void ModMenuPopup::addModuleRow(float x, float yRowTop, float w, mm::F flag, bool hasSubOptions, bool drawBottomLine) {
    float rowH = 26.f;
    float cy = yRowTop - rowH * 0.5f;

    bool enabled = mm::on(flag);
    bool risky = mm::isRisky(flag);
    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    if (drawBottomLine) {
        auto line = makeAlphaDrawNode();
        line->drawSegment(ccp(x + 8.f, yRowTop - rowH), ccp(x + w - 8.f, yRowTop - rowH), 0.5f, P({255, 255, 255}, 0.035f));
        m_contentHost->addChild(line, 2);
    }

    // Название функции (красное только для реальных читов, StartPos и SmartStart — обычного цвета!)
    auto tLbl = CCLabelBMFont::create(mm::title(flag), "chatFont.fnt");
    tLbl->setScale(0.70f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x + 10.f, cy});
    if (risky) {
        tLbl->setColor(enabled ? CHEAT_RED_ON : CHEAT_RED);
    } else {
        tLbl->setColor(enabled ? ccColor3B{255, 255, 255} : mixCol(txtCol, MUTED_TXT, 0.35f));
    }
    m_contentHost->addChild(tLbl, 3);

    // Компактный овальный тумблер справа
    auto swNode = CCNode::create();
    swNode->setContentSize({26.f, 14.f});
    auto swDraw = makeAlphaDrawNode();
    if (enabled) {
        drawSoftWhiteGlow(swDraw, ccp(2.f, 2.f), CCSize(22.f, 10.f), 5.f, mm::state().glowIntensity * 0.65f);
    }
    ccColor3B swCol = enabled ? (risky ? CHEAT_RED : acc) : ccColor3B{36, 38, 48};
    drawRoundRect(swDraw, ccp(2.f, 2.f), CCSize(22.f, 10.f), 5.f, P(swCol, 1.f));
    float knobX = enabled ? 19.f : 7.f;
    drawPolyCircle(swDraw, ccp(knobX, 7.f), 3.6f, P(enabled ? ccColor3B{255, 255, 255} : ccColor3B{110, 115, 130}, 1.f), 12);
    swNode->addChild(swDraw);

    auto swBtn = CCMenuItemSpriteExtra::create(swNode, this, menu_selector(ModMenuPopup::onToggle));
    swBtn->setTag(static_cast<int>(flag));
    swBtn->setPosition({x + w - 18.f, cy});
    m_mainCardsMenu->addChild(swBtn);

    // Три точки "..." слева от тумблера (как на скрине source: 26)
    if (hasSubOptions) {
        auto dotsNode = CCNode::create();
        dotsNode->setContentSize({18.f, 16.f});
        auto dotsDraw = makeAlphaDrawNode();
        for (int i = 0; i < 3; i++) {
            drawPolyCircle(dotsDraw, ccp(4.f + i * 4.5f, 8.f), 1.15f, P({185, 190, 210}, 0.95f), 8);
        }
        dotsNode->addChild(dotsDraw);

        auto dotsBtn = CCMenuItemSpriteExtra::create(dotsNode, this, menu_selector(ModMenuPopup::onOpenSub));
        dotsBtn->setTag(static_cast<int>(flag));
        dotsBtn->setPosition({x + w - 42.f, cy});
        m_mainCardsMenu->addChild(dotsBtn);
    }
}

// Интерактивный ползунок (Slider), который тянется пальцем и сразу меняет значения вживую!
void ModMenuPopup::addSliderControl(CCNode* host, float x, float y, float w, char const* title, char const* suffix,
                                    float minV, float maxV, bool isInt,
                                    std::function<float()> getter, std::function<void(float)> setter) {
    auto d = makeAlphaDrawNode();
    host->addChild(d, 3);

    auto tLbl = CCLabelBMFont::create(title, "chatFont.fnt");
    tLbl->setScale(0.68f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x, y + 11.f});
    tLbl->setColor(mm::themeText());
    host->addChild(tLbl, 4);

    auto vLbl = CCLabelBMFont::create("", "chatFont.fnt");
    vLbl->setScale(0.68f);
    vLbl->setAnchorPoint({1.f, 0.5f});
    vLbl->setPosition({x + w, y + 11.f});
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

    auto acc = mm::accent();
    // Подложка трека
    drawRoundRect(sl.drawNode, ccp(x, y - 2.5f), CCSize(w, 5.f), 2.5f, P({36, 38, 50}, 1.f));
    // Заполненная часть
    if (t > 0.01f) {
        drawRoundRect(sl.drawNode, ccp(x, y - 2.5f), CCSize( std::max(5.f, w * t), 5.f), 2.5f, P(acc, 1.f));
    }
    // Круглая ручка ползунка со свечением
    float kx = x + w * t;
    if (mm::on(mm::F::Glow)) {
        drawPolyCircle(sl.drawNode, ccp(kx, y), 7.5f, P({255, 255, 255}, 0.22f * mm::state().glowIntensity), 12);
    }
    drawPolyCircle(sl.drawNode, ccp(kx, y), 5.0f, P({255, 255, 255}, 1.f), 14);
    drawPolyCircle(sl.drawNode, ccp(kx, y), 3.2f, P(acc, 1.f), 12);

    std::string txt = sl.isInt ? fmt::format("{:.0f}{}", val, sl.suffix)
                               : fmt::format("{:.0f}{}", val, sl.suffix);
    sl.valLabel->setString(txt.c_str());
}

// Визуальная палитра (2D квадрат Saturation/Value + радужная полоса Hue)
void ModMenuPopup::buildVisualPalette(CCNode* host, float x, float y) {
    m_hasPalette = true;
    float svW = 190.f, svH = 68.f;
    float hueH = 11.f;

    m_svRect = CCRect(x, y + 18.f, svW, svH);
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

    // Радужная полоса Hue из 6 градиентных сегментов
    float segW = svW / 6.f;
    for (int i = 0; i < 6; i++) {
        auto c1 = mm::hsv(i / 6.f, 1.f, 1.f);
        auto c2 = mm::hsv((i + 1) / 6.f, 1.f, 1.f);
        auto g = CCLayerGradient::create(ccc4(c1.r, c1.g, c1.b, 255), ccc4(c2.r, c2.g, c2.b, 255), ccp(1.f, 0.f));
        g->setContentSize({segW + 0.5f, hueH});
        g->setPosition({x + i * segW, y});
        host->addChild(g, 3);
    }

    m_svCursor = makeAlphaDrawNode();
    host->addChild(m_svCursor, 6);
    m_hueCursor = makeAlphaDrawNode();
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
    drawPolyCircle(m_hueCursor, hPt, 6.2f, P({255, 255, 255}, 1.f), 12);
    drawPolyCircle(m_hueCursor, hPt, 4.5f, P(pureHue, 1.f), 12);
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

    this->redrawFrame();
    this->redrawTabSlider();

    m_mainCardsMenu = CCMenu::create();
    m_mainCardsMenu->setPosition({0.f, 0.f});
    m_contentHost->addChild(m_mainCardsMenu, 10);

    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    // Текст в верхней строке поиска (как на скрине source: 26)
    auto searchLbl = CCLabelBMFont::create(fmt::format("Category: {}", TAB_TITLES[m_currentTab]).c_str(), "chatFont.fnt");
    searchLbl->setScale(0.68f);
    searchLbl->setAnchorPoint({0.f, 0.5f});
    searchLbl->setPosition({SIDE_W + 18.f, WIN_H - 22.f});
    searchLbl->setColor(MUTED_TXT);
    m_contentHost->addChild(searchLbl, 3);

    auto safeLbl = CCLabelBMFont::create(mm::safeActive() ? "SAFE MODE" : "UNSAFE", "chatFont.fnt");
    safeLbl->setScale(0.62f);
    safeLbl->setAnchorPoint({1.f, 0.5f});
    safeLbl->setPosition({WIN_W - 44.f, WIN_H - 22.f});
    safeLbl->setColor(mm::safeActive() ? ccColor3B{95, 235, 150} : CHEAT_RED);
    m_contentHost->addChild(safeLbl, 3);

    float col0X = SIDE_W + 10.f;
    float col1X = SIDE_W + 182.f;
    float boxW = 166.f;
    float topY = WIN_H - 42.f;

    if (m_currentTab == 0) {
        // PLAYER: 2 группы ("Movement", "Protection") + "Speedhack" внизу
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "Movement");
        this->addModuleRow(col0X, topY - 24.f, boxW, mm::F::Noclip, false, true);
        this->addModuleRow(col0X, topY - 50.f, boxW, mm::F::Jump, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 2, "Protection");
        this->addModuleRow(col1X, topY - 24.f, boxW, mm::F::Safe, false, true);
        this->addModuleRow(col1X, topY - 50.f, boxW, mm::F::AutoSafe, false, false);

        // Секция Speedhack
        float spTop = topY - 90.f;
        this->drawSectionBox(m_contentHost, col0X, spTop, boxW * 2.f + 6.f, 2, "Speedhack Multiplier");
        for (int i = 0; i < static_cast<int>(mm::SPEEDS.size()); i++) {
            bool sel = (mm::state().speedIdx == i);
            auto pNode = CCNode::create();
            pNode->setContentSize({48.f, 22.f});
            auto pDraw = makeAlphaDrawNode();
            drawCleanGlass(pDraw, ccp(0.f, 0.f), CCSize(48.f, 22.f), 5.f,
                           sel ? (i != 3 ? CHEAT_RED : acc) : ccColor3B{34, 36, 46}, 0.98f,
                           {255, 255, 255}, sel ? 0.35f : 0.06f);
            pNode->addChild(pDraw);
            auto pLbl = CCLabelBMFont::create(fmt::format("x{}", mm::SPEEDS[i]).c_str(), "chatFont.fnt");
            pLbl->setScale(0.64f);
            pLbl->setPosition({24.f, 11.f});
            pLbl->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            pNode->addChild(pLbl);

            auto btn = CCMenuItemSpriteExtra::create(pNode, this, menu_selector(ModMenuPopup::onSpeedPill));
            btn->setTag(i);
            btn->setPosition({col0X + 34.f + i * 54.f, spTop - 48.f});
            m_mainCardsMenu->addChild(btn);
        }
    }
    else if (m_currentTab == 1) {
        // LEVEL: StartPos и SmartStart теперь НЕ горят красным!
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 3, "Physics & Bypass");
        this->addModuleRow(col0X, topY - 24.f, boxW, mm::F::TpsBypass, true, true);
        this->addModuleRow(col0X, topY - 50.f, boxW, mm::F::FrameSmooth, false, true);
        this->addModuleRow(col0X, topY - 76.f, boxW, mm::F::Hitboxes, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 3, "Practice Tools");
        this->addModuleRow(col1X, topY - 24.f, boxW, mm::F::StartPos, false, true);
        this->addModuleRow(col1X, topY - 50.f, boxW, mm::F::SmartStart, false, true);
        this->addModuleRow(col1X, topY - 76.f, boxW, mm::F::NoDeath, false, false);

        // Ползунок TPS прямо на вкладке Level для мгновенной настройки!
        this->drawSectionBox(m_contentHost, col0X, topY - 116.f, boxW * 2.f + 6.f, 2, "Live TPS Configuration");
        this->addSliderControl(m_contentHost, col0X + 12.f, topY - 166.f, boxW * 2.f - 18.f,
                               "Physics Ticks (TPS)", " Hz", 60.f, 2400.f, true,
                               [] { return static_cast<float>(mm::state().customTps); },
                               [](float v) { mm::state().customTps = static_cast<int>(std::lround(v)); });
    }
    else if (m_currentTab == 2) {
        // VISUALS: "Interface" и "World & FX" (как на скрине source: 26)
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 4, "Interface");
        this->addModuleRow(col0X, topY - 24.f, boxW, mm::F::Overlay, true, true);
        this->addModuleRow(col0X, topY - 50.f, boxW, mm::F::ShowFPS, false, true);
        this->addModuleRow(col0X, topY - 76.f, boxW, mm::F::HideAttempts, false, true);
        this->addModuleRow(col0X, topY - 102.f, boxW, mm::F::UnlockAll, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 5, "World & FX");
        this->addModuleRow(col1X, topY - 24.f, boxW, mm::F::NoShake, false, true);
        this->addModuleRow(col1X, topY - 50.f, boxW, mm::F::NoMirror, false, true);
        this->addModuleRow(col1X, topY - 76.f, boxW, mm::F::HidePlayer, false, true);
        this->addModuleRow(col1X, topY - 102.f, boxW, mm::F::RainbowIcon, false, true);
        this->addModuleRow(col1X, topY - 128.f, boxW, mm::F::RainbowBG, false, false);
    }
    else if (m_currentTab == 3) {
        // THEMES: Эффекты + Пресеты + Кнопка открытия визуальной палитры
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 1, "Glow Effect");
        this->addModuleRow(col0X, topY - 24.f, boxW, mm::F::Glow, true, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 1, "Surface Style");
        this->addModuleRow(col1X, topY - 24.f, boxW, mm::F::LiquidGlass, true, false);

        for (int i = 0; i < 8; i++) {
            int col = i % 2;
            int row = i / 2;
            float cardW = 166.f, cardH = 28.f;
            float x = (col == 0 ? col0X : col1X);
            float y = topY - 90.f - row * (cardH + 6.f);

            bool sel = (mm::state().themeIdx == i);
            auto node = CCNode::create();
            node->setContentSize({cardW, cardH});
            auto d = makeAlphaDrawNode();
            drawCleanGlass(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 6.f,
                           mm::THEMES[i].card, 0.96f,
                           sel ? acc : ccColor3B{255, 255, 255}, sel ? 0.85f : 0.08f);

            ccColor3B swCol = (i == 7) ? mm::accent() : mm::THEMES[i].accent;
            drawRoundRect(d, ccp(cardW - 32.f, 5.f), CCSize(24.f, 18.f), 4.f, P(swCol, 1.f));
            node->addChild(d);

            auto lbl = CCLabelBMFont::create(mm::THEMES[i].name, "chatFont.fnt");
            lbl->setScale(0.68f);
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
        // CLIENT: Видимость кнопки L ВЕЗДЕ + Живые ползунки (Scale, Opacity, Corner Radius)
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "Floating L-Button");
        this->addModuleRow(col0X, topY - 24.f, boxW, mm::F::BtnHideInGame, false, true);
        this->addModuleRow(col0X, topY - 50.f, boxW, mm::F::BtnShowInPause, false, false);

        // Секция с 3 живыми ползунками для кнопки L (Размер, Прозрачность, Скругление углов!)
        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 4, "L-Button Sliders");
        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 44.f, boxW - 20.f,
                               "Button Size", "%", 55.f, 160.f, true,
                               [] { return mm::state().btnScale * 100.f; },
                               [](float v) { mm::state().btnScale = v / 100.f; });

        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 76.f, boxW - 20.f,
                               "Opacity", "%", 15.f, 100.f, true,
                               [] { return mm::state().btnOpacity * 100.f; },
                               [](float v) { mm::state().btnOpacity = v / 100.f; });

        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 108.f, boxW - 20.f,
                               "Corner Radius", " px", 0.f, 17.f, true,
                               [] { return mm::state().btnRadius; },
                               [](float v) { mm::state().btnRadius = v; });

        // Партиклы и анимация внизу
        float botTop = topY - 92.f;
        this->drawSectionBox(m_contentHost, col0X, botTop, boxW, 2, "Menu Particles");
        char const* pNames[4] = {"Off", "Win", "Aut", "Sak"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().particleType == i);
            auto n = CCNode::create();
            n->setContentSize({35.f, 20.f});
            auto d = makeAlphaDrawNode();
            drawCleanGlass(d, ccp(0.f, 0.f), CCSize(35.f, 20.f), 4.f, sel ? acc : ccColor3B{36, 38, 48}, 0.98f);
            n->addChild(d);
            auto l = CCLabelBMFont::create(pNames[i], "chatFont.fnt");
            l->setScale(0.58f);
            l->setPosition({17.5f, 10.f});
            l->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onParticleSelect));
            b->setTag(i);
            b->setPosition({col0X + 25.f + i * 38.f, botTop - 46.f});
            m_mainCardsMenu->addChild(b);
        }
    }

    if (animateTransition) {
        m_contentHost->stopAllActions();
        m_contentHost->setPosition({6.f, 0.f});
        m_contentHost->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.18f, {0.f, 0.f})));
    }

    this->rebuildSubPopup(false);
}

// ----- Всплывающее окно "..." (с живыми ползунками и визуальной палитрой!) -----
void ModMenuPopup::rebuildSubPopup(bool animateOpen) {
    m_subPopupHost->removeAllChildren();
    if (m_mainCardsMenu) {
        m_mainCardsMenu->setEnabled(!m_hasSubSettings);
    }
    if (!m_hasSubSettings || m_activeSubFeature == mm::F::COUNT) return;

    m_sliders.clear();
    m_hasPalette = false;

    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    auto d = makeAlphaDrawNode();
    drawCleanGlass(d, ccp(SIDE_W + 6.f, 8.f), CCSize(WIN_W - SIDE_W - 12.f, WIN_H - 44.f), 10.f,
                   mm::themeBg(), 0.98f, acc, 0.65f);
    m_subPopupHost->addChild(d, 1);

    auto subMenu = CCMenu::create();
    subMenu->setPosition({0.f, 0.f});

    auto backNode = CCNode::create();
    backNode->setContentSize({54.f, 22.f});
    auto backDraw = makeAlphaDrawNode();
    drawCleanGlass(backDraw, ccp(0.f, 0.f), CCSize(54.f, 22.f), 5.f, mm::themeCard(), 0.98f, acc, 0.7f);
    backNode->addChild(backDraw);
    auto backLbl = CCLabelBMFont::create("< Back", "chatFont.fnt");
    backLbl->setScale(0.68f);
    backLbl->setPosition({27.f, 11.f});
    backNode->addChild(backLbl);
    auto backBtn = CCMenuItemSpriteExtra::create(backNode, this, menu_selector(ModMenuPopup::onCloseSub));
    backBtn->setPosition({SIDE_W + 42.f, WIN_H - 54.f});
    subMenu->addChild(backBtn);

    if (m_activeSubFeature == mm::F::TpsBypass) {
        this->addSliderControl(m_subPopupHost, SIDE_W + 28.f, 145.f, 290.f,
                               "Custom Physics TPS", " Hz", 60.f, 2400.f, true,
                               [] { return static_cast<float>(mm::state().customTps); },
                               [](float v) { mm::state().customTps = static_cast<int>(std::lround(v)); });
    }
    else if (m_activeSubFeature == mm::F::Overlay) {
        mm::F ovFlags[6] = {mm::F::OvPercent, mm::F::OvCPS, mm::F::OvTps, mm::F::OvAttempts, mm::F::OvStatus, mm::F::OvBg};
        char const* ovNames[6] = {"Percent %", "CPS", "TPS", "Attempts", "Safe Status", "Glass Pill"};

        for (int i = 0; i < 6; i++) {
            int c = i % 3, r = i / 3;
            float bx = SIDE_W + 20.f + c * 106.f;
            float by = 176.f - r * 30.f;
            bool onFlag = mm::on(ovFlags[i]);

            auto n = CCNode::create();
            n->setContentSize({98.f, 24.f});
            auto dr = makeAlphaDrawNode();
            drawCleanGlass(dr, ccp(0.f, 0.f), CCSize(98.f, 24.f), 5.f,
                           onFlag ? mixCol(mm::themeCard(), acc, 0.25f) : mm::themeCard(), 0.98f,
                           onFlag ? acc : ccColor3B{255, 255, 255}, onFlag ? 0.8f : 0.1f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(ovNames[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({49.f, 12.f});
            lb->setColor(onFlag ? acc : txtCol);
            n->addChild(lb);

            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onToggle));
            b->setTag(static_cast<int>(ovFlags[i]));
            b->setPosition({bx + 49.f, by + 12.f});
            subMenu->addChild(b);
        }

        this->addSliderControl(m_subPopupHost, SIDE_W + 24.f, 102.f, 142.f,
                               "Overlay Scale", "%", 50.f, 130.f, true,
                               [] { return mm::state().ovScale * 100.f; },
                               [](float v) { mm::state().ovScale = v / 100.f; });

        this->addSliderControl(m_subPopupHost, SIDE_W + 184.f, 102.f, 142.f,
                               "Overlay Alpha", "%", 20.f, 100.f, true,
                               [] { return mm::state().ovOpacity * 100.f; },
                               [](float v) { mm::state().ovOpacity = v / 100.f; });

        char const* corners[4] = {"Top-L", "Top-R", "Bot-L", "Bot-R"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().ovCorner == i);
            auto n = CCNode::create();
            n->setContentSize({68.f, 22.f});
            auto dr = makeAlphaDrawNode();
            drawCleanGlass(dr, ccp(0.f, 0.f), CCSize(68.f, 22.f), 5.f, sel ? acc : mm::themeCard(), 0.98f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(corners[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({34.f, 11.f});
            lb->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onOvCornerSelect));
            b->setTag(i);
            b->setPosition({SIDE_W + 58.f + i * 76.f, 36.f});
            subMenu->addChild(b);
        }
    }
    else if (m_activeSubFeature == mm::F::Glow || m_activeSubFeature == mm::F::LiquidGlass) {
        // Живые ползунки Glow и Glass Opacity + Визуальная палитра (Color Picker)!
        this->addSliderControl(m_subPopupHost, SIDE_W + 22.f, 196.f, 145.f,
                               "White Glow", "%", 0.f, 100.f, true,
                               [] { return mm::state().glowIntensity * 100.f; },
                               [this](float v) {
                                   mm::state().glowIntensity = v / 100.f;
                                   this->redrawFrame();
                                   this->redrawTabSlider();
                               });

        this->addSliderControl(m_subPopupHost, SIDE_W + 182.f, 196.f, 145.f,
                               "Glass Opacity", "%", 45.f, 98.f, true,
                               [] { return mm::state().glassAlpha * 100.f; },
                               [this](float v) {
                                   mm::state().glassAlpha = v / 100.f;
                                   this->redrawFrame();
                               });

        // Справа кнопки выбора слота (Accent / Background / Cards), слева — живая палитра!
        char const* slots[3] = {"Accent Color", "Background", "Containers"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().editSlot == i);
            auto n = CCNode::create();
            n->setContentSize({96.f, 24.f});
            auto dr = makeAlphaDrawNode();
            drawCleanGlass(dr, ccp(0.f, 0.f), CCSize(96.f, 24.f), 5.f, sel ? acc : mm::themeCard(), 0.98f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(slots[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({48.f, 12.f});
            lb->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onColorSlotSelect));
            b->setTag(i);
            b->setPosition({SIDE_W + 275.f, 126.f - i * 30.f});
            subMenu->addChild(b);
        }

        this->buildVisualPalette(m_subPopupHost, SIDE_W + 22.f, 48.f);
    }

    m_subPopupHost->addChild(subMenu, 10);

    if (animateOpen) {
        m_subPopupHost->stopAllActions();
        m_subPopupHost->setScale(0.94f);
        m_subPopupHost->runAction(CCEaseBackOut::create(CCScaleTo::create(0.16f, 1.0f)));
    }
}

// ============================ Обработка касаний для ползунков и палитры ============================

bool ModMenuPopup::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    CCPoint lp = m_mainLayer->convertToNodeSpace(touch->getLocation());

    // 1. Проверяем попадание в визуальную палитру (SV или Hue)
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

    // 2. Проверяем попадание в любой активный ползунок (Slider)
    for (int i = 0; i < static_cast<int>(m_sliders.size()); i++) {
        CCRect hit = m_sliders[i].rect;
        hit.origin.y -= 5.f;
        hit.size.height += 10.f;
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
    m_tabSlider->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.20f, {SIDE_W * 0.5f, tabY(m_currentTab)})));

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
    this->rebuildContent(false);
}

void ModMenuPopup::onModInfo(CCObject* sender) {
    this->animatePress(static_cast<CCNode*>(sender));
    FLAlertLayer::create(
        "About L-Client",
        "<cy>L-Client</c> — Clean Dark Glass Menu for GD 2.2081.\n"
        "<cr>Red Modules</c> are cheats and activate <cg>Auto Safe Mode</c>.\n"
        "<cg>StartPos & Smart StartPos</c> are safe practice tools.\n"
        "<cg>Sliders & Color Picker:</c> Drag any slider or palette to see changes live!",
        "Got It"
    )->show();
}

void ModMenuPopup::onRestartLevel(CCObject*) {
    if (auto pl = PlayLayer::get()) {
        this->onClose(nullptr);
        pl->resetLevel();
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
