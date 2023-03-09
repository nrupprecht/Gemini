//
// Created by Nathaniel Rupprecht on 11/23/22.
//

#include "gemini/core/Image.h"
// Other files.
#include "gemini/core/Canvas.h"
#include <sstream>
#include <iomanip>

using namespace gemini;
using namespace gemini::core;
using Eigen::MatrixXd;

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

std::shared_ptr<Fix> Image::Impl::Relation_Fix(
    Locatable* canvas1,
    CanvasPart canvas1_part,
    Locatable* canvas2,
    CanvasPart canvas2_part,
    double pixels_diff) {
  return AddFix(std::make_shared<FixRelationship>(canvas1, canvas2, canvas1_part, canvas2_part, pixels_diff));
}

std::shared_ptr<Fix> Image::Impl::Scale_Fix(
    Locatable* canvas1,
    CanvasPart canvas1_part,
    Locatable* canvas2,
    CanvasDimension dimension,
    double lambda) {
  return AddFix(std::make_shared<FixScale>(canvas1, canvas2, canvas1_part, dimension, lambda));
}

std::shared_ptr<Fix> Image::Impl::Dimensions_Fix(
    Locatable* canvas,
    CanvasDimension dim,
    double extent) {
  return AddFix(std::make_shared<FixDimensions>(canvas, dim, extent));
}

std::shared_ptr<Fix> Image::Impl::RelativeSize_Fix(
    Locatable* canvas1,
    CanvasDimension dimension1,
    Locatable* canvas2,
    CanvasDimension dimension2,
    double scale) {
  return AddFix(std::make_shared<FixRelativeSize>(canvas1, canvas2, dimension1, dimension2, scale));
}

std::shared_ptr<Fix> Image::Impl::AddFix(const std::shared_ptr<Fix>& fix) {
  fixes_.push_back(fix);
  return fixes_.back();
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

std::optional<std::size_t> Image::Impl::RegisterLocatable(Locatable* locatable) {
  return locatables_.Add(locatable);
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

  // Count how many locatables need relationships for their width or height.
  int additional_fixes = 0;
  for (auto& loc : locatables_.objs_) {
    if (loc->GetWidth()) ++additional_fixes;
    if (loc->GetHeight()) ++additional_fixes;
  }

  // Represents the left, bottom, right, top (in that order) of each canvas (except the master canvas).
  int dimensionality = 4 + additional_fixes + static_cast<int>(fixes_.size());
  // Four columns for each canvas.
  int num_columns = 4 * static_cast<int>(locatables_.Size());
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

  // Add additional fixes for locatables with widths or heights.
  std::vector<std::string> additional_descriptions;
  int count = 0, count_loc = 0;
  for (auto& loc : locatables_.objs_) {
    if (auto width = loc->GetWidth(); width) {
      FixDimensions fix(loc, CanvasDimension::X, *width);
      fix.Create(4 + count, relationships, constants, locatables_);
      additional_descriptions.emplace_back("Implicitly generated width for locatable " + std::to_string(count_loc));
      ++count;
    }
    if (auto height = loc->GetHeight(); height) {
      FixDimensions fix(loc, CanvasDimension::Y, *height);
      fix.Create(4 + count, relationships, constants, locatables_);
      additional_descriptions.emplace_back("Implicitly generated height for locatable " + std::to_string(count_loc));
      ++count;
    }
    ++count_loc;
  }

  for (auto i = 0l ; i < fixes_.size() ; ++i) {
    fixes_[i]->Create(4 + additional_fixes + i, relationships, constants, locatables_);
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
    for (auto i = 0 ; i < v.rows() ; ++i) { x += std::abs(v(i, 0)); }
    return x;
  };

  static std::string pos_names[] = { "Left", "Bottom", "Right", "Top" };

  auto check = relationships * canvas_positions;
  for (auto i = 0 ; i < check.rows() ; ++i) {
    auto product = check(i, 0), expected = constants(i, 0);
    bool constraint_failed = 1.e-4 < std::abs(product - expected);
    if (constraint_failed) {
      std::cout << "Failed to satisfy constraint # " << std::setw(3) << i << " : ";
    }
    else {
      std::cout << "Satisfied constraint # " << std::setw(3) << i << "         : ";
    }

    for (auto j = 0 ; j < locatables_.Size() ; ++j) {
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
    // Description of main canvas constraints.
    if (i < 4) {
      std::cout << ", Fix is auto generated";
    }
    // Description of implicitly generated width/height constraints.
    else if (i < 4 + additional_fixes) {
      std::cout << ", " << additional_descriptions[i - 4];
    }
    // Description of normal constraints.
    else {
      auto idx = i - 4 - additional_fixes;
      std::cout << ", Fix type is '" << fixes_[idx]->Name() << "'";
      if (!fixes_[idx]->description.empty()) {
        std::cout << ", Description: \"" << fixes_[idx]->description << "\"";
      }
    }

    std::cout << std::endl;
  }

  for (auto i = 0 ; i < canvases_.size() ; ++i) {
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

  std::cout << std::endl;

  for (int i = 0 ; i < num_columns / 4 ; ++i) {
    int base = 4 * i;
    CanvasLocation canvas_location{
        static_cast<int>(canvas_positions(base + 0, 0)), // left
        static_cast<int>(canvas_positions(base + 1, 0)), // bottom
        static_cast<int>(canvas_positions(base + 2, 0)), // right
        static_cast<int>(canvas_positions(base + 3, 0))  // top
    };
    locatables_.SetLocation(i, canvas_location);
    canvas_locations_[canvases_[i].get()] = canvas_location;

    std::cout << "Locatable # " << std::setw(3) << i << " location: " << canvas_location << std::endl;
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
  // Register this canvas as a locatable object within the image.
  locatables_.Add(canvas.get());
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
  auto& [min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y] = min_max_coords;

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

std::shared_ptr<Fix> Image::Relation_Fix(
    Locatable* canvas1,
    CanvasPart canvas1_part,
    Locatable* canvas2,
    CanvasPart canvas2_part,
    double pixels_diff) {
  return impl_->Relation_Fix(canvas1, canvas1_part, canvas2, canvas2_part, pixels_diff);
}

std::shared_ptr<Fix> Image::Scale_Fix(
    Locatable* canvas1,
    CanvasPart canvas1_part,
    Locatable* canvas2,
    CanvasDimension dimension,
    double lambda) {
  return impl_->Scale_Fix(canvas1, canvas1_part, canvas2, dimension, lambda);
}

std::shared_ptr<Fix> Image::Dimensions_Fix(
    Locatable* canvas,
    CanvasDimension dim,
    double extent) {
  return impl_->Dimensions_Fix(canvas, dim, extent);
}

std::shared_ptr<Fix> Image::RelativeSize_Fix(
    Locatable* canvas1,
    CanvasDimension dimension1,
    Locatable* canvas2,
    CanvasDimension dimension2,
    double scale) {
  return impl_->RelativeSize_Fix(canvas1, dimension1, canvas2, dimension2, scale);
}

void Image::AddFix(const std::shared_ptr<Fix>& fix) {
  impl_->AddFix(fix);
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

std::optional<std::size_t> Image::RegisterLocatable(Locatable* locatable) {
  return impl_->RegisterLocatable(locatable);
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