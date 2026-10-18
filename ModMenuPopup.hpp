#pragma once
#include <Geode/Geode.hpp>
#include <array>
#include <vector>

class ModMenuPopup : public geode::Popup {
protected:
    std::array<std::vector<cocos2d::CCNode*>, 3> m_tabNodes;
    std::array<CCMenuItemSpriteExtra*, 3> m_tabBtns{};
    cocos2d::CCLabelBMFont* m_speedLabel = nullptr;

    bool init();

    void switchTab(int index);
    void onTab(cocos2d::CCObject* sender);
    void onToggle(cocos2d::CCObject* sender);
    void onSpeed(cocos2d::CCObject* sender);
    void onRestart(cocos2d::CCObject* sender);

    void addToggle(int tab, int id, char const* label, bool value, float y);
    void refreshSpeedLabel();

public:
    static ModMenuPopup* create();
};
