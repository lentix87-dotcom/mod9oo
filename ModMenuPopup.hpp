#pragma once
#include <Geode/Geode.hpp>
#include <array>
#include <functional>
#include <utility>
#include <vector>
#include "Settings.hpp"

using namespace geode::prelude;

class ModMenuPopup : public geode::Popup {
protected:
    static constexpr int TAB_COUNT = 4;
    struct CellInfo { CCPoint o; CCSize s; bool cheat; };
    enum class Drag { None, Window, SV, Hue };

    // ----- главное окно -----
    std::array<std::vector<CCNode*>, TAB_COUNT> m_tabNodes;
    std::array<std::vector<CellInfo>, TAB_COUNT> m_cells;
    std::array<CCDrawNode*, TAB_COUNT> m_cellNodes{};
    std::array<CCMenuItemSpriteExtra*, TAB_COUNT> m_tabBtns{};
    std::array<CCLabelBMFont*, TAB_COUNT> m_tabLabels{};
    std::vector<std::function<void()>> m_redraw;
    CCMenuItemSpriteExtra* m_closeItem = nullptr;
    CCDrawNode* m_bgNode = nullptr;
    CCDrawNode* m_tabInd = nullptr;
    CCLabelBMFont* m_status = nullptr;
    int m_currentTab = 0;

    // ----- плавающее окно выбора цвета -----
    bool m_cwOpen = false;
    CCPoint m_cwPos;
    CCNode* m_cwRoot = nullptr;
    CCDrawNode* m_cwBg = nullptr;
    CCLayerColor* m_svBase = nullptr;
    CCDrawNode* m_svCursor = nullptr;
    CCDrawNode* m_hueCursor = nullptr;
    std::vector<std::pair<CCNode*, CCPoint>> m_cwItems;
    Drag m_drag = Drag::None;
    CCPoint m_dragOff;

    bool init();

    // построение
    void buildPlayer();
    void buildLevel();
    void buildVisual();
    void buildTheme();
    void buildPicker();
    CCPoint cellOrigin(int col, int row) const;
    void addCellLabel(int tab, char const* text, float x, float y, bool muted = false);
    void addSwitchCell(int tab, mm::F flag, char const* label, int col, int row);
    CCMenuItemSpriteExtra* makePill(char const* text, float w, float h, int tag,
                                    SEL_MenuHandler sel, std::function<bool()> selected);

    // отрисовка
    void applyTheme();
    void drawWindow();
    void drawCells();
    void refreshStatus();
    void refreshInteractivity();
    void switchTab(int index, bool animate);

    // окно цвета
    void setPickerOpen(bool open);
    void setPickerPos(CCPoint p);
    void redrawPicker();
    void pickSV(CCPoint lp);
    void pickHue(CCPoint lp);

    // колбэки
    void onTab(CCObject* sender);
    void onToggle(CCObject* sender);
    void onSpeed(CCObject* sender);
    void onGlassOpacity(CCObject* sender);
    void onRestart(CCObject* sender);
    void onClosePressed(CCObject* sender);
    void onOpenPicker(CCObject* sender);
    void onClosePicker(CCObject* sender);
    void onPreset(CCObject* sender);

    // касания (перетаскивание окна и выбор цвета)
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;

public:
    static ModMenuPopup* create();
};
