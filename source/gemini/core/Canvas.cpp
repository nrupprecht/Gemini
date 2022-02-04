//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#include "gemini/core/Canvas.h"
// Other files.
#include <numeric>
#include <Eigen/Dense>

using namespace gemini;
using Eigen::MatrixXd;

Image::Image()
    : master_canvas_(new Canvas(this)) {
  registerCanvas(master_canvas_);
}

Image::Image(int width, int height)
    : master_canvas_(new Canvas(this)), width_(width), height_(height) {
  registerCanvas(master_canvas_);
}

Image::~Image() {
  // Clean up all the canvases associated with this image.
  for (auto ptr: canvases_) {
    delete ptr;
  }
}

void Image::Relation_Fix(int canvas1_num,
                         CanvasPart canvas1_part,
                         int canvas2_num,
                         CanvasPart canvas2_part,
                         double pixels_diff) {
  canvas_fixes_.emplace_back(canvas1_num, canvas2_num, canvas1_part, canvas2_part, pixels_diff);
}

void Image::Relation_Fix(Canvas* canvas1,
                         CanvasPart canvas1_part,
                         Canvas* canvas2,
                         CanvasPart canvas2_part,
                         double pixels_diff) {
  auto it1 = std::find(canvases_.begin(), canvases_.end(), canvas1);
  auto it2 = std::find(canvases_.begin(), canvases_.end(), canvas2);
  GEMINI_REQUIRE(it1 != canvases_.end(), "could not find first canvas");
  GEMINI_REQUIRE(it2 != canvases_.end(), "could not find second canvas");
  auto index1 = std::distance(canvases_.begin(), it1), index2 = std::distance(canvases_.begin(), it2);
  Relation_Fix(static_cast<int>(index1), canvas1_part, static_cast<int>(index2), canvas2_part, pixels_diff);
}

void Image::ClearRelationships() {
  canvas_fixes_.clear();
}

Canvas *Image::GetMasterCanvas() {
  return master_canvas_;
}

const CanvasLocation &Image::GetLocation(const class Canvas *canvas) const {
  if (auto it = canvas_locations_.find(canvas); it != canvas_locations_.end()) {
    return it->second;
  }
  GEMINI_FAIL("could not find specified canvas in the image");
}

Bitmap Image::ToBitmap() const {
  Bitmap output(width_, height_);

  if (needs_calculate_) {
    CalculateImage();
  }
  master_canvas_->writeOnBitmap(output);
  return output;
}

void Image::CalculateImage() const {
  // Determine the actual size and placement of all canvases.
  calculateCanvasLocations();
  // Determine which canvases need coordinate systems.
  calculateCanvasCoordinates();

  needs_calculate_ = false;
}

void Image::registerCanvas(class Canvas *canvas) {
  // Register this canvas in the vector.
  canvases_.push_back(canvas);
  // Create entries for the canvas.
  canvas_locations_[canvas];
  canvas_coordinate_description_[canvas];
}

