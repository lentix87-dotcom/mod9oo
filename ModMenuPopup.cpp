#include "ModMenuPopup.hpp"
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <cmath>
#include <fstream>

namespace {

constexpr float PI = 3.14159265f;
constexpr float WIN_W = 460.f;
constexpr float WIN_H = 290.f;
constexpr float DOCK_Y = 23.f;

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
    return 66.f + idx * 82.f;
}

void drawRoundRect(CCDrawNode* n, CCPoint o, CCSize s, float r, ccColor4F fill,
                   float bw = 0.f, ccColor4F bc = {0.f, 0.f, 0.f, 0.f}) {
    if (s.width < 1.f || s.height < 1.f) return;
    r = std::clamp(r, 0.f, std::min(s.width, s.height) * 0.5f - 0.1f);

    std::vector<CCPoint> pts;
    pts.reserve(48);
    auto pushUnique = [&](float x, float y) {
        CCPoint p = ccp(x, y);
        if (!pts.empty() && std::abs(pts.back().x - p.x) < 0.04f && std::abs(pts.back().y - p.y) < 0.04f) return;
        pts.push_back(p);
    };

    if (r < 0.5f) {
        pushUnique(o.x, o.y);
        pushUnique(o.x + s.width, o.y);
        pushUnique(o.x + s.width, o.y + s.height);
        pushUnique(o.x, o.y + s.height);
    } else {
        constexpr int SEG = 10;
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
    while (pts.size() > 3 && std::abs(pts.front().x - pts.back().x) < 0.04f && std::abs(pts.front().y - pts.back().y) < 0.04f) {
        pts.pop_back();
    }
    if (pts.size() >= 3) {
        n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), fill, bw, bc);
    }
}

