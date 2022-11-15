//
// Created by Nathaniel Rupprecht on 11/28/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_SHAPES_H_
#define GEMINI_INCLUDE_GEMINI_SHAPES_H_

#include "gemini/core/Bitmap.h"
#include "gemini/core/Location.h"


namespace gemini::core {

// Forward declaration to Canvas.
class Canvas;

//! \brief An abstract point in some two dimensional geometric space.
struct GeometricPoint {
  double x{}, y{};
};

//! \brief Return the point that results from rotating the input point an angle theta around the origin.
GeometricPoint Rotate(const GeometricPoint& p, double theta);


//! \brief Abstract base class for shapes that can be drawn on bitmaps.
class GEMINI_EXPORT Shape {
 public:
  //! \brief Raster a shape onto a bitmap.
  virtual void DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const;

  //! \brief Get a bounding box for the shape, in coordinate space. If the shape does not have coordinates defined
  //! in the x or y directions, the bounds may be NaN.
  NO_DISCARD virtual CoordinateBoundingBox GetBoundingBox() const = 0;

  //! \brief Set the default z order for the shape.
  void SetZOrder(double z);

  //! \brief Set whether the shape should be restricted to only draw in the permitted region of the bitmap.
  void SetRestricted(bool r);

 protected:
  //! \brief Private purely virtual method for rendering the shape onto a bitmap.
  virtual void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const = 0;

  //! \brief Write on a bitmap using the proper z-order.
  void write(Bitmap& bitmap, int x, int y, color::PixelColor color) const;

  //! \brief The default zorder for the shape.
  double zorder_ = 1.;

  //! \brief Whether the shape should be restricted to only draw in the permitted region of whatever bitmap it
  //! is rastered onto.
  bool restricted_ = true;
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

 private:
  void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

class GEMINI_EXPORT XiaolinWuLine final : public Line {
 public:
  XiaolinWuLine(Point first, Point second, color::PixelColor color)
  : Line(first, second, color)
  {}

 private:
  void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

class GEMINI_EXPORT XiaolinWuThickLine final : public Line {
 public:
  XiaolinWuThickLine(Point first, Point second, color::PixelColor color, double thickness = 2)
  : Line(first, second, color), pixel_thickness(thickness)
  {}

  double pixel_thickness;

 private:
  void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

class GEMINI_EXPORT Ray final : public Shape {
 public:
  Ray(Point base, Displacement ray, color::PixelColor color, double thickness)
  : base_(base), ray_(ray), color_(color), thickness_(thickness)
  {}

  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;
 private:
  void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;

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

  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;

 private:
  void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;
};

} // namespace gemini::core

#endif //GEMINI_INCLUDE_GEMINI_SHAPES_H_
