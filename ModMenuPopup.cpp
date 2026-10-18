#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>

namespace {

constexpr float PI = 3.14159265f;

// ----- геометрия главного окна -----
constexpr float W = 420.f, H = 272.f;
constexpr float COLX[2] = {16.f, 218.f};
constexpr float CELL_W = 186.f, CELL_H = 32.f, WIDE_W = 388.f;
constexpr float ROW0 = 182.f, ROW_STEP = 38.f;
constexpr char const* TAB_NAMES[4] = {"Player", "Level", "Visual", "Theme"};
constexpr float TAB_X[4] = {58.f, 134.f, 210.f, 286.f};
constexpr float TAB_Y = 222.f;

// ----- геометрия окна цвета -----
constexpr float CW = 156.f, CH = 176.f, HDR = 26.f;
constexpr float SV_X = 10.f, SV_Y = 56.f, SV_W = 136.f, SV_H = 86.f;
constexpr float HUE_X = 10.f, HUE_Y = 36.f, HUE_W = 136.f, HUE_H = 12.f;

// ----- палитра интерфейса -----
const ccColor3B TEXT_C = {228, 231, 238};
const ccColor3B MUTED_C = {132, 139, 152};
const ccColor3B OFF_C = {62, 67, 78};
const ccColor3B WARN_C = {255, 170, 60};
const ccColor3B GOOD_C = {96, 222, 144};
const ccColor3B WHITE_C = {255, 255, 255};

// CCDrawNode использует premultiplied alpha
ccColor4F P(ccColor3B c, float a) {
    return {c.r / 255.f * a, c.g / 255.f * a, c.b / 255.f * a, a};
}

ccColor3B mix(ccColor3B a, ccColor3B b, float t) {
    return {static_cast<GLubyte>(a.r + (b.r - a.r) * t),
            static_cast<GLubyte>(a.g + (b.g - a.g) * t),
            static_cast<GLubyte>(a.b + (b.b - a.b) * t)};
}

ccColor3B onColor(ccColor3B c) {
    float l = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
    return l > 150.f ? ccColor3B{20, 22, 28} : WHITE_C;
}

// Скруглённый прямоугольник одним полигоном (дёшево и со сглаживанием)
void drawRound(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill,
               float bw = 0.f, ccColor4F bc = ccColor4F{0.f, 0.f, 0.f, 0.f}) {
    r = std::min(r, std::min(s.width, s.height) / 2.f);
    constexpr int SEG = 6;
    std::vector<CCPoint> pts;
    pts.reserve((SEG + 1) * 4);
    auto arc = [&](float cx, float cy, float a0) {
        for (int i = 0; i <= SEG; i++) {
            float a = a0 + (PI / 2.f) * i / SEG;
            pts.push_back(ccp(cx + r * std::cos(a), cy + r * std::sin(a)));
        }
    };
    arc(o.x + s.width - r, o.y + s.height - r, 0.f);
    arc(o.x + r, o.y + s.height - r, PI / 2.f);
    arc(o.x + r, o.y + r, PI);
    arc(o.x + s.width - r, o.y + r, 3.f * PI / 2.f);
    n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), fill, bw, bc);
}

void drawSwitch(CCDrawNode* d, bool on) {
    d->clear();
    drawRound(d, ccp(9.f, 7.f), CCSize(38.f, 20.f), 10.f, P(on ? mm::accent() : OFF_C, 1.f));
    d->drawDot(ccp(on ? 9.f + 38.f - 10.f : 9.f + 10.f, 17.f), 7.5f, P(WHITE_C, 1.f));
}

