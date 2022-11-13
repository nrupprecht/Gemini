//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#include "gemini/core/Canvas.h"
// Other files.
#include <numeric>
#include <iomanip>

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

  void Scale_Fix(
      Canvas* canvas1,
      CanvasPart canvas1_part,
      Canvas* canvas2,
      CanvasDimension dimension,
      double lambda);

  void Dimensions_Fix(
      Canvas* canvas,
      CanvasDimension dim,
      double extent);

  void RelativeSize_Fix(
      Canvas* canvas1,
      CanvasDimension dimension1,
      Canvas* canvas2,
      CanvasDimension dimension2,
      double scale);

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

  Image MakeImage() const;

 protected:
  std::size_t getCanvasIndex(Canvas* canvas);

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

  std::vector<std::shared_ptr<Fix>> fixes_;

  //! \brief What coordinate system width to use if there is no coordinate extent in the x or y dimensions.
  //! This can happen either because there is only one point that needs coordinates in x or y, or because, e.g. the
  //! x dimension has coordinate objects, but the y dimension does not. So a coordinate system is still used in the y
  //! dimension [NOTE(Nate): separate the ability to have coordinate systems for x and y?].
  double default_coordinate_epsilon = 0.0001;

  //! \brief Determines the sizes and locations of each canvas.
  mutable std::map<const Canvas*, CanvasLocation> canvas_locations_;

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

  fixes_.push_back(
      std::make_shared<FixRelationship>(
          canvas1_num,
          canvas2_num,
          canvas1_part,
          canvas2_part,
          pixels_diff));
}

void Image::Impl::Relation_Fix(
    Canvas* canvas1,
    CanvasPart canvas1_part,
    Canvas* canvas2,
    CanvasPart canvas2_part,
    double pixels_diff) {
  auto index1 = static_cast<int>(getCanvasIndex(canvas1));
  auto index2 = static_cast<int>(getCanvasIndex(canvas2));
  Relation_Fix(index1, canvas1_part, index2, canvas2_part, pixels_diff);
}

void Image::Impl::Scale_Fix(
    Canvas* canvas1,
    CanvasPart canvas1_part,
    Canvas* canvas2,
    CanvasDimension dimension,
    double lambda) {
  auto index1 = static_cast<int>(getCanvasIndex(canvas1));
  auto index2 = static_cast<int>(getCanvasIndex(canvas2));
  fixes_.push_back(std::make_shared<FixScale>(index1, index2, canvas1_part, dimension, lambda));
}

void Image::Impl::Dimensions_Fix(
    Canvas* canvas,
    CanvasDimension dim,
    double extent) {
  auto index = getCanvasIndex(canvas);
  fixes_.push_back(std::make_shared<FixDimensions>(index, dim, extent));
}

void Image::Impl::RelativeSize_Fix(
    Canvas* canvas1,
    CanvasDimension dimension1,
    Canvas* canvas2,
    CanvasDimension dimension2,
    double scale) {
  auto index1 = static_cast<int>(getCanvasIndex(canvas1));
  auto index2 = static_cast<int>(getCanvasIndex(canvas2));
  fixes_.push_back(std::make_shared<FixRelativeSize>(index1, index2, dimension1, dimension2, scale));
}

