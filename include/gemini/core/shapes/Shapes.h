//
// Created by Nathaniel Rupprecht on 11/28/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_SHAPES_H_
#define GEMINI_INCLUDE_GEMINI_SHAPES_H_

#include "gemini/core/Bitmap.h"

namespace gemini {

// Forward declaration to Canvas.
class Canvas;


enum class GEMINI_EXPORT LocationType {
  Proportional, Coordinate, Pixels
};

struct GEMINI_EXPORT Point {
  double x, y;
  LocationType type_x, type_y;
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
  double left, right, bottom, top;
};

//! \brief Abstract base class for shapes that can be drawn on bitmaps.
class GEMINI_EXPORT Shape {
 public:
  virtual void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const = 0;
  NO_DISCARD virtual CoordinateBoundingBox GetBoundingBox() const = 0;

  void SetZOrder(double z);
 protected:
  //! \brief Write on a bitmap using the proper z-order.
  void write(Bitmap& bitmap, int x, int y, color::PixelColor color) const;

  //! \brief The default zorder for the shape.
  double zorder_ = 1.;
};

class GEMINI_EXPORT Line : public Shape {
 public:
  Line(Point first, Point second, color::PixelColor color)
  : first(first), second(second), color(color)
  {}

  Point first{}, second{};
  color::PixelColor color = color::Black;

  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;
};

// Concrete classes

class GEMINI_EXPORT BresenhamLine final : public Line {
 public:
  BresenhamLine(Point first, Point second, color::PixelColor color)
  : Line(first, second, color)
  {}

  void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

class GEMINI_EXPORT XiaolinWuLine final : public Line {
 public:
  XiaolinWuLine(Point first, Point second, color::PixelColor color)
  : Line(first, second, color)
  {}

  void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

class GEMINI_EXPORT XiaolinWuThickLine final : public Line {
 public:
  XiaolinWuThickLine(Point first, Point second, color::PixelColor color, double thickness = 2)
  : Line(first, second, color), pixel_thickness(thickness)
  {}

  double pixel_thickness;

  void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

class GEMINI_EXPORT Ray final : public Shape {
 public:
  Ray(Point base, Displacement ray, color::PixelColor color, double thickness)
  : base_(base), ray_(ray), color_(color), thickness_(thickness)
  {}

  void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;
 private:
  Point base_;
  Displacement ray_;
  color::PixelColor color_;
  double thickness_;
};

struct GEMINI_EXPORT Circle final : public Shape {
 public:
  Circle(Point center, Distance radius, color::PixelColor color = color::Black)
    : center(center), radius(radius), color(color)
  {}

  Point center{};
  Distance radius{};
  color::PixelColor color = color::Black;

  void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;
};

} // namespace gemini

#endif //GEMINI_INCLUDE_GEMINI_SHAPES_H_