void Image::calculateCanvasLocations() const {
  // Master canvas always takes up all the image space.
  canvas_locations_[master_canvas_] = {0, 0, width_, height_};

  if (canvas_fixes_.empty()) {
    if (canvases_.size() == 1) {
      return;
    }
    GEMINI_FAIL("no relationships, but there are multiple canvases");
  }

  // Represents the left, bottom, right, top (in that order) of each canvas (except the master canvas).
  MatrixXd relationships(canvas_fixes_.size(), 4 * (static_cast<int>(canvases_.size()) - 1));
  MatrixXd constants(canvas_fixes_.size(), 1);

  // Defines how to add entries in the relationship matrix for each given relationship.
  auto set_entry = [&constants, &relationships, this](int c, auto cpart, bool is_first, int count) {
    double mult = is_first ? 1. : -1.;
    switch (cpart) {
      case CanvasPart::Left:
        if (c == 0) {
          constants(count, 0) -= 0.; // For consistency.
        } else {
          relationships(count, 4 * (c - 1) + 0) = mult;
        }
        break;
      case CanvasPart::Bottom:
        if (c == 0) {
          constants(count, 0) -= 0.; // For consistency.
        } else {
          relationships(count, 4 * (c - 1) + 1) = mult;
        }
        break;
      case CanvasPart::Right:
        if (c == 0) {
          constants(count, 0) -= mult * width_;
        } else {
          relationships(count, 4 * (c - 1) + 2) = mult;
        }
        break;
      case CanvasPart::Top:
        if (c == 0) {
          constants(count, 0) -= mult * height_;
        } else {
          relationships(count, 4 * (c - 1) + 3) = mult;
        }
        break;
      case CanvasPart::CenterX:
        if (c == 0) {
          constants(count, 0) -= 0.5 * mult * width_;
        } else {
          relationships(count, 4 * (c - 1) + 2) = 0.5 * mult;
          relationships(count, 4 * (c - 1) + 0) = 0.5 * mult;
        }
        break;
      case CanvasPart::CenterY:
        if (c == 0) {
          constants(count, 0) -= 0.5 * mult * height_;
        } else {
          relationships(count, 4 * (c - 1) + 3) = 0.5 * mult;
          relationships(count, 4 * (c - 1) + 1) = 0.5 * mult;
        }
    }
  };

  int count = 0;
  for (const auto&[c1, c2, cpart1, cpart2, dpx]: canvas_fixes_) {
    set_entry(c1, cpart1, true, count);
    set_entry(c2, cpart2, false, count);
    constants(count, 0) -= dpx;

    ++count;
  }

  // Solve R X + b = 0 for X
  MatrixXd canvas_positions;
  try {
    canvas_positions = relationships.fullPivLu().solve(constants);
  }
  catch (const std::exception &ex) {
    GEMINI_FAIL("could not determine canvas location: " << ex.what());
  }

  for (int i = 0; i < canvas_positions.rows() / 4; ++i) {
    int base = 4 * i;
    CanvasLocation canvas_location{
        (int) canvas_positions(base + 0, 0), // left
        (int) canvas_positions(base + 1, 0), // bottom
        (int) canvas_positions(base + 2, 0), // right
        (int) canvas_positions(base + 3, 0)}; // top

    canvas_locations_[canvases_[i + 1]] = canvas_location;
  }
}

void Image::calculateCanvasCoordinates() const {
  // Check which, if any, canvases need coordinate systems. If so, determine what they should be.
  for (const auto &canvas: canvases_) {
    // Get coordinates, if there are any. A lack of coordinates is signaled by quiet NaN.
    auto min_max_coords = getMinMaxCoordinates(canvas);
    // Determine the coordinate system that should be used for the canvas.
    describeCoordinates(canvas, min_max_coords);
  }
}

std::array<double, 4> Image::getMinMaxCoordinates(class Canvas *canvas) {
  // Check if any object uses coordinates.
  double min_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double min_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  for (const auto &shape: canvas->shapes_) {
    auto[min_x, max_x, min_y, max_y] = shape->GetBoundingBox();

    if (!std::isnan(min_x) && (std::isnan(min_coordinate_x) || min_x < min_coordinate_x)) {
      min_coordinate_x = min_x;
    }
    if (!std::isnan(max_x) && (std::isnan(max_coordinate_x) || max_coordinate_x < max_x)) {
      max_coordinate_x = max_x;
    }
    if (!std::isnan(min_y) && (std::isnan(min_coordinate_y) || min_y < min_coordinate_y)) {
      min_coordinate_y = min_y;
    }
    if (!std::isnan(max_y) && (std::isnan(max_coordinate_y) || max_coordinate_y < max_y)) {
      max_coordinate_y = max_y;
    }
  }

  return {min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y};
}

void Image::describeCoordinates(Canvas *canvas, const std::array<double, 4> &min_max_coords) const {
  auto[min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y] = min_max_coords;

  // Check whether there were any coordinates in either x or y.
  if (!std::isnan(min_coordinate_x) || !std::isnan(min_coordinate_y)) {
    auto &description = canvas_coordinate_description_[canvas];
    description.has_coordinates = true;

    // If a coordinate system has been specified, use that. Otherwise, use minimum coordinate point.

    // Left coordinate.
    if (!std::isnan(canvas->coordinate_system_.left)) {
      description.left = canvas->coordinate_system_.left;
    } else {
      if (std::isnan(min_coordinate_x)) {
        description.left = -default_coordinate_epsilon;
      } else if (min_coordinate_x == max_coordinate_x) {
        description.left = min_coordinate_x - default_coordinate_epsilon;
      } else {
        description.left = min_coordinate_x;
      }
    }

    // Right coordinate.
    if (!std::isnan(canvas->coordinate_system_.right)) {
      description.right = canvas->coordinate_system_.right;
    } else {
      if (std::isnan(min_coordinate_x)) {
        description.right = default_coordinate_epsilon;
      } else if (min_coordinate_x == max_coordinate_x) {
        description.right = min_coordinate_x + default_coordinate_epsilon;
      } else {
        description.right = max_coordinate_x;
      }
    }

    // Bottom coordinate.
    if (!std::isnan(canvas->coordinate_system_.bottom)) {
      description.bottom = canvas->coordinate_system_.bottom;
    } else {
      if (std::isnan(min_coordinate_y)) {
        description.bottom = -default_coordinate_epsilon;
      } else if (min_coordinate_y == max_coordinate_y) {
        description.bottom = min_coordinate_y - default_coordinate_epsilon;
      } else {
        description.bottom = min_coordinate_y;
      }
    }

    // Top coordinate.
    if (!std::isnan(canvas->coordinate_system_.top)) {
      description.top = canvas->coordinate_system_.top;
    } else {
      if (std::isnan(min_coordinate_y)) {
        description.top = default_coordinate_epsilon;
      } else if (min_coordinate_y == max_coordinate_y) {
        description.top = min_coordinate_y + default_coordinate_epsilon;
      } else {
        description.top = max_coordinate_y;
      }
    }
  }
}

