// check include/nodes/ColorDrawNode.hpp for additional comments
#include <nodes/ColorDrawNode.hpp>
#include <constants.hpp>

ColorDrawNode* ColorDrawNode::create(matjson::Value& data) {
  auto* ret = new ColorDrawNode;
  if (ret && ret->init(data)) { ret->autorelease(); return ret; }
  else { delete ret; return nullptr; }
}

// macros to help parse the options
#define SETUP_COLOR(option, member) if(\
    data.get<std::string>(option)\
    .andThen([](std::string value){ return cc3bFromHexString(value); })\
    .isOkAnd([this](_ccColor3B value) { member = value; return true; })\
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
    .andThen([](std::string value) -> Result<_ccBlendFunc> {\
      if (blendFuncMap.contains(value)) return Ok(blendFuncMap.at(value));\
      return Err("invalid blending function");\
    }).isOkAnd([this](_ccBlendFunc value){ member = value; return true; })\
  ) {} else if(\
    data.get<std::vector<matjson::Value>&>(option)\
    .andThen([](std::vector<matjson::Value> value) -> Result<_ccBlendFunc> {\
      if (value.size() != 2) return Err("invalid blending function");\
      if (!value[0].isString() || !value[1].isString()) return Err("invalid blending function");\
      if (!blendEnumMap.contains(*value[0].asString().ok()) || !blendEnumMap.contains(*value[1].asString().ok())) return Err("invalid blending function");\
      return Ok(_ccBlendFunc{blendEnumMap.at(*value[0].asString().ok()), blendEnumMap.at(*value[1].asString().ok())});\
    }).isOkAnd([this](_ccBlendFunc value){ member = value; return true; })\
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

void ColorDrawNode::drawTriangle(CCPoint const& p1, CCPoint const& p2, CCPoint const& p3, _ccColor4B color) {
  unsigned int vertex_count = 3;

  // START ensureCapacity(vertex_count);
  if(m_nBufferCount + vertex_count > m_uBufferCapacity) {
    m_uBufferCapacity += std::max(m_uBufferCapacity, vertex_count);
    m_pBuffer = (ccV2F_C4B_T2F*)realloc(m_pBuffer, m_uBufferCapacity*sizeof(ccV2F_C4B_T2F));
  }
  // END ensureCapacity(vertex_count);
  
  _ccV2F_C4B_T2F a = {_ccVertex2F(p1.x, p1.y), color, _ccTex2F(0.0, 0.0) };
  _ccV2F_C4B_T2F b = {_ccVertex2F(p2.x, p2.y), color, _ccTex2F(0.0, 0.0) };
  _ccV2F_C4B_T2F c = {_ccVertex2F(p3.x, p3.y), color, _ccTex2F(0.0, 0.0) };
  
  _ccV2F_C4B_T2F_Triangle* triangles = (_ccV2F_C4B_T2F_Triangle *)(m_pBuffer + m_nBufferCount);
  _ccV2F_C4B_T2F_Triangle triangle = {a, b, c};
  triangles[0] = triangle;
  
  m_nBufferCount += vertex_count;
  m_bDirty = true;
}

void ColorDrawNode::draw() {
  // clear the leftover trail before drawing it again
  clear();

  HardStreak* parent = static_cast<HardStreak*>(getParent());
  bool isSolid = parent->m_isSolid;
  _ccBlendFunc parentBlending = parent->getBlendFunc();

  if (isSolid && m_skipIfSolid) return;
  if (!isSolid && m_skipIfNonSolid) return;
  
  _ccColor4B color;
  _ccColor3B streakColor = static_cast<HardStreak*>(getParent())->getColor();
  GLubyte streakOpacity = static_cast<HardStreak*>(getParent())->getOpacity();
  if (isSolid && !m_solidColor) {
    color = to4B(streakColor, static_cast<GLubyte>(std::clamp(m_solidOpacityFactor * streakOpacity, 0.f, 255.f)));
  } else if (!isSolid && !m_nonSolidColor) {
    color = to4B(streakColor, static_cast<GLubyte>(std::clamp(m_nonSolidOpacityFactor * streakOpacity, 0.f, 255.f)));
  } else if (isSolid) {
    color = to4B(*m_solidColor, static_cast<GLubyte>(std::clamp(m_solidOpacityFactor * streakOpacity, 0.f, 255.f)));
  } else /*if (!isSolid)*/ {
    color = to4B(*m_nonSolidColor, static_cast<GLubyte>(std::clamp(m_nonSolidOpacityFactor * streakOpacity, 0.f, 255.f)));
  }
  
  // 
  std::vector<CCPoint> points1 = static_cast<ObjWrapper<std::vector<CCPoint>>*>(getUserObject("nytelyte.wave_trail_draw_fix/offsetPath1"))->getValue();
  std::vector<CCPoint> points2 = static_cast<ObjWrapper<std::vector<CCPoint>>*>(getUserObject("nytelyte.wave_trail_draw_fix/offsetPath2"))->getValue();
  for (unsigned int i = 0; i+1 < points1.size(); i++) {
    if (points1[i] != points1[i+1]) drawTriangle(points1[i], points1[i+1], points2[i+1], color);
    if (points2[i] != points2[i+1]) drawTriangle(points2[i+1], points2[i], points1[i], color);
  }
  
  // do the drawing
  // copy of a part of CCDrawNode::draw, without the blending function setup, since we're doing that above
  // if you aren't doing any blending shenanigans in your own node, you can just call CCDrawNode::draw()

  CC_NODE_DRAW_SETUP();

  // our custom blending function setup, this is why reimplementing CCDrawNode::draw was necessary
  if (isSolid) {
    glBlendFuncSeparate(m_solidColorBlendFunc.value_or(parentBlending).src, m_solidColorBlendFunc.value_or(parentBlending).dst, GL_ZERO, GL_ONE);
    glBlendEquationSeparate(m_solidColorBlendEq, GL_FUNC_ADD);
  } else {
    glBlendFuncSeparate(m_nonSolidColorBlendFunc.value_or(parentBlending).src, m_nonSolidColorBlendFunc.value_or(parentBlending).dst, GL_ZERO, GL_ONE);
    glBlendEquationSeparate(m_nonSolidColorBlendEq, GL_FUNC_ADD);
  }
  
  // rest of CCDrawNode::draw is unchanged
  if (m_bDirty) {
      glBindBuffer(GL_ARRAY_BUFFER, m_uVbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(ccV2F_C4B_T2F)*m_uBufferCapacity, m_pBuffer, GL_STREAM_DRAW);
      m_bDirty = false;
  }
  ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);
  glBindBuffer(GL_ARRAY_BUFFER, m_uVbo);
  glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(ccV2F_C4B_T2F), (GLvoid *)offsetof(ccV2F_C4B_T2F, vertices));
  glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ccV2F_C4B_T2F), (GLvoid *)offsetof(ccV2F_C4B_T2F, colors));
  glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(ccV2F_C4B_T2F), (GLvoid *)offsetof(ccV2F_C4B_T2F, texCoords));
  glDrawArrays(GL_TRIANGLES, 0, m_nBufferCount);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  CC_INCREMENT_GL_DRAWS(1);
  CHECK_GL_ERROR_DEBUG();
  
  // reset blending after finishing the draw
  ccGLBlendResetToCache();
}

$execute {
    new EventListener(+[](HardStreak* node, matjson::Value data) {
        ColorDrawNode* myNode = ColorDrawNode::create(data);
        if (myNode) node->addChild(myNode);
        return ListenerResult::Stop;
    }, geode::DispatchFilter<HardStreak*, matjson::Value>("color"_spr));
};
