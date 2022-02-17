//
// Created by Nathaniel Rupprecht on 2/13/22.
//

#ifndef GEMINI_INCLUDE_GEMINI_PLOT_MARKER_H_
#define GEMINI_INCLUDE_GEMINI_PLOT_MARKER_H_

#include "gemini/core/shapes/BezierCuve.h"

namespace gemini::plot {

class Marker : public core::Shape {
 protected:
  void drawOnBitmap(core::Bitmap& bitmap, const core::Canvas* canvas) const override;

  core::shapes::BezierCurve marker_curve_;

  double scale_ = 1;

  core::color::PixelColor color_ = core::color::Black;
};

}
#endif //GEMINI_INCLUDE_GEMINI_PLOT_MARKER_H_
