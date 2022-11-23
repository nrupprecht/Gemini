//
// Created by Nathaniel Rupprecht on 2/5/22.
//

#pragma once

#include <gemini/export.hpp>
#include <numeric>
#include <optional>

namespace gemini {

using Byte = unsigned char;

enum class GEMINI_EXPORT LocationType {
  Proportional, Coordinate, Pixels
};

enum LocationTypeFlags : Byte {
  PixelsX       = 0b00000001,
  CoordinatesX  = 0b00000010,
  ProportionalX = 0b00000100,
  RelativeX     = 0b00001000,
  PixelsY       = 0b00010000,
  CoordinatesY  = 0b00100000,
  ProportionalY = 0b01000000,
  RelativeY     = 0b10000000,
};

inline bool IsPixelsX(Byte flags) {
  return PixelsX & flags;
}

inline bool IsCoordinatesX(Byte flags) {
  return CoordinatesX & flags;
}

inline bool IsProportionalX(Byte flags) {
  return ProportionalX & flags;
}

inline bool IsRelativeX(Byte flags) {
  return RelativeX & flags;
}

inline bool IsPixelsY(Byte flags) {
  return PixelsY & flags;
}

inline bool IsCoordinatesY(Byte flags) {
  return CoordinatesY & flags;
}

inline bool IsProportionalY(Byte flags) {
  return ProportionalY & flags;
}

inline bool IsRelativeY(Byte flags) {
  return RelativeY & flags;
}

// --> End speculative section.


//! \brief A point on a canvas. The meaning of the x and y values can be specified separately.
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

//! \brief Represents a distance. Can be used to represent, e.g., the radius of a circle.
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


//! \brief Encodes where on a canvas an object should be located.
struct GEMINI_EXPORT CanvasLocation {
  int left{}, bottom{}, right{}, top{};

  constexpr bool operator==(const CanvasLocation& rhs) const {
    return std::tie(left, bottom, right, top) == std::tie(rhs.left, rhs.bottom, rhs.right, rhs.top);
  }

  friend std::ostream& operator<<(std::ostream& out, const CanvasLocation& location) {
    out << "{ L=" << location.left << ", R=" << location.right
        << ", B=" << location.bottom << ", T=" << location.top << " }";
    return out;
  }
};


//! \brief Base interface for objects that can be put in relationships with other Locatable objects in an image.
class Locatable {
 public:
  //! \brief If the locatable has a predefined width, get it. By default, returns that the width is not defined yet.
  NO_DISCARD virtual std::optional<double> GetWidth() const { return {}; }
  //! \brief If the locatable has a predefined height, get it. By default, returns that the height is not defined yet.
  NO_DISCARD virtual std::optional<double> GetHeight() const { return {}; }

  //! \brief Set the location of the locatable object after calculating where it should go.
  virtual void SetLocation(const CanvasLocation& location) = 0;
};


}
