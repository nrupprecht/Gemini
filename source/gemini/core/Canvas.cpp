//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#include "gemini/core/Canvas.h"
// Other files.
#include <numeric>
#include <Eigen/Dense>

using namespace gemini;
using namespace gemini::core;
using Eigen::MatrixXd;

class Image::Impl {
  friend class Canvas;

 public:
  Impl();
  Impl(int width, int height);

  void Relation_Fix(
      int canvas1_num,
      CanvasPart canvas1_part,
      int canvas2_num,
      CanvasPart canvas2_part,
      double pixels_diff = 0.);

  void Relation_Fix(
      Canvas* canvas1,
      CanvasPart canvas1_part,
      Canvas* canvas2,
      CanvasPart canvas2_part,
      double pixels_diff = 0.);

  void Dimensions_Fix(
      Canvas* canvas,
      CanvasDimension dim,
      double extent);

  void ClearRelationships();

  std::shared_ptr<Canvas> GetMasterCanvas();

  const CanvasLocation& GetLocation(const Canvas* canvas) const;

  int GetWidth() const;
  int GetHeight() const;

  NO_DISCARD Bitmap ToBitmap() const;

  void CalculateImage() const;

  void CalculateCanvasLocations() const;

  void CalculateCanvasCoordinates() const;

  const CoordinateDescription& GetCanvasCoordinateDescription(const std::shared_ptr<const Canvas>& canvas) const;

  void RegisterCanvas(const std::shared_ptr<Canvas>& canvas);

 protected:

  //! \brief Determine the maximum and minimum coordinate points of a canvas in the x and y directions.
  //!
  //! If there are no objects with coordinates in x or y, those max/min coordinates will be quiet NaNs.
  static std::array<double, 4> getMinMaxCoordinates(Canvas* canvas);
  void describeCoordinates(class Canvas* canvas, const std::array<double, 4>& min_max_coords) const;

  ///========================================================================
  /// Protected variables.
  ///========================================================================

  //! \brief Brief the "master" canvas. This represents the surface of the entire image.
  //!
  //! All canvases for the image will either be this canvas, or a child (grandchild, etc.) canvas of this canvas.
  std::shared_ptr<Canvas> master_canvas_ = nullptr;

  //! \brief A vector of all the canvas that are associated with this image.
  //!
  //! This will include the master_canvas_, its children, their children, etc.
  std::vector<std::shared_ptr<Canvas>> canvases_;

  std::vector<FixRelationship> canvas_fixes_;
  std::vector<FixDimensions> canvas_dimensions_fixes_;

  //! \brief What coordinate system width to use if there is no coordinate extent in the x or y dimensions.
  //! This can happen either because there is only one point that needs coordinates in x or y, or because, e.g. the
  //! x dimension has coordinate objects, but the y dimension does not. So a coordinate system is still used in the y
  //! dimension [NOTE(Nate): separate the ability to have coordinate systems for x and y?].
  double default_coordinate_epsilon = 0.0001;

  //! \brief Determines the sizes and locations of each canvas.
  mutable std::map<const Canvas*, CanvasLocation> canvas_locations_;

  //! \brief Describes whether each canvas has a coordinate description, and if so, what it is.
  mutable std::map<const Canvas*, CoordinateDescription> canvas_coordinate_description_;

  //! \brief True whenever canvases and image parameters may need to be recalculated.
  mutable bool needs_calculate_ = false;

  //! \brief Store the width and heigh of the whole image, in pixels.
  //!
  //! Since the final image will (at least in e.g. the bitmap case) be in pixels, we store these as ints, not as doubles.
  mutable int width_ = 100, height_ = 100;
};


// ==========================================================================================
//  Image::Impl.
// ==========================================================================================

Image::Impl::Impl()
    : master_canvas_(new Canvas(this)) {
  RegisterCanvas(master_canvas_);
}

Image::Impl::Impl(int width, int height)
    : master_canvas_(new Canvas(this)), width_(width), height_(height) {
  RegisterCanvas(master_canvas_);
}

void Image::Impl::Relation_Fix(
    int canvas1_num,
    CanvasPart canvas1_part,
    int canvas2_num,
    CanvasPart canvas2_part,
    double pixels_diff) {
  canvas_fixes_.emplace_back(canvas1_num, canvas2_num, canvas1_part, canvas2_part, pixels_diff);
}

