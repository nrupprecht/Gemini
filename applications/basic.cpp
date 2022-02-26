//
// Created by Nathaniel Rupprecht on 11/20/21.
//

#include <iostream>
#include "gemini/plot/Plot.h"
#include "gemini/text/TrueTypeFontEngine.h"
#include "gemini/text/TextBox.h"

#include <thread>
#include <new>

using namespace gemini::plot;

int main(int argc, char** argv) {
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
  figure.Plot(x, y1, "First plot");
  figure.Plot(x, y2);
  figure.Plot(x, y3, "Third plot");
  figure.Plot(x, y4);
  figure.Plot(x, y5);
  figure.Plot(x, y6);
  figure.PlotErrorbars(x, y7, err, "Error bars");

  {
    gemini::plot::ScatterPlotOptions options{};
    options.marker = std::make_shared<gemini::plot::marker::Circle>();
    options.marker->SetScale(10.);
    options.Color(gemini::core::color::Blue);
    figure.Scatter(x, y8, options.Label("Scatter!"));
  }

  figure.XLabel("My x axis");
  figure.YLabel("And a y-axis");

  figure.Title("Big Sample Graph");
  figure.ToFile("../../out/figure.bmp");

  return 0;
}