bool inRect(CCPoint p, float x, float y, float w, float h) {
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
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
    if (!Popup::init(W, H)) return false;

    // прячем стандартный декор Popup: рисуем свой, минималистичный
    m_bgSprite->setVisible(false);
    if (m_title) m_title->setVisible(false);
    if (m_closeBtn) {
        m_closeBtn->setVisible(false);
        m_closeBtn->setEnabled(false);
    }

    // координаты (0,0) = левый нижний угол окна
    m_buttonMenu->setContentSize(m_size);
    m_buttonMenu->setAnchorPoint({0.f, 0.f});
    m_buttonMenu->setPosition({0.f, 0.f});

    m_bgNode = CCDrawNode::create();
    m_mainLayer->addChild(m_bgNode, -10);

    // заголовок
    auto title = CCLabelBMFont::create("Mod Menu", "bigFont.fnt");
    title->setScale(0.42f);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({20.f, H - 22.f});
    title->setColor(TEXT_C);
    m_mainLayer->addChild(title);

    // кнопка закрытия
    {
        auto node = CCNode::create();
        node->setContentSize({32.f, 32.f});
        auto d = CCDrawNode::create();
        d->drawDot(ccp(16.f, 16.f), 11.f, P(WHITE_C, 0.08f));
        d->drawSegment(ccp(11.5f, 11.5f), ccp(20.5f, 20.5f), 0.8f, P(MUTED_C, 1.f));
        d->drawSegment(ccp(11.5f, 20.5f), ccp(20.5f, 11.5f), 0.8f, P(MUTED_C, 1.f));
        node->addChild(d);
        m_closeItem = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onClosePressed));
        m_closeItem->setPosition({W - 24.f, H - 22.f});
        m_buttonMenu->addChild(m_closeItem);
    }

    // индикатор активной вкладки (двигается плавно)
    m_tabInd = CCDrawNode::create();
    m_mainLayer->addChild(m_tabInd, 1);
    m_tabInd->setPosition({TAB_X[0], TAB_Y - 17.f});

    // вкладки
    for (int i = 0; i < TAB_COUNT; i++) {
        auto node = CCNode::create();
        node->setContentSize({76.f, 28.f});
        auto lbl = CCLabelBMFont::create(TAB_NAMES[i], "chatFont.fnt");
        lbl->setScale(1.05f);
        lbl->setPosition({38.f, 14.f});
        node->addChild(lbl);
        auto btn = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onTab));
        btn->setTag(i);
        btn->setPosition({TAB_X[i], TAB_Y});
        m_buttonMenu->addChild(btn);
        m_tabBtns[i] = btn;
        m_tabLabels[i] = lbl;

        m_cellNodes[i] = CCDrawNode::create();
        m_mainLayer->addChild(m_cellNodes[i], -5);
        m_tabNodes[i].push_back(m_cellNodes[i]);
    }

    this->buildPlayer();
    this->buildLevel();
    this->buildVisual();
    this->buildTheme();
    this->buildPicker();

    this->switchTab(0, false);
    this->applyTheme();
    return true;
}

// ============================ построение ============================

CCPoint ModMenuPopup::cellOrigin(int col, int row) const {
    return ccp(COLX[col], ROW0 - row * ROW_STEP - CELL_H / 2.f);
}

void ModMenuPopup::addCellLabel(int tab, char const* text, float x, float y, bool muted) {
    auto lbl = CCLabelBMFont::create(text, "chatFont.fnt");
    lbl->setScale(muted ? 0.85f : 0.95f);
    lbl->setAnchorPoint({0.f, 0.5f});
    lbl->setPosition({x, y});
    lbl->setColor(muted ? MUTED_C : TEXT_C);
    m_mainLayer->addChild(lbl);
    m_tabNodes[tab].push_back(lbl);
}

