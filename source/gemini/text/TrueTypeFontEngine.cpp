//
// Created by Nathaniel Rupprecht on 1/22/22.
//

#include "gemini/text/TrueTypeFontEngine.h"

using namespace gemini;
using namespace gemini::text;

TrueTypeFontEngine::TrueTypeFontEngine(std::shared_ptr<TrueType> font, PointSize point_size, unsigned int resolution)
    : font_(std::move(font)), point_size_(point_size), resolution_(resolution) {
  // Execute font engine program (in FPGM table).

}

Bitmap TrueTypeFontEngine::MakeCharacter(uint16 char_number) const {
  auto glyph_index = getGlyphIndex(char_number);
  auto &glyph = font_->glyf_table_.entries.at(glyph_index);
  auto &spacing = font_->spacing_map_.at(glyph_index);

  auto width = spacing.width, height = spacing.height;
  auto xmin = glyph.xmin, ymin = glyph.ymin;

  // Calculate scale.
  double scale = GetScale();

  int pixels_width = static_cast<int>(std::ceil(scale * width)) + 1;
  int pixels_height = static_cast<int>(std::ceil(scale * height)) + 1;
  Bitmap bmp(pixels_width, pixels_height);

  WriteCharacter(char_number, bmp, int16_t(-xmin), int16_t(-ymin), false);

  return bmp;
}

void TrueTypeFontEngine::WriteCharacter(uint16 char_number,
                                        Bitmap &bmp,
                                        int16 x,
                                        int16 y,
                                        bool shift_is_pixels) const {
  PrepareCharacter(char_number);
  if (shift_is_pixels) {
    spline_.ShiftScaled(GetScale(), x, y);
  } else {
    spline_.ScaleShifted(GetScale(), x, y);
  }
  WriteCharacter(bmp, color::Black);
}

double TrueTypeFontEngine::GetScale() const {
  return point_size_ * resolution_ / (72. * font_->head_table_.units_per_em);
}

std::tuple<uint16, uint16, uint16> TrueTypeFontEngine::GetSpacing(uint16 char_number) const {
  auto glyph_index = getGlyphIndex(char_number);
  const auto&[x, y, width, height, lsb, rsb, advance] = font_->GetSpacing(glyph_index);

  double scale = GetScale();
  return {scale * width, scale * height, scale * advance};
}

void TrueTypeFontEngine::SetFontSize(PointSize point) {
  point_size_ = point;
}

void TrueTypeFontEngine::SetResolution(unsigned int resolution) {
  resolution_ = resolution;
}

void TrueTypeFontEngine::PrepareCharacter(uint16 char_number) const {
  auto glyph_index = getGlyphIndex(char_number);
  spline_ = font_->glyf_table_.entries.at(glyph_index).spline.Copy();

  spline_.Scale(GetScale());
}

shapes::BezierCurve& TrueTypeFontEngine::GetCharacter() const {
  return spline_;
}

void TrueTypeFontEngine::WriteCharacter(Bitmap& bmp, const color::PixelColor& color, double z) const {
  gemini::shapes::RasterBezierCurve(spline_, bmp, color, z);
}

uint16 TrueTypeFontEngine::getGlyphIndex(uint16 char_number) const {
  if (auto gt = font_->cmap_table_.glyph_index_map.find(char_number); gt != font_->cmap_table_.glyph_index_map.end()) {
    return gt->second;
  }
  return 0;
}

const TrueType::GlyphData &TrueTypeFontEngine::getGlyph(uint16 char_number) const {
  if (auto gt = font_->cmap_table_.glyph_index_map.find(char_number); gt != font_->cmap_table_.glyph_index_map.end()) {
    auto glyph_index = gt->second;
    return font_->glyf_table_.entries.at(glyph_index);
  }
  // Return the zero glyph.
  return font_->glyf_table_.entries[0];
}