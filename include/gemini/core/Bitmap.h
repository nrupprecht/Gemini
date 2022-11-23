//
// Created by Nathaniel Rupprecht on 11/26/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_BITMAP_H_
#define GEMINI_INCLUDE_GEMINI_BITMAP_H_

#include "gemini/export.hpp"
#include "Utility.h"
#include "EasyBMP/EasyBMP.h"

#include <random>

namespace gemini::core {

namespace color {

struct GEMINI_EXPORT PixelColor {
  constexpr PixelColor(int r, int g, int b, int a = 255)
      : red(r)
      , green(g)
      , blue(b)
      , alpha(a) {}

  unsigned char red{}, green{}, blue{}, alpha = 255;

  //! \brief Default comparison operator.
  friend auto operator<=>(const PixelColor&, const PixelColor&) = default;

  friend PixelColor operator*(double mult, const PixelColor& color) {
    return PixelColor{
        static_cast<unsigned char>(mult * color.red),
        static_cast<unsigned char>(mult * color.green),
        static_cast<unsigned char>(mult * color.blue),
        color.alpha};
  }

  friend PixelColor operator+(const PixelColor& lhs, const PixelColor& rhs) {
    return PixelColor{
        static_cast<unsigned char>(lhs.red + rhs.red),
        static_cast<unsigned char>(lhs.green + rhs.green),
        static_cast<unsigned char>(lhs.blue + rhs.blue),
        255};
  }
};

constexpr GEMINI_EXPORT PixelColor Red{255, 000, 000, 255};
constexpr GEMINI_EXPORT PixelColor Green{000, 255, 000, 255};
constexpr GEMINI_EXPORT PixelColor Blue{000, 000, 255, 255};
constexpr GEMINI_EXPORT PixelColor Black{000, 000, 000, 255};
constexpr GEMINI_EXPORT PixelColor White{255, 255, 255, 255};

GEMINI_EXPORT PixelColor RandomUniformColor();

PixelColor Interpolate(const PixelColor& base, const PixelColor& other, double mult);

} // namespace color


enum class ZOverwriteType {
  Greater,
  GreaterOrEqual,
};

class GEMINI_EXPORT Bitmap {
 public:
  class Impl;

  Bitmap();
  Bitmap(int width, int height);

  //! \brief Create a bitmap from an implementation.
  explicit Bitmap(std::unique_ptr<Impl> impl);

  //! \brief Set the size of the image. Also resets the permitted region.
  void SetSize(int width, int height);

  void SetPixel(int x, int y, const color::PixelColor& color, double z = 0);
  NO_DISCARD color::PixelColor GetPixel(int x, int y) const;

  //! \brief Write a bitmap to a file.
  void ToFile(const std::string& filepath) const;

  //! \brief Set the (half-open) bitmap permission region, [xlow, xhi) x [ylow, yhi).
  void SetPermittedRegion(int xlow, int xhi, int ylow, int yhi);

  //! \brief Set the restrict region flag.
  void SetRestrictRegion(bool r);

  NO_DISCARD unsigned int GetHeight() const;
  NO_DISCARD unsigned int GetWidth() const;
 private:
  std::unique_ptr<Impl> impl_;
};

//! \brief Implementation class for Bitmaps.
class Bitmap::Impl {
 public:
  //! \brief Construct a default image with zero width and height.
  Impl() = default;

  Impl(int width, int height)
      : width_(width)
      , height_(height)
      , pxhi_(width)
      , pyhi_(height) {}

  virtual ~Impl() = default;

  //! \brief Set a pixel in the bitmap. Only occurs if setting within the permitted region.
  //!
  //! \param x X position of the pixel, relative to the permitted region.
  //! \param y Y position of the pixel, relative to the permitted region.
  //! \param color The color to write the pixel.
  //! \param z The ordering of the pixel. Higher z-values overwrite lower z-values. If the z-value ties with the current
  //!             z-value, the behavior is determined by the z-overwrite type.
  void SetPixel(int x, int y, const color::PixelColor& color, double z);
  void SetSize(int width, int height);

  //! \brief Set the (half-open) bitmap permission region, [xlow, xhi) x [ylow, yhi).
  void SetPermittedRegion(int xlow, int xhi, int ylow, int yhi);

  //! \brief Set the restrict region flag.
  void SetRestrictRegion(bool r);

  NO_DISCARD virtual unsigned int GetHeight() const = 0;
  NO_DISCARD virtual unsigned int GetWidth() const = 0;

  NO_DISCARD virtual color::PixelColor GetPixel(int x, int y) const = 0;
  virtual void ToFile(const std::string& filepath) = 0;

 protected:
  //! \brief Set a pixel anywhere on the bitmap. This is only called if the pixel is in the permitted region.
  virtual void setPixel(int x, int y, const color::PixelColor& color, double z) = 0;

  //! \brief Set the size of the image. The permission region has already been reset when this is called.
  virtual void setSize(int width, int height) = 0;

  //! \brief Stores the width and height of the image.
  int width_ = 0, height_ = 0;

  //! \brief Encodes the permission region.
  int pxlow_ = 0, pxhi_ = 0, pylow_ = 0, pyhi_ = 0;

  //! \brief If true, only the permitted region can be written to .
  bool restrict_region_ = false;

  //! \brief Whether or not a tie in z-level results in a pixel being overwritten.
  ZOverwriteType overwrite_type_ = ZOverwriteType::GreaterOrEqual;
};

//! A bitmap impl that uses EasyBMP as the bitmap image.
//!
//! This is the default implementation used by the Bitmap object.
class EasyBMPImpl : public Bitmap::Impl {
 public:
  //! \brief Construct a default image with zero width and height.
  EasyBMPImpl() = default;

  EasyBMPImpl(int width, int height)
      : Impl(width, height) {
    bitmap_.SetSize(width, height);
    zarray_.reserve(width * height);
    std::fill(zarray_.begin(), zarray_.end(), std::numeric_limits<double>::quiet_NaN());
  }

  NO_DISCARD color::PixelColor GetPixel(int x, int y) const override;
  void ToFile(const std::string& filepath) override;

  NO_DISCARD unsigned int GetHeight() const override;
  NO_DISCARD unsigned int GetWidth() const override;

 private:
  void setPixel(int x, int y, const color::PixelColor& color, double z) override;
  void setSize(int width, int height) override;

  //! \brief Array of pixel precedences. New writes must have z value greater than (or equal to depending on the
  //! overwrite type) this value to overwrite the pixel.
  std::vector<double> zarray_;
  BMP bitmap_;
};

} // namespace gemini

#endif //GEMINI_INCLUDE_GEMINI_BITMAP_H_