void ModMenuPopup::addSwitchCell(int tab, mm::F flag, char const* label, int col, int row) {
    CCPoint o = this->cellOrigin(col, row);
    bool cheat = mm::isCheat(flag);
    m_cells[tab].push_back({o, CCSize(CELL_W, CELL_H), cheat});
    this->addCellLabel(tab, label, o.x + (cheat ? 24.f : 14.f), o.y + CELL_H / 2.f);

    auto node = CCNode::create();
    node->setContentSize({56.f, 34.f});
    auto d = CCDrawNode::create();
    node->addChild(d);
    auto item = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onToggle));
    item->setTag(static_cast<int>(flag));
    item->setPosition({o.x + CELL_W - 10.f - 28.f, o.y + CELL_H / 2.f});
    m_buttonMenu->addChild(item);
    m_tabNodes[tab].push_back(item);
    m_redraw.push_back([d, flag] { drawSwitch(d, mm::on(flag)); });
}

CCMenuItemSpriteExtra* ModMenuPopup::makePill(char const* text, float w, float h, int tag,
                                              SEL_MenuHandler sel, std::function<bool()> selected) {
    auto node = CCNode::create();
    node->setContentSize({w, h});
    auto d = CCDrawNode::create();
    node->addChild(d);
    auto lbl = CCLabelBMFont::create(text, "chatFont.fnt");
    lbl->setScale(0.85f);
    lbl->setPosition({w / 2.f, h / 2.f});
    node->addChild(lbl);

    auto item = CCMenuItemSpriteExtra::create(node, this, sel);
    item->setTag(tag);
    m_redraw.push_back([d, lbl, w, h, selected] {
        bool s = selected();
        d->clear();
        auto acc = mm::accent();
        drawRound(d, ccp(0.f, 0.f), CCSize(w, h), h / 2.f, P(s ? acc : OFF_C, 1.f));
        lbl->setColor(s ? onColor(acc) : TEXT_C);
    });
    return item;
}

void ModMenuPopup::buildPlayer() {
    constexpr int T = 0;
    this->addSwitchCell(T, mm::F::Noclip, "Noclip", 0, 0);
    this->addSwitchCell(T, mm::F::Safe, "Safe mode", 1, 0);
    this->addSwitchCell(T, mm::F::Jump, "Jump hack", 0, 1);
    this->addSwitchCell(T, mm::F::AutoSafe, "Auto safe mode", 1, 1);

    // скорость: широкая ячейка + сегментированный выбор
    CCPoint o = this->cellOrigin(0, 2);
    m_cells[T].push_back({o, CCSize(WIDE_W, CELL_H), true});
    this->addCellLabel(T, "Speed", o.x + 24.f, o.y + CELL_H / 2.f);
    for (int i = 0; i < static_cast<int>(mm::SPEEDS.size()); i++) {
        auto pill = this->makePill(fmt::format("x{}", mm::SPEEDS[i]).c_str(), 44.f, 22.f, i,
                                   menu_selector(ModMenuPopup::onSpeed),
                                   [i] { return mm::state().speedIdx == i; });
        pill->setPosition({o.x + WIDE_W - 10.f - 284.f + 22.f + i * 48.f, o.y + CELL_H / 2.f});
        m_buttonMenu->addChild(pill);
        m_tabNodes[T].push_back(pill);
    }

    // статус Safe mode и легенда
    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.9f);
    m_status->setAnchorPoint({0.f, 0.5f});
    m_status->setPosition({20.f, ROW0 - 3 * ROW_STEP});
    m_mainLayer->addChild(m_status);
    m_tabNodes[T].push_back(m_status);

    float ly = ROW0 - 4 * ROW_STEP;
    m_cells[T].push_back({ccp(22.f, ly), CCSize(0.f, 0.f), true});
    this->addCellLabel(T, "Marked features disable leaderboard records", 32.f, ly, true);
}

