//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_CANVAS_H_
#define GEMINI_INCLUDE_GEMINI_CANVAS_H_

#include "gemini/core/shapes/Shapes.h"
#include "Utility.h"
#include <sstream>
#include <vector>
#include <map>
#include <array>
#include <deque>

namespace gemini::core {

struct GEMINI_EXPORT CanvasLocation {
  int left{}, bottom{}, right{}, top{};
};

struct GEMINI_EXPORT CoordinateDescription {
  bool has_coordinates = false;
  double left{}, bottom{}, right{}, top{};
};

struct GEMINI_EXPORT CanvasCoordinates {
  double left = std::numeric_limits<double>::quiet_NaN();
  double right = std::numeric_limits<double>::quiet_NaN();
  double bottom = std::numeric_limits<double>::quiet_NaN();
  double top = std::numeric_limits<double>::quiet_NaN();
};

enum class GEMINI_EXPORT CanvasPart {
  Left, Right, Bottom, Top, CenterX, CenterY,
};

enum class GEMINI_EXPORT CanvasDimension {
  Width, Height,
};

//! \brief Represents a relationship between two canvases.
struct FixRelationship {
  FixRelationship(int c1_num, int c2_num, CanvasPart c1_part, CanvasPart c2_part, double px_diff)
      : canvas1_num(c1_num)
      , canvas2_num(c2_num)
      , canvas1_part(c1_part)
      , canvas2_part(c2_part)
      , pixels_diff(px_diff) {}

  int canvas1_num, canvas2_num;
  CanvasPart canvas1_part, canvas2_part;
  double pixels_diff;
};

//! \brief Represents a specified canvas has a fixed width or height.
struct FixDimensions {
  FixDimensions(int c_num, CanvasDimension dim, double len)
      : canvas_num(c_num)
      , dimension(dim)
      , extent(len) {}

  int canvas_num;
  CanvasDimension dimension;
  double extent;
};

// Forward declare canvas
class Canvas;

//! \brief The image class collects infomation on sets of canvases and subcanvases.
class GEMINI_EXPORT Image {
  friend class Canvas;
 public:
  //! \brief Create an image.
  Image();

  Image(int width, int height);

  //! \brief Describe a relationship between two canvases that are children of this image.
  //!
  //! The relationship is Canvas[N1].{Left, Right, ...} + pixels_diff = Canvas[N2].{Left, Right, ...}
  //!
  //! \param canvas1_num Which canvas (as an entry in the canvas vector) is the first canvas in the relationship.
  //! \param canvas1_part Which part of the first canvas is specified in the relationship.
  //! \param canvas2_num Which canvas (as an entry in the canvas vector) is the second canvas in the relationship.
  //! \param canvas2_part Which part of the second canvas is specified in the relationship.
  //! \param pixels_diff The pixel adjustment in the relationship.
  //!
  //! TODO(Nate): Eventually, I want to make this more general, e.g. non-absolute pixel differences, relationships
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

  void Dimensions_Fix(
      Canvas* canvas,
      CanvasDimension dim,
      double extent);

  //! \brief Clear all relationships.
  void ClearRelationships();

  //! \brief Get the master canvas for the image.
  std::shared_ptr<Canvas> GetMasterCanvas();

  //! \brief Get the location of one of the Image's canvases.
  const CanvasLocation& GetLocation(const Canvas* canvas) const;

  NO_DISCARD int GetWidth() const;
  NO_DISCARD int GetHeight() const;

  //! \brief Render the Image to a bitmap.
  NO_DISCARD Bitmap ToBitmap() const;

  //! \brief Calculate all canvas sizes.
  void CalculateImage() const;

  //! \brief Determine the (pixel) locations of each canvas relative to the master canvas.
  void CalculateCanvasLocations() const;

  //! \brief Determine the coordinate system for each canvas that requires a coordinate system.
  void CalculateCanvasCoordinates() const;

  //! \brief Get the coordinate description of a canvas.
  NO_DISCARD const CoordinateDescription& GetCanvasCoordinateDescription(const std::shared_ptr<const Canvas>& canvas) const;

  class Impl;

 private:
  //! \brief The impl for the Image.
  std::shared_ptr<Impl> impl_;
};


class GEMINI_EXPORT Canvas {
  friend class Image;
 public:
  //! \brief Get a floating subcanvas of this canvas.
  std::shared_ptr<Canvas> FloatingSubCanvas();

  //! \brief Add a line on a canvas.
  void AddShape(std::shared_ptr<Shape> shape);

  //! \brief Set the background color.
  void SetBackground(color::PixelColor color);

  //! \brief Set whether to paint the canvas background when rendering the canvas as an image.
  void SetPaintBackground(bool flag);

  //! \brief Set the Canvas's coordinate system.
  void SetCoordinates(const CanvasCoordinates& coordinates);

  //! \brief Returns the canvas's coordinate system. A lack of coordinate in x or y is specified by
  //! a quiet NaN.
  CanvasCoordinates& GetCoordinateSystem();

  //! \brief Get the background color of the canvas.
  NO_DISCARD const color::PixelColor& GetBackgroundColor() const;

  //! \brief Convert a point on the Canvas into a pixels point.
  NO_DISCARD Point PointToPixels(const Point& point) const;
  //! \brief Convert a displacement on the Canvas into a pixels displacement.
  NO_DISCARD Displacement DisplacementToPixels(const Displacement& displacement) const;

 protected:
  //! \brief Construct a canvas as a child of another canvas.
  explicit Canvas(Canvas* parent)
      : parent_(parent)
      , image_(parent == nullptr ? nullptr : parent->image_) {}

  //! \brief Construct a canvas as a direct child of an image.
  //!
  //! \param image The image that will the the parent of this canvas.
  explicit Canvas(Image::Impl* image)
      : parent_(nullptr)
      , image_(image) {}

  //! \brief True if this is the top level canvas.
  NO_DISCARD bool isTopLevelCanvas() const;

  //! \brief Write a canvas on a bitmap.
  void writeOnBitmap(Bitmap& image) const;

  //! \brief Paint the canvas background.
  void paintBackground(Bitmap& image) const;

  ///========================================================================
  /// Protected variables.
  ///========================================================================

  //! \brief The background color of the canvas
  color::PixelColor background_color_ = color::White;

  //! \brief If true, when this canvas draws itself to an image, it will first paint the entire canvas area
  //! to be the background color. If false, the background will not be painted.
  bool paint_background_ = true;

  //! \brief All the shapes owned by the canvas.
  std::vector<std::shared_ptr<Shape>> shapes_;

  //! \brief The parent canvas for this canvas. Null if this is the top level canvas.
  Canvas* parent_ = nullptr;

  //! \brief Keep as a deque so references and pointers are not invalidated.
  std::deque<std::shared_ptr<Canvas>> child_canvases_;

  //! \brief Describes the coordinate system of the canvas, if any.
  CanvasCoordinates coordinate_system_;

  //! \brief The image that this canvas belongs to. You belong to the same image as your parent.
  Image::Impl* image_;
};

} // namespace gemini::core

#endif //GEMINI_INCLUDE_GEMINI_CANVAS_H_
