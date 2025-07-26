#include <constants.hpp>
#include <Geode/Geode.hpp>
#include <utilities.hpp>
using namespace geode::prelude;

namespace utilities {

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
    if (t1 < -epsilon || t1 > 1+epsilon || t2 < -epsilon || t2 > 1+epsilon) return std::nullopt;
    return seg1p1 + seg1dir*t1;
  }

  // offset a point with a miter join
  CCPoint offsetPoint(std::optional<CCPoint> previousPoint, CCPoint currentPoint, std::optional<CCPoint> nextPoint, float distance) {
    CCAssert(previousPoint || nextPoint, L"Need at least a previous or next point.");
    if (!previousPoint) {
      CCPoint normal = (currentPoint - *nextPoint).getPerp().normalize();
      return currentPoint + normal*distance;
    }
    if (!nextPoint) {
      CCPoint normal = (currentPoint - *previousPoint).getRPerp().normalize();
      return currentPoint + normal*distance;
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
  std::tuple<std::vector<CCPoint>, std::vector<bool>> createOffset(std::vector<CCPoint>& trailPath, float offset, float extensionLength) {
    std::vector<CCPoint> points;
    points.reserve(trailPath.size());
    std::vector<bool> labels;
    labels.reserve(trailPath.size()-1);
    for (int i = 0; i < trailPath.size(); i++) {
      std::optional<CCPoint> previousPoint;
      CCPoint currentPoint = trailPath[i];
      std::optional<CCPoint> nextPoint;
      if (i != 0) previousPoint = trailPath[i-1];
      if (i != trailPath.size()-1) nextPoint = trailPath[i+1];

      // extending the endpoints slightly fixes some artifacts,
      // but the trail is then visibly past the player when the pulse is strong enough
      // only extend the trail if it's already shorter than the offset width, and push it back otherwise
      // because the trail being visible past the player bug happens even without extending it
      if (!previousPoint) {
        currentPoint -= (*nextPoint - currentPoint).normalize()*extensionLength;
      } else if (!nextPoint) {
        float length = previousPoint->getDistance(currentPoint);
        currentPoint += (currentPoint - *previousPoint).normalize() * ((length > extensionLength + 5) ? -5 : extensionLength - length);
      }

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
    for (size_t i = 0; i < labels.size(); i++) {
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
      for (size_t j = 2; j < points.size() + 1; j++) {
        if (i+j >= points.size() && !nextFinished) {
          nextFinished = true;
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points.back(), otherPoints.back(), nextSegmentP1, nextSegmentP2);
          if (intersection) {
            for (size_t k = i; k < points.size(); k++) points[k] = *intersection;
            for (size_t k = i; k < labels.size(); k++) labels[k] = true;
            break;
          }
        }
        if (i+1 < j /*i+1-j < 0*/ && !prevFinished) {
          prevFinished = true;
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points[0], otherPoints[0], prevSegmentP1, prevSegmentP2);
          if (intersection) {
            for (size_t k = 0; k < i+2; k++) points[k] = *intersection;
            for (size_t k = 0; k < i+1; k++) labels[k] = true;
            break;
          }
        }
        if (!nextFinished) {
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points[i-1+j], points[i+j], nextSegmentP1, nextSegmentP2);
          if (intersection) {
            for (size_t k = i; k < i+j; k++) points[k] = *intersection;
            for (size_t k = i; k < i+j-1; k++) labels[k] = true;
            break;
          }
        }
        if (!prevFinished) {
          std::optional<CCPoint> intersection;
          intersection = getLineSegmentIntersection(points[i+1-j], points[i+2-j], prevSegmentP1, prevSegmentP2);
          if (intersection) {
            for (size_t k = i+2-j; k < i+2; k++) points[k] = *intersection;
            for (size_t k = i+2-j; k < i+1; k++) labels[k] = true;
            break;
          }
        }
        if (nextFinished && prevFinished) break;
      }
    }
  }
}

Result<utilities::Part> matjson::Serialize<utilities::Part>::fromJson(const matjson::Value& value) {
  
  if (!value.contains("type")) return Err("has to have a type");
  Result<std::string> t = value.get<std::string>("type");
  if (!t.isOk()) return Err("type has to be a string");

  Result<const matjson::Value&> d = value.get("data");
  if (d.isOk() && !(*d).isObject()) return Err("data has to be an object");
  
  Result<float> w = value.get<float>("weight");
  if (!w.isOk() && w.unwrapErr() == "not a number") return Err("weight has to be a number");

  std::string type = *t;
  matjson::Value data = d.unwrapOr(matjson::Value::object());
  float weight = w.unwrapOr(1.f);

  if (weight <= 0) return Err("weights must be positive");
  
  return Ok(utilities::Part{.type=type, .data=data, .weight=weight});

}

matjson::Value matjson::Serialize<utilities::Part>::toJson(const utilities::Part& part) {
  return matjson::makeObject({
      {"type", part.type},
      {"data", part.data},
      {"weight", part.weight},
  });
}

Result<utilities::Unit> matjson::Serialize<utilities::Unit>::fromJson(const matjson::Value& value) {

  Result<std::vector<utilities::Part>> p = value.get<std::vector<utilities::Part>>("parts");
  if (!p.isOk()) {
    if (value.contains("parts")) return Err("invalid parts: " /*+ *p.err()*/);
    return Err("must have parts");
  }
  if ((*p).size() == 0) return Err("need to have at least 1 part");
  
  Result<float> s = value.get<float>("start");
  if (!s.isOk() && *s.err() == "not a number") return Err("start has to be a number");
  
  Result<float> e = value.get<float>("end");
  if (!e.isOk() && *e.err() == "not a number") return Err("end has to be a number");
  
  Result<float> so = value.get<float>("startOffset");
  if (!so.isOk() && *so.err() == "not a number") return Err("startOffset has to be a number");
  
  Result<float> eo = value.get<float>("endOffset");
  if (!eo.isOk() && *eo.err() == "not a number") return Err("endOffset has to be a number");
  
  Result<std::optional<float>> plo = value.get<std::optional<float>>("pulseOverride");
  if (!plo.isOk() && *plo.err() == "not a number") return Err("pulseOverride has to be a number or null");
  
  Result<std::optional<float>> szo = value.get<std::optional<float>>("sizeOverride");
  if (!szo.isOk() && *szo.err() == "not a number") return Err("sizeOverride has to be a number or null");


  float start = s.unwrapOr(-1.f);
  float end = e.unwrapOr(1.f);
  float startOffset = so.unwrapOr(0.f);
  float endOffset = eo.unwrapOr(0.f);
  std::optional<float> pulseOverride = plo.unwrapOr(std::nullopt);
  std::optional<float> sizeOverride = szo.unwrapOr(std::nullopt);
  std::vector<utilities::Part> parts = *p;
  
  return Ok(utilities::Unit{
    .start=start, .end=end, .startOffset=startOffset, .endOffset=endOffset,
    .pulseOverride=pulseOverride, .sizeOverride=sizeOverride, .parts=parts
  });

}

matjson::Value matjson::Serialize<utilities::Unit>::toJson(const utilities::Unit& unit) {
  return matjson::makeObject({
      {"start", unit.start},
      {"end", unit.end},
      {"startOffset", unit.startOffset},
      {"endOffset", unit.endOffset},
      {"pulseOverride", unit.pulseOverride},
      {"sizeOverride", unit.sizeOverride},
      {"part", unit.parts},
  });
}