void ModMenuPopup::buildLevel() {
    constexpr int T = 1;

    // кнопка перезапуска уровня
    {
        CCPoint o = this->cellOrigin(0, 0);
        auto node = CCNode::create();
        node->setContentSize({CELL_W, CELL_H});
        auto d = CCDrawNode::create();
        node->addChild(d);
        auto lbl = CCLabelBMFont::create("Restart level", "chatFont.fnt");
        lbl->setScale(0.95f);
        lbl->setPosition({CELL_W / 2.f, CELL_H / 2.f});
        node->addChild(lbl);
        auto item = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onRestart));
        item->setPosition({o.x + CELL_W / 2.f, o.y + CELL_H / 2.f});
        m_buttonMenu->addChild(item);
        m_tabNodes[T].push_back(item);
        m_redraw.push_back([d, lbl] {
            auto acc = mm::accent();
            d->clear();
            drawRound(d, ccp(0.f, 0.f), CCSize(CELL_W, CELL_H), 10.f, P(acc, 0.16f), 1.f, P(acc, 0.75f));
            lbl->setColor(acc);
        });
    }

    this->addSwitchCell(T, mm::F::Hitboxes, "Show hitboxes", 1, 0);
    this->addSwitchCell(T, mm::F::NoDeath, "No death effect", 0, 1);

    this->addCellLabel(T, "Hitboxes are shown in every mode, not only practice", 20.f,
                       ROW0 - 4 * ROW_STEP, true);
}

void ModMenuPopup::buildVisual() {
    constexpr int T = 2;
    this->addSwitchCell(T, mm::F::Overlay, "Info overlay", 0, 0);
    this->addSwitchCell(T, mm::F::ShowFPS, "Show FPS", 1, 0);
    this->addSwitchCell(T, mm::F::HidePlayer, "Hide player", 0, 1);
    this->addSwitchCell(T, mm::F::HideAttempts, "Hide attempts", 1, 1);
    this->addSwitchCell(T, mm::F::NoShake, "No shake", 0, 2);
    this->addSwitchCell(T, mm::F::NoMirror, "No mirror", 1, 2);
    this->addSwitchCell(T, mm::F::RainbowIcon, "Rainbow icon", 0, 3);
    this->addSwitchCell(T, mm::F::RainbowBG, "Rainbow background", 1, 3);
    this->addSwitchCell(T, mm::F::UnlockAll, "Unlock all icons", 0, 4);
}

void ModMenuPopup::buildTheme() {
    constexpr int T = 3;

    // цвет акцента: одна кнопка, открывает плавающее окно
    {
        CCPoint o = this->cellOrigin(0, 0);
        m_cells[T].push_back({o, CCSize(CELL_W, CELL_H), false});
        this->addCellLabel(T, "Accent color", o.x + 14.f, o.y + CELL_H / 2.f);

        auto node = CCNode::create();
        node->setContentSize({56.f, 34.f});
        auto d = CCDrawNode::create();
        node->addChild(d);
        auto item = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onOpenPicker));
        item->setPosition({o.x + CELL_W - 10.f - 28.f, o.y + CELL_H / 2.f});
        m_buttonMenu->addChild(item);
        m_tabNodes[T].push_back(item);
        m_redraw.push_back([d] {
            d->clear();
            d->drawDot(ccp(28.f, 17.f), 11.f, P(WHITE_C, 0.28f));
            d->drawDot(ccp(28.f, 17.f), 9.f, P(mm::accent(), 1.f));
        });
    }

    this->addSwitchCell(T, mm::F::Glass, "Liquid glass", 1, 0);

    // прозрачность стекла
    CCPoint o = this->cellOrigin(0, 1);
    m_cells[T].push_back({o, CCSize(WIDE_W, CELL_H), false});
    this->addCellLabel(T, "Glass opacity", o.x + 14.f, o.y + CELL_H / 2.f);
    char const* names[3] = {"Low", "Mid", "High"};
    for (int i = 0; i < 3; i++) {
        auto pill = this->makePill(names[i], 54.f, 22.f, i,
                                   menu_selector(ModMenuPopup::onGlassOpacity),
                                   [i] { return mm::state().glassOpacity == i; });
        pill->setPosition({o.x + WIDE_W - 10.f - 170.f + 27.f + i * 58.f, o.y + CELL_H / 2.f});
        m_buttonMenu->addChild(pill);
        m_tabNodes[T].push_back(pill);
    }

    this->addCellLabel(T, "Tip: drag the color window aside to preview the result live", 20.f,
                       ROW0 - 3 * ROW_STEP, true);
}

