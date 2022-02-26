//
// Created by Nathaniel Rupprecht on 2/13/22.
//

#ifndef GEMINI_INCLUDE_GEMINI_PLOT_MARKER_H_
#define GEMINI_INCLUDE_GEMINI_PLOT_MARKER_H_

#include "gemini/core/shapes/BezierCuve.h"

namespace gemini::plot::marker {

class GEMINI_EXPORT Marker : public core::Shape {
 public:
  //! \brief "Place" the marker at a coordinate point.
  void PlaceMarker(const Point& center);

  //! \brief Set the color of the marker.
  void SetColor(const core::color::PixelColor& color);

  //! \brief Set the marker scale.
  void SetScale(double scale);

  //! \brief Returns a pointer to a deep copy of the marker.
  NO_DISCARD virtual std::shared_ptr<Marker> Copy() const = 0;

  //! \brief Base override, based on markers. Can be overridden by children.
  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;

 protected:
  void drawOnBitmap(core::Bitmap& bitmap, const core::Canvas* canvas) const override;

  void setToMirror(const std::shared_ptr<Marker>& marker) const;

  core::shapes::BezierCurve marker_curve_;

  //! \brief The marker scale.
  double scale_ = 5;

  core::color::PixelColor color_ = core::color::Black;

  Point center_;
};

#define MARKER_CLASS(name) class GEMINI_EXPORT name : public Marker { \
  public:                                                             \
    name();                                                           \
    NO_DISCARD std::shared_ptr<Marker> Copy() const override {        \
      auto ptr = std::make_shared<name>();                            \
      setToMirror(ptr);                                               \
      return ptr;                                                     \
    }                                                                 \
}

// ==========================================================================================
//  Define some default simple Markers classes.
// ==========================================================================================

MARKER_CLASS(Point);
MARKER_CLASS(Circle);
MARKER_CLASS(Diamond);
MARKER_CLASS(UpperTriangle);
MARKER_CLASS(LowerTriangle);
MARKER_CLASS(Square);
MARKER_CLASS(Cross);
MARKER_CLASS(Ex);
MARKER_CLASS(Rectangle);

}
#endif //GEMINI_INCLUDE_GEMINI_PLOT_MARKER_H_