Canvas *Canvas::FloatingSubCanvas() {
  auto *sub_canvas = new Canvas(this);
  image_->registerCanvas(sub_canvas);
  child_canvases_.push_back(sub_canvas);

  return sub_canvas;
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

void Canvas::SetCoordinates(const CanvasCoordinates &coordinates) {
  coordinate_system_ = coordinates;
}

CanvasCoordinates &Canvas::GetCoordinateSystem() {
  return coordinate_system_;
}

const color::PixelColor &Canvas::GetBackgroundColor() const {
  return background_color_;
}

Point Canvas::PointToPixels(const Point &point) const {
  Point pixelPoint{};
  pixelPoint.type_x = pixelPoint.type_y = LocationType::Pixels;

  auto &location = image_->canvas_locations_.find(this)->second;

  // Handle x coordinate of the point.
  switch (point.type_x) {
    case LocationType::Coordinate: {
      auto &description = image_->canvas_coordinate_description_.find(this)->second;
      auto proportion_x = (point.x - description.left) / (description.right - description.left);
      pixelPoint.x = location.left + (location.right - location.left) * proportion_x;
      break;
    }
    case LocationType::Pixels: {
      pixelPoint.x = point.x; // Already in pixels.
      break;
    }
    case LocationType::Proportional: {
      pixelPoint.x = location.left + (location.right - location.left) * point.x;
      break;
    }
    default:GEMINI_FAIL("unrecognized LocationType");
  }

  // Handle y coordinate of the point.
  switch (point.type_y) {
    case LocationType::Coordinate: {
      auto &description = image_->canvas_coordinate_description_.find(this)->second;
      auto proportion_y = (point.y - description.bottom) / (description.top - description.bottom);
      pixelPoint.y = location.bottom + (location.top - location.bottom) * proportion_y;
      break;
    }
    case LocationType::Pixels: {
      pixelPoint.y = point.y; // Already in pixels.
      break;
    }
    case LocationType::Proportional: {
      pixelPoint.y = location.bottom + (location.top - location.bottom) * point.y;
      break;
    }
    default:GEMINI_FAIL("unrecognized LocationType");
  }
  return pixelPoint;
}

Displacement Canvas::DisplacementToPixels(const Displacement &displacement) const {
  // A displacement is the same as a point measured from the origin.
  Point point{displacement.dx, displacement.dy, displacement.type_dx, displacement.type_dy};
  auto pixel_point = PointToPixels(point);
  Displacement pixelsDisplacement{pixel_point.x, pixel_point.y, LocationType::Pixels, LocationType::Pixels};
  return pixelsDisplacement;
}

bool Canvas::isTopLevelCanvas() const {
  return parent_ == nullptr;
}

void Canvas::writeOnBitmap(Bitmap &image) const {
  CanvasLocation location = image_->GetLocation(this);
  image.SetPermittedRegion(std::floor(location.left),
                           std::ceil(location.right),
                           std::floor(location.bottom),
                           std::ceil(location.top));

  if (paint_background_) {
    paintBackground(image);
  }

  for (const auto &shape: shapes_) {
    shape->DrawOnBitmap(image, this);
  }

  for (const auto &child: child_canvases_) {
    child->writeOnBitmap(image);
  }
}

void Canvas::paintBackground(Bitmap &image) const {
  auto &location = image_->canvas_locations_.find(this)->second;

  for (int x = location.left; x < location.right; ++x) {
    for (int y = location.bottom; y < location.top; ++y) {
      image.SetPixel(x, y, background_color_);
    }
  }
}
