//
// Created by Nathaniel Rupprecht on 11/25/21.
//

#pragma once

#include "gemini/core/shapes/Shapes.h"
#include "Utility.h"
#include <sstream>
#include <vector>
#include <map>
#include <array>
#include <deque>
#include <Eigen/Dense>

namespace gemini::core {

struct GEMINI_EXPORT CanvasLocation {
  int left{}, bottom{}, right{}, top{};

  constexpr bool operator==(const CanvasLocation& rhs) const {
    return std::tie(left, bottom, right, top) == std::tie(rhs.left, rhs.bottom, rhs.right, rhs.top);
  }

  friend std::ostream& operator<<(std::ostream& out, const CanvasLocation& location) {
    out << "{ L=" << location.left << ", R=" << location.right
        << ", B=" << location.bottom << ", T=" << location.top << " }";
    return out;
  }
};

//! \brief Base interface for objects that can be put in relationships with other Locatable objects in an image.
class Locatable {
 public:
  //! \brief If the locatable has a predefined width, get it. By default, returns that the width is not defined yet.
  NO_DISCARD virtual std::optional<double> GetWidth() const { return {}; }
  //! \brief If the locatable has a predefined height, get it. By default, returns that the height is not defined yet.
  NO_DISCARD virtual std::optional<double> GetHeight() const { return {}; }

  //! \brief Set the location of the locatable object after calculating where it should go.
  virtual void SetLocation(const CanvasLocation& location) = 0;
};

struct GEMINI_EXPORT CoordinateDescription {
  bool has_coordinates = false;
  double left = std::numeric_limits<double>::quiet_NaN();
  double bottom = std::numeric_limits<double>::quiet_NaN();
  double right = std::numeric_limits<double>::quiet_NaN();
  double top = std::numeric_limits<double>::quiet_NaN();
};

enum class GEMINI_EXPORT CanvasPart {
  Left, Right, Bottom, Top, CenterX, CenterY,
};

enum class GEMINI_EXPORT CanvasDimension {
  X, Y,
};

//! \brief A wrapper around a vector of locatables, and functions to get information about them.
struct IndexedLocatables {
  std::vector<Locatable*> objs_;

  void Add(Locatable* loc) {
    objs_.push_back(loc);
  }

  NO_DISCARD std::size_t Size() const {
    return objs_.size();
  }

  int GetIndex(const Locatable* ptr) const {
    auto it = std::find(objs_.begin(), objs_.end(), ptr);
    return static_cast<int>(std::distance(objs_.begin(), it));
  }
};


struct Fix {
  virtual ~Fix() = default;

  //! \brief Set a row in the relationships matrix and constants that encodes the relationship.
  virtual void Create(long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
                      const IndexedLocatables& locatables) const = 0;

  //! \brief Return a brief name for the fix. This will be used in debugging and monitoring.
  NO_DISCARD virtual std::string Name() const = 0;

  //! \brief Helper function to add parts of a relationship to a matrix.
  static void AddToMatrixForCanvasPart(
      long index,
      double value,
      CanvasPart part,
      int locatable_index,
      Eigen::MatrixXd& relationships) {
    // Relationships matrix represents canvases in blocks of four columns, representing the Left, Bottom, Right, Top
    // positions of the locatable.

    switch (part) {
      case CanvasPart::Left: {
        relationships(index, 4 * locatable_index + 0) += value;
        break;
      }
      case CanvasPart::Right: {
        relationships(index, 4 * locatable_index + 2) += value;
        break;
      }
      case CanvasPart::Bottom: {
        relationships(index, 4 * locatable_index + 1) += value;
        break;
      }
      case CanvasPart::Top: {
        relationships(index, 4 * locatable_index + 3) += value;
        break;
      }
      case CanvasPart::CenterX: {
        relationships(index, 4 * locatable_index + 0) += 0.5 * value;
        relationships(index, 4 * locatable_index + 2) += 0.5 * value;
        break;
      }
      case CanvasPart::CenterY: {
        relationships(index, 4 * locatable_index + 1) += 0.5 * value;
        relationships(index, 4 * locatable_index + 3) += 0.5 * value;
        break;
      }
      default: {
        GEMINI_FAIL("unrecognized canvas part");
      }
    }
  }
};

//! \brief Represents a relationship between two canvases.
//!
//! The relationship is
//!     Canvas[N1].{Left, Right, ...} + pixels_diff = Canvas[N2].{Left, Right, ...}
//!         or
//!     Canvas[N2].{Left, Right, ...} - Canvas[N1].{Left, Right, ...} = pixels_diff
//!
struct FixRelationship : public Fix {
  FixRelationship(const Locatable* c1_num, const Locatable* c2_num, CanvasPart c1_part, CanvasPart c2_part, double px_diff)
      : canvas1_num(c1_num)
      , canvas2_num(c2_num)
      , canvas1_part(c1_part)
      , canvas2_part(c2_part)
      , pixels_diff(px_diff) {}

