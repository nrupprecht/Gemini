//
// Created by Nathaniel Rupprecht on 8/21/22.
//

#include "gemini/plot/Figure.h"
// Other files.
#include <utility>
#include <gemini/text/TextBox.h>
#include "gemini/text/TrueTypeFontEngine.h"


using namespace gemini;
using namespace gemini::plot;
using namespace gemini::core;

namespace gemini::plot {

class GlobalFontManager::Impl {
 public:
  std::shared_ptr<gemini::text::TrueType> true_type{};
  std::shared_ptr<gemini::text::TrueTypeFontEngine> engine{};
};

} // namespace gemini::plot

// Initialize the global font manager.
std::unique_ptr<GlobalFontManager::Impl> GlobalFontManager::singleton_;
int GlobalFontManager::dummy_ = GlobalFontManager::initialize();

int GlobalFontManager::initialize() {
  if (!singleton_) {
    // Create singleton object.
    singleton_ = std::unique_ptr<Impl>(new Impl());
    singleton_->true_type = std::make_shared<gemini::text::TrueType>();

    // Look for included ttf font.
    std::filesystem::path this_file = __FILE__;
    auto font_file = this_file.parent_path().parent_path().parent_path().parent_path() / "fonts" / "times.ttf";

    // Load the font engine.
    LoadFontEngine(font_file);
  }
  return 0;
}

std::shared_ptr<gemini::text::TrueTypeFontEngine> GlobalFontManager::GetFontEngine() {
  return GlobalFontManager::singleton_->engine;
}

void GlobalFontManager::LoadFontEngine(const std::filesystem::path& ttf_file) {
  if (std::filesystem::exists(ttf_file)) {
    singleton_->true_type->ReadTTF(ttf_file.string());
    singleton_->engine = std::make_shared<gemini::text::TrueTypeFontEngine>(singleton_->true_type, 20, 250);
  }
}


// ===========================================================================
//  Plot.
// ===========================================================================

Plot& Plot::AddRender(const Render& render) {
  renders_.push_back(render);

  // TODO: Here is where we could register renders with a manager that would later be used to create legends.

  return *this;
}

void Plot::SetXLabel(const std::string& label) {
  xlabel_ = label;
}

void Plot::ClearXLabel() {
  xlabel_ = {};
}

void Plot::SetYLabel(const std::string& label) {
  ylabel_ = label;
}

void Plot::ClearYLabel() {
  ylabel_ = {};
}

void Plot::addRendersToCanvas() const {
  for (auto& render: renders_) {
    GEMINI_REQUIRE(render.Validate(), "invalid Render object detected");
    render.WriteToCanvas(*plot_surface_);
  }

  // =====================================================================================
  // Add additional renders, like those that form ticks, legends, axes, labels, etc.
  // =====================================================================================

  // Check whether we need a x-axis label.
  if (xlabel_) {
    auto label = std::make_shared<text::TextBox>(GlobalFontManager::GetFontEngine());
    label->AddText(*xlabel_);
    label->SetFontSize(8);  // TODO: Make controllable.
    label->SetAnchor(gemini::Point{ 0.5, 10, LocationType::Proportional, LocationType::Pixels });

    full_canvas_->AddShape(label);
  }
  // Check whether we need a y-axis label.
  if (ylabel_) {
    auto label = std::make_shared<text::TextBox>(GlobalFontManager::GetFontEngine());
    label->AddText(*ylabel_);
    label->SetFontSize(8);
    label->SetAngle(0.5 * math::PI);
    label->SetAnchor(gemini::Point{ 25, 0.5, LocationType::Pixels, LocationType::Proportional });

    full_canvas_->AddShape(label);
  }
}