void drawSmoothCircle(CCDrawNode* n, CCPoint c, float r, ccColor4F col, int segs = 32) {
    std::vector<CCPoint> pts(segs);
    for (int i = 0; i < segs; i++) {
        float a = 2.f * PI * static_cast<float>(i) / static_cast<float>(segs);
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
    // Если включены свои обои/GIF, делаем карточки чуть прозрачнее, чтобы обои красиво просвечивали!
    if (mm::on(mm::F::CustomWallpaper) && !mm::state().customBgPath.empty()) {
        glassK *= 0.88f;
    }
    float finalAlpha = std::clamp(alpha * glassK, 0.28f, 0.98f);

    drawRoundRect(n, o, s, r, P(col, finalAlpha), 0.85f, P(borderCol, borderAlpha));

    if (glassOn) {
        float hazeIntensity = (1.0f - mm::state().glassAlpha) * 0.16f + 0.025f;
        drawRoundRect(n, ccp(o.x + 1.f, o.y + 1.f), CCSize(s.width - 2.f, s.height - 2.f),
                      std::max(0.f, r - 1.f), P({235, 242, 255}, hazeIntensity));
    }
}

// ----- Встроенный лёгкий декодер GIF89a (без внешних библиотек, с поддержкой даунскейла 360p/640p/1080p) -----
bool decodeGifFile(std::string const& path, int qualityMode,
                   std::vector<Ref<CCTexture2D>>& outFrames,
                   std::vector<float>& outDelays) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    auto fsize = file.tellg();
    if (fsize < 14 || fsize > 45 * 1024 * 1024) return false; // Защита от слишком огромных файлов
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> d(static_cast<size_t>(fsize));
    if (!file.read(reinterpret_cast<char*>(d.data()), fsize)) return false;

    if (d[0] != 'G' || d[1] != 'I' || d[2] != 'F') return false;

    int canvasW = d[6] | (d[7] << 8);
    int canvasH = d[8] | (d[9] << 8);
    if (canvasW <= 0 || canvasH <= 0 || canvasW > 4096 || canvasH > 4096) return false;

    uint8_t packed = d[10];
    size_t pos = 13;
    std::vector<uint8_t> gct;
    if (packed & 0x80) {
        int gctSize = 1 << ((packed & 7) + 1);
        if (pos + gctSize * 3 > d.size()) return false;
        gct.assign(d.begin() + pos, d.begin() + pos + gctSize * 3);
        pos += gctSize * 3;
    }

    // Определяем целевое разрешение под выбранное качество (360p / 640p / 1080p) для отсутствия лагов
    int maxTargetW = (qualityMode == 0) ? 360 : (qualityMode == 1 ? 640 : 1080);
    int downScale = std::max(1, canvasW / maxTargetW);
    int outW = std::max(1, canvasW / downScale);
    int outH = std::max(1, canvasH / downScale);

    std::vector<uint8_t> canvas(static_cast<size_t>(canvasW) * canvasH * 4, 0);
    std::vector<uint8_t> scaledBuf(static_cast<size_t>(outW) * outH * 4, 0);

    int transIndex = -1;
    float delaySec = 0.08f;

    while (pos < d.size()) {
        uint8_t block = d[pos++];
        if (block == 0x3B) break; // Конец GIF

        if (block == 0x21) { // Extension
            if (pos >= d.size()) break;
            uint8_t extLabel = d[pos++];
            if (extLabel == 0xF9 && pos + 6 <= d.size()) { // Graphic Control Extension
                uint8_t blkSize = d[pos++];
                if (blkSize >= 4) {
                    uint8_t gcePacked = d[pos];
                    uint16_t delayCs = d[pos + 1] | (d[pos + 2] << 8);
                    uint8_t tIdx = d[pos + 3];
                    transIndex = (gcePacked & 1) ? tIdx : -1;
                    delaySec = std::max(0.03f, static_cast<float>(delayCs) / 100.f);
                }
                pos += blkSize;
                if (pos < d.size() && d[pos] == 0) pos++;
            } else {
                while (pos < d.size()) {
                    uint8_t subLen = d[pos++];
                    if (subLen == 0) break;
                    pos += subLen;
                }
            }
        }
        else if (block == 0x2C) { // Image Descriptor (Кадр)
            if (pos + 9 > d.size()) break;
            int left = d[pos] | (d[pos + 1] << 8);
            int top  = d[pos + 2] | (d[pos + 3] << 8);
            int w    = d[pos + 4] | (d[pos + 5] << 8);
            int h    = d[pos + 6] | (d[pos + 7] << 8);
            uint8_t imgPacked = d[pos + 8];
            pos += 9;

            std::vector<uint8_t> const* pal = &gct;
            std::vector<uint8_t> lct;
            if (imgPacked & 0x80) {
                int lctSize = 1 << ((imgPacked & 7) + 1);
                if (pos + lctSize * 3 > d.size()) break;
                lct.assign(d.begin() + pos, d.begin() + pos + lctSize * 3);
                pos += lctSize * 3;
                pal = &lct;
            }

            if (pos >= d.size()) break;
            int minCodeSize = d[pos++];
            if (minCodeSize < 2 || minCodeSize > 11) break;

            std::vector<uint8_t> compressed;
            while (pos < d.size()) {
                uint8_t subLen = d[pos++];
                if (subLen == 0) break;
                if (pos + subLen > d.size()) break;
                compressed.insert(compressed.end(), d.begin() + pos, d.begin() + pos + subLen);
                pos += subLen;
            }

            // Быстрая распаковка LZW
            int clearCode = 1 << minCodeSize;
            int eoiCode = clearCode + 1;
            int availCode = clearCode + 2;
            int codeSize = minCodeSize + 1;
            int codeMask = (1 << codeSize) - 1;
            int oldCode = -1;

            std::vector<int> prefix(4096, 0);
            std::vector<uint8_t> suffix(4096, 0);
            std::vector<uint8_t> stack(4096, 0);
            for (int i = 0; i < clearCode; i++) suffix[i] = static_cast<uint8_t>(i);

            int bitBuf = 0, bitCnt = 0, firstChar = 0, sp = 0;
            size_t bytePos = 0;
            int pxIdx = 0, totalPx = w * h;

            while (pxIdx < totalPx) {
                while (sp > 0 && pxIdx < totalPx) {
                    uint8_t colorIdx = stack[--sp];
                    int cx = left + (pxIdx % w);
                    int cy = top + (pxIdx / w);
                    if (cx >= 0 && cx < canvasW && cy >= 0 && cy < canvasH) {
                        if (colorIdx != transIndex && static_cast<size_t>(colorIdx) * 3 + 2 < pal->size()) {
                            size_t dst = (static_cast<size_t>(cy) * canvasW + cx) * 4;
                            canvas[dst + 0] = (*pal)[colorIdx * 3 + 0];
                            canvas[dst + 1] = (*pal)[colorIdx * 3 + 1];
                            canvas[dst + 2] = (*pal)[colorIdx * 3 + 2];
                            canvas[dst + 3] = 255;
                        }
                    }
                    pxIdx++;
                }
                if (pxIdx >= totalPx) break;

                while (bitCnt < codeSize && bytePos < compressed.size()) {
                    bitBuf |= static_cast<int>(compressed[bytePos++]) << bitCnt;
                    bitCnt += 8;
                }
                if (bitCnt < codeSize) break;

                int code = bitBuf & codeMask;
                bitBuf >>= codeSize;
                bitCnt -= codeSize;

                if (code == clearCode) {
                    codeSize = minCodeSize + 1;
                    codeMask = (1 << codeSize) - 1;
                    availCode = clearCode + 2;
                    oldCode = -1;
                    continue;
                }
                if (code == eoiCode) break;

                if (oldCode == -1) {
                    firstChar = suffix[code];
                    stack[sp++] = static_cast<uint8_t>(firstChar);
                    oldCode = code;
                    continue;
                }

                int inCode = code;
                if (code >= availCode) {
                    stack[sp++] = static_cast<uint8_t>(firstChar);
                    code = oldCode;
                }
                while (code >= clearCode && sp < 4095) {
                    stack[sp++] = suffix[code];
                    code = prefix[code];
                }
                firstChar = suffix[code];
                if (sp < 4095) stack[sp++] = static_cast<uint8_t>(firstChar);

                if (availCode < 4096) {
                    prefix[availCode] = oldCode;
                    suffix[availCode] = static_cast<uint8_t>(firstChar);
                    availCode++;
                    if ((availCode & codeMask) == 0 && availCode < 4096) {
                        codeSize++;
                        codeMask = (1 << codeSize) - 1;
                    }
                }
                oldCode = inCode;
            }

            // Даунскейл кадра под выбранное качество (360p/640p/1080p) и загрузка в текстуру GPU
            for (int y = 0; y < outH; y++) {
                int srcY = std::min(canvasH - 1, y * downScale);
                for (int x = 0; x < outW; x++) {
                    int srcX = std::min(canvasW - 1, x * downScale);
                    size_t sOff = (static_cast<size_t>(srcY) * canvasW + srcX) * 4;
                    size_t dOff = (static_cast<size_t>(y) * outW + x) * 4;
                    scaledBuf[dOff + 0] = canvas[sOff + 0];
                    scaledBuf[dOff + 1] = canvas[sOff + 1];
                    scaledBuf[dOff + 2] = canvas[sOff + 2];
                    scaledBuf[dOff + 3] = canvas[sOff + 3];
                }
            }

            auto tex = new CCTexture2D();
            if (tex->initWithData(scaledBuf.data(), kCCTexture2DPixelFormat_RGBA8888, outW, outH, CCSize(outW, outH))) {
                outFrames.push_back(tex);
                outDelays.push_back(delaySec);
            }
            tex->release();

            // Ограничиваем максимум 48 кадров в RAM для стабильности на мобильных устройствах
            if (outFrames.size() >= 48) break;
        } else {
            break;
        }
    }

    return !outFrames.empty();
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

void ModMenuPopup::createScreenBlur() {
    auto ws = CCDirector::get()->getWinSize();
    m_screenBlurHost = CCNode::create();
    m_screenBlurHost->setPosition({0.f, 0.f});
    this->addChild(m_screenBlurHost, -50);
    m_blurPasses.clear();

    auto scene = CCDirector::get()->getRunningScene();
    if (scene) {
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

// ----- Полное заполнение обоев/GIF ровно по форме окна меню со скруглёнными углами! -----
void ModMenuPopup::updateWallpaperClipMask() {
    if (!m_wallpaperStencil) return;
    m_wallpaperStencil->clear();
    float menuRad = std::clamp(mm::state().menuRadius, 0.f, 28.f);
    drawRoundRect(m_wallpaperStencil, ccp(1.f, 1.f), CCSize(WIN_W - 2.f, WIN_H - 2.f),
                  std::max(0.f, menuRad - 1.f), {1.f, 1.f, 1.f, 1.f});
}

void ModMenuPopup::fitWallpaperSprite(CCTexture2D* tex) {
    if (!m_wallpaperSprite || !tex) return;
    m_wallpaperSprite->setTexture(tex);
    CCSize ts = tex->getContentSize();
    m_wallpaperSprite->setTextureRect(CCRect(0.f, 0.f, ts.width, ts.height));
    m_wallpaperSprite->setPosition({WIN_W * 0.5f, WIN_H * 0.5f});

    // Режим Aspect-Fill (Cover): заполняет 100% ширины и высоты меню без чёрных полос!
    float scaleX = WIN_W / std::max(1.f, ts.width);
    float scaleY = WIN_H / std::max(1.f, ts.height);
    float coverScale = std::max(scaleX, scaleY);
    m_wallpaperSprite->setScale(coverScale);
}

void ModMenuPopup::reloadCustomWallpaper() {
    m_gifFrames.clear();
    m_gifDelays.clear();
    m_gifCurFrame = 0;
    m_gifTimer = 0.f;

    std::string const& path = mm::state().customBgPath;
    m_loadedBgPath = path;
    m_loadedGifQuality = mm::state().gifQuality;

    if (path.empty() || !std::filesystem::exists(path)) {
        if (m_wallpaperClip) m_wallpaperClip->setVisible(false);
        return;
    }

    // 1. Пробуем декодировать как анимированный .GIF
    if (decodeGifFile(path, m_loadedGifQuality, m_gifFrames, m_gifDelays)) {
        if (m_wallpaperSprite && !m_gifFrames.empty()) {
            this->fitWallpaperSprite(m_gifFrames[0]);
        }
        return;
    }

    // 2. Иначе загружаем как обычное изображение (.png / .jpg / .webp)
    CCTextureCache::sharedTextureCache()->removeTextureForKey(path.c_str());
    if (auto tex = CCTextureCache::sharedTextureCache()->addImage(path.c_str(), false)) {
        m_gifFrames.push_back(tex);
        m_gifDelays.push_back(1.0f);
        if (m_wallpaperSprite) {
            this->fitWallpaperSprite(tex);
        }
    }
}

void ModMenuPopup::updateCustomWallpaper(float dt) {
    if (!m_wallpaperClip || !m_wallpaperSprite) return;

    bool wpOn = mm::on(mm::F::CustomWallpaper) && !mm::state().customBgPath.empty();
    if (!wpOn) {
        m_wallpaperClip->setVisible(false);
        return;
    }

    if (m_loadedBgPath != mm::state().customBgPath || m_loadedGifQuality != mm::state().gifQuality) {
        this->reloadCustomWallpaper();
    }

    if (m_gifFrames.empty()) {
        m_wallpaperClip->setVisible(false);
        return;
    }

    m_wallpaperClip->setVisible(true);
    m_wallpaperSprite->setOpacity(static_cast<GLubyte>(255.f * std::clamp(mm::state().gifOpacity, 0.15f, 1.f)));

    // Если это GIF с несколькими кадрами — переключаем кадры по их таймингу
    if (m_gifFrames.size() > 1) {
        float realDt = std::clamp(dt / std::max(mm::speed(), 0.25f), 0.002f, 0.05f);
        m_gifTimer += realDt;
        float curDelay = m_gifDelays[m_gifCurFrame];
        if (m_gifTimer >= curDelay) {
            m_gifTimer -= curDelay;
            m_gifCurFrame = (m_gifCurFrame + 1) % m_gifFrames.size();
            this->fitWallpaperSprite(m_gifFrames[m_gifCurFrame]);
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

    // Скруглённый контейнер CCClippingNode для пользовательских обоев и GIF на 100% площади меню!
    m_wallpaperStencil = CCDrawNode::create();
    m_wallpaperClip = CCClippingNode::create(m_wallpaperStencil);
    m_wallpaperClip->setAlphaThreshold(0.05f);
    m_wallpaperClip->setPosition({0.f, 0.f});
    m_wallpaperClip->setVisible(false);

    m_wallpaperSprite = CCSprite::create();
    m_wallpaperSprite->setPosition({WIN_W * 0.5f, WIN_H * 0.5f});
    m_wallpaperClip->addChild(m_wallpaperSprite, 1);
    m_mainLayer->addChild(m_wallpaperClip, -8);

    this->updateWallpaperClipMask();
    this->reloadCustomWallpaper();

    m_particleNode = CCDrawNode::create();
    m_mainLayer->addChild(m_particleNode, -5);

    // ----- Верхняя шапка Loon Client -----
    auto logoL = CCLabelBMFont::create("L", "bigFont.fnt");
    logoL->setScale(0.46f);
    logoL->setPosition({24.f, WIN_H - 21.f});
    m_mainLayer->addChild(logoL, 5);

    auto brandLbl = CCLabelBMFont::create("LOON CLIENT", "chatFont.fnt");
    brandLbl->setScale(0.78f);
    brandLbl->setAnchorPoint({0.f, 0.5f});
    brandLbl->setPosition({38.f, WIN_H - 21.f});
    m_mainLayer->addChild(brandLbl, 5);

    // Основная иконка игрока из гаража GD + Ник игрока
    {
        auto gm = GameManager::get();
        std::string playerName = (gm && !gm->m_playerName.empty()) ? std::string(gm->m_playerName) : "Player";
        int cubeFrame = gm ? gm->getPlayerFrame() : 1;

        auto pNode = CCNode::create();
        pNode->setContentSize({145.f, 26.f});

        auto playerIcon = SimplePlayer::create(cubeFrame);
        if (playerIcon && gm) {
            playerIcon->setColor(gm->colorForIdx(gm->getPlayerColor()));
            playerIcon->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
            if (gm->getPlayerGlow()) {
                playerIcon->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));
            }
            playerIcon->setScale(0.55f);
            playerIcon->setPosition({14.f, 13.f});
            pNode->addChild(playerIcon, 2);
        }

        auto uName = CCLabelBMFont::create(playerName.c_str(), "chatFont.fnt");
        uName->setScale(0.68f);
        uName->setAnchorPoint({0.f, 0.5f});
        uName->setPosition({29.f, 13.f});
        uName->setColor({240, 244, 255});
        pNode->addChild(uName, 2);

        auto qBtn = CCMenuItemSpriteExtra::create(pNode, this, menu_selector(ModMenuPopup::onModInfo));
        qBtn->m_scaleMultiplier = 1.04f;
        qBtn->setPosition({215.f, WIN_H - 21.f});
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

    // ----- НИЖНЯЯ ДОК-ПАНЕЛЬ ВКЛАДОК -----
    m_tabSlider = CCDrawNode::create();
    m_tabSlider->setPosition({tabX(0), DOCK_Y});
    m_mainLayer->addChild(m_tabSlider, 4);

    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({0.f, 0.f});
    for (int i = 0; i < TAB_COUNT; i++) {
        auto bNode = CCNode::create();
        bNode->setContentSize({76.f, 28.f});

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
        btn->m_scaleMultiplier = 1.05f;
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
    m_gifFrames.clear();
    Popup::onExit();
}

void ModMenuPopup::initParticles() {
    m_particles.clear();
    m_particles.resize(24);
    for (int i = 0; i < 24; i++) {
        m_particles[i] = {
            ccp(std::rand() % static_cast<int>(WIN_W), std::rand() % static_cast<int>(WIN_H)),
            ccp(((std::rand() % 100) / 100.f - 0.5f) * 16.f, -16.f - (std::rand() % 20)),
            2.2f + (std::rand() % 24) / 10.f,
            static_cast<float>(std::rand() % 360),
            ((std::rand() % 100) / 100.f - 0.5f) * 2.8f,
            0.40f + (std::rand() % 45) / 100.f
        };
    }
}

void ModMenuPopup::update(float dt) {
    // 1. Обновляем импортированные обои / GIF
    this->updateCustomWallpaper(dt);

    // 2. Обновляем партиклы меню
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
            drawSmoothCircle(m_particleNode, p.pos, p.size * 1.25f, P({240, 248, 255}, p.alpha * 0.28f), 20);
            drawSmoothCircle(m_particleNode, p.pos, p.size * 0.78f, P({255, 255, 255}, p.alpha), 28);
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

    m_bgNode->drawSegment(ccp(14.f, WIN_H - 38.f), ccp(WIN_W - 14.f, WIN_H - 38.f), 0.5f, P({255, 255, 255}, 0.08f));

    float dockRad = std::min(12.f, menuRad * 0.6f + 2.f);
    drawLoonGlass(m_bgNode, ccp(20.f, 7.f), CCSize(WIN_W - 40.f, 32.f), dockRad, card, 0.92f, {255, 255, 255}, 0.12f);

    this->updateWallpaperClipMask();
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
    m_previewBtnRoot->setScale(std::clamp(mm::state().btnScale * 0.85f, 0.50f, 1.25f));
}

void ModMenuPopup::drawSectionBox(CCNode* parent, float x, float yTop, float w, int rowCount, char const* sectionTitle) {
    float hdrH = 22.f;
    float rowH = 26.f;
    float totalH = hdrH + rowCount * rowH + 4.f;
    float yBot = yTop - totalH;
    float boxRad = std::clamp(mm::state().menuRadius * 0.45f, 2.f, 11.f);

    auto d = CCDrawNode::create();
    drawLoonGlass(d, ccp(x, yBot), CCSize(w, totalH), boxRad, mm::themeCard(), 0.90f, {255, 255, 255}, 0.10f);
    d->drawSegment(ccp(x + 8.f, yTop - hdrH), ccp(x + w - 8.f, yTop - hdrH), 0.5f, P({255, 255, 255}, 0.07f));
    parent->addChild(d, 1);

    auto lbl = CCLabelBMFont::create(sectionTitle, "chatFont.fnt");
    lbl->setScale(0.70f);
    lbl->setAnchorPoint({0.f, 0.5f});
    lbl->setPosition({x + 10.f, yTop - 11.f});
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
    ccColor3B swCol = enabled ? (risky ? CHEAT_RED : slCol) : ccColor3B{40, 43, 55};
    drawRoundRect(swDraw, ccp(2.f, 2.f), CCSize(22.f, 10.f), 5.f, P(swCol, 1.f));
    float knobX = enabled ? 19.f : 7.f;
    drawSmoothCircle(swDraw, ccp(knobX, 7.f), 3.8f, P(enabled ? ccColor3B{255, 255, 255} : ccColor3B{130, 136, 152}, 1.f), 32);
    swNode->addChild(swDraw);

    auto swBtn = CCMenuItemSpriteExtra::create(swNode, this, menu_selector(ModMenuPopup::onToggle));
    swBtn->m_scaleMultiplier = 1.05f;
    swBtn->setTag(static_cast<int>(flag));
    swBtn->setPosition({x + w - 18.f, cy});
    m_mainCardsMenu->addChild(swBtn);

    if (hasSubOptions) {
        auto dotsNode = CCNode::create();
        dotsNode->setContentSize({18.f, 16.f});
        auto dotsDraw = CCDrawNode::create();
        for (int i = 0; i < 3; i++) {
            drawSmoothCircle(dotsDraw, ccp(4.f + i * 4.5f, 8.f), 1.2f, P({200, 206, 225}, 0.95f), 16);
        }
        dotsNode->addChild(dotsDraw);

        auto dotsBtn = CCMenuItemSpriteExtra::create(dotsNode, this, menu_selector(ModMenuPopup::onOpenSub));
        dotsBtn->m_scaleMultiplier = 1.08f;
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
    tLbl->setScale(0.64f);
    tLbl->setAnchorPoint({0.f, 0.5f});
    tLbl->setPosition({x, y + 9.5f});
    tLbl->setColor(mm::themeText());
    host->addChild(tLbl, 4);

    auto vLbl = CCLabelBMFont::create("", "chatFont.fnt");
    vLbl->setScale(0.64f);
    vLbl->setAnchorPoint({1.f, 0.5f});
    vLbl->setPosition({x + w, y + 9.5f});
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
    float y = sl.rect.origin.y + 5.5f;
    float w = sl.rect.size.width;

    auto barCol = mm::sliderColor();
    drawRoundRect(sl.drawNode, ccp(x, y - 2.2f), CCSize(w, 4.4f), 2.2f, P({36, 40, 52}, 1.f));
    if (t > 0.01f) {
        drawRoundRect(sl.drawNode, ccp(x, y - 2.2f), CCSize(std::max(4.4f, w * t), 4.4f), 2.2f, P(barCol, 1.f));
    }
    float kx = x + w * t;
    if (mm::on(mm::F::Glow)) {
        drawSmoothCircle(sl.drawNode, ccp(kx, y), 7.2f, P({255, 255, 255}, 0.25f * mm::state().glowIntensity), 28);
    }
    drawSmoothCircle(sl.drawNode, ccp(kx, y), 4.8f, P({255, 255, 255}, 1.f), 32);
    drawSmoothCircle(sl.drawNode, ccp(kx, y), 2.8f, P(barCol, 1.f), 32);

    std::string txt = fmt::format("{:.0f}{}", val, sl.suffix);
    sl.valLabel->setString(txt.c_str());
}

void ModMenuPopup::buildVisualPalette(CCNode* host, float x, float y) {
    m_hasPalette = true;
    float svW = 240.f, svH = 82.f;
    float hueH = 12.f;

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
    drawSmoothCircle(m_svCursor, svPt, 5.5f, P({255, 255, 255}, 1.f), 28);
    drawSmoothCircle(m_svCursor, svPt, 4.0f, P(picked, 1.f), 28);

    m_hueCursor->clear();
    CCPoint hPt = ccp(m_hueRect.origin.x + h * m_hueRect.size.width,
                      m_hueRect.origin.y + m_hueRect.size.height * 0.5f);
    drawSmoothCircle(m_hueCursor, hPt, 6.0f, P({255, 255, 255}, 1.f), 28);
    drawSmoothCircle(m_hueCursor, hPt, 4.4f, P(pureHue, 1.f), 28);
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
            CCEaseSineOut::create(CCScaleTo::create(0.05f, pwr)),
            CCEaseBackOut::create(CCScaleTo::create(0.12f, 1.0f)),
            nullptr
        ));
    } else {
        target->runAction(CCSequence::create(
            CCScaleTo::create(0.04f, pwr),
            CCEaseExponentialOut::create(CCScaleTo::create(0.11f, 1.0f)),
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

    float col0X = 16.f;
    float col1X = 236.f;
    float boxW = 208.f;
    float topY = WIN_H - 45.f;

    if (m_currentTab == 0) {
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "Movement");
        this->addModuleRow(col0X, topY - 22.f, boxW, mm::F::Noclip, false, true);
        this->addModuleRow(col0X, topY - 48.f, boxW, mm::F::Jump, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 2, "Protection");
        this->addModuleRow(col1X, topY - 22.f, boxW, mm::F::Safe, false, true);
        this->addModuleRow(col1X, topY - 48.f, boxW, mm::F::AutoSafe, false, false);

        float spTop = topY - 90.f;
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
            btn->m_scaleMultiplier = 1.04f;
            btn->setTag(i);
            btn->setPosition({col0X + 42.f + i * 68.f, spTop - 48.f});
            m_mainCardsMenu->addChild(btn);
        }
    }
    else if (m_currentTab == 1) {
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 3, "Level & Bypass");
        this->addModuleRow(col0X, topY - 22.f, boxW, mm::F::TpsBypass, false, true);
        this->addModuleRow(col0X, topY - 48.f, boxW, mm::F::Hitboxes, false, true);
        this->addModuleRow(col0X, topY - 74.f, boxW, mm::F::NoDeath, false, false);

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
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 4, "Interface");
        this->addModuleRow(col0X, topY - 22.f, boxW, mm::F::Overlay, true, true);
        this->addModuleRow(col0X, topY - 48.f, boxW, mm::F::ShowFPS, false, true);
        this->addModuleRow(col0X, topY - 74.f, boxW, mm::F::HideAttempts, false, true);
        this->addModuleRow(col0X, topY - 100.f, boxW, mm::F::UnlockAll, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 5, "World & FX");
        this->addModuleRow(col1X, topY - 22.f, boxW, mm::F::NoShake, false, true);
        this->addModuleRow(col1X, topY - 48.f, boxW, mm::F::NoMirror, false, true);
        this->addModuleRow(col1X, topY - 74.f, boxW, mm::F::HidePlayer, false, true);
        this->addModuleRow(col1X, topY - 100.f, boxW, mm::F::RainbowIcon, false, true);
        this->addModuleRow(col1X, topY - 126.f, boxW, mm::F::RainbowBG, false, false);
    }
    else if (m_currentTab == 3) {
        // THEMES: Loon Glass + Screen Blur + Импорт своих обоев/GIF!
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "Loon Surface");
        this->addModuleRow(col0X, topY - 22.f, boxW, mm::F::LiquidGlass, true, true);
        this->addModuleRow(col0X, topY - 48.f, boxW, mm::F::BgBlur, true, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 2, "Wallpaper & Bloom");
        this->addModuleRow(col1X, topY - 22.f, boxW, mm::F::Glow, true, true);
        this->addModuleRow(col1X, topY - 48.f, boxW, mm::F::CustomWallpaper, true, false); // "..." открывает импорт своих фото и GIF!

        // Кнопки выбора тем с ОТКЛЮЧЁННЫМ раздуванием (m_scaleMultiplier = 1.0f), чтобы никогда не налезали друг на друга!
        for (int i = 0; i < 8; i++) {
            int col = i % 2;
            int row = i / 2;
            float cardW = boxW, cardH = 24.f;
            float x = (col == 0 ? col0X : col1X);
            float y = topY - 106.f - row * (cardH + 5.f);

            bool sel = (mm::state().themeIdx == i);
            auto node = CCNode::create();
            node->setContentSize({cardW, cardH});
            auto d = CCDrawNode::create();
            drawLoonGlass(d, ccp(0.f, 0.f), CCSize(cardW, cardH), 6.f,
                          mm::THEMES[i].card, 0.94f,
                          sel ? acc : ccColor3B{255, 255, 255}, sel ? 0.90f : 0.10f);

            ccColor3B swCol = (i == 7) ? mm::hsv(mm::state().customH[0], mm::state().customS[0], mm::state().customV[0])
                                       : mm::THEMES[i].accent;
            drawRoundRect(d, ccp(cardW - 30.f, 3.f), CCSize(22.f, 18.f), 4.f, P(swCol, 1.f));
            node->addChild(d);

            auto lbl = CCLabelBMFont::create(mm::THEMES[i].name, "chatFont.fnt");
            lbl->setScale(0.65f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({10.f, cardH * 0.5f});
            lbl->setColor(sel ? acc : txtCol);
            node->addChild(lbl);

            auto btn = CCMenuItemSpriteExtra::create(node, this, menu_selector(ModMenuPopup::onThemeSelect));
            btn->m_scaleMultiplier = 1.0f; // НЕ увеличивается и НЕ вылезает за соседние кнопки!
            btn->setTag(i);
            btn->setPosition({x + cardW * 0.5f, y + cardH * 0.5f});
            m_mainCardsMenu->addChild(btn);
        }
    }
    else if (m_currentTab == 4) {
        // CLIENT
        this->drawSectionBox(m_contentHost, col0X, topY, boxW, 2, "L-Button Visibility");
        this->addModuleRow(col0X, topY - 22.f, boxW, mm::F::BtnHideInGame, false, true);
        this->addModuleRow(col0X, topY - 48.f, boxW, mm::F::BtnShowInPause, false, false);

        this->drawSectionBox(m_contentHost, col1X, topY, boxW, 3, "L-Button Sliders");
        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 38.f, boxW - 20.f,
                               "Button Size", "%", 55.f, 150.f, true,
                               [] { return mm::state().btnScale * 100.f; },
                               [this](float v) {
                                   mm::state().btnScale = v / 100.f;
                                   this->redrawLButtonPreview();
                               });

        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 64.f, boxW - 20.f,
                               "Opacity", "%", 20.f, 100.f, true,
                               [] { return mm::state().btnOpacity * 100.f; },
                               [this](float v) {
                                   mm::state().btnOpacity = v / 100.f;
                                   this->redrawLButtonPreview();
                               });

        this->addSliderControl(m_contentHost, col1X + 10.f, topY - 90.f, boxW - 20.f,
                               "Corner Radius", " px", 0.f, 17.f, true,
                               [] { return mm::state().btnRadius; },
                               [this](float v) {
                                   mm::state().btnRadius = v;
                                   this->redrawLButtonPreview();
                               });

        float botTop = topY - 117.f;
        this->drawSectionBox(m_contentHost, col0X, botTop, boxW, 2, "Menu Particles");
        char const* pNames[4] = {"Off", "Win", "Aut", "Sak"};
        for (int i = 0; i < 4; i++) {
            bool sel = (mm::state().particleType == i);
            auto n = CCNode::create();
            n->setContentSize({42.f, 22.f});
            auto d = CCDrawNode::create();
            drawLoonGlass(d, ccp(0.f, 0.f), CCSize(42.f, 22.f), 5.f, sel ? slCol : ccColor3B{38, 42, 56}, 0.96f);
            n->addChild(d);
            auto l = CCLabelBMFont::create(pNames[i], "chatFont.fnt");
            l->setScale(0.60f);
            l->setPosition({21.f, 11.f});
            l->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            n->addChild(l);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onParticleSelect));
            b->m_scaleMultiplier = 1.04f;
            b->setTag(i);
            b->setPosition({col0X + 30.f + i * 48.f, botTop - 48.f});
            m_mainCardsMenu->addChild(b);
        }

        this->drawSectionBox(m_contentHost, col1X, botTop, boxW, 2, "Live Button Preview");
        m_previewBtnRoot = CCNode::create();
        m_previewBtnRoot->ignoreAnchorPointForPosition(false);
        m_previewBtnRoot->setAnchorPoint({0.5f, 0.5f});
        m_previewBtnRoot->setContentSize({38.f, 38.f});
        m_previewBtnRoot->setPosition({col1X + boxW * 0.5f, botTop - 50.f});

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
    backBtn->m_scaleMultiplier = 1.04f;
    backBtn->setPosition({52.f, WIN_H - 58.f});
    subMenu->addChild(backBtn);

    if (m_activeSubFeature == mm::F::CustomWallpaper) {
        // Меню импорта своих изображений (.png / .jpg) и своих GIF (.gif) на весь фон меню!
        auto hdr = CCLabelBMFont::create("Custom Wallpaper & GIF Importer", "chatFont.fnt");
        hdr->setScale(0.75f);
        hdr->setColor(acc);
        hdr->setPosition({WIN_W * 0.5f + 20.f, WIN_H - 58.f});
        m_subPopupHost->addChild(hdr, 2);

        // Кнопка "Import Image / GIF"
        {
            auto impNode = CCNode::create();
            impNode->setContentSize({185.f, 28.f});
            auto impDraw = CCDrawNode::create();
            drawLoonGlass(impDraw, ccp(0.f, 0.f), CCSize(185.f, 28.f), 7.f, slCol, 0.96f, acc, 0.9f);
            impNode->addChild(impDraw);
            auto impLbl = CCLabelBMFont::create("Import Image / GIF...", "chatFont.fnt");
            impLbl->setScale(0.68f);
            impLbl->setPosition({92.5f, 14.f});
            impNode->addChild(impLbl);

            auto impBtn = CCMenuItemSpriteExtra::create(impNode, this, menu_selector(ModMenuPopup::onImportWallpaper));
            impBtn->m_scaleMultiplier = 1.03f;
            impBtn->setPosition({132.f, 190.f});
            subMenu->addChild(impBtn);
        }

        // Кнопка "Clear Wallpaper"
        {
            auto clrNode = CCNode::create();
            clrNode->setContentSize({165.f, 28.f});
            auto clrDraw = CCDrawNode::create();
            drawLoonGlass(clrDraw, ccp(0.f, 0.f), CCSize(165.f, 28.f), 7.f, mm::themeCard(), 0.96f, CHEAT_RED, 0.7f);
            clrNode->addChild(clrDraw);
            auto clrLbl = CCLabelBMFont::create("Clear Wallpaper", "chatFont.fnt");
            clrLbl->setScale(0.68f);
            clrLbl->setColor(CHEAT_RED_ON);
            clrLbl->setPosition({82.5f, 14.f});
            clrNode->addChild(clrLbl);

            auto clrBtn = CCMenuItemSpriteExtra::create(clrNode, this, menu_selector(ModMenuPopup::onClearWallpaper));
            clrBtn->m_scaleMultiplier = 1.03f;
            clrBtn->setPosition({332.f, 190.f});
            subMenu->addChild(clrBtn);
        }

        // Выбор качества GIF (360p / 640p / 1080p) для отсутствия лагов
        auto qTitle = CCLabelBMFont::create("GIF Decode Quality (Optimization):", "chatFont.fnt");
        qTitle->setScale(0.64f);
        qTitle->setAnchorPoint({0.f, 0.5f});
        qTitle->setPosition({34.f, 152.f});
        m_subPopupHost->addChild(qTitle, 2);

        char const* qNames[3] = {"360p (Fast)", "640p (Balanced)", "1080p (Original)"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().gifQuality == i);
            auto n = CCNode::create();
            n->setContentSize({122.f, 24.f});
            auto dr = CCDrawNode::create();
            drawLoonGlass(dr, ccp(0.f, 0.f), CCSize(122.f, 24.f), 6.f, sel ? slCol : mm::themeCard(), 0.96f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(qNames[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({61.f, 12.f});
            lb->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onGifQualitySelect));
            b->m_scaleMultiplier = 1.03f;
            b->setTag(i);
            b->setPosition({95.f + i * 134.f, 126.f});
            subMenu->addChild(b);
        }

        this->addSliderControl(m_subPopupHost, 34.f, 74.f, 392.f,
                               "Wallpaper / GIF Opacity", "%", 15.f, 100.f, true,
                               [] { return mm::state().gifOpacity * 100.f; },
                               [](float v) { mm::state().gifOpacity = v / 100.f; });
    }
    else if (m_activeSubFeature == mm::F::Overlay) {
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
            b->m_scaleMultiplier = 1.03f;
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
            b->m_scaleMultiplier = 1.03f;
            b->setTag(i);
            b->setPosition({78.f + i * 100.f, 66.f});
            subMenu->addChild(b);
        }
    }
    else if (m_activeSubFeature == mm::F::LiquidGlass || m_activeSubFeature == mm::F::BgBlur) {
        // Настройки Loon Glass / Screen Blur / Menu Corners (БЕЗ палитры, чтобы случайно не сбивать тему на Custom Palette!)
        auto hdr = CCLabelBMFont::create("Loon Surface & Blur Settings", "chatFont.fnt");
        hdr->setScale(0.76f);
        hdr->setColor(acc);
        hdr->setPosition({WIN_W * 0.5f + 20.f, WIN_H - 58.f});
        m_subPopupHost->addChild(hdr, 2);

        this->addSliderControl(m_subPopupHost, 40.f, 182.f, 380.f,
                               "Screen Blur Strength", "%", 0.f, 100.f, true,
                               [] { return mm::state().bgBlurStrength * 100.f; },
                               [this](float v) {
                                   mm::state().bgBlurStrength = v / 100.f;
                                   this->updateScreenBlurOpacity();
                               });

        this->addSliderControl(m_subPopupHost, 40.f, 138.f, 380.f,
                               "Glass Opacity", "%", 40.f, 96.f, true,
                               [] { return mm::state().glassAlpha * 100.f; },
                               [this](float v) {
                                   mm::state().glassAlpha = v / 100.f;
                                   this->redrawFrame();
                               });

        this->addSliderControl(m_subPopupHost, 40.f, 94.f, 380.f,
                               "Menu Corner Radius", " px", 0.f, 28.f, true,
                               [] { return mm::state().menuRadius; },
                               [this](float v) {
                                   mm::state().menuRadius = v;
                                   this->redrawFrame();
                               });
    }
    else if (m_activeSubFeature == mm::F::Glow) {
        // Если нажали "..." на White Glow ИЛИ выбрали тему "Custom Palette":
        this->addSliderControl(m_subPopupHost, 32.f, 198.f, 396.f,
                               "White Glow Intensity", "%", 0.f, 100.f, true,
                               [] { return mm::state().glowIntensity * 100.f; },
                               [this](float v) {
                                   mm::state().glowIntensity = v / 100.f;
                                   this->redrawFrame();
                                   this->redrawTabSlider();
                               });

        char const* slots[3] = {"Accent Color", "Background", "Containers"};
        for (int i = 0; i < 3; i++) {
            bool sel = (mm::state().editSlot == i);
            auto n = CCNode::create();
            n->setContentSize({116.f, 24.f});
            auto dr = CCDrawNode::create();
            drawLoonGlass(dr, ccp(0.f, 0.f), CCSize(116.f, 24.f), 6.f, sel ? slCol : mm::themeCard(), 0.96f);
            n->addChild(dr);
            auto lb = CCLabelBMFont::create(slots[i], "chatFont.fnt");
            lb->setScale(0.62f);
            lb->setPosition({58.f, 12.f});
            lb->setColor(sel ? ccColor3B{255, 255, 255} : txtCol);
            n->addChild(lb);
            auto b = CCMenuItemSpriteExtra::create(n, this, menu_selector(ModMenuPopup::onColorSlotSelect));
            b->m_scaleMultiplier = 1.03f;
            b->setTag(i);
            b->setPosition({358.f, 134.f - i * 32.f});
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
        "<cg>Wallpaper & GIF Importer:</c> Import any .png, .jpg or animated .gif to fill the entire menu background!\n"
        "<cg>Quality Modes:</c> Choose 360p, 640p or 1080p for zero lag.",
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
        m_activeSubFeature = mm::F::Glow;
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

void ModMenuPopup::onGifQualitySelect(CCObject* sender) {
    mm::state().gifQuality = static_cast<CCNode*>(sender)->getTag();
    mm::saveAll();
    this->reloadCustomWallpaper();
    this->rebuildSubPopup(false);
}

// Импорт своей картинки (.png / .jpg) или анимации (.gif) через системный проводник Android!
void ModMenuPopup::onImportWallpaper(CCObject*) {
    file::FilePickOptions opts;
    opts.filters.push_back({"Images & GIFs", {"*.gif", "*.png", "*.jpg", "*.jpeg"}});

    async::spawn(
        file::pick(file::PickMode::OpenFile, opts),
        [self = Ref<ModMenuPopup>(this)](Result<std::optional<std::filesystem::path>> res) {
            if (!res.isOk()) return;
            auto optPath = res.unwrap();
            if (!optPath.has_value()) return;

            std::string chosen = optPath.value().string();
            if (chosen.empty()) return;

            mm::state().customBgPath = chosen;
            mm::set(mm::F::CustomWallpaper, true);
            mm::save(mm::F::CustomWallpaper);
            mm::saveAll();

            if (self) {
                self->reloadCustomWallpaper();
                self-> redrawFrame();
                self->rebuildSubPopup(false);
            }
        }
    );
}

void ModMenuPopup::onClearWallpaper(CCObject*) {
    mm::state().customBgPath.clear();
    mm::set(mm::F::CustomWallpaper, false);
    mm::save(mm::F::CustomWallpaper);
    mm::saveAll();
    this->reloadCustomWallpaper();
    this->redrawFrame();
    this->rebuildSubPopup(false);
}