void Image::Impl::ClearRelationships() {
  fixes_.clear();
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
  canvas_locations_[master_canvas_.get()] = { 0, 0, width_, height_ };

  if (fixes_.empty()) {
    if (canvases_.size() == 1) {
      return;
    }
    GEMINI_FAIL("no relationships, but there are multiple canvases");
  }

  // Represents the left, bottom, right, top (in that order) of each canvas (except the master canvas).
  int dimensionality = 4 + static_cast<int>(fixes_.size());
  // Four columns for each canvas.
  int num_columns = 4 * static_cast<int>(canvases_.size());
  MatrixXd relationships = MatrixXd::Zero(dimensionality, num_columns);
  MatrixXd constants = MatrixXd::Zero(dimensionality, 1);

  // Add the master canvas constraints. The master canvas should cover the entire image.
  relationships(0, 0) = 1.; // Left
  relationships(1, 1) = 1.; // Bottom
  relationships(2, 2) = 1.; // Right
  relationships(3, 3) = 1.; // Top
  constants(0, 0) = 0.;
  constants(1, 0) = 0.;
  constants(2, 0) = width_;
  constants(3, 0) = height_;

  for (auto i = 0l ; i < fixes_.size() ; ++i) {
    fixes_[i]->Create(i + 4, relationships, constants);
  }

  // Solve R X + b = 0 for X
  MatrixXd canvas_positions;
  try {
    canvas_positions = relationships.fullPivLu().solve(constants);
  }
  catch (const std::exception& ex) {
    GEMINI_FAIL("could not determine canvas location: " << ex.what());
  }

  auto abs_sum = [](const Eigen::MatrixXd& v) {
    double x = 0;
    for (auto i = 0; i < v.rows(); ++i) x += std::abs(v(i, 0));
    return x;
  };

  static std::string pos_names[] = {"Left", "Bottom", "Right", "Top"};

  auto check = relationships * canvas_positions;
  for (auto i = 0; i < check.rows(); ++i) {
    auto product = check(i, 0), expected = constants(i, 0);
    bool constraint_failed = 1.e-4 < std::abs(product - expected);
    if (constraint_failed) {
      std::cout << "Failed to satisfy constraint # " << std::setw(3) << i << " : ";
    }
    else {
      std::cout << "Satisfied constraint # " << std::setw(3) << i << "         : ";
    }

    for (auto j = 0; j < canvases_.size(); ++j) {
      for (auto k = 0 ; k < 4 ; ++k) {
        auto c = relationships(i, 4 * j + k);
        if (1.e-4 < std::abs(c)) {
          std::cout << " + ( ";
          if (c != 1) {
            std::cout << c << " x ";
          }
          std::cout << pos_names[k] << "[" << j << "] )";
        }
      }
    }
    std::cout << " = " << expected;
    std::cout << ", Actually = " << product;
    if (i < 4) {
      std::cout << ", Fix is auto generated";
    }
    else {
      std::cout << ", Fix type is '" << fixes_[i - 4]->Name() << "'";
    }
    std::cout << std::endl;

    if (i < 4) {
      // pass
    }
    if (constraint_failed) {
//      MatrixXd test = MatrixXd::Zero(1, num_columns);
//      MatrixXd test_constants = MatrixXd::Zero(1, 1);
//      fixes_[i - 4]->Create(0, test, test_constants);
//
//      std::cout << "Result of test = " << test << std::endl;
    }

  }

  for (auto i = 0; i < canvases_.size(); ++i) {
    auto test_constraint = canvas_positions;
    // Check whether left is constrained.
    test_constraint(4 * i, 0) += 0.1;
    auto residual = abs_sum(relationships * test_constraint - constants) / 0.1;
    if (residual < 1.e-6) {
      std::cout << "Unconstrained left";
    }

    // Check whether the width is constrained.
    test_constraint(4 * i + 1, 0) += 0.1;
    residual = abs_sum(relationships * test_constraint - constants) / 0.1;
    if (residual < 1.e-6) {
      std::cout << "Unconstrained right";
    }

    // Reset left/right.
    test_constraint(4 * i, 0) -= 0.1, test_constraint(4 * i + 1, 0) -= 0.1;

    // Check whether the bottom is constrained.
    test_constraint(4 * i + 2, 0) += 0.1;
    residual = abs_sum(relationships * test_constraint - constants) / 0.1;
    if (residual < 1.e-6) {
      std::cout << "Unconstrained bottom";
    }

    // Check whether the height is constrained.
    test_constraint(4 * i + 3, 0) += 0.1;
    residual = abs_sum(relationships * test_constraint - constants) / 0.1;
    if (residual < 1.e-6) {
      std::cout << "Unconstrained height";
    }
  }

  for (int i = 0 ; i < num_columns / 4 ; ++i) {
    int base = 4 * i;
    CanvasLocation canvas_location{
        static_cast<int>(canvas_positions(base + 0, 0)), // left
        static_cast<int>(canvas_positions(base + 1, 0)), // bottom
        static_cast<int>(canvas_positions(base + 2, 0)), // right
        static_cast<int>(canvas_positions(base + 3, 0))  // top
    };
    canvas_locations_[canvases_[i].get()] = canvas_location;

    std::cout << "Canvas # " << std::setw(3) << i << " location: " << canvas_location << std::endl;
  }

  // Make sure that the master canvas's location was determined correctly.
  GEMINI_ASSERT((canvas_locations_[master_canvas_.get()] == CanvasLocation{ 0, 0, width_, height_ }),
                "master canvas positioned incorrectly");
}

