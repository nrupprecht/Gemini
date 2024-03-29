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

Marker& Marker::PlaceMarker(const gemini::Point& center) {
  center_ = center;
  return *this;
}

Marker& Marker::SetColor(const color::PixelColor& color) {
  color_ = color;
  return *this;
}

Marker& Marker::SetScale(double scale) {
  scale_ = scale;
  return *this;
}

CoordinateBoundingBox Marker::GetBoundingBox() const {
  return CoordinateBoundingBox{center_.x, center_.x, center_.y, center_.y};

  CoordinateBoundingBox bounding_box;
  for (auto& p : marker_curve_.points) {
    auto x = p.x, y = p.y;
    if (std::isnan(bounding_box.left) || x < bounding_box.left) {
      bounding_box.left = x;
    }
    if (std::isnan(bounding_box.right) || bounding_box.right < x) {
      bounding_box.right = x;
    }

    if (std::isnan(bounding_box.bottom) || y < bounding_box.bottom) {
      bounding_box.bottom = y;
    }
    if (std::isnan(bounding_box.top) || y < bounding_box.top) {
      bounding_box.top = y;
    }
  }
  return bounding_box;
  // return {};
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
//  Point.
// ==========================================================================================

marker::Point::Point() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1, 0, false},
          {0, 1, false},
          {1, 0, false},
          {0, -1, false}
      });
}

// ==========================================================================================
//  CircleMarker.
// ==========================================================================================

marker::Circle::Circle() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1, 0, false},
          {0, 1, false},
          {1, 0, false},
          {0, -1, false}
      });
  marker_curve_.Append(marker_curve_.Copy().ReverseWinding().Scale(0.8));
}

// ==========================================================================================
//  Diamond.
// ==========================================================================================

Diamond::Diamond() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1, 0, true},
          {0, 1, true},
          {1, 0, true},
          {0, -1, true}
      });
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

// ==========================================================================================
//  LineSegment.
// ==========================================================================================

Rectangle::Rectangle() {
  marker_curve_ = BezierCurve::MakeSingleContourCurve(
      {
          {-1.0, +0.25, true},
          {+1.0, +0.25, true},
          {+1.0, -0.25, true},
          {-1.1, -0.25, true},
      });
}