#include <hooks/HardStreak.hpp>
#include <utilities.hpp>
#include <constants.hpp>
#include <globals.hpp>

void HookedHardStreak::drawTriangle(
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

void HookedHardStreak::setConfiguration(std::vector<utilities::Part>& config) {
  m_fields->m_configuration = config;
}

bool HookedHardStreak::init() {
  if (!HardStreak::init()) return false;
  setConfiguration(configuration);
  return true;
}

void HookedHardStreak::updateStroke(float dt) {
  clear();
  if (!m_drawStreak) return;

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
  
  for (utilities::Part& part : m_fields->m_configuration) {

    float waveSize = part.sizeOverride.value_or(m_waveSize);
    float pulseSize = part.pulseOverride.value_or(m_pulseSize);
    float relativeFactor = 3*waveSize*pulseSize;

    float totalWidth = (part.endOffset + part.end*relativeFactor) - (part.startOffset + part.start*relativeFactor);
    float totalWeight = 0;
    for (utilities::Subpart& subpart : part.subparts)
      totalWeight += subpart.weight;
    float oneWidth = totalWidth/totalWeight;

    float firstOffset = m_isFlipped ? (part.endOffset + part.end*relativeFactor) : (part.startOffset + part.start*relativeFactor);
    float lastOffset = m_isFlipped ? (part.startOffset + part.start*relativeFactor) : (part.endOffset + part.end*relativeFactor);
    if (m_isFlipped) oneWidth *= -1;

    std::vector<CCPoint> previousPoints, nextPoints, firstPoints, lastPoints;
    std::vector<bool> previousLabels, nextLabels, firstLabels, lastLabels;
    float previousOffset, nextOffset;

    std::tie(firstPoints, firstLabels) = utilities::createOffset(trailPath, firstOffset, relativeFactor);
    std::tie(lastPoints, lastLabels) = utilities::createOffset(trailPath, lastOffset, relativeFactor);
    std::tie(previousPoints, previousLabels, previousOffset) = {firstPoints, firstLabels, firstOffset};

    utilities::fixPoints(trailPath, previousOffset, previousPoints, previousLabels, lastPoints);

    totalWeight = part.subparts[0].weight;
    for (size_t i = 1; i < part.subparts.size() + 1; i++) {

      nextOffset = (i == part.subparts.size()) ? lastOffset : (firstOffset + totalWeight*oneWidth);
      std::tie(nextPoints, nextLabels) = utilities::createOffset(trailPath, nextOffset, relativeFactor);
      utilities::fixPoints(trailPath, nextOffset, nextPoints, nextLabels, firstPoints);

      if (
        (!part.subparts[i-1].solidOnly && !part.subparts[i-1].nonSolidOnly) ||
        (part.subparts[i-1].solidOnly && m_isSolid) ||
        (part.subparts[i-1].nonSolidOnly && !m_isSolid)
      ) {
        ccColor4B color = to4B(
          part.subparts[i-1].color.value_or(getColor()),
          std::clamp(part.subparts[i-1].opacity * getOpacity(), 0.f, 255.f)
        );

        for (size_t i = 0; i+1 < previousPoints.size(); i++) {
          if (previousPoints[i] != previousPoints[i+1])
            drawTriangle(previousPoints[i], previousPoints[i+1], nextPoints[i+1], color);
          if (nextPoints[i] != nextPoints[i+1])
            drawTriangle(nextPoints[i+1], nextPoints[i], previousPoints[i], color);
        }
      }

      previousOffset = nextOffset;
      std::tie(previousPoints, previousLabels) = {nextPoints, nextLabels};

      if (i == part.subparts.size()) break;
      totalWeight += part.subparts[i].weight;
    }
  }
}
