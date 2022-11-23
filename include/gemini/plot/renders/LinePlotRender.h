//
// Created by Nathaniel Rupprecht on 9/17/22.
//

#pragma once

#include "gemini/plot/Render.h"
// Other files.
#include "gemini/core/shapes/Shapes.h"

namespace gemini::plot::renders {


class LinePlotRender : public Render {
 public:

  class Impl : public Render::Impl {
   public:
    NO_DISCARD bool Validate() const override {
      return x.size() == y.size() && !x.empty();
    }

    void RegisterWithManager(Manager& manager) override {}

    void WriteToCanvas(core::Canvas& plotting_canvas) const override {
      if (x.empty()) {
        return;
      }

      double line_thickness = 3.;
      core::color::PixelColor plot_color = core::color::Red; // color_palette_[plot_palette_index_];
      if (color) {
        plot_color = *color;
      }

      // Plot points to cover gaps.
      auto last_point = gemini::MakeCoordinatePoint(x[0], y[0]);
      plotting_canvas.AddShape(
          std::make_shared<gemini::core::Circle>(
              last_point,
              Distance{0.5 * line_thickness, LocationType::Pixels},
              plot_color));
      for (int i = 1; i < x.size(); ++i) {
        auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
        plotting_canvas.AddShape(
            std::make_shared<gemini::core::Circle>(
                point,
                Distance{0.5 * line_thickness, LocationType::Pixels},
                plot_color));
        last_point = point;
      }

      // Plot lines.
      last_point = gemini::MakeCoordinatePoint(x[0], y[0]);
      for (int i = 1; i < x.size(); ++i) {
        auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
        plotting_canvas.AddShape(
            std::make_shared<gemini::core::XiaolinWuThickLine>(
                last_point,
                point,
                plot_color,
                line_thickness));
        last_point = point;
      }
    }

    NO_DISCARD std::shared_ptr<Render::Impl> Copy() const override {
      auto ptr = std::make_shared<Impl>();
      *ptr = *this;
      return ptr;
    }

    std::vector<double> x{}, y{};

    std::optional<core::color::PixelColor> color{};

    std::shared_ptr<marker::Marker> plot_marker{};
  };

  // =====================================================================
  //  Options.
  // =====================================================================
  LinePlotRender& XValues(const std::vector<double>& x) { impl<LinePlotRender>()->x = x; return *this; }
  LinePlotRender& YValues(const std::vector<double>& y) { impl<LinePlotRender>()->y = y; return *this; }
  LinePlotRender& Label(const std::string& label) { return *this; }
  LinePlotRender& LineStyle(const std::string& style) { return *this; }
  LinePlotRender& LineWidth(unsigned int width) { return *this; }
  LinePlotRender& Markers(std::shared_ptr<marker::Marker> marker) {
    impl<LinePlotRender>()->plot_marker = std::move(marker);
    return *this;
  }
  LinePlotRender& MarkerSize(unsigned int size) { return *this; }

  LinePlotRender& Color(const core::color::PixelColor& color) {
    impl<LinePlotRender>()->color = color;
    return *this;
  }

  LinePlotRender() : Render(std::make_shared<Impl>()) {}
};

} // namespace gemini::plot
