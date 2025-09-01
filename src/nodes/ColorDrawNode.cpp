// check include/nodes/ColorDrawNode.hpp for additional comments
#include <nodes/ColorDrawNode.hpp>
#include <constants.hpp>
#include <api/api.hpp>

ColorDrawNode* ColorDrawNode::create(matjson::Value& data) {
  auto* ret = new ColorDrawNode;
  if (ret && ret->init(data)) { ret->autorelease(); return ret; }
  else { delete ret; return nullptr; }
}

// macros to help parse the options
#define SETUP_COLOR(option, member) if(\
    data.get<std::string>(option)\
    .andThen([](std::string value){ return cc3bFromHexString(value); })\
    .isOkAnd([this](ccColor3B value) { member = value; return true; })\
  ) {} else if(data.get(option).isOkAnd([](matjson::Value& value){return value.isNull();})) {\
    member = std::nullopt;\
  } else if(data.contains(option)) return false;

#define SETUP_OPACITY_FACTOR(option, member) if(\
    data.get<double>(option)\
    .isOkAnd([this](double value){ member = static_cast<float>(value); return true; })\
  ) {} else if(data.contains(option)) return false

#define SETUP_SKIP(option, member) if(\
    data.get<bool>(option)\
    .isOkAnd([this](bool value){ member = value; return true; })\
  ){} else if (data.contains(option)) return false

#define SETUP_BLENDING_FUNCTION(option, member) if(\
    data.get<std::string>(option)\
    .andThen([](std::string value) -> Result<ccBlendFunc> {\
      if (blendFuncMap.contains(value)) return Ok(blendFuncMap.at(value));\
      return Err("invalid blending function");\
    }).isOkAnd([this](ccBlendFunc value){ member = value; return true; })\
  ) {} else if(\
    data.get<std::vector<matjson::Value>&>(option)\
    .andThen([](std::vector<matjson::Value> value) -> Result<ccBlendFunc> {\
      if (value.size() != 2) return Err("invalid blending function");\
      if (!value[0].isString() || !value[1].isString()) return Err("invalid blending function");\
      if (!blendEnumMap.contains(*value[0].asString().ok()) || !blendEnumMap.contains(*value[1].asString().ok())) return Err("invalid blending function");\
      return Ok(ccBlendFunc{blendEnumMap.at(*value[0].asString().ok()), blendEnumMap.at(*value[1].asString().ok())});\
    }).isOkAnd([this](ccBlendFunc value){ member = value; return true; })\
  ) {} else if(data.get(option).isOkAnd([](matjson::Value& value){return value.isNull();})) {\
    member = std::nullopt;\
  } else if (data.contains(option)) return false
    
#define SETUP_BLENDING_EQUATION(option, member) if(\
    data.get<std::string>(option)\
    .andThen([](std::string value) -> Result<GLenum> {\
      if (blendEqMap.contains(value)) return Ok(blendEqMap.at(value));\
      return Err("invalid blending equation");\
    }).isOkAnd([this](GLenum value){ member = value; return true; })\
  ) {} else if (data.contains(option)) return false

// the actual code using those macros
bool ColorDrawNode::init(matjson::Value& data) {
  if (!CCDrawNode::init()) return false;
  
  // options are applied in this order, so, e.g. solid-color may override color if both are set
  SETUP_COLOR("color", m_solidColor = m_nonSolidColor);
  SETUP_COLOR("solid-color", m_solidColor);
  SETUP_COLOR("non-solid-color", m_nonSolidColor);

  SETUP_OPACITY_FACTOR("opacity-factor", m_solidOpacityFactor = m_nonSolidOpacityFactor);
  SETUP_OPACITY_FACTOR("solid-opacity-factor", m_solidOpacityFactor);
  SETUP_OPACITY_FACTOR("non-solid-opacity-factor", m_nonSolidOpacityFactor);

  SETUP_SKIP("skip", m_skipIfSolid = m_skipIfNonSolid);
  SETUP_SKIP("skip-if-solid", m_skipIfSolid);
  SETUP_SKIP("skip-if-non-solid", m_skipIfNonSolid);

  SETUP_BLENDING_FUNCTION("blending-function", m_solidColorBlendFunc = m_nonSolidColorBlendFunc);
  SETUP_BLENDING_FUNCTION("solid-blending-function", m_solidColorBlendFunc);
  SETUP_BLENDING_FUNCTION("non-solid-blending-function", m_nonSolidColorBlendFunc);
  
  SETUP_BLENDING_EQUATION("blending-equation", m_solidColorBlendEq = m_nonSolidColorBlendEq);
  SETUP_BLENDING_EQUATION("solid-blending-equation", m_solidColorBlendEq);
  SETUP_BLENDING_EQUATION("non-solid-blending-equation", m_nonSolidColorBlendEq);

  // the options were parsed successfully, so the node may be created
  // you could choose a different path, and just ignore incorrect options
  // TODO: also return false if the node is given unknown options, for example a misspelling
  return true;
}

#undef SETUP_COLOR
#undef SETUP_OPACITY_FACTOR
#undef SETUP_SKIP
#undef SETUP_BLENDING_FUNCTION
#undef SETUP_BLENDING_EQUATION