void ModMenuPopup::buildPicker() {
    m_cwRoot = CCNode::create();
    m_cwRoot->setVisible(false);
    m_buttonMenu->addChild(m_cwRoot, 100);

    m_cwBg = CCDrawNode::create();
    m_cwRoot->addChild(m_cwBg);

    auto title = CCLabelBMFont::create("Color", "chatFont.fnt");
    title->setScale(0.95f);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({12.f, CH - HDR / 2.f});
    title->setColor(TEXT_C);
    m_cwRoot->addChild(title);

    // квадрат насыщенность/яркость: 3 дешёвых слоя без шейдеров
    m_svBase = CCLayerColor::create(ccc4(255, 0, 0, 255), SV_W, SV_H);
    m_svBase->setPosition({SV_X, SV_Y});
    m_cwRoot->addChild(m_svBase, 1);

    auto sat = CCLayerGradient::create(ccc4(255, 255, 255, 255), ccc4(255, 255, 255, 0), ccp(1.f, 0.f));
    sat->setContentSize({SV_W, SV_H});
    sat->setPosition({SV_X, SV_Y});
    m_cwRoot->addChild(sat, 2);

    auto val = CCLayerGradient::create(ccc4(0, 0, 0, 0),
                                       ccc4(0, 0, 0, static_cast<GLubyte>(255.f * (1.f - mm::VMIN))),
                                       ccp(0.f, -1.f));
    val->setContentSize({SV_W, SV_H});
    val->setPosition({SV_X, SV_Y});
    m_cwRoot->addChild(val, 3);

    // полоса оттенка: 6 градиентных сегментов
    float segW = HUE_W / 6.f;
    for (int i = 0; i < 6; i++) {
        auto c1 = mm::rainbow(i / 6.f);
        auto c2 = mm::rainbow((i + 1) / 6.f);
        auto g = CCLayerGradient::create(ccc4(c1.r, c1.g, c1.b, 255), ccc4(c2.r, c2.g, c2.b, 255), ccp(1.f, 0.f));
        g->setContentSize({segW + 0.5f, HUE_H});
        g->setPosition({HUE_X + i * segW, HUE_Y});
        m_cwRoot->addChild(g, 1);
    }

    m_svCursor = CCDrawNode::create();
    m_cwRoot->addChild(m_svCursor, 5);
    m_hueCursor = CCDrawNode::create();
    m_cwRoot->addChild(m_hueCursor, 5);

    // крестик закрытия
    {
        auto node = CCNode::create();
        node->setContentSize({24.f, 24.f});
        auto d = CCDrawNode::create();
        d->drawSegment(ccp(8.f, 8.f), ccp(16.f, 16.f), 0.8f, P(MUTED_C, 1.f));
        d->drawSegment(ccp(8.f, 16.f), ccp(16.f, 8.f), 0.8f, P(MUTED_C, 1.f));
        node->addChild(d);
        auto item = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onClosePicker));
        item->setVisible(false);
        item->setEnabled(false);
        m_buttonMenu->addChild(item, 101);
        m_cwItems.push_back({item, ccp(CW - 16.f, CH - HDR / 2.f)});
    }

    // быстрые цвета
    for (int i = 0; i < 8; i++) {
        auto node = CCNode::create();
        node->setContentSize({20.f, 20.f});
        auto d = CCDrawNode::create();
        d->drawDot(ccp(10.f, 10.f), 7.f, P(mm::presets()[i], 1.f));
        node->addChild(d);
        auto item = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onPreset));
        item->setTag(i);
        item->setVisible(false);
        item->setEnabled(false);
        m_buttonMenu->addChild(item, 101);
        m_cwItems.push_back({item, ccp(17.f + i * 17.f, 18.f)});
    }

    m_cwPos = ccp(W - CW - 12.f, 12.f);
    this->setPickerPos(m_cwPos);
}

