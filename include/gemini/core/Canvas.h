//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#pragma once

#include "gemini/core/shapes/Shapes.h"
#include "gemini/core/Image.h"

#include <utility>
#include <vector>
#include <array>
#include <deque>

namespace gemini::core {

class GEMINI_EXPORT Canvas : public Locatable {
  friend class Image;

 public:
  //! \brief The Locatable "SetLocation" function that allows the canvas to participate in relationships.
  void SetLocation(const CanvasLocation& location) override;

  //! \brief Get a floating subcanvas of this canvas.
  std::shared_ptr<Canvas> FloatingSubCanvas();

  //! \brief Add a line on a canvas.
  void AddShape(std::shared_ptr<Shape> shape);

  //! \brief Set the background color.
  void SetBackground(color::PixelColor color);

  //! \brief Set whether to paint the canvas background when rendering the canvas as an image.
  void SetPaintBackground(bool flag);

  //! \brief Set the Canvas's coordinate system.
  void SetCoordinates(const CoordinateDescription& coordinates);

  //! \brief Returns the canvas's coordinate system. A lack of coordinate in x or y is specified by
  //! a quiet NaN.
  CoordinateDescription& GetCoordinateSystem();

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
  std::vector<std::shared_ptr<Shape>> shapes_{};

  //! \brief The parent canvas for this canvas. Null if this is a top level canvas for some image.
  Canvas* parent_ = nullptr;

  //! \brief Keep as a deque so references and pointers are not invalidated.
  std::deque<std::shared_ptr<Canvas>> child_canvases_{};

  //! \brief Describes the coordinate system of the canvas, if any.
  //!
  //! This can be set via the SetCoordinates function, and is otherwise calculated by the image calling
  //! its DescribeCoordinates function.
  CoordinateDescription coordinate_system_{};

  //! \brief The image that this canvas belongs to. You belong to the same image as your parent.
  Image::Impl* image_{};
};

} // namespace gemini::core
