#pragma once

#include "game/kart/KartMove.hh"

namespace Kinoko::Kart {

class KartObject;

/// @brief Hosts a few classes and the high level per-frame calc functions.
/// @nosubgrouping
class KartSub : KartObjectProxy {
public:
    KartSub();
    ~KartSub();

    void createSubsystems(bool isBike, const KartParam::Stats &stats);
    void copyPointers(KartAccessor &pointers);

    void init();
    void initAABB(KartAccessor &accessor, KartObject *object);
    void initPhysicsValues();
    void resetPhysics();

    void calcPass0();
    // mkw-3ds: calcPass0 in two halves (see KartObjectManager::calc).
    void calcPass0A();
    void calcPass0B();
    void calcPass1();
    void resizeAABB(f32 radiusScale);
    void addFloor(const CollisionData &, bool);
    void updateSuspOvertravel(const EGG::Vector3f &suspOvertravel);
    void tryEndHWG();
    void calcMovingObj();
    void calcMovingWater();

    /// @beginGetters
    [[nodiscard]] f32 someScale() {
        return m_someScale;
    }
    /// @endGetters

private:
    bool m_pass0MoveDone = false;
    KartMove *m_move;
    KartAction *m_action;
    KartCollide *m_collide;
    KartState *m_state;
    EGG::Vector3f m_maxSuspOvertravel;
    EGG::Vector3f m_minSuspOvertravel;
    u16 m_floorCollisionCount;
    u16 m_movingObjCollisionCount;
    u16 m_movingWaterCollisionCount;
    EGG::Vector3f m_objVel;
    s16 m_sideCollisionTimer;  ///< Number of frames to apply movement from wall collision.
    f32 m_colPerpendicularity; ///< Dot product between floor and colliding wall normals.
    f32 m_someScale;           /// @rename

    static constexpr f32 DT = 1.0f; ///< Delta time.
};

} // namespace Kinoko::Kart
