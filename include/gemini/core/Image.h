//
// Created by Nathaniel Rupprecht on 11/23/22.
//

#pragma once

#include "gemini/core/Utility.h"
#include "gemini/core/Location.h"
#include "gemini/core/Bitmap.h"
#include <Eigen/Dense>
#include <map>

namespace gemini::core {

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

  //! \brief Add a locatable to the container. Only adds it if it is not already contained.
  //!
  //! \param loc The locatable to add.
  //! \return If the locatable already existed in the container, return nullopt, otherwise, return the index of the
  //!     locatable in the container.
  std::optional<std::size_t> Add(Locatable* loc) {
    // Don't add if it is already contained.
    if (std::find(objs_.begin(), objs_.end(), loc) == objs_.end()) {
      objs_.push_back(loc);
      return objs_.size() - 1;
    }
    return {};
  }

  NO_DISCARD std::size_t Size() const {
    return objs_.size();
  }

  int GetIndex(const Locatable* ptr) const {
    auto it = std::find(objs_.begin(), objs_.end(), ptr);
    return static_cast<int>(std::distance(objs_.begin(), it));
  }

  void SetLocation(int index, const CanvasLocation& location) const {
    GEMINI_REQUIRE(index < objs_.size(), "index out of range in IndexedLocatables::SetLocation");
    objs_[index]->SetLocation(location);
  }
};

struct Fix {
  Fix() = default;
  explicit Fix(std::string describe)
      : description(std::move(describe)) {}
  virtual ~Fix() = default;

  //! \brief Set the description string.
  void SetDescription(std::string describe) {
    description = std::move(describe);
  }

  //! \brief Set a row in the relationships matrix and constants that encodes the relationship.
  virtual void Create(
      long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
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

  //! \brief An optional (often empty) description of what the fix is doing, for debugging purposes.
  std::string description;
};

//! \brief Represents a relationship between two canvases.
//!
//! The relationship is
//!     Canvas[N1].{Left, Right, ...} + pixels_diff = Canvas[N2].{Left, Right, ...}
//!         or
//!     Canvas[N2].{Left, Right, ...} - Canvas[N1].{Left, Right, ...} = pixels_diff
//!
struct FixRelationship : public Fix {
  FixRelationship(
      const Locatable* c1_num,
      const Locatable* c2_num,
      CanvasPart c1_part,
      CanvasPart c2_part,
      double px_diff)
      : canvas1_num(c1_num)
        , canvas2_num(c2_num)
        , canvas1_part(c1_part)
        , canvas2_part(c2_part)
        , pixels_diff(px_diff) {}

  void Create(
      long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
      const IndexedLocatables& locatables) const override {
    auto idx1 = locatables.GetIndex(canvas1_num), idx2 = locatables.GetIndex(canvas2_num);
    Fix::AddToMatrixForCanvasPart(index, -1.0, canvas1_part, idx1, relationships);
    Fix::AddToMatrixForCanvasPart(index, 1.0, canvas2_part, idx2, relationships);
    constants(index, 0) = pixels_diff;
  }

  NO_DISCARD std::string Name() const override {
    return "FixRelationship";
  }

  const Locatable* canvas1_num, * canvas2_num;
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