void ColorDrawNode::drawTriangle(
    CCPoint const& p1, CCPoint const& p2, CCPoint const& p3,
    ccColor4B color
) {
  
  // START ensureCapacity(vertex_count);
  if(m_nBufferCount + 3u >= m_uBufferCapacity) {
    m_uBufferCapacity += std::max(m_uBufferCapacity, 3u);
    m_pBuffer = static_cast<ccV2F_C4B_T2F*>(realloc(m_pBuffer, m_uBufferCapacity*sizeof(ccV2F_C4B_T2F)));
  }
  // END ensureCapacity(vertex_count);

  m_pBuffer[m_nBufferCount++] = {{p1.x, p1.y}, color, {0.f, 0.f}};
  m_pBuffer[m_nBufferCount++] = {{p2.x, p2.y}, color, {0.f, 0.f}};
  m_pBuffer[m_nBufferCount++] = {{p3.x, p3.y}, color, {0.f, 0.f}};

  m_bDirty = true;
}

void ColorDrawNode::draw() {
  CC_NODE_DRAW_SETUP();
  HardStreak* parentStreak = static_cast<HardStreak*>(getParent());

  // our custom blending function/equation setup, this
  // is why reimplementing CCDrawNode::draw was necessary
  // this supports setting both blending functions *and* blending equations
  // and you can set different ones depending on whether the trail is
  // supposed to be solid or not
  if (parentStreak->m_isSolid) {
    glBlendFuncSeparate(
      m_solidColorBlendFunc.value_or(parentStreak->m_sBlendFunc).src,
      m_solidColorBlendFunc.value_or(parentStreak->m_sBlendFunc).dst,
      GL_ZERO, GL_ONE
    );
    glBlendEquationSeparate(m_solidColorBlendEq, GL_FUNC_ADD);
  } else {
    glBlendFuncSeparate(
      m_nonSolidColorBlendFunc.value_or(parentStreak->m_sBlendFunc).src,
      m_nonSolidColorBlendFunc.value_or(parentStreak->m_sBlendFunc).dst,
      GL_ZERO, GL_ONE
    );
    glBlendEquationSeparate(m_nonSolidColorBlendEq, GL_FUNC_ADD);
  }
  
  // rest of CCDrawNode::draw is unchanged
  if (m_bDirty) {
    glBindBuffer(GL_ARRAY_BUFFER, m_uVbo);
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(ccV2F_C4B_T2F)*m_uBufferCapacity,
      m_pBuffer,
      GL_STREAM_DRAW
    );
    m_bDirty = false;
  }

  ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);
  glBindBuffer(GL_ARRAY_BUFFER, m_uVbo);

  glVertexAttribPointer(
    kCCVertexAttrib_Position,
    2, GL_FLOAT, GL_FALSE,
    sizeof(ccV2F_C4B_T2F),
    reinterpret_cast<GLvoid*>(offsetof(ccV2F_C4B_T2F, vertices))
  );
  glVertexAttribPointer(
    kCCVertexAttrib_Color,
    4, GL_UNSIGNED_BYTE, GL_TRUE,
    sizeof(ccV2F_C4B_T2F),
    reinterpret_cast<GLvoid*>(offsetof(ccV2F_C4B_T2F, colors))
  );
  glVertexAttribPointer(
    kCCVertexAttrib_TexCoords,
    2, GL_FLOAT, GL_FALSE,
    sizeof(ccV2F_C4B_T2F),
    reinterpret_cast<GLvoid*>(offsetof(ccV2F_C4B_T2F, texCoords))
  );

  glDrawArrays(GL_TRIANGLES, 0, m_nBufferCount);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

#if defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
  static_assert(
    GEODE_COMP_GD_VERSION == 22074,
    "Please update macOS and iOS offsets"
  );
  *reinterpret_cast<unsigned int*>(
    geode::base::get() +
    GEODE_ARM_MAC(0x8b0f60)
    GEODE_INTEL_MAC(0x98bf30)
    GEODE_IOS(0x8791d0)
  ) += 1;
#else
  CC_INCREMENT_GL_DRAWS(1);
#endif

  CHECK_GL_ERROR_DEBUG();
  
  // reset blending after finishing the draw
  ccGLBlendResetToCache();
}

void ColorDrawNode::drawTrailPart(std::vector<CCPoint>& offset1, std::vector<CCPoint>& offset2) {
  HardStreak* parentStreak = static_cast<HardStreak*>(getParent());

  if (
    (parentStreak->m_isSolid && m_skipIfSolid) ||
    (!parentStreak->m_isSolid && m_skipIfNonSolid)
  ) return;
  
  ccColor4B color = parentStreak->m_isSolid ? to4B(
    m_solidColor.value_or(parentStreak->getColor()),
    std::clamp(m_solidOpacityFactor*parentStreak->getOpacity(), 0.f, 255.f)
  ) : to4B(
    m_nonSolidColor.value_or(parentStreak->getColor()),
    std::clamp(m_nonSolidOpacityFactor*parentStreak->getOpacity(), 0.f, 255.f)
  );

  for (size_t i = 0; i+1 < offset1.size(); i++) {
    if (offset1[i] != offset1[i+1])
      drawTriangle(offset1[i], offset1[i+1], offset2[i+1], color);
    if (offset2[i] != offset2[i+1])
      drawTriangle(offset2[i+1], offset2[i], offset1[i], color);
  }

}


$on_mod(Loaded) {
  nytelyte::wave_trail_draw_fix::registerDrawer<ColorDrawNode>("color"_spr);
}