// ============================ отрисовка ============================

void ModMenuPopup::drawWindow() {
    auto acc = mm::accent();
    bool glass = mm::on(mm::F::Glass);
    float a = glass ? mm::GLASS_OPACITY[mm::state().glassOpacity] : 0.985f;

    m_bgNode->clear();
    ccColor3B base = mix(ccColor3B{17, 19, 25}, acc, 0.07f);
    drawRound(m_bgNode, ccp(0.f, 0.f), CCSize(W, H), 16.f, P(base, a), 1.f, P(WHITE_C, glass ? 0.22f : 0.08f));
    if (glass) {
        // блик в верхней половине и тонкая "кромка" света
        drawRound(m_bgNode, ccp(2.f, H * 0.5f), CCSize(W - 4.f, H * 0.5f - 2.f), 14.f, P(WHITE_C, 0.045f));
        m_bgNode->drawSegment(ccp(20.f, H - 1.5f), ccp(W - 20.f, H - 1.5f), 0.75f, P(WHITE_C, 0.38f));
    }
    // разделитель под вкладками
    m_bgNode->drawSegment(ccp(16.f, 205.f), ccp(W - 16.f, 205.f), 0.5f, P(WHITE_C, 0.08f));
}

void ModMenuPopup::drawCells() {
    bool glass = mm::on(mm::F::Glass);
    for (int t = 0; t < TAB_COUNT; t++) {
        auto n = m_cellNodes[t];
        n->clear();
        for (auto const& c : m_cells[t]) {
            if (c.s.width > 0.f) {
                drawRound(n, c.o, c.s, 10.f, P(WHITE_C, glass ? 0.06f : 0.05f));
                if (c.cheat) n->drawDot(ccp(c.o.x + 13.f, c.o.y + c.s.height / 2.f), 3.f, P(WARN_C, 1.f));
            } else if (c.cheat) {
                n->drawDot(c.o, 3.f, P(WARN_C, 1.f)); // точка легенды
            }
        }
    }
}

void ModMenuPopup::refreshStatus() {
    if (!m_status) return;
    std::string s;
    ccColor3B col = MUTED_C;
    if (mm::on(mm::F::Safe)) {
        s = "Safe mode: ON (manual). Nothing is saved";
        col = GOOD_C;
    } else if (mm::on(mm::F::AutoSafe)) {
        if (mm::cheating() || mm::tainted()) {
            s = "Safe mode: ON (auto: " + mm::cheatList() + ")";
            col = GOOD_C;
        } else {
            s = "Safe mode: auto, waiting for cheats";
        }
    } else if (mm::cheating()) {
        s = "Safe mode: OFF. Cheats can get you banned!";
        col = WARN_C;
    } else {
        s = "Safe mode: OFF";
    }
    m_status->setString(s.c_str());
    m_status->setColor(col);
}

void ModMenuPopup::applyTheme() {
    this->drawWindow();
    this->drawCells();
    for (auto& fn : m_redraw) fn();

    auto acc = mm::accent();
    m_tabInd->clear();
    drawRound(m_tabInd, ccp(-14.f, -1.5f), CCSize(28.f, 3.f), 1.5f, P(acc, 1.f));

    this->refreshStatus();
    if (m_cwOpen) this->redrawPicker();
}

void ModMenuPopup::refreshInteractivity() {
    for (int t = 0; t < TAB_COUNT; t++) {
        bool on = (t == m_currentTab);
        for (auto node : m_tabNodes[t]) {
            node->setVisible(on);
            if (auto item = typeinfo_cast<CCMenuItem*>(node)) item->setEnabled(on && !m_cwOpen);
        }
        m_tabBtns[t]->setEnabled(!m_cwOpen);
        m_tabLabels[t]->setColor(on ? TEXT_C : MUTED_C);
    }
    m_closeItem->setEnabled(!m_cwOpen);
}

