//
// Created by Nathaniel Rupprecht on 1/22/22.
//

#include "gemini/text/TrueTypeFontEngine.h"

using namespace gemini;
using namespace gemini::core;
using namespace gemini::text;
using namespace gemini::core::shapes;

TrueTypeFontEngine::TrueTypeFontEngine(std::shared_ptr<TrueType> font, PointSize point_size, unsigned int resolution)
    : font_(std::move(font)), point_size_(point_size), resolution_(resolution) {
  // Find a default glyph map to use.
  int index = 0;
  for (auto&[platform, encoding, offset]: font_->cmap_table_.encoding_records) {
    if (!font_->cmap_table_.glyph_index_map[index].empty() &&
        ((platform == 0 && 0 <= encoding && encoding < 5)
            || (platform == 3 && (encoding == 0 || encoding == 1 || encoding == 10)))) {
      platform_id_ = platform;
      encoding_id_ = encoding;
      glyph_map = &font_->cmap_table_.glyph_index_map[index];
      break;
    }
    ++index;
  }

  // Execute font engine program (in FPGM table).

}

Bitmap TrueTypeFontEngine::MakeCharacter(uint16 char_number) const {
  auto glyph_index = getGlyphIndex(char_number);
  auto& glyph = font_->glyf_table_.entries.at(glyph_index);
  auto& spacing = font_->spacing_map_.at(glyph_index);

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

void TrueTypeFontEngine::WriteCharacter(
    uint16 char_number,
    Bitmap& bmp,
    int16 x,
    int16 y,
    bool shift_is_pixels) const {
  PrepareCharacter(char_number);
  if (shift_is_pixels) {
    spline_.ShiftScaled(GetScale(), x, y);
  }
  else {
    spline_.ScaleShifted(GetScale(), x, y);
  }
  WriteCharacter(bmp, color::Black);
}

double TrueTypeFontEngine::GetScale() const {
  return point_size_ * resolution_ / (72. * font_->head_table_.units_per_em);
}

SpacingInfo TrueTypeFontEngine::GetSpacing(uint16 char_number) const {
  auto glyph_index = getGlyphIndex(char_number);
  auto spacing = font_->GetSpacing(glyph_index);

  double scale = GetScale();
  spacing.xmin *= scale;
  spacing.ymin *= scale;
  spacing.width *= scale;
  spacing.height *= scale;
  spacing.lsb *= scale;
  spacing.rsb *= scale;
  spacing.advance *= scale;

  return spacing;
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
  RasterBezierCurve(spline_, bmp, color, z);
}

uint16 TrueTypeFontEngine::getGlyphIndex(uint16 char_number) const {
  GEMINI_REQUIRE(glyph_map, "glyph map is not set in TrueTypeFontEngine");
  if (auto gt = glyph_map->find(char_number); gt != glyph_map->end()) {
    return gt->second;
  }
  return 0;
}

const TrueType::GlyphData& TrueTypeFontEngine::getGlyph(uint16 char_number) const {
  GEMINI_REQUIRE(glyph_map, "glyph map is not set in TrueTypeFontEngine");
  if (auto gt = glyph_map->find(char_number); gt != glyph_map->end()) {
    auto glyph_index = gt->second;
    return font_->glyf_table_.entries.at(glyph_index);
  }
  // Return the zero glyph.
  return font_->glyf_table_.entries[0];
}