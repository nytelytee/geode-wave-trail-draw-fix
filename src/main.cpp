#include <Geode/Geode.hpp>
#include <Geode/modify/HardStreak.hpp>

using namespace geode::prelude;

float epsilon = 10e-6f;

class $modify(WTDFHardStreak, HardStreak) {
	static void onModify(auto& self) {
		(void) self.setHookPriority("HardStreak::updateStroke", 99999998);
	}
  
  // CCDrawNode::drawPolygon is broken for some reason, it can't even draw triangles correctly
  void drawTriangle(CCPoint const& p1, CCPoint const& p2, CCPoint const& p3, cocos2d::_ccColor4F const& color) {
    unsigned int vertex_count = 3;
    
    // ensureCapacity(vertex_count);
    if(m_nBufferCount + vertex_count > m_uBufferCapacity) {
		  m_uBufferCapacity += std::max(m_uBufferCapacity, vertex_count);
		  m_pBuffer = (ccV2F_C4B_T2F*)realloc(m_pBuffer, m_uBufferCapacity*sizeof(ccV2F_C4B_T2F));
    }
    // END ensureCapacity(vertex_count);
    
    _ccColor4B col = ccc4BFromccc4F(color);
    _ccV2F_C4B_T2F a = {_ccVertex2F(p1.x, p1.y), col, _ccTex2F(0.0, 0.0) };
    _ccV2F_C4B_T2F b = {_ccVertex2F(p2.x, p2.y), col, _ccTex2F(0.0, 0.0) };
    _ccV2F_C4B_T2F c = {_ccVertex2F(p3.x, p3.y), col, _ccTex2F(0.0, 0.0) };
    
    _ccV2F_C4B_T2F_Triangle *triangles = (_ccV2F_C4B_T2F_Triangle *)(m_pBuffer + m_nBufferCount);
    _ccV2F_C4B_T2F_Triangle triangle = {a, b, c};
    triangles[0] = triangle;
    
    m_nBufferCount += vertex_count;
    m_bDirty = true;
  }
  
  std::optional<CCPoint> getLineSegmentIntersection(CCPoint seg1p1, CCPoint seg1p2, CCPoint seg2p1, CCPoint seg2p2) {
    if (seg1p1 == seg1p2 || seg2p1 == seg2p2) return std::nullopt;
    CCPoint seg1dir = seg1p2 - seg1p1;
    CCPoint seg2dir = seg2p2 - seg2p1;
    float cross = seg1dir.cross(seg2dir);
    // parallel; i don't care about the intersection, even if they do happen to be colinear and overlap
    if (fabs(cross) < epsilon) return std::nullopt;
    CCPoint toseg2 = seg2p1 - seg1p1;
    float t1 = toseg2.cross(seg2dir) / cross;
    float t2 = toseg2.cross(seg1dir) / cross;
    if (t1 < 0 || t1 > 1 || t2 < 0 || t2 > 1) return std::nullopt;
    return seg1p1 + seg1dir*t1;
  }
  
  // offset a point with a miter join
  CCPoint offsetPoint(std::optional<CCPoint> previousPoint, CCPoint currentPoint, std::optional<CCPoint> nextPoint, float distance) {
    CCAssert(previousPoint || nextPoint, L"Need at least a previous or next point.");
    if (!previousPoint) {
      CCPoint direction = (*nextPoint - currentPoint).normalize();
      CCPoint normal = (currentPoint - *nextPoint).getPerp().normalize();
      return (currentPoint - direction*fabs(distance)) + normal*distance;
    }
    if (!nextPoint) {
      // extending the endpoints slightly fixes some artifacts,
      // but the trail is then visibly past the player when the pulse is strong enough
      // only extend the trail if it's already shorter than the offset width, and push it back otherwise
      // because the trail being visible past the player bug happens even without extending it
      CCPoint direction = (currentPoint - *previousPoint).normalize();
      CCPoint normal = (currentPoint - *previousPoint).getRPerp().normalize();
      float length = previousPoint->getDistance(currentPoint);
      float offset = (length > fabs(distance) + 5) ? -5 : fabs(distance) - length;
      return (currentPoint + direction*offset) + normal*distance;
    }
    CCPoint n1 = (currentPoint - *previousPoint).getRPerp().normalize();
    CCPoint n2 = (currentPoint - *nextPoint).getPerp().normalize();
    CCPoint bisector = (n1 + n2).normalize();
    float length = distance/sqrt(0.5 + 0.5*n1.dot(n2));
    return currentPoint + bisector*length;
  }
  
  // generate an offset path from a given path
  // returns the offset path and the 'labels' for each line segment in the path;
  // a label is true if it is moving in the same direction as its corresponding line segment in the actual path (or if it's 0)
  // a label is false if it is moving in the opposite direction as its corresponding line segment in the actual path
  std::pair<std::vector<CCPoint>, std::vector<bool>> createOffset(std::vector<CCPoint>& trailPath, float offset) {
    std::vector<CCPoint> points;
    std::vector<bool> labels;
    for (int i = 0; i < trailPath.size(); i++) {
      std::optional<CCPoint> previousPoint;
      CCPoint currentPoint = trailPath[i];
      std::optional<CCPoint> nextPoint;
      if (i != 0) previousPoint = trailPath[i-1];
      if (i != trailPath.size()-1) nextPoint = trailPath[i+1];
      CCPoint point = offsetPoint(previousPoint, currentPoint, nextPoint, offset);
      points.push_back(point);
      if (!previousPoint) continue;
      CCPoint actualDirection = (currentPoint - *previousPoint).normalize();
      CCPoint trailDirection = point - points[points.size()-2];
      if (trailDirection.isZero()) {
        labels.push_back(true);
        continue;
      }
      trailDirection = trailDirection.normalize();
      if (actualDirection.dot(trailDirection) < -1.0f + epsilon) labels.push_back(false);
      else labels.push_back(true);
    }
    return {points, labels};
  }
  
