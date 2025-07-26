#pragma once

#include <Geode/Geode.hpp>
using namespace geode::prelude;

// the only drawer Wave Trail Draw Fix implements; can be used as an example to implement your own
// things that the drawer can do:
//   draw a path using a color, which may be the hardstreak color (the player color)
//   draw that path using a custom blending function and equation
//   skip drawing if the hardstreak is marked as solid/nonsolid
//   additionally, you can specify an opacity factor (gets multiplied by the parent hardstreak opacity to get a final opacity)
//      e.g. the inner white part of the wave trail is 65% opacity (0.65f), the outer part is 100% opacity (1.f)
class ColorDrawNode : public CCDrawNode {
public:
  // nullopt is player color
  std::optional<_ccColor3B> m_solidColor = std::nullopt;
  std::optional<_ccColor3B> m_nonSolidColor = std::nullopt;
  
  float m_solidOpacityFactor = 1.f;
  float m_nonSolidOpacityFactor = 1.f;
  
  bool m_skipIfSolid = false;
  bool m_skipIfNonSolid = false;
  
  _ccBlendFunc m_solidColorBlendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
  _ccBlendFunc m_nonSolidColorBlendFunc = {GL_SRC_ALPHA, GL_ONE};

  GLenum m_nonSolidColorBlendEq = GL_FUNC_ADD;
  GLenum m_solidColorBlendEq = GL_FUNC_ADD;

  static ColorDrawNode* create(matjson::Value&);

  // instead of using drawPolygon, which is broken in geometry dash for some reason, implement a custom method to draw a triangle
  //   - CCDrawNode::drawPolygon cannot draw triangles correctly, their points are in entirely different positions, for reasons unknown to me
  //   - CCDrawNode::drawPolygon can draw quads, but parts of them overlap, also for reasons unknown to me
  // disadvantage: some mod menus hook drawPolygon in order to implement solid wave trail, so solid wave trail will be broken
  // unless mod menus decide to properly implement solid wave trails; psst, for any mod menu makers reading this, do the following:
  //   - set the blending function to regular alpha blending if solid wave trail is toggled; do it somewhere that isn't drawPolygon, for example in updateStroke (after original);
  //     this will make solid wave trail act correctly if Wave Trail Draw Fix is not installed as well, even if it's not required if WTDrawF is being used, since this node
  //     ignores the blending function, and uses its own, according to the configuration (the default is the correct one for solid wave trail, though)
  //   - set m_isSolid to true, instead of hooking drawPolyon to check if the color being drawn is a non-opaque white and manually ignoring it (WHY WOULD YOU DO THAT
  //     also it breaks a pure white trail fading out when you die lol);
  //     this allows even vanilla gd to automatically ignore the inner part of the trail, since that is the behavior vanilla has with m_isSolid;
  //     this node reimplements such behavior through m_skipIfSolid and m_skipIfNonSolid, so it will also work with WTDrawF
  // here is how i would implement solid wave trail personally:
  /*
    class $modify(MyHardStreak, HardStreak) {
        struct Fields {
            std::optional<bool> m_prevIsSolid = std::nullopt;
        };
        void updateStroke(float dt) {
            if (solidWaveTrailIsOn && !m_fields->m_prevIsSolid) {
                // solid wave trail was just turned on
                m_fields->m_prevIsSolid = m_isSolid;
                m_isSolid = true;
            }  else if (!solidWaveTrailIsOn && m_fields->m_prevIsSolid) {
                // solid wave trail was just turned off
                m_isSolid = *m_fields->m_prevIsSolid;
                m_fields->m_prevIsSolid = std::nullopt;
            }
            HardStreak::updateStroke(dt);
            // afaik the only thing m_isSolid on its own controls in vanilla is whether the inner white part of the trail gets drawn
            // the rest is controlled by the blending function, which has to be set manually
            if (m_isSolid) setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
            else setBlendFunc({GL_SRC_ALPHA, GL_ONE});
        }
    };
  */
  // i guess a drawback of that is that it needs to constantly hook, you cannot dynamically disable the hook depending on whether a hack is on or not
  // do note that the m_prevIsSolid field is required to get it to behave nicely with black trails, which are solid by default; we have to restore the solid state
  // back to solid in case the trail is black, otherwise a black trail would turn invisible after turning off the hack
  void drawTriangle(CCPoint const&, CCPoint const&, CCPoint const&, _ccColor4B);

  bool init(matjson::Value&);
  void draw() override;

};
