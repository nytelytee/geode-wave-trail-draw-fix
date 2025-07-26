#include <hooks/HardStreak.hpp>
#include <utilities.hpp>
#include <constants.hpp>
#include <globals.hpp>

void HookedHardStreak::onModify(auto& self) {
  // override TrailFix if the user has it installed
  (void) self.setHookPriority("HardStreak::updateStroke", 99999998);
}

void HookedHardStreak::setConfiguration(std::vector<utilities::Unit>& config) {
  m_fields->m_configuration = config;
  removeAllChildren();
  unsigned int length = 0;
  for (utilities::Unit& unit : m_fields->m_configuration) {
    if (unit.parts.empty()) {
        m_fields->m_broken = true;
        break;
    }
    for (utilities::Part& part : unit.parts) {
      length += 1;
      // this event should add its draw node to the provided hardstreak, and only add one
      // if it does not add anything (potentially mistyped, no event fired) or adds multiple (potentially multiple events under the same id defined)
      // the trail is marked as "broken", and it does not get drawn
      geode::DispatchEvent<HardStreak*, matjson::Value>(part.type, this, part.data).post();

      if (length != getChildrenCount() || part.weight <= 0) {
        m_fields->m_broken = true;
        break;
      }
      getChild(this, -1)->setUserObject("offsetPath1"_spr, ObjWrapper<std::vector<CCPoint>>::create({}));
      getChild(this, -1)->setUserObject("offsetPath2"_spr, ObjWrapper<std::vector<CCPoint>>::create({}));
    }
    // invalid configuration; remove everything, and refuse to draw the trail inside updateStroke
    if (m_fields->m_broken) {
      removeAllChildren();
      break;
    }
  }
}

bool HookedHardStreak::init() {
  if (!HardStreak::init()) return false;
  setConfiguration(configuration);
  return true;
}

void HookedHardStreak::clearAllChildren() {
  CCArrayExt<CCNode*> children = getChildren();
  for (CCNode* node : children) {
    static_cast<ObjWrapper<std::vector<CCPoint>>*>(node->getUserObject("offsetPath1"_spr))->getValue().clear();
    static_cast<ObjWrapper<std::vector<CCPoint>>*>(node->getUserObject("offsetPath2"_spr))->getValue().clear();
  }
}

void HookedHardStreak::stopStroke() {
  HardStreak::stopStroke();
  clearAllChildren();
}

void HookedHardStreak::updateStroke(float) {
  clear();  // just in case
  clearAllChildren();
  if (!m_drawStreak) return;
  // if the setup failed, just don't draw anything
  if (m_fields->m_broken) return;

  if (m_pointArray->count() == 0) return;
  if (getOpacity() == 0) return;
  CCPoint position = getPosition();
  
  // the points the wave trail should follow, including the player's current position as the last one
  std::vector<CCPoint> trailPath;
  trailPath.reserve(m_pointArray->count());
  for (unsigned int i = 0; i < m_pointArray->count(); i++) {
    PointNode* currentPointNode = static_cast<PointNode*>(m_pointArray->objectAtIndex(i));
    if (!currentPointNode) break;
    if (!trailPath.empty() && trailPath.back() == (currentPointNode->m_point - position)) continue;
    trailPath.push_back(currentPointNode->m_point - position);
  }
  if (trailPath.empty() || trailPath.back() != (m_currentPoint - position)) trailPath.push_back(m_currentPoint - position);
  
  // generate offsets, fix them, assign them to the drawers
  // TODO: maybe make the third argument to createOffset customizable
  int drawerIndex = 0;
  for (utilities::Unit& unit : m_fields->m_configuration) {
    float waveSize = unit.sizeOverride.value_or(m_waveSize);
    float pulseSize = unit.pulseOverride.value_or(m_pulseSize);
    float relativeFactor = 3*waveSize*pulseSize;
    float totalWidth = (unit.endOffset + unit.end*relativeFactor) - (unit.startOffset + unit.start*relativeFactor);
    float totalWeight = 0; for (utilities::Part& part : unit.parts) totalWeight += part.weight;
    float oneWidth = totalWidth/totalWeight;
    float firstOffset = m_isFlipped ? (unit.endOffset + unit.end*relativeFactor) : (unit.startOffset + unit.start*relativeFactor);
    float lastOffset = m_isFlipped ? (unit.startOffset + unit.start*relativeFactor) : (unit.endOffset + unit.end*relativeFactor);
    if (m_isFlipped) oneWidth *= -1;
    std::vector<CCPoint> previousPoints, firstPoints, lastPoints;
    std::vector<bool> previousLabels, firstLabels, lastLabels;
    float previousOffset;
    std::tie(firstPoints, firstLabels) = utilities::createOffset(trailPath, firstOffset, relativeFactor);
    std::tie(lastPoints, lastLabels) = utilities::createOffset(trailPath, lastOffset, relativeFactor);
    std::tie(previousPoints, previousLabels, previousOffset) = {firstPoints, firstLabels, firstOffset};
    totalWeight = unit.parts[0].weight;
    for (size_t i = 1; i < unit.parts.size() + 1; i++) {
      CCNode* drawer = getChild(this, drawerIndex);
      float newOffset = (i == unit.parts.size()) ? lastOffset : (firstOffset + totalWeight*oneWidth);
      utilities::fixPoints(trailPath, previousOffset, previousPoints, previousLabels, lastPoints);
      static_cast<ObjWrapper<std::vector<CCPoint>>*>(drawer->getUserObject("offsetPath1"_spr))->getValue() = previousPoints;
      std::tie(previousPoints, previousLabels) = utilities::createOffset(trailPath, newOffset, relativeFactor);
      previousOffset = newOffset;
      utilities::fixPoints(trailPath, previousOffset, previousPoints, previousLabels, firstPoints);
      static_cast<ObjWrapper<std::vector<CCPoint>>*>(drawer->getUserObject("offsetPath2"_spr))->getValue() = previousPoints;
      drawerIndex += 1;
      if (i == unit.parts.size()) break;
      totalWeight += unit.parts[i].weight;
    }
  }
}
