//
// Created by Nathaniel Rupprecht on 11/26/21.
//

#include <gemini/core/Location.h>
#include "gemini/plot/Plot.h"
// Other files.
#include "gemini/text/TextBox.h"
#include "gemini/core/shapes/Shapes.h"
#include <filesystem>
#include <iomanip>

using namespace gemini;
using namespace gemini::core;
using namespace gemini::plot;

Figure::Figure(int width, int height)
    : image_(width, height)
    , color_palette_(ColorPaletteHLS()) {
  plotting_canvas_ = image_.GetMasterCanvas()->FloatingSubCanvas();

  image_.GetMasterCanvas()->SetBackground(color::PixelColor(232, 232, 232));

  // Set up the true type font engine. TODO: Don't hard code this.
  auto true_type = std::make_shared<gemini::text::TrueType>();
  true_type->ReadTTF("/Users/nathaniel/Documents/times.ttf");
  auto engine = std::make_shared<gemini::text::TrueTypeFontEngine>(true_type, 20, 250);
}

void Figure::Title(const std::string& title) {
  title_ = title;
}

void Figure::Plot(const std::vector<double>& x, const std::vector<double>& y, const std::string& label) {
  GEMINI_REQUIRE(x.size() == y.size(), "x and y do not have the same number of points");

  if (x.empty()) {
    return;
  }

  double line_thickness = 3.;
  auto& plot_color = color_palette_[plot_palette_index_];

  // Plot points to cover gaps.
  auto last_point = gemini::MakeCoordinatePoint(x[0], y[0]);
  plotting_canvas_->AddShape(
      std::make_shared<Circle>(
          last_point,
          Distance{0.5 * line_thickness, LocationType::Pixels},
          plot_color));
  for (int i = 1; i < x.size(); ++i) {
    auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
    plotting_canvas_->AddShape(
        std::make_shared<Circle>(
            point,
            Distance{0.5 * line_thickness, LocationType::Pixels},
            plot_color));
    last_point = point;
  }

  // Plot lines.
  last_point = gemini::MakeCoordinatePoint(x[0], y[0]);
  for (int i = 1; i < x.size(); ++i) {
    auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
    plotting_canvas_->AddShape(
        std::make_shared<XiaolinWuThickLine>(
            last_point,
            point,
            plot_color,
            line_thickness));
    last_point = point;
  }

  if (!label.empty()) {
    legend_data_.push_back(LegendEntry{plot_color, label});
  }
  updateColorPalette(plot_palette_index_);
}

void Figure::Scatter(const std::vector<double>& x, const std::vector<double>& y, const std::string& label) {
  Scatter(x, y, ScatterPlotOptions{}.Label(label));
}

void Figure::Scatter(const std::vector<double>& x, const std::vector<double>& y, const ScatterPlotOptions& options) {
  GEMINI_REQUIRE(x.size() == y.size(), "x and y do not have the same number of points");

  if (x.empty()) {
    return;
  }

  // Plot points.
  auto plot_color = options.color.value_or(color_palette_[scatter_palette_index_]);
  for (int i = 0; i < x.size(); ++i) {
    auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
    options.marker->PlaceMarker(point);
    options.marker->SetColor(plot_color);
    plotting_canvas_->AddShape(options.marker->Copy());
  }

  if (!options.label.empty()) {
    legend_data_.push_back(LegendEntry{plot_color, options.label});
  }

  // If a color was not specified, increment the color palette.
  if (!options.color.has_value()) {
    updateColorPalette(scatter_palette_index_);
  }
}

