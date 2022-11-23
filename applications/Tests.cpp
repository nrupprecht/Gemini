//
// Created by Nathaniel Rupprecht on 9/16/22.
//

#include <iostream>
#include "gemini/plot/Figure.h"
#include "gemini/plot/renders/LinePlotRender.h"
#include "gemini/plot/renders/ScatterPlotRender.h"
#include "gemini/plot/renders/ErrorBarsRender.h"

#include <random>
#include <numbers>

#include <thread>
#include <new>

using namespace gemini::plot;

std::vector<double> Map(const std::function<double(double)>& f, double x0, double x1, std::size_t num_points = 100) {
  if (num_points == 0) {
    return {};
  }
  if (num_points == 1) {
    return { f(0.5 * (x0 + x1)) };
  }

  std::vector<double> output;

  double dx = (x1 - x0) / static_cast<double>(num_points - 1);
  for (std::size_t i = 0 ; i < num_points - 1 ; ++i) {
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
    return { 0.5 * (x0 + x1) };
  }

  std::vector<double> output;
  double dx = (x1 - x0) / static_cast<double>(num_points - 1);
  for (std::size_t i = 0 ; i < num_points - 1 ; ++i) {
    auto x = x0 + static_cast<double>(i) * dx;
    output.push_back(x);
  }
  output.push_back(x1);

  return output;
}

void test_1() {
  gemini::plot::Figure figure(1024, 1024);

  std::vector<double> x{ 0, 1 }, y1{ 0, 1 };
  std::vector<double> x2{ 0, 0.1, 0.2, 0.5, 0.9, 1.0 }, y2{ 2, 1.9, 1.7, 1.5, 1.3, 1.4 };

  auto& plt = figure.GetSubspace(0, 0).AsPlot();

  plt.AddRender(
      gemini::plot::renders::LinePlotRender()
          .XValues(x)
          .YValues(y1)
          .Label("My plot"));

  plt.AddRender(
      gemini::plot::renders::LinePlotRender()
          .XValues(x2)
          .YValues(y2)
          .Label("My second plot")
          .Color(gemini::core::color::Green)
  );

  gemini::core::Bitmap bmp;
  try {
    bmp = figure.ToBitmap();
  }
  catch (const std::exception& ex) {
    std::cout << "Exception in ToBitmap: " << ex.what() << std::endl;
  }
  try {
    bmp.ToFile("../../out/Test-1.bmp");
  }
  catch (const std::exception& ex) {
    std::cout << "Exception in ToFile: " << ex.what() << std::endl;
  }
}

void test_2() {
  gemini::plot::Figure figure(1024, 1024);

  constexpr auto PI = std::numbers::pi;

  int npoints = 100;
  auto x = Linspace(0, 2 * PI, npoints);
  auto y1 = Map([=](double x) { return std::sin(x); }, x);
  auto y2 = Map([=](double x) { return std::sin(x) * std::sin(x); }, x);
  auto y3 = Map([=](double x) { return std::cos(x) * std::cos(x); }, x);

  figure.SetSubSpaces(2, 1);
  auto& plt_left = figure.GetSubspace(0, 0).AsPlot();
  auto& plt_right = figure.GetSubspace(1, 0).AsPlot();

  plt_left.AddRender(
      gemini::plot::renders::LinePlotRender()
          .XValues(x)
          .YValues(y1)
          .Label("My plot")
          .Color(gemini::core::color::Red));

  plt_right.AddRender(
      gemini::plot::renders::LinePlotRender()
          .XValues(x)
          .YValues(y2)
          .Label("My second plot")
          .Color(gemini::core::color::Green)
  );

  plt_right.AddRender(
      gemini::plot::renders::LinePlotRender()
          .XValues(x)
          .YValues(y3)
          .Label("My third plot")
          .Color(gemini::core::color::Blue)
  );

  gemini::core::Bitmap bmp;
  try {
    bmp = figure.ToBitmap();
  }
  catch (const std::exception& ex) {
    std::cout << "Exception in ToBitmap: " << ex.what() << std::endl;
  }
  try {
    bmp.ToFile("../../out/Test-2.bmp");
  }
  catch (const std::exception& ex) {
    std::cout << "Exception in ToFile: " << ex.what() << std::endl;
  }
}