void Image::Impl::CalculateCanvasCoordinates() const {
  // Check which, if any, canvases need coordinate systems. If so, determine what they should be.
  for (const auto& canvas : canvases_) {
    // Get coordinates, if there are any. A lack of coordinates is signaled by quiet NaN.
    auto min_max_coords = getMinMaxCoordinates(canvas.get());
    // Determine the coordinate system that should be used for the canvas.
    describeCoordinates(canvas.get(), min_max_coords);
  }
}

void Image::Impl::RegisterCanvas(const std::shared_ptr<Canvas>& canvas) {
  if (!canvas) {
    return;
  }
  // Register this canvas in the vector.
  canvases_.push_back(canvas);
  // Create entries for the canvas.
  canvas_locations_[canvas.get()];

  // The canvas belongs to this image.
  canvas->image_ = this;
}

std::size_t Image::Impl::getCanvasIndex(Canvas* canvas) {
  auto it = std::find_if(
      canvases_.begin(), canvases_.end(), [=](auto& ptr) {
        return ptr.get() == canvas;
      });
  GEMINI_REQUIRE(it != canvases_.end(), "could not find the canvas");
  return std::distance(canvases_.begin(), it);
}

std::array<double, 4> Image::Impl::getMinMaxCoordinates(class Canvas* canvas) {
  // Check if any object uses coordinates.
  double min_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double min_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  for (const auto& shape : canvas->shapes_) {
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

  return { min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y };
}

void Image::Impl::describeCoordinates(Canvas* canvas, const std::array<double, 4>& min_max_coords) const {
  auto&[min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y] = min_max_coords;

  // Check whether there were any coordinates in either x or y.
  if (!std::isnan(min_coordinate_x) || !std::isnan(min_coordinate_y)) {
    auto& description = canvas->coordinate_system_;
    description.has_coordinates = true;

    // If a coordinate system has been specified, use that. Otherwise, use minimum coordinate point.

    // Left coordinate.
    if (std::isnan(canvas->coordinate_system_.left)) {
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
    if (std::isnan(canvas->coordinate_system_.right)) {
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
    if (std::isnan(canvas->coordinate_system_.bottom)) {
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
    if (std::isnan(canvas->coordinate_system_.top)) {
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

void Image::Scale_Fix(
    Canvas* canvas1,
    CanvasPart canvas1_part,
    Canvas* canvas2,
    CanvasDimension dimension,
    double lambda) {
  impl_->Scale_Fix(canvas1, canvas1_part, canvas2, dimension, lambda);
}

void Image::Dimensions_Fix(
    Canvas* canvas,
    CanvasDimension dim,
    double extent) {
  impl_->Dimensions_Fix(canvas, dim, extent);
}

void Image::RelativeSize_Fix(
    Canvas* canvas1,
    CanvasDimension dimension1,
    Canvas* canvas2,
    CanvasDimension dimension2,
    double scale) {
  impl_->RelativeSize_Fix(canvas1, dimension1, canvas2, dimension2, scale);
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

//const CoordinateDescription& Image::GetCanvasCoordinateDescription(const std::shared_ptr<const Canvas>& canvas) const {
//  return impl_->GetCanvasCoordinateDescription(canvas);
//}


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
