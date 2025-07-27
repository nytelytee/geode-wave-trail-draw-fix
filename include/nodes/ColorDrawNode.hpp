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
  
  std::optional<_ccBlendFunc> m_solidColorBlendFunc = std::nullopt;
  std::optional<_ccBlendFunc> m_nonSolidColorBlendFunc = std::nullopt;

  GLenum m_nonSolidColorBlendEq = GL_FUNC_ADD;
  GLenum m_solidColorBlendEq = GL_FUNC_ADD;

  static ColorDrawNode* create(matjson::Value&);

  // instead of using drawPolygon, which is broken in geometry dash for some reason, implement a custom method to draw a triangle
  //   - CCDrawNode::drawPolygon cannot draw triangles correctly, their points are in entirely different positions, for reasons unknown to me
  //   - CCDrawNode::drawPolygon can draw quads, but parts of them overlap, also for reasons unknown to me
  // disadvantage: some mod menus hook drawPolygon in order to implement solid wave trail (formerly, Eclipse did this), so solid wave trail will be broken
  // for mod creators: for a proper implementation of solid wave trail, that works with both this mod and with vanilla, have a look at Eclipse's implementation
  void drawTriangle(CCPoint const&, CCPoint const&, CCPoint const&, _ccColor4B);

  bool init(matjson::Value&);
  void draw() override;

};