void Figure::PlotErrorbars(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& err,
    const std::string& label) {
  GEMINI_REQUIRE(x.size() == y.size(), "x and y do not have the same number of points");
  GEMINI_REQUIRE(x.size() == err.size(), "x and y do not have the same number of as err");

  if (x.empty()) {
    return;
  }

  const double thickness = 2.;
  auto& plot_color = color_palette_[error_palette_index_];

  // Plot points.
  for (int i = 0; i < x.size(); ++i) {
    auto point1 = gemini::MakeCoordinatePoint(x[i], y[i] - err[i]);
    auto point2 = gemini::MakeCoordinatePoint(x[i], y[i] + err[i]);
    // Vertical error spread.
    plotting_canvas_->AddShape(
        std::make_shared<XiaolinWuThickLine>(
            point1,
            point2,
            plot_color,
            thickness));

    // Top bar.
    plotting_canvas_->AddShape(
        std::make_shared<Ray>(
            point1,
            gemini::Displacement{
                5, 0,
                LocationType::Pixels,
                LocationType::Pixels},
            plot_color,
            thickness));
    plotting_canvas_->AddShape(
        std::make_shared<Ray>(
            point1,
            gemini::Displacement{
                -5, 0,
                LocationType::Pixels,
                LocationType::Pixels},
            plot_color,
            thickness));

    // Bottom bar.
    plotting_canvas_->AddShape(
        std::make_shared<Ray>(
            point2,
            gemini::Displacement{
                5, 0,
                LocationType::Pixels,
                LocationType::Pixels},
            plot_color,
            thickness));
    plotting_canvas_->AddShape(
        std::make_shared<Ray>(
            point2,
            gemini::Displacement{
                -5, 0,
                LocationType::Pixels,
                LocationType::Pixels},
            plot_color,
            thickness));
  }

  if (!label.empty()) {
    legend_data_.push_back(LegendEntry{plot_color, label});
  }
  updateColorPalette(error_palette_index_);
}

void Figure::SetXRange(double xmin, double xmax) {
  plotting_canvas_->GetCoordinateSystem().left = xmin;
  plotting_canvas_->GetCoordinateSystem().right = xmax;
}

void Figure::SetYRange(double ymin, double ymax) {
  plotting_canvas_->GetCoordinateSystem().bottom = ymin;
  plotting_canvas_->GetCoordinateSystem().top = ymax;
}