void test_3() {
  gemini::plot::Figure figure(2048, 2048);

  auto PI = 3.14159265;

  int n_points = 100;
  auto x = Linspace(0, 2 * PI, n_points);
  auto y1 = Map([=](double x) { return std::sin(x); }, x);
  auto y2 = Map([=](double x) { return std::sin(x) * std::sin(x); }, x);
  auto y3 = Map([=](double x) { return std::cos(x) * std::cos(x); }, x);

  std::vector<double> err(n_points, 0.1);

  std::mt19937 mt;
  std::uniform_real_distribution<double> dist;
  auto randomPoints = [&](int n) {
    std::vector<double> x, y;
    for (auto i = 0 ; i < n ; ++i) {
      x.push_back(dist(mt));
      y.push_back(dist(mt));
    }
    return std::pair(x, y);
  };

  auto[x4, y4] = randomPoints(n_points);
  auto[x5, y5] = randomPoints(n_points);
  auto[x6, y6] = randomPoints(n_points);

  figure.SetSubSpaces(3, 2);
  figure.SetSubSpaceRelativeSizes({1, 1, 2}, {1, 1});

  // figure.GetSubspace(1, 0).MakeFigure();
  auto& f0_0 = figure.GetOrMakePlot(0, 0);
  auto& f0_1 = figure.GetOrMakePlot(1, 0);
  auto& f0_2 = figure.GetOrMakePlot(2, 0);
  auto& f1_0 = figure.GetOrMakePlot(0, 1);
  auto& f1_1 = figure.GetOrMakePlot(1, 1);
  auto& f1_2 = figure.GetOrMakePlot(2, 1);

  gemini::plot::marker::Circle marker;
  marker.SetScale(10.);
  marker.SetColor(gemini::core::color::Blue);

  gemini::plot::marker::Point point;
  point.SetScale(15.);
  point.SetColor(gemini::core::color::Green);

  f0_0.AddRender(
      gemini::plot::renders::ScatterPlotRender()
          .XValues(x)
          .YValues(y1)
          .Label("My plot"));

  f0_1.AddRender(
      gemini::plot::renders::LinePlotRender()
          .XValues(x)
          .YValues(y2)
          .Label("My first right sub-plot")
          .Color(gemini::core::color::Green)
  );

  f0_2.AddRender(
      gemini::plot::renders::ErrorBarsRender()
          .XValues(x)
          .YValues(y3)
          .YErr(err)
          .Label("My second right sub-plot")
          .Color(gemini::core::color::Blue)
  );

  f1_0.AddRender(
      gemini::plot::renders::ScatterPlotRender()
          .XValues(x4)
          .YValues(y4)
          .Label("My third right sub-plot")
          .Markers(point.Copy()->SetColor(gemini::core::color::Black))
          .Color(gemini::core::color::Black)
  );

  f1_1.AddRender(
      gemini::plot::renders::ScatterPlotRender()
          .XValues(x5)
          .YValues(y5)
          .Label("My fourth right sub-plot")
          .Markers(point)
          .Color(gemini::core::color::PixelColor(200, 120, 15))
  );

  f1_2.AddRender(
      gemini::plot::renders::ScatterPlotRender()
          .Values(x6, y6)
          .Label("My fourth right sub-plot")
          .Markers(marker)
          .Color(gemini::core::color::PixelColor(155, 155, 155))
  );

  f1_2.SetXLabel("My very good label");

  gemini::core::Bitmap bmp;
  try {
    auto begin = std::chrono::high_resolution_clock::now();
    bmp = figure.ToBitmap();
    auto end = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "Image render time: " << dt.count() / 1.e6 << std::endl;
  }
  catch (const std::exception& ex) {
    std::cout << "Exception in ToBitmap: " << ex.what() << std::endl;
    throw;
  }
  try {
    auto begin = std::chrono::high_resolution_clock::now();
    bmp.ToFile("../../out/Test-3.bmp");
    auto end = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout << "Image write time: " << dt.count() / 1.e6 << std::endl;
  }
  catch (const std::exception& ex) {
    std::cout << "Exception in ToFile: " << ex.what() << std::endl;
    throw;
  }
}

int main(int argc, char** argv) {
  // test_1();
  // test_2();
  test_3();
  return 0;
}