void Image::Impl::Relation_Fix(
    Canvas* canvas1,
    CanvasPart canvas1_part,
    Canvas* canvas2,
    CanvasPart canvas2_part,
    double pixels_diff) {
  auto it1 = std::find_if(
      canvases_.begin(), canvases_.end(), [=](auto& ptr) {
        return ptr.get() == canvas1;
      });
  auto it2 = std::find_if(
      canvases_.begin(), canvases_.end(), [=](auto& ptr) {
        return ptr.get() == canvas2;
      });
  GEMINI_REQUIRE(it1 != canvases_.end(), "could not find first canvas");
  GEMINI_REQUIRE(it2 != canvases_.end(), "could not find second canvas");

  auto index1 = std::distance(canvases_.begin(), it1), index2 = std::distance(canvases_.begin(), it2);
  Relation_Fix(static_cast<int>(index1), canvas1_part, static_cast<int>(index2), canvas2_part, pixels_diff);
}

void Image::Impl::Dimensions_Fix(
    Canvas* canvas,
    CanvasDimension dim,
    double extent) {
  auto it = std::find_if(
      canvases_.begin(), canvases_.end(), [=](auto& ptr) {
        return ptr.get() == canvas;
      });
  GEMINI_REQUIRE(it != canvases_.end(), "could not find the canvas");
  auto index = std::distance(canvases_.begin(), it);

  canvas_dimensions_fixes_.emplace_back(index, dim, extent);
}

void Image::Impl::ClearRelationships() {
  canvas_fixes_.clear();
}

std::shared_ptr<Canvas> Image::Impl::GetMasterCanvas() {
  return master_canvas_;
}

const CanvasLocation& Image::Impl::GetLocation(const Canvas* canvas) const {
  if (auto it = canvas_locations_.find(canvas); it != canvas_locations_.end()) {
    return it->second;
  }
  GEMINI_FAIL("could not find specified canvas in the image");
}

int Image::Impl::GetWidth() const {
  return width_;
}

int Image::Impl::GetHeight() const {
  return height_;
}

Bitmap Image::Impl::ToBitmap() const {
  Bitmap output(width_, height_);

  if (needs_calculate_) {
    CalculateImage();
  }
  master_canvas_->writeOnBitmap(output);
  return output;
}

void Image::Impl::CalculateImage() const {
  // Determine which canvases need coordinate systems.
  CalculateCanvasCoordinates();

  // Determine the actual size and placement of all canvases.
  CalculateCanvasLocations();

  needs_calculate_ = false;
}