void Figure::ToFile(const std::string& filepath) {
  // TODO: How to handle this?
  std::filesystem::path this_file_path(__FILE__);
  auto gemini_directory = this_file_path.parent_path().parent_path().parent_path().parent_path();
  auto true_type = std::make_shared<gemini::text::TrueType>();
  true_type->ReadTTF(gemini_directory / "fonts" / "times.ttf");
  ttf_engine_ = std::make_shared<gemini::text::TrueTypeFontEngine>(true_type, 20, 250);


  // Left vertical
  plotting_canvas_->AddShape(
      std::make_shared<XiaolinWuThickLine>(
          MakeRelativePoint(0, 0),
          MakeRelativePoint(0, 1),
          color::Black));

  // Right vertical
  plotting_canvas_->AddShape(
      std::make_shared<XiaolinWuThickLine>(
          MakeRelativePoint(1, 0),
          MakeRelativePoint(1, 1),
          color::Black));

  // Bottom horizontal
  plotting_canvas_->AddShape(
      std::make_shared<XiaolinWuThickLine>(
          MakeRelativePoint(0, 0),
          MakeRelativePoint(1, 0),
          color::Black));

  // Top horizontal
  plotting_canvas_->AddShape(
      std::make_shared<XiaolinWuThickLine>(
          MakeRelativePoint(0, 1),
          MakeRelativePoint(1, 1),
          color::Black));

  // ==================================================================
  // Set up relationships
  // ==================================================================

  image_.ClearRelationships();

  // Calculate coordinates.
  image_.CalculateCanvasCoordinates();

  // Check whether we need a legend.
  if (!legend_data_.empty()) {
    auto legend = plotting_canvas_->FloatingSubCanvas();
    auto master = image_.GetMasterCanvas();

    image_.Relation_Fix(plotting_canvas_, CanvasPart::Right, legend, CanvasPart::Left, 5);
    image_.Relation_Fix(legend, CanvasPart::Right, master, CanvasPart::Left, 5);
    image_.Dimensions_Fix(legend, CanvasDimension::Width, 50);
    image_.Dimensions_Fix(legend, CanvasDimension::Height, 50);

    auto text = std::make_shared<gemini::text::TextBox>(ttf_engine_);
    legend->AddShape(text);
  }
  else {
    image_.Relation_Fix(0, CanvasPart::Right, 1, CanvasPart::Right, -15);
  }

  // Check whether we need a title.
  if (!title_.empty()) {
    image_.Relation_Fix(0, CanvasPart::Top, 1, CanvasPart::Top, -60);

    // Title text box.
    auto label = std::make_shared<text::TextBox>(ttf_engine_);
    plotting_canvas_->AddShape(label);
    label->AddText(title_);
    label->SetZOrder(10);
    label->SetAnchor(gemini::Point{0.5, 1.015, LocationType::Proportional, LocationType::Proportional});
    label->SetFontSize(15);
  }
  else {
    image_.Relation_Fix(0, CanvasPart::Top, 1, CanvasPart::Top, -15);
  }

  image_.Relation_Fix(0, CanvasPart::Left, 1, CanvasPart::Left, 64);
  // image_.Relation_Fix(0, CanvasPart::Right, 1, CanvasPart::Right, -15);

  image_.Relation_Fix(0, CanvasPart::Bottom, 1, CanvasPart::Bottom, 64);

  // Create any plot axis ticks and numbering.
  auto coordinates = image_.GetCanvasCoordinateDescription(plotting_canvas_);

  // Determine the length, in pixels, of the x and y ticks so that they are the same size.
  double tick_length = std::max(5, static_cast<int>(0.01 * std::min(image_.GetWidth(), image_.GetHeight())));



  // TODO: Improve tick positioning choices.



  // Determine where to make x - ticks.
  if (!std::isnan(coordinates.left)) {
    int target_num_ticks = 1;
    auto logscale = static_cast<int>(std::floor(std::log10(coordinates.right - coordinates.left)));
    auto scale = std::pow(10, logscale);
    auto dtick = scale / target_num_ticks;
    auto left_coord = std::floor(coordinates.left / dtick) * dtick;

    double coord = left_coord;
    while (coord <= coordinates.right) {
      if (coord < coordinates.left) {
        coord += dtick;
        continue;
      }

      // Add the tick.
      auto tick = std::make_shared<gemini::core::Ray>(
          gemini::Point{coord, 0, LocationType::Coordinate, LocationType::Proportional},
          Displacement{0, tick_length, LocationType::Pixels, LocationType::Pixels},
          color::Black,
          2);
      tick->SetRestricted(false);
      plotting_canvas_->AddShape(tick);

      // Add the number.
      auto label = std::make_shared<text::TextBox>(ttf_engine_);
      plotting_canvas_->AddShape(label);
      std::stringstream text;
      text << std::fixed << std::setprecision(logscale + 1) << coord;
      label->AddText(text.str());
      label->SetAnchor(gemini::Point{coord, -40, LocationType::Coordinate, LocationType::Pixels});
      label->SetFontSize(6);
      label->SetAngle(0.5 * math::PI);

      coord += dtick;
    }
  }
  // Determine where to make y - ticks.
  if (!std::isnan(coordinates.top)) {

    int target_num_ticks = 1;
    auto logscale = static_cast<int>(std::floor(std::log10(coordinates.top - coordinates.bottom)));
    auto scale = std::pow(10, logscale);
    auto dtick = scale / target_num_ticks;
    auto bottom_coord = std::floor(coordinates.bottom / dtick) * dtick;

    double coord = bottom_coord;
    while (coord <= coordinates.top) {
      if (coord < coordinates.bottom) {
        coord += dtick;
        continue;
      }

      // Add the tick.
      auto tick = std::make_shared<gemini::core::Ray>(
          gemini::Point{0, coord, LocationType::Proportional, LocationType::Coordinate},
          Displacement{tick_length, 0., LocationType::Pixels, LocationType::Pixels},
          color::Black,
          2);
      tick->SetRestricted(false);
      plotting_canvas_->AddShape(tick);

      // Add the number.
      auto label = std::make_shared<text::TextBox>(ttf_engine_);
      plotting_canvas_->AddShape(label);
      std::stringstream text;
      text << std::fixed << std::setprecision(logscale + 1) << coord;
      label->AddText(text.str());
      label->SetAnchor(gemini::Point{-40, coord, LocationType::Pixels, LocationType::Coordinate});
      label->SetFontSize(6);
      label->SetAngle(0.);

      coord += dtick;
    }
  }

  auto bitmap = image_.ToBitmap();
  bitmap.ToFile(filepath);
}

Image& Figure::GetImage() {
  return image_;
}

void Figure::updateColorPalette(int& index) {
  ++index;
  if (color_palette_.size() <= index) {
    index = 0;
  }
}