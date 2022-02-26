//
// Created by Nathaniel Rupprecht on 2/5/22.
//

#ifndef GEMINI_INCLUDE_GEMINI_CORE_LOCATION_H_
#define GEMINI_INCLUDE_GEMINI_CORE_LOCATION_H_

#include <gemini/export.hpp>
#include <numeric>

namespace gemini {

enum class GEMINI_EXPORT LocationType {
  Proportional, Coordinate, Pixels
};

struct GEMINI_EXPORT Point {
  double x = std::numeric_limits<double>::quiet_NaN();
  double y = std::numeric_limits<double>::quiet_NaN();

  LocationType type_x = LocationType::Pixels;
  LocationType type_y = LocationType::Pixels;

  bool relative_to_master_x = false;
  bool relative_to_master_y = false;
};

inline GEMINI_EXPORT Point MakeCoordinatePoint(double x, double y) {
  return Point{x, y, LocationType::Coordinate, LocationType::Coordinate};
}

inline GEMINI_EXPORT Point MakeRelativePoint(double x, double y) {
  return Point{x, y, LocationType::Proportional, LocationType::Proportional};
}

inline GEMINI_EXPORT Point MakePixelPoint(double x, double y) {
  return Point{x, y, LocationType::Pixels, LocationType::Pixels};
}

//! \brief Represents a displacement, e.g. from a point.
struct GEMINI_EXPORT Displacement {
  double dx, dy;
  LocationType type_dx, type_dy;
};

struct GEMINI_EXPORT Distance {
  double distance;
  LocationType type;
};

//! \brief A bounding box, in coordinate space.
struct GEMINI_EXPORT CoordinateBoundingBox {
  double left = std::numeric_limits<double>::quiet_NaN();
  double right = std::numeric_limits<double>::quiet_NaN();
  double bottom = std::numeric_limits<double>::quiet_NaN();
  double top = std::numeric_limits<double>::quiet_NaN();
};

}
#endif //GEMINI_INCLUDE_GEMINI_CORE_LOCATION_H_
