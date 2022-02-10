//
// Created by Nathaniel Rupprecht on 1/30/22.
//

#include "gemini/text/TextBox.h"

using namespace gemini;
using namespace gemini::core;
using namespace gemini::text;


TextBox::TextBox(std::shared_ptr<TrueTypeFontEngine> ttf)
    : ttf_(std::move(ttf)) {}

void TextBox::AddText(const std::string &text) {
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
  return CoordinateBoundingBox{};
}

void TextBox::DrawOnBitmap(Bitmap &bitmap, const Canvas *canvas) const {
  GEMINI_REQUIRE(0 < font_size_, "font size must be > 0");

  ttf_->SetFontSize(static_cast<PointSize>(font_size_));

  auto anchor = canvas->PointToPixels(anchor_point_);
  double position_x = anchor.x, position_y = anchor.y;
  for (auto c: text_) {
    auto[width, height, advance] = ttf_->GetSpacing(c);

    // Prepare the character to be written.
    ttf_->PrepareCharacter(c);
    // Get the character's spline and modify it, to put the character in the right position, apply rotation, skew, etc.
    auto &spline = ttf_->GetCharacter();
    spline.Translate(std::floor(position_x), std::floor(position_y));
    if (angle_ != 0) {
      spline.Rotate(angle_);
    }
    // Tell the engine to write the character to the bitmap.
    ttf_->WriteCharacter(bitmap, color::Black, zorder_);

    position_x += advance;
  }
}