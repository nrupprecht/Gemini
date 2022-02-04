//
// Created by Nathaniel Rupprecht on 11/26/21.
//

#include "gemini/plot/Plot.h"
// Other files.
#include "gemini/text/TextBox.h"

using namespace gemini;
using namespace gemini::plot;

Figure::Figure(int width, int height)
  : image_(width, height)
  , color_palette_(ColorPaletteHLS())
{
  plotting_canvas_ = image_.GetMasterCanvas()->FloatingSubCanvas();

  image_.GetMasterCanvas()->SetBackground(color::PixelColor(232, 232, 232));

  // Set up the true type font engine. TODO: Don't hard code this.
  auto true_type = std::make_shared<gemini::text::TrueType>();
  true_type->ReadTTF("/Users/nathaniel/Documents/times.ttf");
  auto engine = std::make_shared<gemini::text::TrueTypeFontEngine>(true_type, 20, 250);
}

void Figure::Plot(const std::vector<double>& x, const std::vector<double>& y, const std::string& label) {
  GEMINI_REQUIRE(x.size() == y.size(), "x and y do not have the same number of points");

  if (x.empty()) {
    return;
  }

  double line_thickness = 3.;
  auto& plot_color = color_palette_[palette_index_];

  // Plot points to cover gaps.
  auto last_point = gemini::MakeCoordinatePoint(x[0], y[0]);
  plotting_canvas_->AddShape(
      std::make_shared<gemini::Circle>(last_point,
                                       Distance{0.5 * line_thickness, LocationType::Pixels},
                                       color_palette_[palette_index_]));
  for (int i = 1; i < x.size(); ++i) {
    auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
    plotting_canvas_->AddShape(
        std::make_shared<gemini::Circle>(point,
                                         Distance{0.5 * line_thickness, LocationType::Pixels},
                                         color_palette_[palette_index_]));
    last_point = point;
  }

  // Plot lines.
  last_point = gemini::MakeCoordinatePoint(x[0], y[0]);
  for (int i = 1; i < x.size(); ++i) {
    auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
    plotting_canvas_->AddShape(
        std::make_shared<gemini::XiaolinWuThickLine>(last_point,
                                                     point,
                                                     color_palette_[palette_index_],
                                                     line_thickness));
    last_point = point;
  }

  if (!label.empty()) {
    legend_data_.push_back(LegendEntry{plot_color, label});
  }
  updateColorPalette();
}

void Figure::Scatter(const std::vector<double>& x, const std::vector<double>& y, const std::string& label) {
  GEMINI_REQUIRE(x.size() == y.size(), "x and y do not have the same number of points");

  if (x.empty()) {
    return;
  }

  // Plot points.
  auto& plot_color = color_palette_[palette_index_];
  for (int i = 1; i < x.size(); ++i) {
    auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
    plotting_canvas_->AddShape(
        std::make_shared<gemini::Circle>(point,
                                         Distance{2.5, LocationType::Pixels},
                                         plot_color));
  }

  if (!label.empty()) {
    legend_data_.push_back(LegendEntry{plot_color, label});
  }
  updateColorPalette();
}

void Figure::PlotErrorbars(const std::vector<double>& x,
                           const std::vector<double>& y,
                           const std::vector<double>& err,
                           const std::string& label) {
  GEMINI_REQUIRE(x.size() == y.size(), "x and y do not have the same number of points");
  GEMINI_REQUIRE(x.size() == err.size(), "x and y do not have the same number of as err");

  if (x.empty()) {
    return;
  }

  const double thickness = 2.;
  auto& plot_color = color_palette_[palette_index_];

  // Plot points.
  for (int i = 0; i < x.size(); ++i) {
    auto point1 = gemini::MakeCoordinatePoint(x[i], y[i] - err[i]);
    auto point2 = gemini::MakeCoordinatePoint(x[i], y[i] + err[i]);
    // Vertical error spread.
    plotting_canvas_->AddShape(
        std::make_shared<gemini::XiaolinWuThickLine>(point1,
                                                     point2,
                                                     plot_color,
                                                     thickness));

    // Top bar.
    plotting_canvas_->AddShape(
        std::make_shared<gemini::Ray>(point1,
                                      gemini::Displacement{5, 0,
                                                           LocationType::Pixels,
                                                           LocationType::Pixels},
                                      plot_color,
                                      thickness));
    plotting_canvas_->AddShape(
        std::make_shared<gemini::Ray>(point1,
                                      gemini::Displacement{-5, 0,
                                                           LocationType::Pixels,
                                                           LocationType::Pixels},
                                      plot_color,
                                      thickness));

    // Bottom bar.
    plotting_canvas_->AddShape(
        std::make_shared<gemini::Ray>(point2,
                                      gemini::Displacement{5, 0,
                                                           LocationType::Pixels,
                                                           LocationType::Pixels},
                                      plot_color,
                                      thickness));
    plotting_canvas_->AddShape(
        std::make_shared<gemini::Ray>(point2,
                                      gemini::Displacement{-5, 0,
                                                           LocationType::Pixels,
                                                           LocationType::Pixels},
                                      plot_color,
                                      thickness));
  }

  if (!label.empty()) {
    legend_data_.push_back(LegendEntry{plot_color, label});
  }
  updateColorPalette();
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

  // Check whether we need a legend.
  if (!legend_data_.empty()) {
    auto legend = plotting_canvas_->FloatingSubCanvas();
    auto master = image_.GetMasterCanvas();

    image_.Relation_Fix(plotting_canvas_, CanvasPart::Right, legend, CanvasPart::Left, 5);
    image_.Relation_Fix(legend, CanvasPart::Right, master, CanvasPart::Left, 5);

    auto text = std::make_shared<gemini::text::TextBox>(ttf_engine_);
    legend->AddShape(text);
  }
  else {
    image_.Relation_Fix(0, CanvasPart::Right, 1, CanvasPart::Right, -15);
  }

  image_.Relation_Fix(0, CanvasPart::Left, 1, CanvasPart::Left, 64);
  // image_.Relation_Fix(0, CanvasPart::Right, 1, CanvasPart::Right, -15);
  image_.Relation_Fix(0, CanvasPart::Top, 1, CanvasPart::Top, -15);
  image_.Relation_Fix(0, CanvasPart::Bottom, 1, CanvasPart::Bottom, 64);

  auto bitmap = image_.ToBitmap();
  bitmap.ToFile(filepath);
}

Image& Figure::GetImage() {
  return image_;
}

void Figure::updateColorPalette() {
  ++palette_index_;
  if (color_palette_.size() <= palette_index_) {
    palette_index_ = 0;
  }
}