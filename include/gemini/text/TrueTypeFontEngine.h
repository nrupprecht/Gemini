//
// Created by Nathaniel Rupprecht on 1/22/22.
//

#ifndef GEMINI_INCLUDE_GEMINI_TEXT_TRUETYPEFONTENGINE_H_
#define GEMINI_INCLUDE_GEMINI_TEXT_TRUETYPEFONTENGINE_H_

#include "gemini/text/TrueTypeReader.h"
#include "gemini/core/Bitmap.h"

namespace gemini::text {

using PointSize = unsigned int;

//! \brief A TrueType font engine.
//!
//! 1) The master outline description of the glyph is scaled to the appropriate size.
//! 2) The scaled outline is grid-fitted according to its associated instructions.
//! 3) The grid-fitted outline is scan converted to produce a bitmap image suitable for raster display.
//!
//! *   https://developer.apple.com/fonts/TrueType-Reference-Manual/RM02/Chap2.html
class GEMINI_EXPORT TrueTypeFontEngine {
 public:
  explicit TrueTypeFontEngine(std::shared_ptr<TrueType> font, PointSize point_size, unsigned int resolution);

  NO_DISCARD core::Bitmap MakeCharacter(uint16 char_number) const;

  void WriteCharacter(uint16 char_number, core::Bitmap& bmp, int16 x, int16 y, bool shift_is_pixels = true) const;

  NO_DISCARD double GetScale() const;

  NO_DISCARD SpacingInfo GetSpacing(uint16 char_number) const;

  void SetFontSize(PointSize point);

  void SetResolution(unsigned int resolution);

  //! \brief Prepare a character to be written to a bitmap. Loads the spline of the requested character, allowing for
  //! it to be retrieved and modified via the GetCharacter() command. When loaded, it will be in its default
  //! (origin-centered) coordinate system.
  void PrepareCharacter(uint16 char_number) const;

  //! \brief Get the spline of the character that will be written with WriteCharacter.
  NO_DISCARD core::shapes::BezierCurve& GetCharacter() const;

  //! \brief Write the character to a bitmap.
  void WriteCharacter(core::Bitmap& bmp, const core::color::PixelColor& color, double z = 0.) const;

 private:

  //! \brief Get the glyph index of a unicode character. The glyph index is how the TrueType font internally references
  //! glyphs.
  NO_DISCARD uint16 getGlyphIndex(uint16 char_number) const;

  //! \brief Get a glyph by character number.
  NO_DISCARD const TrueType::GlyphData& getGlyph(uint16 char_number) const;

  //! \brief The spline of the character being prepared.
  mutable core::shapes::BezierCurve spline_;

  //! \brief The true type font that this engine runs.
  std::shared_ptr<TrueType> font_;

  //! \brief Font point size, e.g. 12 point font.
  PointSize point_size_;

  //! \brief The application resolution.
  unsigned int resolution_;

  uint16 platform_id_;
  uint16 encoding_id_;
  //! \brief The glyph map that should be used, based on the selected platform/encoding ID.
  std::map<uint16, uint16>* glyph_map = nullptr;
};

}
#endif //GEMINI_INCLUDE_GEMINI_TEXT_TRUETYPEFONTENGINE_H_