void Image::Impl::CalculateCanvasLocations() const {
  // Master canvas always takes up all the image space.
  canvas_locations_[master_canvas_.get()] = {0, 0, width_, height_};

  if (canvas_fixes_.empty()) {
    if (canvases_.size() == 1) {
      return;
    }
    GEMINI_FAIL("no relationships, but there are multiple canvases");
  }

  // Represents the left, bottom, right, top (in that order) of each canvas (except the master canvas).
  int dimensionality = static_cast<int>(canvas_fixes_.size()) + static_cast<int>(canvas_dimensions_fixes_.size());
  MatrixXd relationships = MatrixXd::Zero(
      dimensionality,
      4 * (static_cast<int>(canvases_.size()) - 1));
  MatrixXd constants = MatrixXd::Zero(dimensionality, 1);

  // Defines how to add entries in the relationship matrix for each given relationship.
  auto set_entry = [&constants, &relationships, this](int c, auto cpart, bool is_first, int count) {
    double mult = is_first ? 1. : -1.;
    switch (cpart) {
      case CanvasPart::Left:
        if (c == 0) {
          constants(count, 0) -= 0.; // For consistency.
        }
        else {
          relationships(count, 4 * (c - 1) + 0) = mult;
        }
        break;
      case CanvasPart::Bottom:
        if (c == 0) {
          constants(count, 0) -= 0.; // For consistency.
        }
        else {
          relationships(count, 4 * (c - 1) + 1) = mult;
        }
        break;
      case CanvasPart::Right:
        if (c == 0) {
          constants(count, 0) -= mult * width_;
        }
        else {
          relationships(count, 4 * (c - 1) + 2) = mult;
        }
        break;
      case CanvasPart::Top:
        if (c == 0) {
          constants(count, 0) -= mult * height_;
        }
        else {
          relationships(count, 4 * (c - 1) + 3) = mult;
        }
        break;
      case CanvasPart::CenterX:
        if (c == 0) {
          constants(count, 0) -= 0.5 * mult * width_;
        }
        else {
          relationships(count, 4 * (c - 1) + 2) = 0.5 * mult;
          relationships(count, 4 * (c - 1) + 0) = 0.5 * mult;
        }
        break;
      case CanvasPart::CenterY:
        if (c == 0) {
          constants(count, 0) -= 0.5 * mult * height_;
        }
        else {
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

  // Set dimensions fixes (widths or heights).
  for (const auto&[c, dim, extent]: canvas_dimensions_fixes_) {
    if (dim == CanvasDimension::Width) {
      relationships(count, 4 * (c - 1) + 0) = -1;
      relationships(count, 4 * (c - 1) + 2) = +1;
      constants(count, 0) = extent;
    }
    else {
      relationships(count, 4 * (c - 1) + 1) = -1;
      relationships(count, 4 * (c - 1) + 3) = +1;
      constants(count, 0) = extent;
    }

    ++count;
  }

  // Solve R X + b = 0 for X
  MatrixXd canvas_positions;
  try {
    canvas_positions = relationships.fullPivLu().solve(constants);
  }
  catch (const std::exception& ex) {
    GEMINI_FAIL("could not determine canvas location: " << ex.what());
  }

  for (int i = 0; i < canvas_positions.rows() / 4; ++i) {
    int base = 4 * i;
    CanvasLocation canvas_location{
        (int) canvas_positions(base + 0, 0), // left
        (int) canvas_positions(base + 1, 0), // bottom
        (int) canvas_positions(base + 2, 0), // right
        (int) canvas_positions(base + 3, 0)}; // top

    canvas_locations_[canvases_[i + 1].get()] = canvas_location;
  }
}

void Image::Impl::CalculateCanvasCoordinates() const {
  // Check which, if any, canvases need coordinate systems. If so, determine what they should be.
  for (const auto& canvas: canvases_) {
    // Get coordinates, if there are any. A lack of coordinates is signaled by quiet NaN.
    auto min_max_coords = getMinMaxCoordinates(canvas.get());
    // Determine the coordinate system that should be used for the canvas.
    describeCoordinates(canvas.get(), min_max_coords);
  }
}

const CoordinateDescription& Image::Impl::GetCanvasCoordinateDescription(const std::shared_ptr<const Canvas>& canvas) const {
  if (auto it = canvas_coordinate_description_.find(canvas.get()); it != canvas_coordinate_description_.end()) {
    return it->second;
  }
  GEMINI_FAIL("the canvas was not a member of this image");
}

void Image::Impl::RegisterCanvas(const std::shared_ptr<Canvas>& canvas) {
  if (!canvas) {
    return;
  }
  // Register this canvas in the vector.
  canvases_.push_back(canvas);
  // Create entries for the canvas.
  canvas_locations_[canvas.get()];
  canvas_coordinate_description_[canvas.get()];

  // The canvas belongs to this image.
  canvas->image_ = this;
}

std::array<double, 4> Image::Impl::getMinMaxCoordinates(class Canvas* canvas) {
  // Check if any object uses coordinates.
  double min_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double min_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  for (const auto& shape: canvas->shapes_) {
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

void Image::Impl::describeCoordinates(Canvas* canvas, const std::array<double, 4>& min_max_coords) const {
  auto&[min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y] = min_max_coords;

  // Check whether there were any coordinates in either x or y.
  if (!std::isnan(min_coordinate_x) || !std::isnan(min_coordinate_y)) {
    auto& description = canvas_coordinate_description_[canvas];
    description.has_coordinates = true;

    // If a coordinate system has been specified, use that. Otherwise, use minimum coordinate point.

    // Left coordinate.
    if (!std::isnan(canvas->coordinate_system_.left)) {
      description.left = canvas->coordinate_system_.left;
    }
    else {
      if (std::isnan(min_coordinate_x)) {
        description.left = -default_coordinate_epsilon;
      }
      else if (min_coordinate_x == max_coordinate_x) {
        description.left = min_coordinate_x - default_coordinate_epsilon;
      }
      else {
        description.left = min_coordinate_x;
      }
    }

    // Right coordinate.
    if (!std::isnan(canvas->coordinate_system_.right)) {
      description.right = canvas->coordinate_system_.right;
    }
    else {
      if (std::isnan(min_coordinate_x)) {
        description.right = default_coordinate_epsilon;
      }
      else if (min_coordinate_x == max_coordinate_x) {
        description.right = min_coordinate_x + default_coordinate_epsilon;
      }
      else {
        description.right = max_coordinate_x;
      }
    }

    // Bottom coordinate.
    if (!std::isnan(canvas->coordinate_system_.bottom)) {
      description.bottom = canvas->coordinate_system_.bottom;
    }
    else {
      if (std::isnan(min_coordinate_y)) {
        description.bottom = -default_coordinate_epsilon;
      }
      else if (min_coordinate_y == max_coordinate_y) {
        description.bottom = min_coordinate_y - default_coordinate_epsilon;
      }
      else {
        description.bottom = min_coordinate_y;
      }
    }

    // Top coordinate.
    if (!std::isnan(canvas->coordinate_system_.top)) {
      description.top = canvas->coordinate_system_.top;
    }
    else {
      if (std::isnan(min_coordinate_y)) {
        description.top = default_coordinate_epsilon;
      }
      else if (min_coordinate_y == max_coordinate_y) {
        description.top = min_coordinate_y + default_coordinate_epsilon;
      }
      else {
        description.top = max_coordinate_y;
      }
    }
  }
}

// ==========================================================================================
//  Image.
// ==========================================================================================

Image::Image()
    : impl_(std::make_shared<Impl>()) {}

Image::Image(int width, int height)
    : impl_(std::make_shared<Impl>(width, height)) {}

void Image::Relation_Fix(
    int canvas1_num,
    CanvasPart canvas1_part,
    int canvas2_num,
    CanvasPart canvas2_part,
    double pixels_diff) {
  impl_->Relation_Fix(canvas1_num, canvas1_part, canvas2_num, canvas2_part, pixels_diff);
}

void Image::Relation_Fix(
    Canvas* canvas1,
    CanvasPart canvas1_part,
    Canvas* canvas2,
    CanvasPart canvas2_part,
    double pixels_diff) {
  impl_->Relation_Fix(canvas1, canvas1_part, canvas2, canvas2_part, pixels_diff);
}

void Image::Dimensions_Fix(
    Canvas* canvas,
    CanvasDimension dim,
    double extent) {
  impl_->Dimensions_Fix(canvas, dim, extent);
}

void Image::ClearRelationships() {
  impl_->ClearRelationships();
}

std::shared_ptr<Canvas> Image::GetMasterCanvas() {
  return impl_->GetMasterCanvas();
}

const CanvasLocation& Image::GetLocation(const Canvas* canvas) const {
  return impl_->GetLocation(canvas);
}

int Image::GetWidth() const {
  return impl_->GetWidth();
}

int Image::GetHeight() const {
  return impl_->GetHeight();
}

Bitmap Image::ToBitmap() const {
  return impl_->ToBitmap();
}

void Image::CalculateImage() const {
  impl_->CalculateImage();
}

void Image::CalculateCanvasLocations() const {
  impl_->CalculateCanvasLocations();
}

void Image::CalculateCanvasCoordinates() const {
  impl_->CalculateCanvasCoordinates();
}

const CoordinateDescription& Image::GetCanvasCoordinateDescription(const std::shared_ptr<const Canvas>& canvas) const {
  return impl_->GetCanvasCoordinateDescription(canvas);
}


// ==========================================================================================
//  Canvas.
// ==========================================================================================

std::shared_ptr<Canvas> Canvas::FloatingSubCanvas() {
  auto sub_canvas = std::shared_ptr<Canvas>(new Canvas(this));
  image_->RegisterCanvas(sub_canvas);
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

void Canvas::SetCoordinates(const CanvasCoordinates& coordinates) {
  coordinate_system_ = coordinates;
}

CanvasCoordinates& Canvas::GetCoordinateSystem() {
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
      auto& description = image_->canvas_coordinate_description_.find(this)->second;
      auto proportion_x = (point.x - description.left) / (description.right - description.left);
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
      auto& description = image_->canvas_coordinate_description_.find(this)->second;
      auto proportion_y = (point.y - description.bottom) / (description.top - description.bottom);
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
      auto& description = image_->canvas_coordinate_description_.find(this)->second;
      auto proportion_x = displacement.dx / (description.right - description.left);
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
    default:GEMINI_FAIL("unrecognized LocationType");
  }

  // Handle y coordinate of the point.
  switch (displacement.type_dy) {
    case LocationType::Coordinate: {
      auto& description = image_->canvas_coordinate_description_.find(this)->second;
      auto proportion_y = displacement.dy / (description.top - description.bottom);
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

  for (const auto& shape: shapes_) {
    shape->DrawOnBitmap(image, this);
  }

  for (const auto& child: child_canvases_) {
    child->writeOnBitmap(image);
  }
}

void Canvas::paintBackground(Bitmap& image) const {
  auto& location = image_->canvas_locations_.find(this)->second;

  for (int x = location.left; x < location.right; ++x) {
    for (int y = location.bottom; y < location.top; ++y) {
      image.SetPixel(x, y, background_color_);
    }
  }
}
