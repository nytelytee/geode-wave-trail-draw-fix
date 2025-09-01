#pragma once


#include <Geode/Geode.hpp>
using namespace geode::prelude;

namespace utilities {

  std::optional<CCPoint> getLineSegmentIntersection(CCPoint seg1p1, CCPoint seg1p2, CCPoint seg2p1, CCPoint seg2p2);

  // offset a point with a miter join
  CCPoint offsetPoint(std::optional<CCPoint> previousPoint, CCPoint currentPoint, std::optional<CCPoint> nextPoint, float distance);

  // generate an offset path from a given path
  // returns the offset path and the 'labels' for each line segment in the path;
  // a label is true if it is moving in the same direction as its corresponding line segment in the actual path (or if it's 0)
  // a label is false if it is moving in the opposite direction as its corresponding line segment in the actual path
  std::tuple<std::vector<CCPoint>, std::vector<bool>> createOffset(std::vector<CCPoint>& trailPath, float offset, float extensionLength);

  // fix the offset points to remove as many self intersections from the trail as possible
  // this does not remove all of them, if you have better ideas for handling this, feel free to make a PR
  // this algorithm was mostly made by staring at images of the trail being broken and drawing lines onto
  // them until i trial-and-errored into something that kind of works but not completely
  // people who actually know how to generate proper offset polylines: please tell me what i am missing
  void fixPoints(std::vector<CCPoint>& trailPath, float offset, std::vector<CCPoint>& points, std::vector<bool>& labels, std::vector<CCPoint>& otherPoints);

  struct Subpart {
    std::optional<ccColor3B> color;
    float weight, opacity;
    bool solidOnly, nonSolidOnly;
  };

  struct Part {
    float start, end, startOffset, endOffset;
    std::optional<float> pulseOverride, sizeOverride;
    std::vector<Subpart> subparts;
  };

}


template <>
struct matjson::Serialize<utilities::Subpart> {
  static Result<utilities::Subpart> fromJson(const matjson::Value& value);
  static matjson::Value toJson(const utilities::Subpart& subpart);
};

template <>
struct matjson::Serialize<utilities::Part> {
  static Result<utilities::Part> fromJson(const matjson::Value& value);
  static matjson::Value toJson(const utilities::Part& part);
};
