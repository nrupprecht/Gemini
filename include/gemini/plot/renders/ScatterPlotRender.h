//
// Created by Nathaniel Rupprecht on 10/22/22.
//

#pragma once

#include "gemini/plot/Render.h"

#include <utility>
// Other files.
#include "gemini/core/shapes/Shapes.h"

namespace gemini::plot::renders {


class ScatterPlotRender : public Render {
 public:

  class Impl : public Render::Impl {
   public:
    NO_DISCARD bool Validate() const override {
      // We will choose a default (circle) plot marker if the plot marker is null.
      return x.size() == y.size() && !x.empty();
    }

    void RegisterWithManager(Manager& manager) override {}

    void WriteToCanvas(core::Canvas& plotting_canvas) const override {
      if (x.empty()) {
        return;
      }

      // Plot points.
      core::color::PixelColor plot_color = core::color::Red; // color_palette_[plot_palette_index_];
      if (color) {
        plot_color = *color;
      }

      if (!plot_marker) {
        plot_marker = std::make_shared<marker::Circle>();
        plot_marker->SetScale(10.);
      }
      plot_marker->SetColor(plot_color);

      for (int i = 0; i < x.size(); ++i) {
        auto point = gemini::MakeCoordinatePoint(x[i], y[i]);
        plot_marker->PlaceMarker(point);
        plotting_canvas.AddShape(plot_marker->Copy());
      }

//      if (!options.label.empty()) {
//        auto marker = plot_marker->Copy();
//        marker->SetColor(plot_color);
//        // legend_data_.push_back(LegendEntry{std::move(marker), options.label});
//      }

      // If a color was not specified, increment the color palette.
//      if (!options.color.has_value()) {
//        updateColorPalette(scatter_palette_index_);
//      }
    }

    NO_DISCARD std::shared_ptr<Render::Impl> Copy() const override {
      auto ptr = std::make_shared<Impl>();
      *ptr = *this;
      return ptr;
    }

    std::vector<double> x{}, y{};

    std::optional<core::color::PixelColor> color{};

    std::optional<std::string> label;

    mutable std::shared_ptr<marker::Marker> plot_marker{};
  };

  // =====================================================================
  //  Options.
  // =====================================================================
  ScatterPlotRender& Values(const std::vector<double>& x, const std::vector<double>& y) {
    impl<ScatterPlotRender>()->x = x;
    impl<ScatterPlotRender>()->y = y;
    return *this;
  }

  ScatterPlotRender& XValues(const std::vector<double>& x) {
    impl<ScatterPlotRender>()->x = x;
    return *this;
  }

  ScatterPlotRender& YValues(const std::vector<double>& y) {
    impl<ScatterPlotRender>()->y = y;
    return *this;
  }

  ScatterPlotRender& Label(const std::string& label) {
    impl<ScatterPlotRender>()->label = label;
    return *this;
  }

  ScatterPlotRender& Markers(const marker::Marker& marker) {
    impl<ScatterPlotRender>()->plot_marker = marker.Copy();
    return *this;
  }

  ScatterPlotRender& MarkerSize(unsigned int size) {
    GEMINI_FAIL("ScatterPlotRender::MarkerSize not yet implemented");
    return *this;
  }

  ScatterPlotRender& Color(const core::color::PixelColor& color) {
    impl<ScatterPlotRender>()->color = color;
    return *this;
  }

  ScatterPlotRender() : Render(std::make_shared<Impl>()) {}
};

} // namespace gemini::plot
