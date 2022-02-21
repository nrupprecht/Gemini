//
// Created by Nathaniel Rupprecht on 11/26/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_PLOT_H_
#define GEMINI_INCLUDE_GEMINI_PLOT_H_

#include "gemini/core/Canvas.h"
#include "gemini/text/TrueTypeFontEngine.h"
#include "gemini/plot/Marker.h"
#include <optional>

namespace gemini::plot {

inline GEMINI_EXPORT std::vector<core::color::PixelColor> DefaultColorPalette() {
  return {
      { 204, 000, 000, 255},
      { 255, 255, 000, 255},
      { 000, 204, 102, 255},
      { 051, 251, 051, 255},
      { 000, 255, 255, 255},
      { 051, 153, 255, 255},
      { 102, 102, 255, 255},
      { 204, 000, 204, 255},
  };
}

inline GEMINI_EXPORT std::vector<core::color::PixelColor> ColorPaletteHLS() {
  return {
      { 204, 102,  92, 255},
      { 215, 195, 104, 255},
      { 161, 217, 106, 255},
      { 125, 216, 137, 255},
      { 122, 208, 217, 255},
      {  92, 111, 212, 255},
      { 152,  91, 212, 255},
      { 203,  95, 174, 255},
  };
}

struct LinePlotOptions {
  core::color::PixelColor color;

};

//! \brief A structure that encapsulates the options available for a scatter plot.
struct ScatterPlotOptions {
  //! \brief The color that should be used for the scatter plot.
  //!
  //! TODO: This could be really general, like a function that determines a color as a function of where the marker is and what order its in.
  std::optional<core::color::PixelColor> color;

  //! \brief The marker to use for the scatterplot.
  //!
  //! TODO: This could also be really general, like a function that determines a Marker, as a function of where the marker is and what order its in.
  std::shared_ptr<marker::Marker> marker;

  //! \brief The label that should be used with the plot legend. Empty corresponds to no label.
  std::string label{};

  // ==========================================================================================
  //  Builder functions.
  // ==========================================================================================

  //! \brief Set the plot color.
  ScatterPlotOptions& Color(const core::color::PixelColor& col) {
    color = col;
    return *this;
  }

  //! \brief Set the plot marker.
  ScatterPlotOptions& Marker(const std::shared_ptr<marker::Marker>& mark) {
    marker = mark;
    return *this;
  }

  //! \brief Set the plot label.
  ScatterPlotOptions& Label(const std::string& lbl) {
    label = lbl;
    return *this;
  }

};

struct ErrorPlotOptions {
  core::color::PixelColor color;
};


class GEMINI_EXPORT Figure {
 public:
  //! \brief Create a figure with a specified pixel width and height.
  Figure(int width, int height);

  //! \brief Set the figure title.
  void Title(const std::string& title);

  void Plot(const std::vector<double>& x, const std::vector<double>& y, const std::string& label = "");
  void Scatter(const std::vector<double>& x, const std::vector<double>& y, const std::string& label = "");
  void Scatter(const std::vector<double>& x, const std::vector<double>& y, const ScatterPlotOptions& options);
  void PlotErrorbars(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& err, const std::string& label = "");

  //! \brief Set the x range of the image.
  void SetXRange(double xmin, double xmax);

  //! \brief Set the y range of the image.
  void SetYRange(double ymin, double ymax);

  //! \brief Write the figure, as a Bitmap, to a file.
  void ToFile(const std::string& filepath);

  //! \brief Get the underlying Image for the figure.
  core::Image& GetImage();

 private:
  //! \brief Switch to the next color in the palette.
  void updateColorPalette(int& index);

  //! \brief The image the figure writes on.
  core::Image image_;

  //! \brief The canvas on which the main plotting occurs.
  core::Canvas* plotting_canvas_;

  std::vector<core::color::PixelColor> color_palette_;
  int plot_palette_index_ = 0;
  int scatter_palette_index_ = 0;
  int error_palette_index_ = 0;

  //! \brief A structure that stores some data about entries in a legend.
  struct LegendEntry {
    core::color::PixelColor color;
    std::string label;
  };

  //! \brief Data about how to construct a legend.
  std::vector<LegendEntry> legend_data_;

  //! The plot title. Only used if non-empty.
  //! TODO: More options for the title, size, color, etc.
  std::string title_;

  std::shared_ptr<gemini::text::TrueTypeFontEngine> ttf_engine_;

};

} // namespace gemini::plot

#endif //GEMINI_INCLUDE_GEMINI_PLOT_H_