void ModMenuPopup::switchTab(int index, bool animate) {
    m_currentTab = index;
    this->refreshInteractivity();
    CCPoint target = ccp(TAB_X[index], TAB_Y - 17.f);
    m_tabInd->stopAllActions();
    if (animate) {
        m_tabInd->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.22f, target)));
    } else {
        m_tabInd->setPosition(target);
    }
}

// ============================ окно цвета ============================

void ModMenuPopup::setPickerOpen(bool open) {
    m_cwOpen = open;
    m_cwRoot->setVisible(open);
    for (auto& it : m_cwItems) {
        it.first->setVisible(open);
        if (auto item = typeinfo_cast<CCMenuItem*>(it.first)) item->setEnabled(open);
    }
    this->refreshInteractivity();
    if (open) this->redrawPicker();
}

void ModMenuPopup::setPickerPos(CCPoint p) {
    auto ws = CCDirector::get()->getWinSize();
    float minX = W / 2.f - ws.width / 2.f + 4.f;
    float maxX = W / 2.f + ws.width / 2.f - CW - 4.f;
    float minY = H / 2.f - ws.height / 2.f + 4.f;
    float maxY = H / 2.f + ws.height / 2.f - CH - 4.f;
    p.x = std::clamp(p.x, minX, maxX);
    p.y = std::clamp(p.y, minY, maxY);
    m_cwPos = p;
    m_cwRoot->setPosition(p);
    for (auto& it : m_cwItems) it.first->setPosition(p + it.second);
}

void ModMenuPopup::redrawPicker() {
    auto& s = mm::state();
    auto acc = mm::accent();

    m_cwBg->clear();
    drawRound(m_cwBg, ccp(0.f, 0.f), CCSize(CW, CH), 12.f, P(ccColor3B{20, 22, 29}, 0.985f), 1.f, P(WHITE_C, 0.14f));
    m_cwBg->drawSegment(ccp(10.f, CH - HDR), ccp(CW - 10.f, CH - HDR), 0.5f, P(WHITE_C, 0.08f));

    auto hc = mm::hsv(s.hue, 1.f, 1.f);
    m_svBase->setColor(hc);

    float t = (std::max(s.val, mm::VMIN) - mm::VMIN) / (1.f - mm::VMIN);
    m_svCursor->clear();
    m_svCursor->drawDot(ccp(0.f, 0.f), 6.f, P(WHITE_C, 1.f));
    m_svCursor->drawDot(ccp(0.f, 0.f), 4.6f, P(acc, 1.f));
    m_svCursor->setPosition({SV_X + s.sat * SV_W, SV_Y + t * SV_H});

    m_hueCursor->clear();
    m_hueCursor->drawDot(ccp(0.f, 0.f), 7.5f, P(WHITE_C, 1.f));
    m_hueCursor->drawDot(ccp(0.f, 0.f), 6.f, P(hc, 1.f));
    m_hueCursor->setPosition({HUE_X + s.hue * HUE_W, HUE_Y + HUE_H / 2.f});
}

void ModMenuPopup::pickSV(CCPoint lp) {
    auto& s = mm::state();
    s.sat = std::clamp((lp.x - SV_X) / SV_W, 0.f, 1.f);
    float t = std::clamp((lp.y - SV_Y) / SV_H, 0.f, 1.f);
    s.val = mm::VMIN + (1.f - mm::VMIN) * t;
    this->applyTheme();
}

void ModMenuPopup::pickHue(CCPoint lp) {
    mm::state().hue = std::clamp((lp.x - HUE_X) / HUE_W, 0.f, 1.f) * 0.999f;
    this->applyTheme();
}

// ============================ касания ============================