  void Create(long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
              const IndexedLocatables& locatables) const override {
    auto idx1 = locatables.GetIndex(canvas1_num), idx2 = locatables.GetIndex(canvas2_num);
    Fix::AddToMatrixForCanvasPart(index, -1.0, canvas1_part, idx1, relationships);
    Fix::AddToMatrixForCanvasPart(index, 1.0, canvas2_part, idx2, relationships);
    constants(index, 0) = pixels_diff;
  }

  NO_DISCARD std::string Name() const override {
    return "FixRelationship";
  }

  const Locatable* canvas1_num, *canvas2_num;
  CanvasPart canvas1_part, canvas2_part;
  double pixels_diff;
};

//! \brief Represents a specified canvas has a fixed width or height.
//!
//! The relationship is
//!     Canvas[N1].[Left, Bottom] + pixels_diff = Canvas[N1].[Right, Top]
//!         or
//!     Canvas[N1].[Right, Top] - Canvas[N1].[Left, Bottom] = pixels_diff
//!
struct FixDimensions : public Fix {
  FixDimensions(const Locatable* loc, CanvasDimension dim, double len)
      : locatable(loc)
        , dimension(dim)
        , extent(len) {}

  void Create(long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
              const IndexedLocatables& locatables) const override {
    auto lidx = locatables.GetIndex(locatable);
    if (dimension == CanvasDimension::X) {
      Fix::AddToMatrixForCanvasPart(index, -1.0, CanvasPart::Left, lidx, relationships);
      Fix::AddToMatrixForCanvasPart(index, +1.0, CanvasPart::Right, lidx, relationships);
    }
    else if (dimension == CanvasDimension::Y) {
      Fix::AddToMatrixForCanvasPart(index, -1.0, CanvasPart::Bottom, lidx, relationships);
      Fix::AddToMatrixForCanvasPart(index, +1.0, CanvasPart::Top, lidx, relationships);
    }
    else {
      GEMINI_FAIL("unrecognized CanvasDimension");
    }
    constants(index, 0) = extent;
  }

  NO_DISCARD std::string Name() const override {
    return "FixDimensions";
  }

  const Locatable* locatable;
  CanvasDimension dimension;
  double extent;
};

//! \brief Represents a proportionality relationship between two canvases, where some part of one canvas is positioned
//!     relative to another canvas.
//!
//! The relationship is (picking a dimension Dim)
//!     Canvas[N1].{Left, Right, ...} = Canvas[N2].Dim.Lesser + lambda * (Canvas[N2].Dim.Greater - Canvas[N2].Dim.Lesser)
//!         or
//!     Canvas[N1].{Left, Right, ...} = (1 - lambda) * Canvas[N2].Dim.Lesser + lambda * Canvas[N2].Dim.Greater
//!         or
//!     Canvas[N1].{Left, Right, ...} - (1 - lambda) * Canvas[N2].Dim.Lesser - lambda * Canvas[N2].Dim.Greater = 0
//!
struct FixScale : public Fix {
  FixScale(const Locatable* c1_num, const Locatable* c2_num, CanvasPart c1_part, CanvasDimension dimension, double lambda)
      : canvas1_num(c1_num)
      , canvas2_num(c2_num)
      , canvas1_part(c1_part)
      , dimension(dimension)
      , lambda(lambda) {}

  void Create(long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
              const IndexedLocatables& locatables) const override {
    auto idx1 = locatables.GetIndex(canvas1_num), idx2 = locatables.GetIndex(canvas2_num);
    Fix::AddToMatrixForCanvasPart(index, +1, canvas1_part, idx1, relationships);

    if (dimension == CanvasDimension::X) {
      Fix::AddToMatrixForCanvasPart(index, -(1. - lambda), CanvasPart::Left, idx2, relationships);
      Fix::AddToMatrixForCanvasPart(index, -lambda, CanvasPart::Right, idx2, relationships);
    }
    else if (dimension == CanvasDimension::Y) {
      Fix::AddToMatrixForCanvasPart(index, -(1. - lambda), CanvasPart::Bottom, idx2, relationships);
      Fix::AddToMatrixForCanvasPart(index, -lambda, CanvasPart::Top, idx2, relationships);
    }
    else {
      GEMINI_FAIL("unrecognized CanvasDimension");
    }
  }