void Plot::initializeCanvases(const std::shared_ptr<core::Canvas>& canvas) {
  full_canvas_ = canvas;

  plot_surface_ = full_canvas_->FloatingSubCanvas();

  // Setting this so I can see where the plotting surfaces are.
  plot_surface_->SetBackground(color::PixelColor(240, 240, 240));

  // Set relationships between full canvas and the plotting surface.
  image_.Scale_Fix(plot_surface_.get(), CanvasPart::Left, full_canvas_.get(), CanvasDimension::X, 0.05);
  image_.Scale_Fix(plot_surface_.get(), CanvasPart::Right, full_canvas_.get(), CanvasDimension::X, 0.95);
  image_.Scale_Fix(plot_surface_.get(), CanvasPart::Bottom, full_canvas_.get(), CanvasDimension::Y, 0.05);
  image_.Scale_Fix(plot_surface_.get(), CanvasPart::Top, full_canvas_.get(), CanvasDimension::Y, 0.95);
}

void Plot::postCalculate() {

  auto& coordinates = plot_surface_->GetCoordinateSystem();

  auto width = coordinates.right - coordinates.left;
  if (!std::isnan(width)) {
    coordinates.right += 0.1 * width;
    coordinates.left -= 0.1 * width;
  }
  auto height = coordinates.top - coordinates.bottom;
  if (!std::isnan(height)) {
    coordinates.top += 0.1 * height;
    coordinates.bottom -= 0.1 * height;
  }

}

// ===========================================================================
//  FigureSpace.
// ===========================================================================

FigureSpace::FigureSpace() {
  space_ = std::make_shared<Plot>();
}

bool FigureSpace::IsFigure() const {
  return space_.index() == 1;
}

bool FigureSpace::IsPlot() const {
  return space_.index() == 0;
}

SubFigure& FigureSpace::MakeFigure() {
  auto fig = std::shared_ptr<SubFigure>(new SubFigure(image_));
  space_ = fig;
  return *fig;
}

Plot& FigureSpace::MakePlot() {
  auto plt = std::make_shared<Plot>();
  space_ = plt;
  return *plt;
}

NO_DISCARD SubFigure& FigureSpace::AsFigure() const {
  GEMINI_REQUIRE(IsFigure(), "cannot get the FigureSpace as a figure, since it not a figure");
  return *std::get<FigPtr>(space_);
}

Plot& FigureSpace::AsPlot() const {
  GEMINI_REQUIRE(IsPlot(), "cannot get the FigureSpace as a plot, since it not a plot");
  return *std::get<PlotPtr>(space_);
}

FigureSpace::FigureSpace(core::Image image)
    : image_(std::move(image))
    , space_(std::make_shared<Plot>()) {}

void FigureSpace::setImage(core::Image image) {
  image_ = std::move(image);
  // If the subspace is a figure, set the image.
  if (space_.index() == 0) {
    std::get<0>(space_)->image_ = image_;
  }
  else {
    std::get<1>(space_)->image_ = image_;
  }
}

// ===========================================================================
//  SubFigure.
// ===========================================================================


void SubFigure::SetSubSpaces(std::size_t num_x, std::size_t num_y) {
  subspaces_.clear();
  grid_x_ = num_x, grid_y_ = num_y;
  subspaces_ = std::vector<FigureSpace>(num_x * num_y);
  std::for_each(
      subspaces_.begin(), subspaces_.end(),
      [this](auto& subspace) {
        subspace = FigureSpace(image_);
      });
  x_relative_sizes_.clear();
  y_relative_sizes_.clear();
}

void SubFigure::SetSubSpaceRelativeSizes(
    const std::vector<double>& x_relative_sizes,
    const std::vector<double>& y_relative_sizes) {
  GEMINI_REQUIRE(x_relative_sizes.size() == grid_x_, ""
      << "the number of x relative sizes (" << x_relative_sizes.size()
      << ") must equal grid x (" << grid_x_ << ")");
  GEMINI_REQUIRE(y_relative_sizes.size() == grid_y_, ""
      << "the number of x relative sizes (" << y_relative_sizes.size()
      << ") must equal grid x (" << grid_y_ << ")");
  GEMINI_REQUIRE(0 <= *std::min_element(x_relative_sizes.begin(), x_relative_sizes.end()),
                 "cannot have negative relative size (x values)");
  GEMINI_REQUIRE(0 <= *std::min_element(y_relative_sizes.begin(), y_relative_sizes.end()),
                 "cannot have negative relative size (y values)");
  x_relative_sizes_ = x_relative_sizes;
  y_relative_sizes_ = y_relative_sizes;
}

