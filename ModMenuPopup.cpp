#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>
#include <cmath>

namespace {

constexpr float PI = 3.14159265f;
constexpr float WIN_W = 460.f;
constexpr float WIN_H = 284.f;
constexpr float SIDE_W = 104.f;

constexpr char const* TAB_TITLES[5] = {"Player", "Level", "Visuals", "Themes", "Client"};

ccColor4F P(ccColor3B c, float a) {
    return {c.r / 255.f * a, c.g / 255.f * a, c.b / 255.f * a, a};
}

bool nearPt(CCPoint a, CCPoint b) {
    return std::abs(a.x - b.x) < 0.02f && std::abs(a.y - b.y) < 0.02f;
}

void drawRoundRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill,
                   float bw = 0.f, ccColor4F bc = {0.f, 0.f, 0.f, 0.f}) {
    if (s.width < 1.f || s.height < 1.f) return;
    r = std::clamp(r, 0.f, std::min(s.width, s.height) * 0.5f);

    std::vector<CCPoint> pts;
    pts.reserve(28);
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
        constexpr int SEG = 5; // Оптимизировано для мобильных GPU
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

// Отрисовка мягкого белого свечения (Glow) вокруг прямоугольника
void drawWhiteGlow(CCDrawNode* n, CCPoint o, CCSize s, float r, float intensity) {
    if (!mm::on(mm::F::Glow) || intensity <= 0.01f) return;
    for (int layer = 1; layer <= 3; layer++) {
        float expand = layer * 2.2f;
        float alpha = (0.09f / layer) * intensity;
        drawRoundRect(n, ccp(o.x - expand, o.y - expand),
                      CCSize(s.width + expand * 2.f, s.height + expand * 2.f),
                      r + expand, P({255, 255, 255}, alpha));
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

    m_glowNode = CCDrawNode::create();
    m_mainLayer->addChild(m_glowNode, -15);

    m_bgNode = CCDrawNode::create();
    m_mainLayer->addChild(m_bgNode, -10);

    m_particleNode = CCDrawNode::create();
    m_mainLayer->addChild(m_particleNode, -5);

    m_contentHost = CCNode::create();
    m_contentHost->setContentSize(m_size);
    m_mainLayer->addChild(m_contentHost, 5);

    m_subPopupHost = CCNode::create();
    m_subPopupHost->setContentSize(m_size);
    m_mainLayer->addChild(m_subPopupHost, 50);

    // ----- Верхняя левая шапка в стиле Nursultan: иконка L + Название + кнопка "?" -----
    auto logoBadge = CCDrawNode::create();
    drawRoundRect(logoBadge, ccp(12.f, WIN_H - 36.f), CCSize(22.f, 22.f), 6.f, P({12, 12, 16}, 1.f), 1.f, P(mm::accent(), 0.9f));
    m_mainLayer->addChild(logoBadge, 2);

    auto logoL = CCLabelBMFont::create("L", "bigFont.fnt");
    logoL->setScale(0.38f);
    logoL->setPosition({23.f, WIN_H - 25.f});
    m_mainLayer->addChild(logoL, 3);

    auto brandLbl = CCLabelBMFont::create("L-CLIENT", "chatFont.fnt");
    brandLbl->setScale(0.82f);
    brandLbl->setAnchorPoint({0.f, 0.5f});
    brandLbl->setPosition({39.f, WIN_H - 25.f});
    m_mainLayer->addChild(brandLbl, 3);

    // Маленькая кнопка "?" возле названия мода (показывает описание мода)
    {
        auto qNode = CCNode::create();
        qNode->setContentSize({18.f, 18.f});
        auto qDraw = CCDrawNode::create();
        drawRoundRect(qDraw, ccp(1.f, 1.f), CCSize(16.f, 16.f), 8.f, P({255, 255, 255}, 0.12f));
        qNode->addChild(qDraw);
        auto qLbl = CCLabelBMFont::create("?", "chatFont.fnt");
        qLbl->setScale(0.75f);
        qLbl->setPosition({9.f, 9.f});
        qNode->addChild(qLbl);

        auto qBtn = CCMenuItemSpriteExtra::create(qNode, this, menu_selector(ModMenuPopup::onModInfo));
        qBtn->setPosition({92.f, WIN_H - 25.f});
        m_buttonMenu->addChild(qBtn, 10);
    }

    // Кнопка закрытия справа вверху
    {
        auto cNode = CCNode::create();
        cNode->setContentSize({24.f, 24.f});
        auto cDraw = CCDrawNode::create();
        drawRoundRect(cDraw, ccp(2.f, 2.f), CCSize(20.f, 20.f), 6.f, P({255, 255, 255}, 0.08f));
        cDraw->drawSegment(ccp(8.f, 8.f), ccp(16.f, 16.f), 0.9f, P({220, 220, 230}, 1.f));
        cDraw->drawSegment(ccp(8.f, 16.f), ccp(16.f, 8.f), 0.9f, P({220, 220, 230}, 1.f));
        cNode->addChild(cDraw);

        auto cBtn = CCMenuItemSpriteExtra::create(cNode, this, menu_selector(ModMenuPopup::onClosePopup));
        cBtn->setPosition({WIN_W - 18.f, WIN_H - 18.f});
        m_buttonMenu->addChild(cBtn, 10);
    }

    this->initParticles();
    this->redrawFrame();
    this->rebuildContent();
    this->scheduleUpdate();
    return true;
}

void ModMenuPopup::initParticles() {
    m_particles.clear();
    m_particles.resize(26); // 26 частиц — плавно и без просадок на мобильных устройствах
    for (int i = 0; i < 26; i++) {
        m_particles[i] = {
            ccp(std::rand() % static_cast<int>(WIN_W), std::rand() % static_cast<int>(WIN_H)),
            ccp(((std::rand() % 100) / 100.f - 0.5f) * 18.f, -18.f - (std::rand() % 22)),
            2.2f + (std::rand() % 30) / 10.f,
            static_cast<float>(std::rand() % 360),
            ((std::rand() % 100) / 100.f - 0.5f) * 3.f,
            0.35f + (std::rand() % 45) / 100.f
        };
    }
}

void ModMenuPopup::update(float dt) {
    m_particleNode->clear();
    int pType = mm::state().particleType;
    if (pType <= 0) return;

    // Не зависим от Speedhack уровня, чтобы партиклы в меню всегда двигались плавно
    float realDt = std::clamp(dt / std::max(mm::speed(), 0.25f), 0.002f, 0.05f);

    for (auto& p : m_particles) {
        p.pos.x += p.vel.x * realDt;
        p.pos.y += p.vel.y * realDt;
        p.angle += p.spin * realDt;

        if (p.pos.y < 8.f) {
            p.pos.y = WIN_H - 8.f;
            p.pos.x = 8.f + (std::rand() % static_cast<int>(WIN_W - 16.f));
        }
        if (p.pos.x < 8.f) p.pos.x = WIN_W - 10.f;
        if (p.pos.x > WIN_W - 8.f) p.pos.x = 10.f;

        if (pType == 1) {
            // Winter: мягкие снежинки
            drawPolyCircle(m_particleNode, p.pos, p.size * 0.75f, P({240, 248, 255}, p.alpha), 8);
        } else if (pType == 2) {
            // Autumn: тёплые осенние листья (ромбики с вращением)
            ccColor3B leafCol = (static_cast<int>(p.size * 10) % 2 == 0) ? ccColor3B{235, 125, 45} : ccColor3B{215, 75, 40};
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
            // Sakura: нежно-розовые лепестки сакуры
            ccColor3B sakCol = {255, 165, 205};
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

    // Белое свечение вокруг главного окна (только в меню!)
    drawWhiteGlow(m_glowNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), 14.f, glow);

    // Главный фон в стиле Nursultan
    drawRoundRect(m_bgNode, ccp(0.f, 0.f), CCSize(WIN_W, WIN_H), 14.f, P(bg, 0.98f), 1.f, P({255, 255, 255}, 0.10f));

    // Левая боковая панель (Sidebar)
    drawRoundRect(m_bgNode, ccp(4.f, 4.f), CCSize(SIDE_W - 4.f, WIN_H - 8.f), 11.f, P(card, 0.75f));

    // Верхняя строка хлебных крошек (как "wildclient.org > Visuals" на скрине)
    drawRoundRect(m_bgNode, ccp(SIDE_W + 8.f, WIN_H - 32.f), CCSize(WIN_W - SIDE_W - 40.f, 22.f), 7.f, P(card, 0.85f));
}

void ModMenuPopup::animatePress(CCNode* target) {
    if (!target) return;
    int type = mm::state().pressAnimType;
    if (type == 0) return;

    float pwr = mm::state().pressAnimPower;
    target->stopAllActions();
    target->setScale(1.f);
    if (type == 1) {
        // Пружинящий Bounce
        target->runAction(CCSequence::create(
            CCEaseSineOut::create(CCScaleTo::create(0.06f, pwr)),
            CCEaseBackOut::create(CCScaleTo::create(0.14f, 1.0f)),
            nullptr
        ));
    } else {
        // Мягкое нажатие Smooth Shrink
        target->runAction(CCSequence::create(
            CCScaleTo::create(0.05f, pwr),
            CCEaseExponentialOut::create(CCScaleTo::create(0.12f, 1.0f)),
            nullptr
        ));
    }
}

void ModMenuPopup::addModuleCard(int col, int row, mm::F flag, bool hasSubOptions) {
    float cardW = 166.f;
    float cardH = 38.f;
    float x = SIDE_W + 10.f + col * (cardW + 8.f);
    float y = WIN_H - 78.f - row * (cardH + 6.f);

    bool enabled = mm::on(flag);
    auto cardCol = mm::themeCard();
    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    auto d = CCDrawNode::create();
    if (enabled && mm::on(mm::F::Glow)) {
        drawWhiteGlow(d, ccp(x, y), CCSize(cardW, cardH), 8.f, mm::state().glowIntensity * 0.55f);
    }
    drawRoundRect(d, ccp(x, y), CCSize(cardW, cardH), 8.f, P(cardCol, 0.95f),
                  1.f, enabled ? P(acc, 0.55f) : P({255, 255, 255}, 0.05f));
    m_contentHost->addChild(d, 1);

    // Заголовок функции
    auto tLbl = CCLabelBMFont::create(mm::title(flag), "chatFont.fnt");
    tLbl->setScale(0.78f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x + 8.f, y + cardH - 12.f});
    tLbl->setColor(enabled ? acc : txtCol);
    m_contentHost->addChild(tLbl, 2);

    // Бейдж RISKY (как в Nursultan/WildClient)
    if (mm::isRisky(flag)) {
        auto rLbl = CCLabelBMFont::create("RISKY", "chatFont.fnt");
        rLbl->setScale(0.45f);
        rLbl->setColor({255, 165, 70});
        rLbl->setAnchorPoint({0.f, 0.5f});
        rLbl->setPosition({x + 12.f + tLbl->getContentSize().width * 0.78f, y + cardH - 12.f});
        m_contentHost->addChild(rLbl, 2);
    }

    // Описание функции под названием
    auto dLbl = CCLabelBMFont::create(mm::desc(flag), "chatFont.fnt");
    float maxDescW = cardW - (hasSubOptions ? 64.f : 44.f);
    float sc = 0.52f;
    if (dLbl->getContentSize().width * sc > maxDescW) {
        sc = maxDescW / std::max(1.f, dLbl->getContentSize().width);
    }
    dLbl->setScale(sc);
    dLbl->setAnchorPoint({0.f, 0.5f});
    dLbl->setPosition({x + 8.f, y + 10.f});
    dLbl->setColor({145, 150, 165});
    m_contentHost->addChild(dLbl, 2);

    // Три точки "..." возле ползунка (открывают дополнительные настройки функции)
    if (hasSubOptions) {
        auto dotsNode = CCNode::create();
        dotsNode->setContentSize({20.f, 20.f});
        auto dotsDraw = CCDrawNode::create();
        for (int i = 0; i < 3; i++) {
            drawPolyCircle(dotsDraw, ccp(5.f + i * 5.f, 10.f), 1.3f, P({200, 205, 220}, 0.9f), 8);
        }
        dotsNode->addChild(dotsDraw);
        auto dotsBtn = CCMenuItemSpriteExtra::create(dotsNode, this, menu_selector(ModMenuPopup::onOpenSub));
        dotsBtn->setTag(static_cast<int>(flag));
        dotsBtn->setPosition({x + cardW - 42.f, y + cardH * 0.5f});
        m_contentHost->addChild(dotsBtn, 5);
    }

    // Тумблер-переключатель в стиле Nursultan
    auto swNode = CCNode::create();
    swNode->setContentSize({28.f, 16.f});
    auto swDraw = CCDrawNode::create();
    if (enabled && mm::on(mm::F::Glow)) {
        drawWhiteGlow(swDraw, ccp(2.f, 2.f), CCSize(24.f, 12.f), 6.f, mm::state().glowIntensity * 0.8f);
    }
    drawRoundRect(swDraw, ccp(2.f, 2.f), CCSize(24.f, 12.f), 6.f,
                  enabled ? P(acc, 1.f) : P({52, 54, 66}, 1.f));
    float knobX = enabled ? 20.f : 8.f;
    drawPolyCircle(swDraw, ccp(knobX, 8.f), 4.5f, P({255, 255, 255}, 1.f), 12);
    swNode->addChild(swDraw);

    auto swBtn = CCMenuItemSpriteExtra::create(swNode, this, menu_selector(ModMenuPopup::onToggle));
    swBtn->setTag(static_cast<int>(flag));
    swBtn->setPosition({x + cardW - 16.f, y + cardH * 0.5f});

    auto cardMenu = CCMenu::create();
    cardMenu->setPosition({0.f, 0.f});
    cardMenu->addChild(swBtn);
    if (hasSubOptions) {
        // Переносим кнопку "..." в активное CCMenu
        auto dotsNode = CCNode::create();
        dotsNode->setContentSize({22.f, 22.f});
        auto dotsDraw = CCDrawNode::create();
        drawRoundRect(dotsDraw, ccp(1.f, 4.f), CCSize(20.f, 14.f), 4.f, P({255, 255, 255}, 0.06f));
        for (int i = 0; i < 3; i++) {
            drawPolyCircle(dotsDraw, ccp(6.f + i * 5.f, 11.f), 1.3f, P({215, 220, 235}, 0.95f), 8);
        }
        dotsNode->addChild(dotsDraw);
        auto dotsBtn = CCMenuItemSpriteExtra::create(dotsNode, this, menu_selector(ModMenuPopup::onOpenSub));
        dotsBtn->setTag(static_cast<int>(flag));
        dotsBtn->setPosition({x + cardW - 44.f, y + cardH * 0.5f});
        cardMenu->addChild(dotsBtn);
    }
    m_contentHost->addChild(cardMenu, 6);
}

void ModMenuPopup::addActionCard(int col, int row, char const* titleText, char const* subText, SEL_MenuHandler cb) {
    float cardW = 166.f;
    float cardH = 38.f;
    float x = SIDE_W + 10.f + col * (cardW + 8.f);
    float y = WIN_H - 78.f - row * (cardH + 6.f);

    auto node = CCNode::create();
    node->setContentSize({cardW, cardH});
    auto d = CCDrawNode::create();
    drawRoundRect(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 8.f, P(mm::themeCard(), 0.95f), 1.f, P(mm::accent(), 0.5f));
    node->addChild(d);

    auto tLbl = CCLabelBMFont::create(titleText, "chatFont.fnt");
    tLbl->setScale(0.8f);
    tLbl->setColor(mm::accent());
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({10.f, cardH - 12.f});
    node->addChild(tLbl);

    auto sLbl = CCLabelBMFont::create(subText, "chatFont.fnt");
    sLbl->setScale(0.52f);
    sLbl->setColor({150, 155, 170});
    sLbl->setAnchorPoint({0.f, 0.5f});
    sLbl->setPosition({10.f, 10.f});
    node->addChild(sLbl);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    auto btn = CCMenuItemSpriteExtra::create(node, this, cb);
    btn->setPosition({x + cardW * 0.5f, y + cardH * 0.5f});
    menu->addChild(btn);
    m_contentHost->addChild(menu, 6);
}

void ModMenuPopup::rebuildContent() {
    m_contentHost->removeAllChildren();
    this->redrawFrame();

    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    // Навигационный путь сверху ("l-client.gd > Category")
    auto pathLbl = CCLabelBMFont::create(fmt::format("l-client.gd  >  {}", TAB_TITLES[m_currentTab]).c_str(), "chatFont.fnt");
    pathLbl->setScale(0.75f);
    pathLbl->setAnchorPoint({0.f, 0.5f});
    pathLbl->setPosition({SIDE_W + 16.f, WIN_H - 21.f});
    pathLbl->setColor(txtCol);
    m_contentHost->addChild(pathLbl, 3);

    // Статус Safe Mode в верхней полоске
    auto safeLbl = CCLabelBMFont::create(mm::safeActive() ? "SAFE MODE: ON" : "SAFE MODE: OFF", "chatFont.fnt");
    safeLbl->setScale(0.65f);
    safeLbl->setAnchorPoint({1.f, 0.5f});
    safeLbl->setPosition({WIN_W - 40.f, WIN_H - 21.f});
    safeLbl->setColor(mm::safeActive() ? ccColor3B{95, 230, 145} : ccColor3B{160, 165, 180});
    m_contentHost->addChild(safeLbl, 3);

    // Кнопки бокового меню (Sidebar)
    auto sideMenu = CCMenu::create();
    sideMenu->setPosition({0.f, 0.f});
    for (int i = 0; i < TAB_COUNT; i++) {
        bool active = (i == m_currentTab);
        float by = WIN_H - 68.f - i * 34.f;

        auto bNode = CCNode::create();
        bNode->setContentSize({SIDE_W - 14.f, 26.f});
        auto bDraw = CCDrawNode::create();
        if (active) {
            if (mm::on(mm::F::Glow)) {
                drawWhiteGlow(bDraw, ccp(0.f, 0.f), CCSize(SIDE_W - 14.f, 26.f), 7.f, mm::state().glowIntensity * 0.65f);
            }
            drawRoundRect(bDraw, ccp(0.f, 0.f), CCSize(SIDE_W - 14.f, 26.f), 7.f, P(acc, 0.22f), 1.f, P(acc, 0.85f));
        }
        bNode->addChild(bDraw);

        auto bLbl = CCLabelBMFont::create(TAB_TITLES[i], "chatFont.fnt");
        bLbl->setScale(0.82f);
        bLbl->setAnchorPoint({0.f, 0.5f});
        bLbl->setPosition({12.f, 13.f});
        bLbl->setColor(active ? acc : ccColor3B{150, 155, 170});
        bNode->addChild(bLbl);

        auto btn = CCMenuItemSpriteExtra::create(bNode, this, menu_selector(ModMenuPopup::onTab));
        btn->setTag(i);
        btn->setPosition({SIDE_W * 0.5f, by});
        sideMenu->addChild(btn);
    }
    m_contentHost->addChild(sideMenu, 10);

    // ----- Содержимое вкладок -----
    if (m_currentTab == 0) {
        // PLAYER
        this->addModuleCard(0, 0, mm::F::Noclip, true);    // "..." открывает настройку Speedhack рядом
        this->addModuleCard(1, 0, mm::F::Safe, false);
        this->addModuleCard(0, 1, mm::F::Jump, false);
        this->addModuleCard(1, 1, mm::F::AutoSafe, false);

        // Панель выбора скорости (Speedhack)
        float sx = SIDE_W + 10.f, sy = WIN_H - 78.f - 2 * 44.f;
        auto spDraw = CCDrawNode::create();
        drawRoundRect(spDraw, ccp(sx, sy), CCSize(340.f, 38.f), 8.f, P(mm::themeCard(), 0.95f));
        m_contentHost->addChild(spDraw, 1);

        auto spTitle = CCLabelBMFont::create(fmt::format("Speedhack: x{}", mm::speed()).c_str(), "chatFont.fnt");
        spTitle->setScale(0.78f);
        spTitle->setAnchorPoint({0.f, 0.5f});
        spTitle->setPosition({sx + 10.f, sy + 19.f});
        spTitle->setColor(txtCol);
        m_contentHost->addChild(spTitle, 2);

        auto spMenu = CCMenu::create();
        spMenu->setPosition({0.f, 0.f});
        for (int i = 0; i < static_cast<int>(mm::SPEEDS.size()); i++) {
            bool sel = (mm::state().speedIdx == i);
            auto pNode = CCNode::create();
            pNode->setContentSize({34.f, 20.f});
            auto pDraw = CCDrawNode::create();
            drawRoundRect(pDraw, ccp(0.f, 0.f), CCSize(34.f, 20.f), 5.f, sel ? P(acc, 1.f) : P({48, 50, 62}, 1.f));
            pNode->addChild(pDraw);
            auto pLbl = CCLabelBMFont::create(fmt::format("x{}", mm::SPEEDS[i]).c_str(), "chatFont.fnt");
            pLbl->setScale(0.62f);
            pLbl->setPosition({17.f, 10.f});
            pLbl->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            pNode->addChild(pLbl);

            auto btn = CCMenuItemSpriteExtra::create(pNode, this, menu_selector(ModMenuPopup::onSpeedPill));
            btn->setTag(i);
            btn->setPosition({sx + 132.f + i * 36.f, sy + 19.f});
            spMenu->addChild(btn);
        }
        m_contentHost->addChild(spMenu, 6);
    }
    else if (m_currentTab == 1) {
        // LEVEL (с кастомным TPS Bypass и кнопкой "...")
        this->addModuleCard(0, 0, mm::F::TpsBypass, true);  // "..." открывает ввод своего TPS
        this->addModuleCard(1, 0, mm::F::FrameSmooth, false);
        this->addModuleCard(0, 1, mm::F::StartPos, false);
        this->addModuleCard(1, 1, mm::F::SmartStart, false);
        this->addModuleCard(0, 2, mm::F::Hitboxes, false);
        this->addModuleCard(1, 2, mm::F::NoDeath, false);
        this->addActionCard(0, 3, "Restart Level", "Мгновенный перезапуск уровня с 0%", menu_selector(ModMenuPopup::onRestartLevel));
    }
    else if (m_currentTab == 2) {
        // VISUALS (Overlay с кнопкой "..." для детальной настройки)
        this->addModuleCard(0, 0, mm::F::Overlay, true);
        this->addModuleCard(1, 0, mm::F::ShowFPS, false);
        this->addModuleCard(0, 1, mm::F::NoShake, false);
        this->addModuleCard(1, 1, mm::F::NoMirror, false);
        this->addModuleCard(0, 2, mm::F::HidePlayer, false);
        this->addModuleCard(1, 2, mm::F::HideAttempts, false);
        this->addModuleCard(0, 3, mm::F::RainbowIcon, false);
        this->addModuleCard(1, 3, mm::F::RainbowBG, false);
        this->addModuleCard(0, 4, mm::F::UnlockAll, false);
    }
    else if (m_currentTab == 3) {
        // THEMES (пресеты Nursultan / Чёрно-Белая / Сакура + своя палитра + Glow)
        this->addModuleCard(0, 0, mm::F::Glow, true); // "..." настраивает силу белого свечения

        auto tMenu = CCMenu::create();
        tMenu->setPosition({0.f, 0.f});
        for (int i = 0; i < 8; i++) {
            int col = i % 2;
            int row = 1 + (i / 2);
            float cardW = 166.f, cardH = 30.f;
            float x = SIDE_W + 10.f + col * (cardW + 8.f);
            float y = WIN_H - 78.f - row * (cardH + 6.f);

            bool sel = (mm::state().themeIdx == i);
            auto node = CCNode::create();
            node->setContentSize({cardW, cardH});
            auto d = CCDrawNode::create();
            drawRoundRect(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 7.f,
                          P(mm::THEMES[i].card, 0.95f), 1.f,
                          sel ? P(acc, 0.95f) : P({255, 255, 255}, 0.08f));
            // Превью цвета темы справа (как в окне Themes на скрине)
            ccColor3B swCol = (i == 7) ? mm::accent() : mm::THEMES[i].accent;
            drawRoundRect(d, ccp(cardW - 34.f, 6.f), CCSize(26.f, 18.f), 5.f, P(swCol, 1.f));
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
            tMenu->addChild(btn);
        }
        m_contentHost->addChild(tMenu, 6);
    }
    else if (m_currentTab == 4) {
        // CLIENT (Партиклы меню, Кнопка "L", Анимация нажатий)
        float sx = SIDE_W + 10.f;
        auto cMenu = CCMenu::create();
        cMenu->setPosition({0.f, 0.f});

        // 1. Выбор партиклов в меню (Off / Winter / Autumn / Sakura)
        float py = WIN_H - 78.f;
        auto pDraw = CCDrawNode::create();
        drawRoundRect(pDraw, ccp(sx, py), CCSize(340.f, 38.f), 8.f, P(mm::themeCard(), 0.95f));
        m_contentHost->addChild(pDraw, 1);

        auto pLbl = CCLabelBMFont::create("Menu Particles:", "chatFont.fnt");
        pLbl->setScale(0.75f);
        pLbl->setAnchorPoint({0.f, 0.5f});
        pLbl->setPosition({sx + 10.f, py + 19.f});
        m_contentHost->addChild(pLbl, 2);

        char const* pNames[4] = {"Off", "Winter", "Autumn", "Sakura"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().particleType == i);
            auto n = CCNode::create();
            n->setContentSize({50.f, 22.f});
            auto d = CCDrawNode::create();
            drawRoundRect(d, ccp(0.f, 0.f), CCSize(50.f, 22.f), 5.f, sel ? P(acc, 1.f) : P({48, 50, 62}, 1.f));
            n->addChild(d);
            auto l = CCLabelBMFont::create(pNames[i], "chatFont.fnt");
            l->setScale(0.62f);
            l->setPosition({25.f, 11.f});
            l->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onParticleSelect));
            b->setTag(i);
            b->setPosition({sx + 145.f + i * 54.f, py + 19.f});
            cMenu->addChild(b);
        }

        // 2. Анимация нажатия кнопок
        float ay = py - 44.f;
        auto aDraw = CCDrawNode::create();
        drawRoundRect(aDraw, ccp(sx, ay), CCSize(340.f, 38.f), 8.f, P(mm::themeCard(), 0.95f));
        m_contentHost->addChild(aDraw, 1);

        auto aLbl = CCLabelBMFont::create("Click Animation:", "chatFont.fnt");
        aLbl->setScale(0.75f);
        aLbl->setAnchorPoint({0.f, 0.5f});
        aLbl->setPosition({sx + 10.f, ay + 19.f});
        m_contentHost->addChild(aLbl, 2);

        char const* aNames[3] = {"Off", "Bounce", "Shrink"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().pressAnimType == i);
            auto n = CCNode::create();
            n->setContentSize({58.f, 22.f});
            auto d = CCDrawNode::create();
            drawRoundRect(d, ccp(0.f, 0.f), CCSize(58.f, 22.f), 5.f, sel ? P(acc, 1.f) : P({48, 50, 62}, 1.f));
            n->addChild(d);
            auto l = CCLabelBMFont::create(aNames[i], "chatFont.fnt");
            l->setScale(0.62f);
            l->setPosition({29.f, 11.f});
            l->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onAnimTypeSelect));
            b->setTag(i);
            b->setPosition({sx + 165.f + i * 64.f, ay + 19.f});
            cMenu->addChild(b);
        }

        m_contentHost->addChild(cMenu, 6);

        // 3. Настройки плавающей кнопки "L"
        this->addModuleCard(0, 2, mm::F::BtnHideInGame, true); // "..." открывает масштаб и прозрачность кнопки L
        this->addModuleCard(1, 2, mm::F::BtnShowInPause, false);
    }

    this->rebuildSubPopup();
}

