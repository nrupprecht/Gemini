//
// Created by Nathaniel Rupprecht on 10/29/22.
//

#pragma once

#include "gemini/plot/Render.h"

#include <utility>
// Other files.
#include "gemini/core/shapes/Shapes.h"

namespace gemini::plot::renders {


class ErrorBarsRender : public Render {
 public:

  class Impl : public Render::Impl {
   public:
    NO_DISCARD bool Validate() const override {
      // We will choose a default (circle) plot marker if the plot marker is null.
      return x.size() == y.size() && y.size() == yerr.size() && !x.empty();
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

      const double thickness = 2.;

      for (int i = 0 ; i < x.size() ; ++i) {
        auto point1 = gemini::MakeCoordinatePoint(x[i], y[i] - yerr[i]);
        auto point2 = gemini::MakeCoordinatePoint(x[i], y[i] + yerr[i]);
        // Vertical error spread.
        plotting_canvas.AddShape(
            std::make_shared<core::XiaolinWuThickLine>(
                point1,
                point2,
                plot_color,
                thickness));

        // Top bar.
        plotting_canvas.AddShape(
            std::make_shared<core::Ray>(
                point1,
                gemini::Displacement{
                    5, 0,
                    LocationType::Pixels,
                    LocationType::Pixels },
                plot_color,
                thickness));
        plotting_canvas.AddShape(
            std::make_shared<core::Ray>(
                point1,
                gemini::Displacement{
                    -5, 0,
                    LocationType::Pixels,
                    LocationType::Pixels },
                plot_color,
                thickness));

        // Bottom bar.
        plotting_canvas.AddShape(
            std::make_shared<core::Ray>(
                point2,
                gemini::Displacement{
                    5, 0,
                    LocationType::Pixels,
                    LocationType::Pixels },
                plot_color,
                thickness));
        plotting_canvas.AddShape(
            std::make_shared<core::Ray>(
                point2,
                gemini::Displacement{
                    -5, 0,
                    LocationType::Pixels,
                    LocationType::Pixels },
                plot_color,
                thickness));
      }
    }

    NO_DISCARD std::shared_ptr<Render::Impl> Copy() const override {
      auto ptr = std::make_shared<Impl>();
      *ptr = *this;
      return ptr;
    }

    std::vector<double> x{}, y{}, yerr{};

    std::optional<core::color::PixelColor> color{};

    mutable std::shared_ptr<marker::Marker> plot_marker{};
  };

  // =====================================================================
  //  Options.
  // =====================================================================
  ErrorBarsRender& Values(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& err) {
    impl<ErrorBarsRender>()->x = x;
    impl<ErrorBarsRender>()->y = y;
    impl<ErrorBarsRender>()->yerr = err;
    return *this;
  }

  ErrorBarsRender& XValues(const std::vector<double>& x) {
    impl<ErrorBarsRender>()->x = x;
    return *this;
  }

  ErrorBarsRender& YValues(const std::vector<double>& y) {
    impl<ErrorBarsRender>()->y = y;
    return *this;
  }

  ErrorBarsRender& XErr(const std::vector<double>& xerr) {
    GEMINI_FAIL("ErrorBarsRender::XErr not yet implemented");
    return *this;
  }

  ErrorBarsRender& YErr(const std::vector<double>& yerr) {
    impl<ErrorBarsRender>()->yerr = yerr;
    return *this;
  }

  ErrorBarsRender& Label(const std::string& label) {
    return *this;
  }

  ErrorBarsRender& Color(const core::color::PixelColor& color) {
    impl<ErrorBarsRender>()->color = color;
    return *this;
  }

  ErrorBarsRender() : Render(std::make_shared<Impl>()) {}
};

} // namespace gemini::plot

