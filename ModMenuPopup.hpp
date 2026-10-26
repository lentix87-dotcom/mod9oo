#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/file.hpp>
#include <array>
#include <functional>
#include <vector>
#include "Settings.hpp"

using namespace geode::prelude;

class ModMenuPopup : public geode::Popup {
protected:
    static constexpr int TAB_COUNT = 5;

    struct Particle {
        CCPoint pos;
        CCPoint vel;
        float size;
        float angle;
        float spin;
        float alpha;
    };

    struct SliderTrack {
        CCRect rect;
        float minVal;
        float maxVal;
        std::function<float()> getter;
        std::function<void(float)> setter;
        CCDrawNode* drawNode = nullptr;
        CCLabelBMFont* valLabel = nullptr;
        std::string title;
        std::string suffix;
        bool isInt = false;
    };

    enum class TouchMode { None, Slider, PaletteSV, PaletteHue };

    int m_currentTab = 0;
    bool m_hasSubSettings = false;
    mm::F m_activeSubFeature = mm::F::COUNT;

    // Многослойный Screen Blur
    CCNode* m_screenBlurHost = nullptr;
    std::vector<CCSprite*> m_blurPasses;
    CCDrawNode* m_blurFrostOverlay = nullptr;

    // Слои окна Loon Client
    CCDrawNode* m_bgNode = nullptr;
    CCDrawNode* m_glowNode = nullptr;
    CCDrawNode* m_particleNode = nullptr;
    CCDrawNode* m_tabSlider = nullptr;
    CCNode* m_contentHost = nullptr;
    CCNode* m_subPopupHost = nullptr;
    CCMenu* m_mainCardsMenu = nullptr;

    // Импортированные обои (PNG / JPG / Анимированный GIF) на 100% окна со скруглением!
    CCClippingNode* m_wallpaperClip = nullptr;
    CCDrawNode* m_wallpaperStencil = nullptr;
    CCSprite* m_wallpaperSprite = nullptr;
    std::vector<Ref<CCTexture2D>> m_gifFrames;
    std::vector<float> m_gifDelays;
    size_t m_gifCurFrame = 0;
    float m_gifTimer = 0.f;
    std::string m_loadedBgPath;
    int m_loadedGifQuality = -1;

    // Живое превью кнопки L
    CCNode* m_previewBtnRoot = nullptr;
    CCDrawNode* m_previewBtnBg = nullptr;
    CCLabelBMFont* m_previewBtnLbl = nullptr;

    // Визуальная палитра (открывается ТОЛЬКО для Custom Palette)
    bool m_hasPalette = false;
    CCRect m_svRect;
    CCRect m_hueRect;
    CCLayerColor* m_svBase = nullptr;
    CCDrawNode* m_svCursor = nullptr;
    CCDrawNode* m_hueCursor = nullptr;

    TouchMode m_touchMode = TouchMode::None;
    int m_activeSliderIdx = -1;
    std::vector<SliderTrack> m_sliders;

    std::array<CCLabelBMFont*, TAB_COUNT> m_tabLabels{};
    std::array<CCSprite*, TAB_COUNT> m_tabIcons{};
    std::vector<Particle> m_particles;

    bool init() override;
    void onExit() override;
    void update(float dt) override;

    // Screen Blur
    void createScreenBlur();
    void updateScreenBlurOpacity();

    // Загрузка и отрисовка пользовательских обоев и GIF на 100% окна меню
    void reloadCustomWallpaper();
    void fitWallpaperSprite(CCTexture2D* tex);
    void updateWallpaperClipMask();
    void updateCustomWallpaper(float dt);

    // Отрисовка Loon Client
    void redrawFrame();
    void redrawTabSlider();
    void redrawLButtonPreview();
    void rebuildContent(bool animateTransition = false);
    void rebuildSubPopup(bool animateOpen = false);
    void initParticles();

    void drawSectionBox(CCNode* parent, float x, float yTop, float w, int rowCount, char const* sectionTitle);
    void addModuleRow(float x, float yRowTop, float w, mm::F flag, bool hasSubOptions, bool drawBottomLine);
    void addSliderControl(CCNode* host, float x, float y, float w, char const* title, char const* suffix,
                          float minV, float maxV, bool isInt,
                          std::function<float()> getter, std::function<void(float)> setter);
    void refreshSliderVisual(SliderTrack& sl);
    void buildVisualPalette(CCNode* host, float x, float y);
    void refreshPaletteVisual();

    void animatePress(CCNode* target);

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;

    void onTab(CCObject* sender);
    void onToggle(CCObject* sender);
    void onOpenSub(CCObject* sender);
    void onCloseSub(CCObject* sender);
    void onModInfo(CCObject* sender);
    void onClosePopup(CCObject* sender);

    void onSpeedPill(CCObject* sender);
    void onParticleSelect(CCObject* sender);
    void onThemeSelect(CCObject* sender);
    void onOvCornerSelect(CCObject* sender);
    void onColorSlotSelect(CCObject* sender);
    void onGifQualitySelect(CCObject* sender);
    void onImportWallpaper(CCObject* sender);
    void onClearWallpaper(CCObject* sender);

public:
    static ModMenuPopup* create();
};
