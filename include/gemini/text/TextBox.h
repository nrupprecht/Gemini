//
// Created by Nathaniel Rupprecht on 12/12/21.
//

#ifndef GEMINI_INCLUDE_GEMINI_TEXTBOX_H_
#define GEMINI_INCLUDE_GEMINI_TEXTBOX_H_

#include <utility>

#include "gemini/text/TrueTypeFontEngine.h"
#include "gemini/core/Bitmap.h"
#include "gemini/core/Canvas.h"
#include "gemini/core/shapes/Shapes.h"

namespace gemini::text {

 class GEMINI_EXPORT TextBox : public gemini::core::Shape {
 public:

  explicit TextBox(std::shared_ptr<TrueTypeFontEngine> ttf);

  void AddText(const std::string& text);

  void SetFontSize(double font_size);

  void SetAnchor(Point anchor_point);

  void SetAngle(double theta);

  NO_DISCARD CoordinateBoundingBox GetBoundingBox() const override;

  void DrawOnBitmap(gemini::core::Bitmap& bitmap, const gemini::core::Canvas* canvas) const override;

 private:

  //! \brief The TrueType font this text box uses.
  std::shared_ptr<TrueTypeFontEngine> ttf_;

  //! \brief The font size (in pixels) of the font.
  double font_size_ = 12.;

  //! \brief The angle at which the text box should be oriented.
  double angle_ = 0.;

  //! \brief The bottom left corner of the text box.
  Point anchor_point_{};

  //! \brief The text that the text box should render.
  std::string text_;
};

}
#endif //GEMINI_INCLUDE_GEMINI_TEXTBOX_H_
