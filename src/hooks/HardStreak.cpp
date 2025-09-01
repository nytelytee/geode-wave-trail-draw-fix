#include <hooks/HardStreak.hpp>
#include <utilities.hpp>
#include <constants.hpp>
#include <globals.hpp>
#include <api/api.hpp>
#include <Geode/modify/CCDrawNode.hpp>

void HookedHardStreak::onModify(auto& self) {
  // override TrailFix if the user has it installed
  (void) self.setHookPriority("HardStreak::updateStroke", 99999998);
}

void HookedHardStreak::setConfiguration(std::vector<utilities::Unit>& config) {
  m_fields->m_configuration = config;
  m_fields->m_drawers = CCArray::create();
  removeAllChildren();
  unsigned length = 0;
  for (utilities::Unit& unit : m_fields->m_configuration) {
    if (unit.parts.empty()) {
        m_fields->m_broken = true;
        break;
    }
    for (utilities::Part& part : unit.parts) {
      // this event should add its draw node to the provided hardstreak, and only add one
      // if it does not add anything (potentially mistyped, no event fired) or adds multiple (potentially multiple events under the same id defined)
      // the trail is marked as "broken", and it does not get drawn
      nytelyte::wave_trail_draw_fix::AddTrailPart::Event(part.type, this, part.data).post();
      length += 1; 

      if (getChildrenCount() != length || part.weight <= 0) {
        m_fields->m_broken = true;
        break;
      }
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
  for (CCNode* drawer : CCArrayExt<CCNode>(getChildren())) {
    // call child's clear
    auto funcWrapper = static_cast<nytelyte::wave_trail_draw_fix::ClearWrapper*>(
     drawer->getUserObject("clear-function"_spr)
    );

    if (funcWrapper)
      std::invoke(funcWrapper->getValue());
  }
}

void HookedHardStreak::stopStroke() {
  HardStreak::stopStroke();
  clearAllChildren();
}

void HookedHardStreak::updateStroke(float dt) {
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
  
  // generate offsets, fix them, send them to the drawers to draw
  // TODO: maybe make the third argument to createOffset customizable
  int drawerIndex = 0;
  for (utilities::Unit& unit : m_fields->m_configuration) {

    float waveSize = unit.sizeOverride.value_or(m_waveSize);
    float pulseSize = unit.pulseOverride.value_or(m_pulseSize);
    float relativeFactor = 3*waveSize*pulseSize;

    float totalWidth = (unit.endOffset + unit.end*relativeFactor) - (unit.startOffset + unit.start*relativeFactor);
    float totalWeight = 0;
    for (utilities::Part& part : unit.parts)
      totalWeight += part.weight;
    float oneWidth = totalWidth/totalWeight;

    float firstOffset = m_isFlipped ? (unit.endOffset + unit.end*relativeFactor) : (unit.startOffset + unit.start*relativeFactor);
    float lastOffset = m_isFlipped ? (unit.startOffset + unit.start*relativeFactor) : (unit.endOffset + unit.end*relativeFactor);
    if (m_isFlipped) oneWidth *= -1;

    std::vector<CCPoint> previousPoints, nextPoints, firstPoints, lastPoints;
    std::vector<bool> previousLabels, nextLabels, firstLabels, lastLabels;
    float previousOffset, nextOffset;

    std::tie(firstPoints, firstLabels) = utilities::createOffset(trailPath, firstOffset, relativeFactor);
    std::tie(lastPoints, lastLabels) = utilities::createOffset(trailPath, lastOffset, relativeFactor);
    std::tie(previousPoints, previousLabels, previousOffset) = {firstPoints, firstLabels, firstOffset};

    utilities::fixPoints(trailPath, previousOffset, previousPoints, previousLabels, lastPoints);

    totalWeight = unit.parts[0].weight;
    for (size_t i = 1; i < unit.parts.size() + 1; i++) {
      CCNode* drawer = getChildByIndex(drawerIndex);
      if (!drawer) return;

      nextOffset = (i == unit.parts.size()) ? lastOffset : (firstOffset + totalWeight*oneWidth);
      std::tie(nextPoints, nextLabels) = utilities::createOffset(trailPath, nextOffset, relativeFactor);
      utilities::fixPoints(trailPath, nextOffset, nextPoints, nextLabels, firstPoints);
      
      // call child's drawTrailPart
      auto funcWrapper = static_cast<nytelyte::wave_trail_draw_fix::DrawTrailPartWrapper*>(
        drawer->getUserObject("draw-trail-part-function"_spr)
      );

      if (funcWrapper)
        std::invoke(
          funcWrapper->getValue(),
          previousPoints, nextPoints
        );

      previousOffset = nextOffset;
      std::tie(previousPoints, previousLabels) = {nextPoints, nextLabels};

      drawerIndex += 1;
      if (i == unit.parts.size()) break;
      totalWeight += unit.parts[i].weight;
    }
  }
}