bool ModMenuPopup::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (m_cwOpen) {
        CCPoint lp = m_buttonMenu->convertToNodeSpace(touch->getLocation()) - m_cwPos;
        if (inRect(lp, 0.f, 0.f, CW, CH)) {
            if (inRect(lp, SV_X, SV_Y, SV_W, SV_H)) {
                m_drag = Drag::SV;
                this->pickSV(lp);
            } else if (inRect(lp, HUE_X - 4.f, HUE_Y - 6.f, HUE_W + 8.f, HUE_H + 12.f)) {
                m_drag = Drag::Hue;
                this->pickHue(lp);
            } else if (lp.y >= CH - HDR) {
                m_drag = Drag::Window;
                m_dragOff = lp;
            } else {
                m_drag = Drag::None;
            }
            return true;
        }
    }
    return Popup::ccTouchBegan(touch, event);
}

void ModMenuPopup::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (m_drag == Drag::None) {
        Popup::ccTouchMoved(touch, event);
        return;
    }
    CCPoint p = m_buttonMenu->convertToNodeSpace(touch->getLocation());
    switch (m_drag) {
        case Drag::Window: this->setPickerPos(p - m_dragOff); break;
        case Drag::SV: this->pickSV(p - m_cwPos); break;
        case Drag::Hue: this->pickHue(p - m_cwPos); break;
        default: break;
    }
}

void ModMenuPopup::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    if (m_drag == Drag::SV || m_drag == Drag::Hue) mm::saveTheme();
    m_drag = Drag::None;
    Popup::ccTouchEnded(touch, event);
}

void ModMenuPopup::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    if (m_drag == Drag::SV || m_drag == Drag::Hue) mm::saveTheme();
    m_drag = Drag::None;
    Popup::ccTouchCancelled(touch, event);
}

// ============================ колбэки ============================

void ModMenuPopup::onTab(CCObject* sender) {
    this->switchTab(static_cast<CCNode*>(sender)->getTag(), true);
}

void ModMenuPopup::onToggle(CCObject* sender) {
    auto flag = static_cast<mm::F>(static_cast<CCNode*>(sender)->getTag());
    mm::set(flag, !mm::on(flag));
    mm::save(flag);

    if (flag == mm::F::Hitboxes) mm::applyHitboxes();
    if (flag == mm::F::AutoSafe && !mm::on(flag)) {
        FLAlertLayer::create("Auto safe mode off",
                             "Cheats will no longer enable Safe mode automatically. "
                             "Records made with cheats may get you banned from leaderboards.",
                             "OK")->show();
    }
    this->applyTheme();
}

void ModMenuPopup::onSpeed(CCObject* sender) {
    mm::state().speedIdx = static_cast<CCNode*>(sender)->getTag();
    mm::applySpeed(PlayLayer::get() != nullptr);
    this->applyTheme();
}

void ModMenuPopup::onGlassOpacity(CCObject* sender) {
    mm::state().glassOpacity = static_cast<CCNode*>(sender)->getTag();
    mm::saveTheme();
    this->applyTheme();
}

void ModMenuPopup::onPreset(CCObject* sender) {
    auto c = mm::presets()[static_cast<CCNode*>(sender)->getTag()];
    auto& s = mm::state();
    mm::rgb2hsv(c, s.hue, s.sat, s.val);
    s.val = std::max(s.val, mm::VMIN);
    mm::saveTheme();
    this->applyTheme();
}

void ModMenuPopup::onOpenPicker(CCObject*) {
    this->setPickerOpen(!m_cwOpen);
}

void ModMenuPopup::onClosePicker(CCObject*) {
    this->setPickerOpen(false);
}

void ModMenuPopup::onClosePressed(CCObject*) {
    this->onClose(nullptr);
}

void ModMenuPopup::onRestart(CCObject*) {
    if (auto pl = PlayLayer::get()) {
        this->onClose(nullptr);
        pl->resetLevel();
    } else {
        FLAlertLayer::create("Mod Menu", "Open a level first.", "OK")->show();
    }
}
