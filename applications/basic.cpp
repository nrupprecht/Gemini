//
// Created by Nathaniel Rupprecht on 11/20/21.
//

#include <iostream>
#include "gemini/plot/Plot.h"
#include "gemini/text/TrueTypeFontEngine.h"
#include "gemini/text/TextBox.h"

#include <thread>
#include <new>

std::string interpolateName(const std::string &a, const std::string &b, double lambda) {
  if (lambda < 0 || 1 < lambda) {
    throw std::runtime_error("lambda must be in the range [0, 1]");
  }
  // Since the names might now be the same length, we have to interpolate their length as well.
  auto name_length = static_cast<std::size_t>(std::round(lambda * a.size() + (1. - lambda) * b.size()));
  std::string final_name;
  std::size_t i = 0;
  for (; i < std::min(a.size(), b.size()); ++i) {
    auto interp_char = lambda * std::tolower(a[i]) + (1. - lambda) * std::tolower(b[i]);
    final_name.push_back(static_cast<char>(std::round(interp_char)));
  }
  auto &longer_name = a.size() < b.size() ? b : a;
  for (; i < name_length; ++i) {
    final_name.push_back(longer_name[i]);
  }
  return final_name;
}

int main(int argc, char **argv) {

  // Read font description.
  auto true_type = std::make_shared<gemini::text::TrueType>();
  true_type->ReadTTF("/Users/nathaniel/Documents/times.ttf");
  // true_type->ReadTTF("/Users/nathaniel/Documents/NotoSansSC-Black.otf");

  auto engine = std::make_shared<gemini::text::TrueTypeFontEngine>(true_type, 20, 250);

//  gemini::Image image(3000, 3000);
//  auto canvas = image.GetMasterCanvas();
//
//  auto text = std::make_shared<gemini::text::TextBox>(engine);
//  text->SetAnchor(gemini::MakeRelativePoint(0., 0.5));
//  text->AddText("To all those haters out there, I've just got to say, I don't have any time for you!");
//  canvas->AddShape(text);
//  auto tbmp = image.ToBitmap();
//  tbmp.ToFile("../../out/TextTest.bmp");


  gemini::plot::Figure figure(2048, 1024);

  int npoints = 100;
  std::vector<double> x(npoints);
  std::vector<double> y1(npoints), y2(npoints), y3(npoints), y4(npoints), y5(npoints), y6(npoints), y7(npoints),
      y8(npoints);
  std::vector<double> err(npoints);

  auto PI = 3.14159265;
  for (int i = 0; i < npoints; ++i) {
    x[i] = 2. * PI / npoints * i;
    y1[i] = std::sin(x[i]);
    y2[i] = std::sin(x[i] - 0.1 * PI);
    y3[i] = std::sin(x[i] - 0.2 * PI);
    y4[i] = std::sin(x[i] - 0.3 * PI);
    y5[i] = std::sin(x[i] - 0.4 * PI);
    y6[i] = std::sin(x[i] - 0.5 * PI);
    y7[i] = std::sin(x[i] - 0.6 * PI);
    y8[i] = std::sin(x[i] - 0.7 * PI);

    err[i] = 0.1;
  }
  figure.Plot(x, y1);
  figure.Plot(x, y2);
  figure.Plot(x, y3);
  figure.Plot(x, y4);
  figure.Plot(x, y5);
  figure.Plot(x, y6);
  figure.PlotErrorbars(x, y7, err);
  figure.Scatter(x, y8);

//  auto& image = figure.GetImage();
//  auto text = std::make_shared<gemini::text::TextBox>(engine);
//  text->SetFontSize(10);
//  // text->SetAngle(gemini::math::PI / 2);
//  text->SetAnchor(gemini::MakeRelativePoint(0., 0.10));
//  text->AddText("To all those haters out there, I've just got to say, I don't have any time for you!");
//  text->SetZOrder(5);
//  image.GetMasterCanvas()->AddShape(text);

  figure.ToFile("../../out/figure.bmp");

  return 0;
}