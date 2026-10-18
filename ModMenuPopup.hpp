#pragma once
#include <Geode/Geode.hpp>
#include <array>
#include <vector>
#include "Settings.hpp"

class ModMenuPopup : public geode::Popup {
protected:
    static constexpr int TAB_COUNT = 4;

    std::array<std::vector<cocos2d::CCNode*>, TAB_COUNT> m_tabNodes;
    std::array<CCMenuItemSpriteExtra*, TAB_COUNT> m_tabBtns{};
    std::vector<cocos2d::CCSprite*> m_swatches;
    std::vector<CCMenuItemSpriteExtra*> m_opacityBtns;

    cocos2d::CCLabelBMFont* m_speedLabel = nullptr;
    cocos2d::CCLayerGradient* m_gloss = nullptr;   // блик "жидкого стекла"
    cocos2d::CCLayerColor* m_edge = nullptr;       // светлая кромка сверху
    cocos2d::CCLayerColor* m_accentLine = nullptr; // акцентная линия под вкладками
    int m_currentTab = 0;

    bool init();

    void switchTab(int index);
    void applyTheme();

    void onTab(cocos2d::CCObject* sender);
    void onToggle(cocos2d::CCObject* sender);
    void onSpeed(cocos2d::CCObject* sender);
    void onAccent(cocos2d::CCObject* sender);
    void onGlassOpacity(cocos2d::CCObject* sender);
    void onRestart(cocos2d::CCObject* sender);

    void addToggle(int tab, mm::F flag, char const* label, float x, float y);
    void addLabel(int tab, char const* text, float x, float y, float scale = 0.5f);
    void refreshSpeedLabel();

public:
    static ModMenuPopup* create();
};
