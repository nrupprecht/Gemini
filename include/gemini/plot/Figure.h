//
// Created by Nathaniel Rupprecht on 8/21/22.
//

#pragma once

#include "gemini/core/Canvas.h"
#include "gemini/text/TrueTypeFontEngine.h"
#include "gemini/plot/Marker.h"
#include "gemini/plot/Render.h"
#include "gemini/core/Bitmap.h"

namespace gemini::plot {

class GlobalFontManager {
 private:
  class Impl;
  static std::unique_ptr<Impl> singleton_;
  static int dummy_;

  //! \brief Allocate and set up the singleton object.
  static int initialize();

 public:
  static std::shared_ptr<gemini::text::TrueTypeFontEngine> GetFontEngine();
  static void LoadFontEngine(const std::filesystem::path& ttf_file);

};

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


//! \brief A plotting unit, consisting of a Render space and potentially axes, labels, etc.
class GEMINI_EXPORT Plot {
  friend class FigureSpace;
  friend class SubFigure;
 public:
  //! \brief Add a render to a plot.
  Plot& AddRender(const Render& render);

  //! brief Set the x axis label.
  void SetXLabel(const std::string& label);
  void ClearXLabel();

  //! \brief Set the y axis label.
  void SetYLabel(const std::string& label);
  void ClearYLabel();

 protected:
  void addRendersToCanvas() const;

  void initializeCanvases(const std::shared_ptr<core::Canvas>& canvas);

  void postCalculate();

  //! \brief The full canvas for the plot. Contains the entire plot, labels, axes, titles, the plotting canvas, etc.
  std::shared_ptr<core::Canvas> full_canvas_;

  //! \brief All renders are added to this canvas.
  std::shared_ptr<core::Canvas> plot_surface_;

  std::optional<std::string> xlabel_, ylabel_;

  //! \brief The image the plot writes on.
  core::Image image_;

  //! \brief Vector of objects that should be rendered on the plot.
  std::vector<Render> renders_;
};

class SubFigure;

//! \brief An object that represents one "panel" of a figure, which can either be a single Plot, or a Figure (which can
//!         itself have sub-figures).
class GEMINI_EXPORT FigureSpace {
  friend class SubFigure;
 public:
  using PlotPtr = std::shared_ptr<Plot>;
  using FigPtr = std::shared_ptr<SubFigure>;
  using SpaceOptions = std::variant<PlotPtr, FigPtr>;

  //! \brief Create a FigureSpace. A figure space contains a plot by default.
  FigureSpace();

  //! \brief Query where the figure space is a sub-figure.
  NO_DISCARD bool IsFigure() const;
  //! \brief Query where the figure space is a plot.
  NO_DISCARD bool IsPlot() const;

  //! \brief Turn the space into a sub-figure.
  SubFigure& MakeFigure();

  //! \brief Turn the space into a plot. Returns what the space was before.
  Plot& MakePlot();

  NO_DISCARD SubFigure& AsFigure() const;

  NO_DISCARD Plot& AsPlot() const;

 protected:
  explicit FigureSpace(core::Image image);

  //! \brief Set the image for the FigureSpace.
  void setImage(core::Image image);

  //! \brief A space can either just be a plot, or be a figure, which can contain subplots.
  SpaceOptions space_{};

  //! \brief The image that the plots and subfigures need to plot to.
  core::Image image_;
};


class GEMINI_EXPORT SubFigure {
  friend class FigureSpace;
 public:
  //! \brief Create a subfigure in the figure.
  void SetSubSpaces(std::size_t num_x, std::size_t num_y);

  void SetSubSpaceRelativeSizes(const std::vector<double>& x_relative_sizes,
                                const std::vector<double>& y_relative_sizes);

  NO_DISCARD const FigureSpace& GetSubspace(std::size_t x, std::size_t y) const;
  NO_DISCARD FigureSpace& GetSubspace(std::size_t x, std::size_t y);

  NO_DISCARD Plot& GetOrMakePlot(std::size_t x, std::size_t y);
  NO_DISCARD SubFigure& GetOrMakeSubFig(std::size_t x, std::size_t y);

 protected:
  //! \brief Create a figure that writes on a specific image - this is used to create subfigures.
  explicit SubFigure(core::Image base_image);

  void initializeCanvases(const std::shared_ptr<core::Canvas>& canvas);

  //! \brief Write the figure onto its plotting canvas.
  void addRendersToCanvas() const;

  void postCalculate();

  // Keep track of plots or sub-figures.
  std::vector<FigureSpace> subspaces_{};
  //! \brief The canvases that each figure space writes on.
  std::vector<std::shared_ptr<core::Canvas>> canvases_for_figurespaces_;

  //! \brief Initially start with a single plot in the figure.
  unsigned int grid_x_ = 1, grid_y_ = 1;

  std::vector<double> x_relative_sizes_{};
  std::vector<double> y_relative_sizes_{};

  //! \brief The image the figure writes on.
  core::Image image_;

  //! \brief The canvas on which the main plotting occurs.
  std::shared_ptr<core::Canvas> plotting_canvas_;
};


class GEMINI_EXPORT Figure : public SubFigure {
 public:
  Figure(unsigned int width, unsigned int height);

  //! \brief Render the current figure to a bitmap.
  core::Bitmap ToBitmap();

 protected:
  //! \brief The width and height of the Figure image in pixels.
  unsigned int width_, height_;
};

} // namespace gemini::plot

