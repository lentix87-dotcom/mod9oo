#pragma once
#include <Geode/Geode.hpp>
#include <array>
#include <functional>
#include <vector>
#include "Settings.hpp"

using namespace geode::prelude;

class ModMenuPopup : public geode::Popup {
protected:
    static constexpr int TAB_COUNT = 5; // Player, Level, Visual, Theme, Menu/UI

    struct Particle {
        CCPoint pos;
        CCPoint vel;
        float size;
        float angle;
        float spin;
        float alpha;
    };

    int m_currentTab = 0;
    bool m_hasSubSettings = false;
    mm::F m_activeSubFeature = mm::F::COUNT;

    CCDrawNode* m_bgNode = nullptr;
    CCDrawNode* m_glowNode = nullptr;
    CCDrawNode* m_particleNode = nullptr;
    CCNode* m_contentHost = nullptr;
    CCNode* m_subPopupHost = nullptr;

    std::vector<Particle> m_particles;
    std::array<CCMenuItemSpriteExtra*, TAB_COUNT> m_tabBtns{};

    bool init() override;
    void update(float dt) override;

    // Отрисовка каркаса и эффектов
    void redrawFrame();
    void rebuildContent();
    void rebuildSubPopup();
    void initParticles();

    // Помощники создания карточек в стиле Nursultan
    void addModuleCard(int col, int row, mm::F flag, bool hasSubOptions = false);
    void addActionCard(int col, int row, char const* titleText, char const* subText, SEL_MenuHandler cb);
    void animatePress(CCNode* target);

    // Колбэки
    void onTab(CCObject* sender);
    void onToggle(CCObject* sender);
    void onOpenSub(CCObject* sender);
    void onCloseSub(CCObject* sender);
    void onModInfo(CCObject* sender);
    void onRestartLevel(CCObject* sender);
    void onClosePopup(CCObject* sender);

    // Колбэки внутри меню "..." и настроек
    void onSpeedPill(CCObject* sender);
    void onTpsStep(CCObject* sender);
    void onParticleSelect(CCObject* sender);
    void onThemeSelect(CCObject* sender);
    void onAnimTypeSelect(CCObject* sender);
    void onOvCornerSelect(CCObject* sender);
    void onColorSlotSelect(CCObject* sender);
    void onHueStep(CCObject* sender);
    void onSatStep(CCObject* sender);
    void onValStep(CCObject* sender);
    void onGlowStep(CCObject* sender);
    void onBtnScaleStep(CCObject* sender);
    void onBtnOpacityStep(CCObject* sender);

public:
    static ModMenuPopup* create();
};
