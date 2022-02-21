//
// Created by Nathaniel Rupprecht on 1/30/22.
//

#ifndef GEMINI_INCLUDE_GEMINI_CORE_SHAPES_BEZIERCUVE_H_
#define GEMINI_INCLUDE_GEMINI_CORE_SHAPES_BEZIERCUVE_H_

#include "gemini/core/shapes/Shapes.h"

namespace gemini::core::shapes {

//! \brief A special point for building a BezierCurve. It is a point that also has a flag to indicate whether it is
//! "on the curve" as opposed to being a control point.
struct GEMINI_EXPORT BezierPoint : public GeometricPoint {
  bool is_on_curve = false;
};


//! \brief Basic geometric representation of a closed shape composed of one or more linear or quadratic bezier curves.
//!
//! The form of BezierCurve was influenced by the needs of the TrueType font, which can represent glyph outlines in
//! roughly this fashion, though in a much more compressed form, with flags and repeats etc.
//! The winding number of a point determines whether it will be shaded during rasterization. If the winding number is zero,
//! it will not be. Otherwise, it is determined to lie within the figure.
struct GEMINI_EXPORT BezierCurve {
  // ============================================================================
  //  Member data.
  // ============================================================================

  std::vector<unsigned short> contour_ends{}; // uint16
  std::vector<BezierPoint> points{};

  // ============================================================================
  //  Member functions.
  // ============================================================================

  //! \brief Create a copy of the BezierCurve.
  NO_DISCARD BezierCurve Copy() const;

  //! \brief Get the number of points that the BezierCurve has.
  NO_DISCARD std::size_t NumPoints() const;

  //! \brief Get the number of individual contours that make up the BezierCurve.
  NO_DISCARD std::size_t NumContours() const;

  //! \brief Scales and returns the BezierCurve. This simply scales all the point coordinates by the factor.
  BezierCurve& Scale(double factor);

  //! \brief Translates and returns the BezierCurve. This simply translates all the point coordinates by the factor.
  BezierCurve& Translate(double dx, double dy);

  //! \brief Rotates and returns the BezierCurve. This simply rotate all the point coordinates by the angle about the origin.
  BezierCurve& Rotate(double theta);

  //! \brief Skews and returns the BezierCurve. This simply shifts the x coordinate of all the points by an amount that depends on theta.
  BezierCurve& SkewX(double theta);

  //! \brief Shifts, then scales the BezierCurve.
  //!
  //! This is more efficient then just chaining operations, but is equivalent to
  //!     curve.Translate(dx, dy).Scale(factor)
  BezierCurve& ScaleShifted(double factor, double dx = 0., double dy = 0.);

  //! \brief Scales, then shifts the BezierCurve.
  //!
  //! This is more efficient then just chaining operations, but is equivalent to
  //!     curve.Scale(factor).Translate(dx, dy)
  BezierCurve& ShiftScaled(double factor, double dx = 0., double dy = 0.);

  //! \brief Reverses the order of all the points in each BezierCurve contour.
  BezierCurve& ReverseWinding();

  //! \brief Adds the contours of another BezierCurve to this one.
  BezierCurve& Append(const BezierCurve& curve);

  // ============================================================================
  //  Static functions.
  // ============================================================================

  //! \brief Make a BezierCurve consisting of a single contour.
  static BezierCurve MakeSingleContourCurve(const std::vector<BezierPoint>& points);
};

//! \brief Raster a BezierCurve to a bitmap.
//!
//! \param spline The BezierCurve to raster.
//! \param bmp The bitmap on which to raster the curve.
//! \param color The color to use for the raster.
//! \param z The z order for the raster.
//! \param color_by_spline A debugging feature, if true, each bezier curve segment will be colored differently.
//!
//! NOTE(Nate): Does not currently do any anti-aliasing.
void GEMINI_EXPORT RasterBezierCurve(
    const shapes::BezierCurve& spline,
    Bitmap& bmp,
    color::PixelColor color,
    double z = 0.,
    bool color_by_spline = false);

//! \brief A shape that encapsulates a BezierCurve plus additional shape related information and functionality.
class GEMINI_EXPORT QuadraticBezierCurve : public Shape {
 public:
  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;
 private:
  void drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const override;

  //! \brief The BezierCurve that defines the shape.
  shapes::BezierCurve spline_;

  //! \brief The color of the shape.
  color::PixelColor color_;
};

}
#endif //GEMINI_INCLUDE_GEMINI_CORE_SHAPES_BEZIERCUVE_H_
