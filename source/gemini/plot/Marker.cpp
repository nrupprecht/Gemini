//
// Created by Nathaniel Rupprecht on 2/13/22.
//

#include "gemini/plot/Marker.h"
// Other files.
#include "gemini/core/Canvas.h"

using namespace gemini;
using namespace gemini::plot;
using namespace gemini::core;
using namespace gemini::plot::marker;
using namespace gemini::core::shapes;


void Marker::PlaceMarker(const Point& center) {
  center_ = center;
}

void Marker::SetColor(const core::color::PixelColor& color) {
  color_ = color;
}

void Marker::SetScale(double scale) {
  scale_ = scale;
}

CoordinateBoundingBox Marker::GetBoundingBox() const {
//  CoordinateBoundingBox bounding_box;
//  for (auto& p : marker_curve_.points) {
//    auto x = p.x, y = p.y;
//    if (std::isnan(bounding_box.left) || x < bounding_box.left) {
//      bounding_box.left = x;
//    }
//    if (std::isnan(bounding_box.right) || bounding_box.right < x) {
//      bounding_box.right = x;
//    }
//
//    if (std::isnan(bounding_box.bottom) || y < bounding_box.bottom) {
//      bounding_box.bottom = y;
//    }
//    if (std::isnan(bounding_box.top) || y < bounding_box.top) {
//      bounding_box.top = y;
//    }
//  }
//  return bounding_box;
  return {};
}

void Marker::drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  auto pixels = canvas->PointToPixels(center_);
  RasterBezierCurve(
      marker_curve_.Copy().Scale(scale_).Translate(pixels.x, pixels.y),
      bitmap,
      color_,
      zorder_);
}

void Marker::setToMirror(const std::shared_ptr<Marker>& marker) const {
  *marker = *this;
}

// ==========================================================================================
//  CircleMarker.
// ==========================================================================================

CircleMarker::CircleMarker() {
  marker_curve_ = core::shapes::BezierCurve{
      {3}, {
          {-1, 0, false},
          {0, 1, false},
          {1, 0, false},
          {0, -1, false}
      }
  };
}

// ==========================================================================================
//  CircleMarker.
// ==========================================================================================

DiamondMarker::DiamondMarker() {
  marker_curve_ = core::shapes::BezierCurve{
      {3}, {
          {-1, 0, true},
          {0, 1, true},
          {1, 0, true},
          {0, -1, true}
      }
  };
}

// ==========================================================================================
//  UpperTriangle.
// ==========================================================================================

UpperTriangle::UpperTriangle() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-0.55, 1, true},
          {0, 0, true},
          {0.55, 1, true}
      });
}

// ==========================================================================================
//  LowerTriangle.
// ==========================================================================================

LowerTriangle::LowerTriangle() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-0.55, -1, true},
          {0, 0, true},
          {0.55, -1, true}
      });
}

// ==========================================================================================
//  Square.
// ==========================================================================================

Square::Square() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1, -1, true},
          {-1, +1, true},
          {+1, +1, true},
          {+1, -1, true}
      });
}

// ==========================================================================================
//  Cross.
// ==========================================================================================

Cross::Cross() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1.0, 0.1, true},
          {-0.1, 0.1, true},
          {-0.1, 1, true},
          {+0.1, 1, true},
          {+0.1, +0.1, true},
          {+1.0, +0.1, true},
          {+1.0, -0.1, true},
          {+0.1, -0.1, true},
          {+0.1, -1.0, true},
          {-0.1, -1.0, true},
          {-0.1, -0.1, true},
          {-1.0, -0.1, true},
      });
}

// ==========================================================================================
//  Ex.
// ==========================================================================================

Ex::Ex() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1.0, 0.1, true},
          {-0.1, 0.1, true},
          {-0.1, 1, true},
          {+0.1, 1, true},
          {+0.1, +0.1, true},
          {+1.0, +0.1, true},
          {+1.0, -0.1, true},
          {+0.1, -0.1, true},
          {+0.1, -1.0, true},
          {-0.1, -1.0, true},
          {-0.1, -0.1, true},
          {-1.0, -0.1, true},
      });
  marker_curve_ = marker_curve_.Rotate(0.25 * math::PI);
}