  NO_DISCARD std::string Name() const override {
    return "FixScale";
  }

  const Locatable *canvas1_num, *canvas2_num;
  CanvasPart canvas1_part;
  CanvasDimension dimension;
  double lambda;
};

//! \brief Represents a proportionality relationship between two canvases, how wide or high a canvas is compared to the
//!     width or height of another canvas.
//!
//! The relationship is (picking a dimension Dim)
//!     Canvas[N1].Dim1.Greater - Canvas[N1].Dim1.Lesser = scale * (Canvas[N2].Dim2.Greater - Canvas[N2].Dim2.Lesser)
//!         or
//!     Canvas[N1].Dim1.Greater - Canvas[N1].Dim1.Lesser - scale * (Canvas[N2].Dim2.Greater - Canvas[N2].Dim2.Lesser) = 0
//!
struct FixRelativeSize : public Fix {
  FixRelativeSize(const Locatable* c1_num, const Locatable* c2_num, CanvasDimension dimension1, CanvasDimension dimension2, double scale)
      : canvas1_num(c1_num)
      , canvas2_num(c2_num)
      , canvas1_dimension(dimension1)
      , canvas2_dimension(dimension2)
      , scale(scale) {}

  void Create(long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
              const IndexedLocatables& locatables) const override {
    auto idx1 = locatables.GetIndex(canvas1_num), idx2 = locatables.GetIndex(canvas2_num);
    if (canvas1_dimension == CanvasDimension::X) {
      Fix::AddToMatrixForCanvasPart(index, +1, CanvasPart::Right, idx1, relationships);
      Fix::AddToMatrixForCanvasPart(index, -1, CanvasPart::Left, idx1, relationships);
    }
    else if (canvas1_dimension == CanvasDimension::Y) {
      Fix::AddToMatrixForCanvasPart(index, +1, CanvasPart::Top, idx1, relationships);
      Fix::AddToMatrixForCanvasPart(index, -1, CanvasPart::Bottom, idx1, relationships);
    }
    else {
      GEMINI_FAIL("unrecognized CanvasDimension for canvas 1");
    }

    if (canvas2_dimension == CanvasDimension::X) {
      Fix::AddToMatrixForCanvasPart(index, -scale, CanvasPart::Right, idx2, relationships);
      Fix::AddToMatrixForCanvasPart(index, +scale, CanvasPart::Left, idx2, relationships);
    }
    else if (canvas2_dimension == CanvasDimension::Y) {
      Fix::AddToMatrixForCanvasPart(index, -scale, CanvasPart::Top, idx2, relationships);
      Fix::AddToMatrixForCanvasPart(index, +scale, CanvasPart::Bottom, idx2, relationships);
    }
    else {
      GEMINI_FAIL("unrecognized CanvasDimension for canvas 2");
    }
  }

  NO_DISCARD std::string Name() const override {
    return "FixRelativeSize";
  }

  const Locatable* canvas1_num, *canvas2_num;
  CanvasDimension canvas1_dimension;
  CanvasDimension canvas2_dimension;
  double scale;
};

// Forward declare canvas
class Canvas;

//! \brief The image class collects infomation on sets of canvases and subcanvases.
class GEMINI_EXPORT Image {
  friend class Canvas;

 public:
  //! \brief Create an image.
  Image();

  //! \brief Create an image with a specific width and height, in pixels.
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
  void Relation_Fix(
      Canvas* canvas1,
      CanvasPart canvas1_part,
      Canvas* canvas2,
      CanvasPart canvas2_part,
      double pixels_diff = 0.);

  //! \brief Describes a relationship between two canvases that are children of this canvas.
  //!
  //! The relationship is (picking a dimension "Dim")
  //!     Canvas[N1].{Left, Right, ...} = Canvas[N2].Dim.Lesser + lambda * (Canvas[N2].Dim.Greater - Canvas[N2].Dim.Lesser)
  //!         or
  //!     Canvas[N1].{Left, Right, ...} = (1 - lambda) * Canvas[N2].Dim.Lesser + lambda * Canvas[N2].Dim.Greater
  //!
  void Scale_Fix(
      Canvas* canvas1,
      CanvasPart canvas1_part,
      Canvas* canvas2,
      CanvasDimension dimension,
      double lambda);

  void Dimensions_Fix(
      Canvas* canvas,
      CanvasDimension dim,
      double extent);

  void RelativeSize_Fix(
      Canvas* canvas1,
      CanvasDimension dimension1,
      Canvas* canvas2,
      CanvasDimension dimension2,
      double scale);

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

 private:
  class Impl;

  //! \brief The impl for the Image.
  std::shared_ptr<Impl> impl_;
};

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

