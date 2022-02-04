//
// Created by Nathaniel Rupprecht on 11/26/21.
//

#include "gemini/core/Bitmap.h"

using namespace gemini;
using namespace gemini::color;


namespace {

std::uniform_int_distribution<unsigned char> color_distribution;
std::mt19937 generator;

}

GEMINI_EXPORT PixelColor color::RandomUniformColor() {
  return {
      color_distribution(generator),
      color_distribution(generator),
      color_distribution(generator)
  };
}

PixelColor gemini::color::Interpolate(const PixelColor &base, const PixelColor &other, double mult) {
  return (1. - mult) * base + mult * other;
}

Bitmap::Bitmap()
    : impl_(std::make_unique<EasyBMPImpl>()) {}

Bitmap::Bitmap(int width, int height)
    : impl_(std::make_unique<EasyBMPImpl>(width, height)) {}

Bitmap::Bitmap(std::unique_ptr<Bitmap::Impl> impl)
    : impl_(std::move(impl)) {}

void Bitmap::SetSize(int width, int height) {
  impl_->SetSize(width, height);
}

void Bitmap::SetPixel(int x, int y, const color::PixelColor &color, double z) {
  impl_->SetPixel(x, y, color, z);
}

NO_DISCARD color::PixelColor Bitmap::GetPixel(int x, int y) const {
  return impl_->GetPixel(x, y);
}

void Bitmap::ToFile(const std::string &filepath) const {
  return impl_->ToFile(filepath);
}

void Bitmap::SetPermittedRegion(int xlow, int xhi, int ylow, int yhi) {
  impl_->SetPermittedRegion(xlow, xhi, ylow, yhi);
}

unsigned int Bitmap::GetHeight() const {
  return impl_->GetHeight();
}

unsigned int Bitmap::GetWidth() const {
  return impl_->GetWidth();
}

void Bitmap::Impl::SetPixel(int x, int y, const color::PixelColor &color, double z) {
  if (pxlow_ <= x && x < pxhi_ && pylow_ <= y && y < pyhi_) {
    setPixel(x, y, color, z);
  }
}

void Bitmap::Impl::SetSize(int width, int height) {
  pxlow_ = 0, pxhi_ = width, pylow_ = 0, pyhi_ = height;
  setSize(width, height);
}

void Bitmap::Impl::SetPermittedRegion(int xlow, int xhi, int ylow, int yhi) {
  pxlow_ = std::max(0, xlow), pxhi_ = std::min(width_, xhi), pylow_ = std::min(0, ylow), pyhi_ = std::min(height_, yhi);
}

void EasyBMPImpl::setSize(int width, int height) {
  bitmap_.SetSize(width, height);
  zarray_.resize(width * height);
  // NaN denotes that we always overwrite, regardless of the z value.
  std::fill(zarray_.begin(), zarray_.end(), std::numeric_limits<double>::quiet_NaN());
}

color::PixelColor EasyBMPImpl::GetPixel(int x, int y) const {
  auto width = bitmap_.TellWidth();
  auto height = bitmap_.TellHeight();
  if (0 <= x && x < width && 0 <= y && y < height) {
    auto rgba = bitmap_.GetPixel(x, y);
    return {rgba.Red, rgba.Green, rgba.Blue, rgba.Alpha};
  }
  return color::Black;
}

void EasyBMPImpl::ToFile(const std::string &filepath) {
  bitmap_.WriteToFile(filepath.c_str());
}

void EasyBMPImpl::setPixel(int x, int y, const color::PixelColor &color, double z) {
  // Look up the current z-value.
  auto& zvalue = zarray_[y * width_ + x];
  if (std::isnan(zvalue) || zvalue < z || (zvalue == z && overwrite_type_ == ZOverwriteType::GreaterOrEqual)) {
    bitmap_.SetPixel(x, height_ - y - 1, RGBApixel{color.red, color.green, color.blue});
    zvalue = z;
  }
}

unsigned int EasyBMPImpl::GetWidth() const {
  return bitmap_.TellWidth();
}

unsigned int EasyBMPImpl::GetHeight() const {
  return bitmap_.TellHeight();
}