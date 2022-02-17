//
// Created by Nathaniel Rupprecht on 1/30/22.
//

#include "gemini/text/TextBox.h"

using namespace gemini;
using namespace gemini::core;
using namespace gemini::text;

TextBox::TextBox(std::shared_ptr<TrueTypeFontEngine> ttf)
    : ttf_(std::move(ttf)) {
  // By default, a text box can write anywhere.
  restricted_ = false;
}

void TextBox::AddText(const std::string& text) {
  text_ += text;
}

void TextBox::SetFontSize(double font_size) {
  font_size_ = font_size;
}

void TextBox::SetAnchor(Point anchor_point) {
  anchor_point_ = anchor_point;
}

void TextBox::SetAngle(double theta) {
  angle_ = theta;
}

CoordinateBoundingBox TextBox::GetBoundingBox() const {
  // TODO: Should we cache this?
  auto nan = std::numeric_limits<double>::quiet_NaN();
  return { nan, nan, nan, nan };
}

void TextBox::drawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  GEMINI_REQUIRE(0 < font_size_, "font size must be > 0");

  ttf_->SetFontSize(static_cast<PointSize>(font_size_));

  auto anchor = canvas->PointToPixels(anchor_point_);
  double dx = 0, dy = 0;
  for (auto c: text_) {
    auto spacing = ttf_->GetSpacing(c);

    // Prepare the character to be written.
    ttf_->PrepareCharacter(c);
    // Get the character's spline and modify it, to put the character in the right position, apply rotation, skew, etc.
    auto& spline = ttf_->GetCharacter();
    // Translate to the correct place relative to the text box.
    spline.Translate(dx, dy);
    // Rotate.
    if (angle_ != 0) {
      spline.Rotate(angle_);
    }
    // Translate to the correct position.
    spline.Translate(std::floor(anchor.x), std::floor(anchor.y));

    // Tell the engine to write the character to the bitmap.
    ttf_->WriteCharacter(bitmap, color::Black, zorder_);

    dx += spacing.advance;
  }
}

CoordinateBoundingBox TextBox::calculatePixelsBoundingBox() const {
  GEMINI_REQUIRE(0 < font_size_, "font size must be > 0");

  ttf_->SetFontSize(static_cast<PointSize>(font_size_));

  // NOTE: Assuming angle = 0.

  double dx = 0, dy = 0;
  double left = std::numeric_limits<double>::quiet_NaN(), right = left, bottom = left, top = left;
  for (auto c: text_) {
    auto spacing = ttf_->GetSpacing(c);

    double minx = dx - spacing.xmin, maxx = dx - spacing.xmin + spacing.width;
    double miny = dy - spacing.ymin, maxy = dy - spacing.ymin + spacing.height;

    if (std::isnan(left) || minx < left) {
      left = minx;
    }
    if (std::isnan(right) || right < maxx) {
      right = maxx;
    }

    if (std::isnan(bottom) || miny < bottom) {
      bottom = miny;
    }
    if (std::isnan(top) || top < maxy) {
      top = maxy;
    }

    dx += spacing.advance;
  }

  GeometricPoint bl{left, bottom}, br{right, bottom}, tl{left, top}, tr{right, top};
  bl = Rotate(bl, angle_), br = Rotate(br, angle_), tl = Rotate(tl, angle_), tr = Rotate(tr, angle_);

  // Bounding in pixels.
  left   = std::min({bl.x, br.x, tl.x, tr.x});
  right  = std::max({bl.x, br.x, tl.x, tr.x});
  bottom = std::min({bl.y, br.y, tl.y, tr.y});
  top    = std::max({bl.y, br.y, tl.y, tr.y});

  return CoordinateBoundingBox{left, right, bottom, top};
}