const FigureSpace& SubFigure::GetSubspace(std::size_t x, std::size_t y) const {
  GEMINI_REQUIRE(x < grid_x_ && y < grid_y_, ""
      << "cannot get (" << x << ", " << y << "), out of bounds, "
      << "bounds are (" << grid_x_ << ", " << grid_y_ << ")");
  return subspaces_[y * grid_x_ + x];
}

FigureSpace& SubFigure::GetSubspace(std::size_t x, std::size_t y) {
  GEMINI_REQUIRE(x < grid_x_ && y < grid_y_, ""
      << "cannot get (" << x << ", " << y << "), out of bounds, "
      << "bounds are (" << grid_x_ << ", " << grid_y_ << ")");

  return subspaces_[y * grid_x_ + x];
}

Plot& SubFigure::GetOrMakePlot(std::size_t x, std::size_t y) {
  GEMINI_REQUIRE(x < grid_x_ && y < grid_y_, ""
      << "cannot get (" << x << ", " << y << "), out of bounds, "
      << "bounds are (" << grid_x_ << ", " << grid_y_ << ")");
  auto& entry = subspaces_[y * grid_x_ + x];
  if (entry.IsPlot()) {
    return entry.AsPlot();
  }
  // Otherwise, change the subspace into a figure, and return that.
  return entry.MakePlot();
}

SubFigure& SubFigure::GetOrMakeSubFig(std::size_t x, std::size_t y) {
  GEMINI_REQUIRE(x < grid_x_ && y < grid_y_, ""
      << "cannot get (" << x << ", " << y << "), out of bounds, "
      << "bounds are (" << grid_x_ << ", " << grid_y_ << ")");
  auto& entry = subspaces_[y * grid_x_ + x];
  if (entry.IsFigure()) {
    return entry.AsFigure();
  }
  // Otherwise, change the subspace into a figure, and return that.
  return entry.MakeFigure();
}

SubFigure::SubFigure(core::Image base_image)
    : image_(std::move(base_image))
    , subspaces_(1) {}

