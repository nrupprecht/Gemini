//
// Created by Nathaniel Rupprecht on 2/13/22.
//

#include "gemini/plot/Marker.h"

using namespace gemini::plot;
using namespace gemini::core;

void Marker::drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  RasterBezierCurve(
      marker_curve_,
      bitmap,
      color_,
      zorder_);
}