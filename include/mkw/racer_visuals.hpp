#pragma once
#include <memory>
#include <citro3d.h>
#include <mkw/math.hpp>
namespace mkw {
class Renderer;
class Model;
class RaceItems;
// Each opponent owns its deforming vertices. Textures remain shared by Model.
class RacerVisuals {
public:
    RacerVisuals();
    ~RacerVisuals();
    bool load();
    void reset();
    void resetAnimation();
    void step(unsigned tick,const RaceItems& items);
    // Poses and culls every opponent for this frame, on the job worker where
    // possible. Call after Renderer::begin; race state must not change until draw.
    void prepare(const Renderer& renderer,const RaceItems& items,Vec3 viewer,unsigned tick,bool fullDetail=false);
    // Draws in racer order; prepares inline first when prepare() was not called.
    // Original vehicle shadow volumes resolve on the world before racers draw.
    void draw(Renderer& renderer,const RaceItems& items,Vec3 viewer,unsigned tick,
              const Model* playerShadow=nullptr,const C3D_Mtx* playerTransform=nullptr);
    // Replayed ghosts (RaceConfig Ghost players) are left out of draw(): call
    // after the opaque world, player and its shadow. Translucent, no shadow volume.
    void drawGhosts(Renderer& renderer);
    unsigned count() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
