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

std::vector<double> Map(const std::function<double(double)>& f, double x0, double x1, std::size_t num_points = 100) {
  if (num_points == 0) {
    return {};
  }
  if (num_points == 1) {
    return {f(0.5 * (x0 + x1))};
  }

  std::vector<double> output;

  double dx = (x1 - x0) / static_cast<double>(num_points - 1);
  for (std::size_t i = 0; i < num_points - 1; ++i) {
    auto x = x0 + static_cast<double>(i) * dx;
    output.push_back(f(x));
  }
  output.push_back(f(x1));

  return output;
}

std::vector<double> Map(const std::function<double(double)>& f, const std::vector<double>& x) {
  std::vector<double> output(x.size());
  std::transform(x.begin(), x.end(), output.begin(), f);
  return output;
}

std::vector<double> Linspace(double x0, double x1, std::size_t num_points) {
  if (num_points == 0) {
    return {};
  }
  if (num_points == 1) {
    return {0.5 * (x0 + x1)};
  }

  std::vector<double> output;
  double dx = (x1 - x0) / static_cast<double>(num_points - 1);
  for (std::size_t i = 0; i < num_points - 1; ++i) {
    auto x = x0 + static_cast<double>(i) * dx;
    output.push_back(x);
  }
  output.push_back(x1);

  return output;
}

int main(int argc, char** argv) {
  gemini::plot::Figure figure(2048, 1024);

  auto PI = 3.14159265;

  int npoints = 100;

  auto x = Linspace(0, 2 * PI, npoints);
  auto y1 = Map([=](double x) { return std::sin(x);}, x);
  auto y2 = Map([=](double x) { return std::sin(x - 0.1 * PI);}, x);
  auto y3 = Map([=](double x) { return std::sin(x - 0.2 * PI);}, x);
  auto y4 = Map([=](double x) { return std::sin(x - 0.3 * PI);}, x);
  auto y5 = Map([=](double x) { return std::sin(x - 0.4 * PI);}, x);
  auto y6 = Map([=](double x) { return std::sin(x - 0.5 * PI);}, x);
  auto y7 = Map([=](double x) { return std::sin(x - 0.6 * PI);}, x);
  auto y8 = Map([=](double x) { return std::sin(x - 0.7 * PI);}, x);
  std::vector<double> err(npoints, 0.1);

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