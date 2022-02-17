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
  Left, Right, Bottom, Top, CenterX, CenterY
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

// Forward declare canvas
class Canvas;

//! \brief The image class collects infomation on sets of canvases and subcanvases.
class GEMINI_EXPORT Image {
  friend class Canvas;

 public:
  //! \brief Create an image.
  Image();

  Image(int width, int height);

  //! \brief Clean up all the canvases.
  ~Image();

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

  //! \brief Clear all relationships.
  void ClearRelationships();

  //! \brief Get the master canvas for the image.
  //!
  //! I'm returning a pointer to the canvas since this makes it harder to forget to get the returned canvas
  //! by reference and only set a copy, which is not generally the intended behavior.
  class Canvas* GetMasterCanvas();

  //! \brief Get the location of one of the Image's canvases.
  const CanvasLocation& GetLocation(const Canvas* canvas) const;

  int GetWidth() const;
  int GetHeight() const;

  //! \brief Render the Image to a bitmap.
  NO_DISCARD Bitmap ToBitmap() const;

  //! \brief Calculate all canvas sizes.
  void CalculateImage() const;

  //! \brief Determine the (pixel) locations of each canvas relative to the master canvas.
  void CalculateCanvasLocations() const;

  //! \brief Determine the coordinate system for each canvas that requires a coordinate system.
  void CalculateCanvasCoordinates() const;

  //! \brief Get the coordinate description of a canvas.
  const CoordinateDescription& GetCanvasCoordinateDescription(Canvas* canvas) const;

 protected:
  //! \brief Add a canvas to the image. Also adds entries for storing the canvas locations and canvas coordinate system.
  void registerCanvas(Canvas* canvas);

  //! \brief Determine the maximum and minimum coordinate points of a canvas in the x and y directions.
  //!
  //! If there are no objects with coordinates in x or y, those max/min coordinates will be quiet NaNs.
  static std::array<double, 4> getMinMaxCoordinates(Canvas* canvas);
  void describeCoordinates(class Canvas* canvas, const std::array<double, 4>& min_max_coords) const;

  ///========================================================================
  /// Protected variables.
  ///========================================================================

  //! \brief Brief the "master" canvas. This represents the surface of the entire image.
  //!
  //! All canvases for the image will either be this canvas, or a child (grandchild, etc.) canvas of this canvas.
  Canvas* master_canvas_ = nullptr;

  //! \brief A vector of all the canvas that are associated with this image.
  //!
  //! This will include the master_canvas_, its children, their children, etc.
  std::vector<Canvas*> canvases_;

  std::vector<FixRelationship> canvas_fixes_;

  //! \brief What coordinate system width to use if there is no coordinate extent in the x or y dimensions.
  //! This can happen either because there is only one point that needs coordinates in x or y, or because, e.g. the
  //! x dimension has coordinate objects, but the y dimension does not. So a coordinate system is still used in the y
  //! dimension [NOTE(Nate): separate the ability to have coordinate systems for x and y?].
  double default_coordinate_epsilon = 0.0001;

  //! \brief Determines the sizes and locations of each canvas.
  mutable std::map<const Canvas*, CanvasLocation> canvas_locations_;

  //! \brief Describes whether each canvas has a coordinate description, and if so, what it is.
  mutable std::map<const Canvas*, CoordinateDescription> canvas_coordinate_description_;

  //! \brief True whenever canvases and image parameters may need to be recalculated.
  mutable bool needs_calculate_ = false;

  //! \brief Store the width and heigh of the whole image, in pixels.
  //!
  //! Since the final image will (at least in e.g. the bitmap case) be in pixels, we store these as ints, not as doubles.
  mutable int width_ = 100, height_ = 100;
};

class GEMINI_EXPORT Canvas {
  friend class Image;

 public:
  //! \brief Get a floating subcanvas of this canvas.
  Canvas* FloatingSubCanvas();

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

  NO_DISCARD const color::PixelColor& GetBackgroundColor() const;

  NO_DISCARD Point PointToPixels(const Point& point, bool relative_to_canvas = false) const;
  NO_DISCARD Displacement DisplacementToPixels(const Displacement& displacement) const;

 protected:
  //! \brief Construct a canvas as a child of another canvas.
  explicit Canvas(Canvas* parent)
      : parent_(parent)
      , image_(parent == nullptr ? nullptr : parent->image_) {}

  //! \brief Construct a canvas as a direct child of an image.
  //!
  //! \param image The image that will the the parent of this canvas.
  explicit Canvas(Image* image)
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

  std::vector<Canvas*> child_canvases_;

  //! \brief Describes the coordinate system of the canvas, if any.
  CanvasCoordinates coordinate_system_;

  //! \brief The image that this canvas belongs to.
  Image* image_;
};

}

#endif //GEMINI_INCLUDE_GEMINI_CANVAS_H_