  void Create(
      long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
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
  FixScale(
      const Locatable* c1_num,
      const Locatable* c2_num,
      CanvasPart c1_part,
      CanvasDimension dimension,
      double lambda)
      : canvas1_num(c1_num)
        , canvas2_num(c2_num)
        , canvas1_part(c1_part)
        , dimension(dimension)
        , lambda(lambda) {}

  void Create(
      long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
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

  const Locatable* canvas1_num, * canvas2_num;
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
  FixRelativeSize(
      const Locatable* c1_num,
      const Locatable* c2_num,
      CanvasDimension dimension1,
      CanvasDimension dimension2,
      double scale)
      : canvas1_num(c1_num)
        , canvas2_num(c2_num)
        , canvas1_dimension(dimension1)
        , canvas2_dimension(dimension2)
        , scale(scale) {}

  void Create(
      long index, Eigen::MatrixXd& relationships, Eigen::MatrixXd& constants,
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

  const Locatable* canvas1_num, * canvas2_num;
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
  std::shared_ptr<Fix> Relation_Fix(
      Locatable* canvas1,
      CanvasPart canvas1_part,
      Locatable* canvas2,
      CanvasPart canvas2_part,
      double pixels_diff = 0.);

  //! \brief Describes a relationship between two canvases that are children of this canvas.
  //!
  //! The relationship is (picking a dimension "Dim")
  //!     Canvas[N1].{Left, Right, ...} = Canvas[N2].Dim.Lesser + lambda * (Canvas[N2].Dim.Greater - Canvas[N2].Dim.Lesser)
  //!         or
  //!     Canvas[N1].{Left, Right, ...} = (1 - lambda) * Canvas[N2].Dim.Lesser + lambda * Canvas[N2].Dim.Greater
  //!
  std::shared_ptr<Fix> Scale_Fix(
      Locatable* canvas1,
      CanvasPart canvas1_part,
      Locatable* canvas2,
      CanvasDimension dimension,
      double lambda);

  std::shared_ptr<Fix> Dimensions_Fix(
      Locatable* canvas,
      CanvasDimension dim,
      double extent);

  std::shared_ptr<Fix> RelativeSize_Fix(
      Locatable* canvas1,
      CanvasDimension dimension1,
      Locatable* canvas2,
      CanvasDimension dimension2,
      double scale);

  //! \brief Add a fix to the image.
  void AddFix(const std::shared_ptr<Fix>& fix);

  //! \brief Clear all relationships.
  void ClearRelationships();

  //! \brief Get the master canvas for the image.
  std::shared_ptr<Canvas> GetMasterCanvas();

  //! \brief Get the location of one of the Image's canvases.
  const CanvasLocation& GetLocation(const Canvas* canvas) const;

  NO_DISCARD int GetWidth() const;
  NO_DISCARD int GetHeight() const;

  //! \brief Register a new locatable with the Image.
  std::optional<std::size_t> RegisterLocatable(Locatable* locatable);

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


//! \brief Implementation of an image.
class Image::Impl {
  friend class Canvas;

 public:
  Impl();
  Impl(int width, int height);

  std::shared_ptr<Fix> Relation_Fix(
      Locatable* canvas1,
      CanvasPart canvas1_part,
      Locatable* canvas2,
      CanvasPart canvas2_part,
      double pixels_diff = 0.);

  std::shared_ptr<Fix> Scale_Fix(
      Locatable* canvas1,
      CanvasPart canvas1_part,
      Locatable* canvas2,
      CanvasDimension dimension,
      double lambda);

  std::shared_ptr<Fix> Dimensions_Fix(
      Locatable* canvas,
      CanvasDimension dim,
      double extent);

  std::shared_ptr<Fix> RelativeSize_Fix(
      Locatable* canvas1,
      CanvasDimension dimension1,
      Locatable* canvas2,
      CanvasDimension dimension2,
      double scale);

  std::shared_ptr<Fix> AddFix(const std::shared_ptr<Fix>& fix);

  void ClearRelationships();

  std::shared_ptr<Canvas> GetMasterCanvas();

  const CanvasLocation& GetLocation(const Canvas* canvas) const;

  int GetWidth() const;
  int GetHeight() const;

  std::optional<std::size_t> RegisterLocatable(Locatable* locatable);

  NO_DISCARD Bitmap ToBitmap() const;

  void CalculateImage() const;

  void CalculateCanvasLocations() const;

  void CalculateCanvasCoordinates() const;

  const CoordinateDescription& GetCanvasCoordinateDescription(const std::shared_ptr<const Canvas>& canvas) const;

  void RegisterCanvas(const std::shared_ptr<Canvas>& canvas);

  Image MakeImage() const;

 protected:
  std::size_t getCanvasIndex(Canvas* canvas);

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
  std::shared_ptr<Canvas> master_canvas_ = nullptr;

  //! \brief A vector of all the canvas that are associated with this image.
  //!
  //! This will include the master_canvas_, its children, their children, etc.
  std::vector<std::shared_ptr<Canvas>> canvases_;

  std::vector<std::shared_ptr<Fix>> fixes_;

  //! \brief Locatable objects.
  IndexedLocatables locatables_;

  //! \brief What coordinate system width to use if there is no coordinate extent in the x or y dimensions.
  //! This can happen either because there is only one point that needs coordinates in x or y, or because, e.g. the
  //! x dimension has coordinate objects, but the y dimension does not. So a coordinate system is still used in the y
  //! dimension [NOTE(Nate): separate the ability to have coordinate systems for x and y?].
  double default_coordinate_epsilon = 0.0001;

  //! \brief Determines the sizes and locations of each canvas.
  mutable std::map<const Canvas*, CanvasLocation> canvas_locations_;

  //! \brief True whenever canvases and image parameters may need to be recalculated.
  mutable bool needs_calculate_ = false;

  //! \brief Store the width and heigh of the whole image, in pixels.
  //!
  //! Since the final image will (at least in e.g. the bitmap case) be in pixels, we store these as ints, not as doubles.
  mutable int width_ = 100, height_ = 100;
};

} // namespace gemini::core
