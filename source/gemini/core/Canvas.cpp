//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#include "gemini/core/Canvas.h"
// Other files.
#include <numeric>
#include <iomanip>

using namespace gemini;
using namespace gemini::core;


// ==========================================================================================
//  Canvas.
// ==========================================================================================

std::shared_ptr<Canvas> Canvas::FloatingSubCanvas() {
  auto sub_canvas = std::shared_ptr<Canvas>(new Canvas(this));
  image_->RegisterCanvas(sub_canvas);
  child_canvases_.push_back(sub_canvas);

  return sub_canvas;
}

void Canvas::SetLocation(const CanvasLocation& location) {

}

void Canvas::AddShape(std::shared_ptr<Shape> shape) {
  shapes_.push_back(std::move(shape));
  image_->needs_calculate_ = true;
}

void Canvas::SetBackground(color::PixelColor color) {
  background_color_ = color;
}

void Canvas::SetPaintBackground(bool flag) {
  paint_background_ = flag;
}

void Canvas::SetCoordinates(const CoordinateDescription& coordinates) {
  coordinate_system_ = coordinates;
}

CoordinateDescription& Canvas::GetCoordinateSystem() {
  return coordinate_system_;
}

const color::PixelColor& Canvas::GetBackgroundColor() const {
  return background_color_;
}

Point Canvas::PointToPixels(const Point& point) const {
  Point pixel_point{};
  // Make sure the pixel point is relative to master.
  pixel_point.relative_to_master_x = pixel_point.relative_to_master_y = true;

  auto& location = image_->canvas_locations_.find(this)->second;

  // Handle x coordinate of the point.
  switch (point.type_x) {
    case LocationType::Coordinate: {
      auto proportion_x = (point.x - coordinate_system_.left) / (coordinate_system_.right - coordinate_system_.left);
      pixel_point.x = (location.right - location.left) * proportion_x;
      break;
    }
    case LocationType::Pixels: {
      pixel_point.x = point.x; // Already in pixels.
      break;
    }
    case LocationType::Proportional: {
      pixel_point.x = (location.right - location.left) * point.x;
      break;
    }
    default: {
      GEMINI_FAIL("unrecognized LocationType");
    }
  }

  // Handle y coordinate of the point.
  switch (point.type_y) {
    case LocationType::Coordinate: {
      auto proportion_y = (point.y - coordinate_system_.bottom) / (coordinate_system_.top - coordinate_system_.bottom);
      pixel_point.y = (location.top - location.bottom) * proportion_y;
      break;
    }
    case LocationType::Pixels: {
      pixel_point.y = point.y; // Already in pixels.
      break;
    }
    case LocationType::Proportional: {
      pixel_point.y = (location.top - location.bottom) * point.y;
      break;
    }
    default: {
      GEMINI_FAIL("unrecognized LocationType");
    }
  }

  if (!point.relative_to_master_x) {
    pixel_point.x += location.left;
  }
  if (!point.relative_to_master_y) {
    pixel_point.y += location.bottom;
  }

  return pixel_point;
}

Displacement Canvas::DisplacementToPixels(const Displacement& displacement) const {

  // Sometimes have to remove image shift.
  auto& location = image_->canvas_locations_.find(this)->second;

  Displacement pixels_displacement{};

  // Handle x coordinate of the point.
  switch (displacement.type_dx) {
    case LocationType::Coordinate: {
      auto proportion_x = displacement.dx / (coordinate_system_.right - coordinate_system_.left);
      pixels_displacement.dx = (location.right - location.left) * proportion_x;
      break;
    }
    case LocationType::Pixels: {
      pixels_displacement.dx = displacement.dx; // Already in pixels.
      break;
    }
    case LocationType::Proportional: {
      pixels_displacement.dx = (location.right - location.left) * displacement.dx;
      break;
    }
    default: {
      GEMINI_FAIL("unrecognized LocationType");
    }
  }

  // Handle y coordinate of the point.
  switch (displacement.type_dy) {
    case LocationType::Coordinate: {
      auto proportion_y = displacement.dy / (coordinate_system_.top - coordinate_system_.bottom);
      pixels_displacement.dy = (location.top - location.bottom) * proportion_y;
      break;
    }
    case LocationType::Pixels: {
      pixels_displacement.dy = displacement.dy; // Already in pixels.
      break;
    }
    case LocationType::Proportional: {
      pixels_displacement.dy = (location.top - location.bottom) * displacement.dy;
      break;
    }
    default: {
      GEMINI_FAIL("unrecognized LocationType");
    }
  }

  return pixels_displacement;
}

bool Canvas::isTopLevelCanvas() const {
  return parent_ == nullptr;
}

void Canvas::writeOnBitmap(Bitmap& image) const {
  CanvasLocation location = image_->GetLocation(this);
  image.SetPermittedRegion(
      std::floor(location.left),
      std::ceil(location.right),
      std::floor(location.bottom),
      std::ceil(location.top));

  if (paint_background_) {
    paintBackground(image);
  }

  for (const auto& shape : shapes_) {
    shape->DrawOnBitmap(image, this);
  }

  for (const auto& child : child_canvases_) {
    child->writeOnBitmap(image);
  }
}

void Canvas::paintBackground(Bitmap& image) const {
  auto& location = image_->canvas_locations_.find(this)->second;

  for (int x = location.left ; x < location.right ; ++x) {
    for (int y = location.bottom ; y < location.top ; ++y) {
      image.SetPixel(x, y, background_color_);
    }
  }
}