void SubFigure::initializeCanvases(const std::shared_ptr<core::Canvas>& canvas) {
  plotting_canvas_ = canvas;

  canvases_for_figurespaces_.clear();
  for (auto& _: subspaces_) {
    canvases_for_figurespaces_.push_back(plotting_canvas_->FloatingSubCanvas());
  }

  // Give canvases to each figure space. Create a floating sub-canvas for each figure space.
  // TODO: In the future, if we allow for the entire figure to have, e.g., a title, we will not let the set of
  //  sub-canvases cover the entirety of the image, we will need to leave some room.
  for (auto i = 0u; i < subspaces_.size(); ++i) {
    // Have each figure set up whatever it needs on its own canvas. A plot will just directly plot on the canvas for
    // that figure space.
    subspaces_[i].setImage(image_);
    if (subspaces_[i].IsFigure()) {
      subspaces_[i].AsFigure().initializeCanvases(canvases_for_figurespaces_[i]);
    }
    else {
      subspaces_[i].AsPlot().initializeCanvases(canvases_for_figurespaces_[i]);
    }
  }

  // Calculate what fraction of the total plotting canvas each subplot covers. The default is 1 / grid_x or 1 / grid_y,
  // that is, equal proportions.
  std::vector<double> canvas_fractions_x(grid_x_, 1. / grid_x_), canvas_fractions_y(grid_y_, 1. / grid_y_);
  if (!x_relative_sizes_.empty()) {
    auto x_normalization = std::accumulate(x_relative_sizes_.begin(), x_relative_sizes_.end(), 0.);
    for (auto i = 0u; i < canvas_fractions_x.size(); ++i) {
      canvas_fractions_x[i] = x_relative_sizes_[i] / x_normalization;
    }
  }
  if (!y_relative_sizes_.empty()) {
    auto y_normalization = std::accumulate(y_relative_sizes_.begin(), y_relative_sizes_.end(), 0.);
    for (auto i = 0u; i < canvas_fractions_y.size(); ++i) {
      canvas_fractions_y[i] = y_relative_sizes_[i] / y_normalization;
    }
  }

  // Create relationships between canvases.
  auto get_id = [this](int x, int y) { return y * this->grid_x_ + x; };
  for (int ix = 0; ix < grid_x_; ++ix) {
    for (int iy = 0; iy < grid_y_; ++iy) {
      // TODO... We can just set the canvas locations, we don't have to set up relationships and solve.

      auto id = get_id(ix, iy);
      auto canvas_ptr = canvases_for_figurespaces_[id].get();

      // X-relationships.

      if (ix == 0) {
        image_.Relation_Fix(
            plotting_canvas_.get(), CanvasPart::Left,
            canvas_ptr, CanvasPart::Left);
      }
      else {
        auto left = get_id(ix - 1, iy);
        image_.Relation_Fix(
            canvases_for_figurespaces_[left].get(), CanvasPart::Right,
            canvas_ptr, CanvasPart::Left);
      }
      if (ix == grid_x_ - 1) {
        image_.Relation_Fix(
            plotting_canvas_.get(), CanvasPart::Right,
            canvas_ptr, CanvasPart::Right);
      }

      // Force all the images to be the same width.
      image_.RelativeSize_Fix(canvas_ptr, CanvasDimension::X, plotting_canvas_.get(),
                              CanvasDimension::X, canvas_fractions_x[ix]);

      // Y-relationships.

      if (iy == 0) {
        image_.Relation_Fix(
            plotting_canvas_.get(), CanvasPart::Top,
            canvas_ptr, CanvasPart::Top);
      }
      else {
        auto up = get_id(ix, iy - 1);
        image_.Relation_Fix(
            canvases_for_figurespaces_[up].get(), CanvasPart::Bottom,
            canvas_ptr, CanvasPart::Top);
      }
      if (iy == grid_y_ - 1) {
        auto up = get_id(ix, iy - 1);
        image_.Relation_Fix(
            plotting_canvas_.get(), CanvasPart::Bottom,
            canvas_ptr, CanvasPart::Bottom);
      }

      // Force all the images to be the same height.
      image_.RelativeSize_Fix(canvas_ptr, CanvasDimension::Y, plotting_canvas_.get(),
                              CanvasDimension::Y, canvas_fractions_y[iy]);
    }
  }
}

void SubFigure::addRendersToCanvas() const {
  // Have each subspace add its renders to its canvases.
  for (const auto& subspace: subspaces_) {
    if (subspace.IsFigure()) {
      subspace.AsFigure().addRendersToCanvas();
    }
    else {
      subspace.AsPlot().addRendersToCanvas();
    }
  }
}

void SubFigure::postCalculate() {
  for (const auto& subspace: subspaces_) {
    if (subspace.IsFigure()) {
      subspace.AsFigure().postCalculate();
    }
    else {
      subspace.AsPlot().postCalculate();
    }
  }
}

// ===========================================================================
//  Figure.
// ===========================================================================

Figure::Figure(unsigned int width, unsigned int height)
    : SubFigure(Image(static_cast<int>(width), static_cast<int>(height)))
    , width_(width)
    , height_(height) {
  plotting_canvas_ = image_.GetMasterCanvas()->FloatingSubCanvas();
  image_.GetMasterCanvas()->SetBackground(color::PixelColor(232, 232, 232));
}

core::Bitmap Figure::ToBitmap() {
  // TODO: Check the figure needs to be rendered. If it doesn't, just return image_.ToBitmap().

  // If so, render. First, reset main image.
  image_ = core::Image(static_cast<int>(width_), static_cast<int>(height_));
  initializeCanvases(image_.GetMasterCanvas());

  // Have the figure and its subspaces recursively write themselves onto the canvas.
  addRendersToCanvas();

  // Calculate canvas coordinate systems and locations.
  image_.CalculateImage();

  // Do any steps that should be done post-calculate image, such as adjusting coordinates.
  postCalculate();

  return image_.ToBitmap();
}