  // fix the offset points to remove as many self intersections from the trail as possible
  // this does not remove all of them, if you have better ideas for handling this, feel free to make a PR
  // this algorithm was mostly made by staring at images of the trail being broken and drawing lines onto
  // them until i trial-and-errored into something that kind of works but not completely
  // people who actually know how to generate proper offset polylines: please tell me what i am missing
  void fixPoints(std::vector<CCPoint>& trailPath, float offset, std::vector<CCPoint>& points, std::vector<bool>& labels, std::vector<CCPoint>& otherPoints) {
    // vectors might not be the same size
    if (points.size() != labels.size() || points.size() < 2) return;
    if (otherPoints.size() != points.size()) return;

    for (int i = 0; i < labels.size(); i++) {
      CCPoint nextSegmentP1, nextSegmentP2, prevSegmentP1, prevSegmentP2;
      bool nextFinished = false, prevFinished = false;

      if (labels[i]) continue;
      if (
        i+1 != labels.size() &&
        !labels[i+1] &&
        trailPath[i+1].getDistanceSq(trailPath[i]) > trailPath[i+2].getDistanceSq(trailPath[i+1])
      )
        continue;

      if (i == 0) {
        nextSegmentP1 = points[0];
        nextSegmentP2 = otherPoints[0];
      } else {
        nextSegmentP1 = points[i-1];
        nextSegmentP2 = offsetPoint(trailPath[i-1], trailPath[i], {}, offset);
      }
      if (i+2 >= points.size()) {
        prevSegmentP1 = points.back();
        prevSegmentP2 = otherPoints.back();
      } else {
        prevSegmentP1 = points[i+2];
        prevSegmentP2 = offsetPoint({}, trailPath[i+1], trailPath[i+2], offset);
      }

      for (int j = 2; j < points.size() + 1; j++) {
        if (i+j >= points.size() && !nextFinished) {
          nextFinished = true;
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points.back(), otherPoints.back(), nextSegmentP1, nextSegmentP2);
          if (intersection) {
            for (int k = i; k < i+j; k++) points[k] = *intersection;
            for (int k = i; k < i+j-1; k++) labels[k] = true;
            break;
          }
        }
        if (i+1-j < 0 && !prevFinished) {
          prevFinished = true;
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points[0], otherPoints[0], prevSegmentP1, prevSegmentP2);
          if (intersection) {
            for (int k = i+2-j; k < i+2; k++) { points[k] = *intersection; labels[k] = true; }
            break;
          }
        }
        if (!nextFinished) {
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points[i-1+j], points[i+j], nextSegmentP1, nextSegmentP2);
          if (intersection) {
            for (int k = i; k < i+j; k++) points[k] = *intersection;
            for (int k = i; k < i+j-1; k++) labels[k] = true;
            break;
          }
        }
        if (!prevFinished) {
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points[i+1-j], points[i+2-j], prevSegmentP1, prevSegmentP2);
          if (intersection) {
            for (int k = i+2-j; k < i+2; k++) { points[k] = *intersection; labels[k] = true; }
            break;
          }
        }
        if (nextFinished && prevFinished) break;
      }
    }
  }

  $override 
	void updateStroke(float) {
		if (!m_drawStreak) return;
		clear();
		if (m_pointArray->count() == 0) return;
		if (getOpacity() == 0) return;
		CCPoint position = getPosition();

    
    // the points the wave trail should follow, including the player's current position as the last one
    std::vector<CCPoint> trailPath;
		for (unsigned int i = 0; i < m_pointArray->count(); i++) {
			PointNode* currentPointNode = static_cast<PointNode*>(m_pointArray->objectAtIndex(i));
			if (!currentPointNode) break;
      if (!trailPath.empty() && trailPath.back() == (currentPointNode->m_point - position)) continue;
      trailPath.push_back(currentPointNode->m_point - position);
		}
    if (trailPath.empty() || trailPath.back() != (m_currentPoint - position)) trailPath.push_back(m_currentPoint - position);

    // size and color of the bigger wave trail
    float offset = 3 * m_waveSize * m_pulseSize;
    _ccColor4F color = ccc4FFromccc3B(getColor());
    color.a = getOpacity() / 255.f;
    // if solid, runs only once
    for (int iteration = 0; iteration < 2 - m_isSolid; iteration++) {
      auto [positivePoints, positiveLabels] = createOffset(trailPath, offset);
      auto [negativePoints, negativeLabels] = createOffset(trailPath, -offset);
      fixPoints(trailPath, offset, positivePoints, positiveLabels, negativePoints);
      fixPoints(trailPath, -offset, negativePoints, negativeLabels, positivePoints);
      for (unsigned int i = 0; i < positivePoints.size() - 1; i++) {
        if (positivePoints[i] != positivePoints[i+1]) drawTriangle(positivePoints[i], positivePoints[i+1], negativePoints[i+1], color);
        if (negativePoints[i] != negativePoints[i+1]) drawTriangle(negativePoints[i+1], negativePoints[i], positivePoints[i], color);
      }
      // size and color of the smaller wave trail
      offset /= 3;
      color = {1, 1, 1, color.a*0.65f};
    }
  }
};