// ----- Всплывающее окно дополнительных настроек по нажатию на "..." -----
void ModMenuPopup::rebuildSubPopup() {
    m_subPopupHost->removeAllChildren();
    if (!m_hasSubSettings || m_activeSubFeature == mm::F::COUNT) return;

    auto acc = mm::accent();
    auto txtCol = mm::themeText();

    // Затемнение подложки и карточка настроек
    auto d = CCDrawNode::create();
    drawRoundRect(d, ccp(SIDE_W + 6.f, 8.f), CCSize(WIN_W - SIDE_W - 12.f, WIN_H - 44.f), 10.f,
                  P(mm::themeBg(), 0.97f), 1.f, P(acc, 0.8f));
    m_subPopupHost->addChild(d, 1);

    auto subMenu = CCMenu::create();
    subMenu->setPosition({0.f, 0.f});

    // Кнопка "Назад"
    auto backNode = CCNode::create();
    backNode->setContentSize({56.f, 22.f});
    auto backDraw = CCDrawNode::create();
    drawRoundRect(backDraw, ccp(0.f, 0.f), CCSize(56.f, 22.f), 6.f, P(acc, 0.25f), 1.f, P(acc, 0.9f));
    backNode->addChild(backDraw);
    auto backLbl = CCLabelBMFont::create("< Back", "chatFont.fnt");
    backLbl->setScale(0.7f);
    backLbl->setPosition({28.f, 11.f});
    backNode->addChild(backLbl);
    auto backBtn = CCMenuItemSpriteExtra::create(backNode, this, menu_selector(ModMenuPopup::onCloseSub));
    backBtn->setPosition({SIDE_W + 42.f, WIN_H - 54.f});
    subMenu->addChild(backBtn);

    auto makeStepBtn = [&](char const* txt, CCPoint pos, int tag, SEL_MenuHandler sel) {
        auto n = CCNode::create();
        n->setContentSize({44.f, 24.f});
        auto dr = CCDrawNode::create();
        drawRoundRect(dr, ccp(0.f, 0.f), CCSize(44.f, 24.f), 6.f, P(mm::themeCard(), 1.f), 1.f, P(acc, 0.6f));
        n->addChild(dr);
        auto lb = CCLabelBMFont::create(txt, "chatFont.fnt");
        lb->setScale(0.72f);
        lb->setPosition({22.f, 12.f});
        n->addChild(lb);
        auto b = CCMenuItemSpriteExtra::create(n, this, sel);
        b->setTag(tag);
        b->setPosition(pos);
        subMenu->addChild(b);
    };

    if (m_activeSubFeature == mm::F::TpsBypass) {
        // Пользовательская настройка любого значения TPS
        auto t = CCLabelBMFont::create(fmt::format("Custom Physics TPS: {} Hz", mm::state().customTps).c_str(), "bigFont.fnt");
        t->setScale(0.38f);
        t->setPosition({SIDE_W + 175.f, 175.f});
        m_subPopupHost->addChild(t, 2);

        auto info = CCLabelBMFont::create("Установи абсолютно любое значение TPS (от 60 до 2400)", "chatFont.fnt");
        info->setScale(0.68f);
        info->setColor({160, 165, 180});
        info->setPosition({SIDE_W + 175.f, 145.f});
        m_subPopupHost->addChild(info, 2);

        makeStepBtn("-100", {SIDE_W + 55.f, 105.f}, -100, menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("-10",  {SIDE_W + 105.f, 105.f}, -10, menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("-1",   {SIDE_W + 155.f, 105.f}, -1,  menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("+1",   {SIDE_W + 205.f, 105.f}, 1,   menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("+10",  {SIDE_W + 255.f, 105.f}, 10,  menu_selector(ModMenuPopup::onTpsStep));
        makeStepBtn("+100", {SIDE_W + 305.f, 105.f}, 100, menu_selector(ModMenuPopup::onTpsStep));
    }
    else if (m_activeSubFeature == mm::F::Overlay) {
        // Детальные настройки Overlay
        auto t = CCLabelBMFont::create("Overlay Customization", "chatFont.fnt");
        t->setScale(0.88f);
        t->setColor(acc);
        t->setPosition({SIDE_W + 180.f, WIN_H - 54.f});
        m_subPopupHost->addChild(t, 2);

        mm::F ovFlags[6] = {mm::F::OvPercent, mm::F::OvCPS, mm::F::OvTps, mm::F::OvAttempts, mm::F::OvStatus, mm::F::OvBg};
        char const* ovNames[6] = {"Show Percent %", "Show CPS", "Show TPS", "Show Attempts", "Show Cheat/Safe", "Dark Pill Background"};

        for (int i = 0; i < 6; i++) {
            int c = i % 2, r = i / 2;
            float bx = SIDE_W + 24.f + c * 162.f;
            float by = 165.f - r * 34.f;
            bool onFlag = mm::on(ovFlags[i]);

            auto n = CCNode::create();
            n->setContentSize({150.f, 26.f});
            auto dr = CCDrawNode::create();
            drawRoundRect(dr, ccp(0.f, 0.f), CCSize(150.f, 26.f), 6.f,
                          onFlag ? P(acc, 0.25f) : P(mm::themeCard(), 1.f), 1.f,
                          onFlag ? P(acc, 0.9f) : P({255, 255, 255}, 0.08f));
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(ovNames[i], "chatFont.fnt");
            lb->setScale(0.65f);
            lb->setPosition({75.f, 13.f});
            lb->setColor(onFlag ? acc : txtCol);
            n->addChild(lb);

            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onToggle));
            b->setTag(static_cast<int>(ovFlags[i]));
            b->setPosition({bx + 75.f, by + 13.f});
            subMenu->addChild(b);
        }

        // Выбор угла экрана для Overlay
        char const* corners[4] = {"Top-L", "Top-R", "Bot-L", "Bot-R"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().ovCorner == i);
            auto n = CCNode::create();
            n->setContentSize({68.f, 22.f});
            auto dr = CCDrawNode::create();
            drawRoundRect(dr, ccp(0.f, 0.f), CCSize(68.f, 22.f), 5.f, sel ? P(acc, 1.f) : P(mm::themeCard(), 1.f));
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(corners[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({34.f, 11.f});
            lb->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onOvCornerSelect));
            b->setTag(i);
            b->setPosition({SIDE_W + 56.f + i * 76.f, 44.f});
            subMenu->addChild(b);
        }
    }
    else if (m_activeSubFeature == mm::F::Glow) {
        // Настройка силы белого свечения + Палитра своей темы
        auto t = CCLabelBMFont::create(fmt::format("White Glow Intensity: {:.0f}%", mm::state().glowIntensity * 100.f).c_str(), "chatFont.fnt");
        t->setScale(0.85f);
        t->setPosition({SIDE_W + 175.f, 195.f});
        m_subPopupHost->addChild(t, 2);

        makeStepBtn("-15%", {SIDE_W + 135.f, 168.f}, -15, menu_selector(ModMenuPopup::onGlowStep));
        makeStepBtn("+15%", {SIDE_W + 215.f, 168.f}, 15,  menu_selector(ModMenuPopup::onGlowStep));

        // Конструктор своей палитры (Accent / Background / Cards)
        char const* slots[3] = {"Accent Color", "Background", "Cards"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().editSlot == i);
            auto n = CCNode::create();
            n->setContentSize({96.f, 22.f});
            auto dr = CCDrawNode::create();
            drawRoundRect(dr, ccp(0.f, 0.f), CCSize(96.f, 22.f), 5.f, sel ? P(acc, 1.f) : P(mm::themeCard(), 1.f));
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(slots[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({48.f, 11.f});
            lb->setColor(sel ? ccColor3B{15, 15, 20} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onColorSlotSelect));
            b->setTag(i);
            b->setPosition({SIDE_W + 70.f + i * 104.f, 125.f});
            subMenu->addChild(b);
        }

        makeStepBtn("Hue -", {SIDE_W + 70.f, 82.f},  -1, menu_selector(ModMenuPopup::onHueStep));
        makeStepBtn("Hue +", {SIDE_W + 120.f, 82.f}, 1,  menu_selector(ModMenuPopup::onHueStep));
        makeStepBtn("Sat -", {SIDE_W + 175.f, 82.f}, -1, menu_selector(ModMenuPopup::onSatStep));
        makeStepBtn("Sat +", {SIDE_W + 225.f, 82.f}, 1,  menu_selector(ModMenuPopup::onSatStep));
        makeStepBtn("Val -", {SIDE_W + 280.f, 82.f}, -1, menu_selector(ModMenuPopup::onValStep));
        makeStepBtn("Val +", {SIDE_W + 325.f, 82.f}, 1,  menu_selector(ModMenuPopup::onValStep));
    }
    else if (m_activeSubFeature == mm::F::BtnHideInGame) {
        // Настройки плавающей чёрной кнопки "L"
        auto t = CCLabelBMFont::create(
            fmt::format("L-Button Scale: {:.0f}%   |   Opacity: {:.0f}%",
                        mm::state().btnScale * 100.f, mm::state().btnOpacity * 100.f).c_str(),
            "chatFont.fnt");
        t->setScale(0.8f);
        t->setPosition({SIDE_W + 175.f, 165.f});
        m_subPopupHost->addChild(t, 2);

        makeStepBtn("Size -", {SIDE_W + 95.f, 115.f},  -10, menu_selector(ModMenuPopup::onBtnScaleStep));
        makeStepBtn("Size +", {SIDE_W + 150.f, 115.f}, 10,  menu_selector(ModMenuPopup::onBtnScaleStep));
        makeStepBtn("Alpha -", {SIDE_W + 215.f, 115.f}, -10, menu_selector(ModMenuPopup::onBtnOpacityStep));
        makeStepBtn("Alpha +", {SIDE_W + 270.f, 115.f}, 10,  menu_selector(ModMenuPopup::onBtnOpacityStep));
    }

    m_subPopupHost->addChild(subMenu, 10);
}

// ============================ Колбэки ============================

void ModMenuPopup::onTab(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    m_currentTab = node->getTag();
    m_hasSubSettings = false;
    this->rebuildContent();
}

void ModMenuPopup::onToggle(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    auto flag = static_cast<mm::F>(node->getTag());
    mm::set(flag, !mm::on(flag));
    mm::save(flag);

    if (flag == mm::F::Hitboxes) mm::applyHitboxes();
    if (flag == mm::F::NoShake) mm::applyNoShake();
    this->rebuildContent();
}

void ModMenuPopup::onOpenSub(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    this->animatePress(node);
    m_activeSubFeature = static_cast<mm::F>(node->getTag());
    m_hasSubSettings = true;
    this->rebuildSubPopup();
}

void ModMenuPopup::onCloseSub(CCObject*) {
    m_hasSubSettings = false;
    this->rebuildSubPopup();
}

void ModMenuPopup::onModInfo(CCObject* sender) {
    this->animatePress(static_cast<CCNode*>(sender));
    FLAlertLayer::create(
        "About L-Client Mod Menu",
        "<cy>L-Client</c> — современное и оптимизированное мод-меню для Geometry Dash 2.2081.\n"
        "<cg>Player:</c> Noclip, Jump Hack, Auto Safe Mode и Speedhack.\n"
        "<cg>Level:</c> Честный TPS Bypass (до 2400 Гц), Frame Smoothing без ускорения и умный StartPos Switcher.\n"
        "<cg>Visuals:</c> Настраиваемый Overlay, точный FPS, No Shake и косметика.\n"
        "<cg>Themes & Client:</c> Темы в стиле Nursultan, партиклы и плавающая кнопка L.",
        "Понятно"
    )->show();
}

void ModMenuPopup::onRestartLevel(CCObject*) {
    if (auto pl = PlayLayer::get()) {
        this->onClose(nullptr);
        pl->resetLevel();
    } else {
        FLAlertLayer::create("L-Client", "Сначала открой любой уровень!", "OK")->show();
    }
}

void ModMenuPopup::onClosePopup(CCObject*) {
    mm::saveAll();
    this->onClose(nullptr);
}

void ModMenuPopup::onSpeedPill(CCObject* sender) {
    mm::state().speedIdx = static_cast<CCNode*>(sender)->getTag();
    mm::applySpeed(PlayLayer::get() != nullptr);
    this->rebuildContent();
}

void ModMenuPopup::onTpsStep(CCObject* sender) {
    int delta = static_cast<CCNode*>(sender)->getTag();
    mm::state().customTps = std::clamp(mm::state().customTps + delta, 60, 2400);
    mm::saveAll();
    this->rebuildSubPopup();
}

void ModMenuPopup::onParticleSelect(CCObject* sender) {
    mm::state().particleType = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
    this->rebuildContent();
}

void ModMenuPopup::onThemeSelect(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    mm::state().themeIdx = idx;
    mm::saveAll();
    if (idx == 7) {
        m_activeSubFeature = mm::F::Glow;
        m_hasSubSettings = true;
    }
    this->rebuildContent();
}

void ModMenuPopup::onAnimTypeSelect(CCObject* sender) {
    mm::state().pressAnimType = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
    this->rebuildContent();
}

void ModMenuPopup::onOvCornerSelect(CCObject* sender) {
    mm::state().ovCorner = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
    this->rebuildSubPopup();
}

void ModMenuPopup::onColorSlotSelect(CCObject* sender) {
    mm::state().editSlot = static_cast<CCNode*>(sender)->getTag();
    mm::state().themeIdx = 7;
    this->rebuildSubPopup();
}

void ModMenuPopup::onHueStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    int sl = mm::state().editSlot;
    mm::state().themeIdx = 7;
    mm::state().customH[sl] = std::fmod(mm::state().customH[sl] + d * 0.05f + 1.f, 1.f);
    mm::saveAll();
    this->rebuildContent();
}

