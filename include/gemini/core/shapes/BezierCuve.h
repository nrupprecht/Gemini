//
// Created by Nathaniel Rupprecht on 1/30/22.
//

#ifndef GEMINI_INCLUDE_GEMINI_CORE_SHAPES_BEZIERCUVE_H_
#define GEMINI_INCLUDE_GEMINI_CORE_SHAPES_BEZIERCUVE_H_

#include "gemini/core/shapes/Shapes.h"

namespace gemini::core::shapes {


struct GEMINI_EXPORT BezierPoint : public GeometricPoint {
  bool is_on_curve = false;
};

struct GEMINI_EXPORT BezierCurve {
  // ============================================================================
  //  Member data.
  // ============================================================================

  std::vector<unsigned short> contour_ends{}; // uint16
  std::vector<BezierPoint> points{};

  // ============================================================================
  //  Member functions.
  // ============================================================================

  NO_DISCARD BezierCurve Copy() const;

  NO_DISCARD std::size_t NumPoints() const;

  NO_DISCARD std::size_t NumContours() const;

  BezierCurve& Scale(double factor);

  BezierCurve& Translate(double dx, double dy);

  BezierCurve& Rotate(double theta);

  BezierCurve& SkewX(double theta);

  BezierCurve& ScaleShifted(double factor, double dx = 0., double dy = 0.);

  BezierCurve& ShiftScaled(double factor, double dx = 0., double dy = 0.);
};


void GEMINI_EXPORT RasterBezierCurve(const shapes::BezierCurve& spline,
                                     Bitmap& bmp,
                                     color::PixelColor color,
                                     double z = 0.,
                                     bool color_by_spline = false);


class GEMINI_EXPORT QuadraticBezierCurve : public Shape {
 public:
  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;

 private:
  void drawOnBitmap(Bitmap &bitmap, const Canvas *canvas) const override;

  shapes::BezierCurve spline_;

  color::PixelColor color_;
};

}
#endif //GEMINI_INCLUDE_GEMINI_CORE_SHAPES_BEZIERCUVE_H_
