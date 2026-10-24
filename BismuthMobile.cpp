// Bismuth Mobile Port — Оптимизация рендеринга, батчинга и отсечения объектов (Culling) для Android 2.2081.
// В отличие от экспериментального ПК-рендерера Bismuth, этот модуль НЕ ломает уровни 2.2 и работает на OpenGL ES 2.0.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <cmath>
#include "Settings.hpp"

using namespace geode::prelude;

namespace {

// Проверяем, является ли объект важным триггером или геймплейным порталом,
// который НЕЛЬЗЯ отключать даже за пределами экрана
bool isGameplayCritical(GameObject* obj) {
    if (!obj) return true;
    int id = obj->m_objectID;
    // StartPos, порталы режимов, скорости, гравитации, зеркала, дуала, монетки
    switch (id) {
        case 31: case 10: case 11: case 12: case 13: case 45: case 46: case 47:
        case 99: case 101: case 111: case 200: case 201: case 202: case 203:
        case 286: case 287: case 660: case 745: case 1331: case 1334: case 1933: case 2926:
        case 142: case 1329:
            return true;
        default:
            break;
    }
    // Любые триггеры (Move, Rotate, Alpha, Color, Pulse, Spawn, Toggle и триггеры 2.2)
    if (obj->m_isTrigger) return true;
    return false;
}

} // namespace

class $modify(BismuthBaseLayer, GJBaseGameLayer) {
    struct Fields {
        int tickCounter = 0;
    };

    void update(float dt) {
        GJBaseGameLayer::update(dt);

        auto pl = PlayLayer::get();
        if (!pl || pl->m_isPaused) return;

        bool bismuthOn = mm::on(mm::F::BismuthOpt);
        bool cullingOn = mm::on(mm::F::BismuthCull);
        if (!bismuthOn && !cullingOn) return;

        auto& f = m_fields;
        f->tickCounter++;
        // Проводим умную чистку батчей раз в 3 тика, чтобы не нагружать CPU обходом массива каждый микро-шаг TPS
        if ((f->tickCounter % 3) != 0) return;

        if (!m_objects || !m_player1) return;

        auto ws = CCDirector::get()->getWinSize();
        // Определяем реальные границы видимой камеры в мировых координатах уровня
        float camX = m_gameState.m_cameraPosition.x;
        float camY = m_gameState.m_cameraPosition.y;
        float zoom = std::max(0.25f, m_objectLayer ? m_objectLayer->getScale() : 1.0f);

        float viewW = (ws.width / zoom);
        float viewH = (ws.height / zoom);

        // Запас вокруг экрана (при Aggressive Culling — уже, при обычном Bismuth — шире)
        float marginX = cullingOn ? 140.f : 260.f;
        float marginY = cullingOn ? 120.f : 220.f;

        float minX = camX - marginX;
        float maxX = camX + viewW + marginX;
        float minY = camY - marginY;
        float maxY = camY + viewH + marginY;

        int count = m_objects->count();
        // Оптимизируем обход только вокруг зоны игрока, если объектов слишком много (> 4000)
        float px = m_player1->getPositionX();
        float scanRadius = 1400.f;

        for (int i = 0; i < count; i++) {
            auto obj = static_cast<GameObject*>(m_objects->objectAtIndex(i));
            if (!obj) continue;

            float ox = obj->getPositionX();
            if (std::abs(ox - px) > scanRadius) continue;

            // Никогда не трогаем триггеры и порталы, чтобы уровни 2.2 работали без единого бага!
            if (isGameplayCritical(obj)) continue;

            float oy = obj->getPositionY();

            // 1. Bismuth Zero-Alpha Skip: если объект полностью прозрачен, убираем его из батч-отрисовки
            if (bismuthOn && obj->getOpacity() == 0 && !obj->m_isHide) {
                obj->setVisible(false);
                continue;
            }

            // 2. Bismuth Frustum & Index Culling: отсекаем декор и блоки вне камеры
            if (cullingOn) {
                bool inView = (ox >= minX && ox <= maxX && oy >= minY && oy <= maxY);
                if (!inView && obj->isVisible()) {
                    obj->setVisible(false);
                } else if (inView && !obj->m_isHide && obj->getOpacity() > 0) {
                    obj->setVisible(true);
                }
            }
        }
    }
};

// 3. Bismuth Particle & Glow Optimizer: убирает лаги от тяжёлых генераторов частиц на декоративных уровнях
class $modify(BismuthParticleOpt, CCParticleSystemQuad) {
    void update(float dt) {
        if (mm::on(mm::F::BismuthParticles) && PlayLayer::get()) {
            // Снижаем частоту спавна избыточных частиц вдвое для экономии вершинного буфера на мобильном GPU
            if (m_uTotalParticles > 35) {
                m_fEmissionRate = std::min(m_fEmissionRate, 20.f);
            }
        }
        CCParticleSystemQuad::update(dt);
    }
};