void ModMenuPopup::onSatStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    int sl = mm::state().editSlot;
    mm::state().themeIdx = 7;
    mm::state().customS[sl] = std::clamp(mm::state().customS[sl] + d * 0.08f, 0.f, 1.f);
    mm::saveAll();
    this->rebuildContent();
}

void ModMenuPopup::onValStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    int sl = mm::state().editSlot;
    mm::state().themeIdx = 7;
    mm::state().customV[sl] = std::clamp(mm::state().customV[sl] + d * 0.08f, 0.05f, 1.f);
    mm::saveAll();
    this->rebuildContent();
}

void ModMenuPopup::onGlowStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().glowIntensity = std::clamp(mm::state().glowIntensity + d / 100.f, 0.f, 1.f);
    mm::saveAll();
    this->rebuildContent();
}

void ModMenuPopup::onBtnScaleStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().btnScale = std::clamp(mm::state().btnScale + d / 100.f, 0.6f, 1.5f);
    mm::saveAll();
    this->rebuildSubPopup();
}

void ModMenuPopup::onBtnOpacityStep(CCObject* sender) {
    int d = static_cast<CCNode*>(sender)->getTag();
    mm::state().btnOpacity = std::clamp(mm::state().btnOpacity + d / 100.f, 0.2f, 1.f);
    mm::saveAll();
    this->rebuildSubPopup();
}
