//
// Created by Nathaniel Rupprecht on 11/28/21.
//

#include "gemini/core/shapes/Shapes.h"
// Other files.
#include "gemini/core/Canvas.h"

using namespace gemini;
using namespace gemini::core;

void Shape::write(Bitmap& bitmap, int x, int y, color::PixelColor color) const {
  bitmap.SetPixel(x, y, color, zorder_);
}

void Shape::SetZOrder(double z) {
  zorder_ = z;
}

CoordinateBoundingBox Line::GetBoundingBox() const {
  // Check if any point uses coordinates.
  double min_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double min_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_y = std::numeric_limits<double>::quiet_NaN();

  // First point, X coordinate.
  if (first.type_x == LocationType::Coordinate) {
    if (std::isnan(min_coordinate_x) || first.x < min_coordinate_x) {
      min_coordinate_x = first.x;
    }
  }
  // Second point, max X coordinate.
  if (second.type_x == LocationType::Coordinate) {
    if (std::isnan(max_coordinate_x) || max_coordinate_x < second.x) {
      max_coordinate_x = second.x;
    }
  }

  // First point, min Y coordinate.
  if (first.type_y == LocationType::Coordinate) {
    if (std::isnan(min_coordinate_y) || first.y < min_coordinate_y) {
      min_coordinate_y = first.y;
    }
  }
  // Second point, max Y coordinate.
  if (second.type_y == LocationType::Coordinate) {
    if (std::isnan(max_coordinate_y) || max_coordinate_y < second.y) {
      max_coordinate_y = second.y;
    }
  }

  return { min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y };
}

void BresenhamLine::DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  auto absolute_first = canvas->PointToPixels(first);
  auto absolute_second = canvas->PointToPixels(second);

  auto& left_point = absolute_first.x < absolute_second.x ? absolute_first : absolute_second;
  auto& right_point = absolute_first.x < absolute_second.x ? absolute_second : absolute_first;

  // Just y = m x + b.
  auto m = (right_point.y - left_point.y) / (right_point.x - left_point.x);
  auto b = right_point.y - m * right_point.x;
  for (int x = std::floor(left_point.x); x < std::ceil(right_point.x); ++x) {
    auto yi = m * x + b, yf = m * (x + 1) + b;
    for (int y = std::floor(yi); y <= std::ceil(yf); ++y) {
      bitmap.SetPixel(x, y, color);
    }
  }
}

void XiaolinWuLine::DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  auto absolute_first = canvas->PointToPixels(first);
  auto absolute_second = canvas->PointToPixels(second);

  double x0 = absolute_first.x, y0 = absolute_first.y;
  double x1 = absolute_second.x, y1 = absolute_second.y;

  auto& background_color = canvas->GetBackgroundColor();

  auto fpart = [](double x) { return x - std::floor(x); };
  auto rfpart = [&fpart](double x) { return 1. - fpart(x); };
  auto plot = [&](int x, int y, double mult) {
    write(bitmap, x, y, color::Interpolate(background_color, color, mult));
  };

  bool is_steep = std::abs(x1 - x0) < std::abs(y1 - y0);

  if (is_steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }

  if (x1 < x0) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  double dx = x1 - x0;
  double dy = y1 - y0;
  double gradient = dx == 0. ? 1.0 : dy / dx;

  // Handle first endpoint.
  double xend = std::round(x0);
  double yend = y0 + gradient * (xend - x0);
  double xgap = rfpart(x0 + 0.5);
  int xpxl1 = static_cast<int>(xend); // Used in the main loop.
  int ypxl1 = static_cast<int>(std::floor(yend));

  if (is_steep) {
    plot(ypxl1, xpxl1, rfpart(yend) * xgap);
    plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap);
  }
  else {
    plot(xpxl1, ypxl1, rfpart(yend) * xgap);
    plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
  }
  double intery = yend + gradient;

  // Handle second endpoint.
  xend = std::round(x1);
  yend = y1 + gradient * (xend - x1);
  xgap = fpart(x1 + 0.5);
  int xpxl2 = static_cast<int>(xend); // Used in the main loop.
  int ypxl2 = static_cast<int>(std::floor(yend));
  if (is_steep) {
    plot(ypxl2, xpxl2, rfpart(yend) * xgap);
    plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap);
  }
  else {
    plot(xpxl2, ypxl2, rfpart(yend) * xgap);
    plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);
  }

  // Main loop time.
  if (is_steep) {
    for (int x = xpxl1 + 1; x < xpxl2; ++x) {
      plot(std::floor(intery), x, rfpart(intery));
      plot(static_cast<int>(std::floor(intery)) + 1, x, fpart(intery));
      intery = intery + gradient;
    }
  }
  else {
    for (int x = xpxl1 + 1; x < xpxl2; ++x) {
      plot(x, static_cast<int>(std::floor(intery)), rfpart(intery));
      plot(x, static_cast<int>(std::floor(intery)) + 1, fpart(intery));
      intery = intery + gradient;
    }
  }
}

void XiaolinWuThickLine::DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  auto absolute_first = canvas->PointToPixels(first);
  auto absolute_second = canvas->PointToPixels(second);

  double x0 = absolute_first.x, y0 = absolute_first.y;
  double x1 = absolute_second.x, y1 = absolute_second.y;

  auto& background_color = canvas->GetBackgroundColor();

  auto fpart = [](double x) { return x - std::floor(x); };
  auto rfpart = [&fpart](double x) { return 1. - fpart(x); };
  auto plot = [&](int x, int y, double mult) {
    write(bitmap, x, y, color::Interpolate(background_color, color, mult));
  };

  bool is_steep = std::abs(x1 - x0) < std::abs(y1 - y0);

  if (is_steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }

  if (x1 < x0) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  double dx = x1 - x0;
  double dy = y1 - y0;
  double gradient = dx == 0. ? 1.0 : dy / dx;

  // TODO: Handle endpoints differently.

  // Handle first endpoint.
  double xend = std::round(x0);
  double yend = y0 + gradient * (xend - x0);
  double xgap = rfpart(x0 + 0.5);
  int xpxl1 = static_cast<int>(xend); // Used in the main loop.
  int ypxl1 = static_cast<int>(std::floor(yend));

//  if (is_steep) {
//    plot(ypxl1, xpxl1, rfpart(yend) * xgap);
//    plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap);
//  }
//  else {
//    plot(xpxl1, ypxl1, rfpart(yend) * xgap);
//    plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
//  }
  double intery = yend + gradient;

  // Handle second endpoint.
  xend = std::round(x1);
  yend = y1 + gradient * (xend - x1);
  xgap = fpart(x1 + 0.5);
  int xpxl2 = static_cast<int>(xend); // Used in the main loop.
  int ypxl2 = static_cast<int>(std::floor(yend));
//  if (is_steep) {
//    plot(ypxl2, xpxl2, rfpart(yend) * xgap);
//    plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap);
//  }
//  else {
//    plot(xpxl2, ypxl2, rfpart(yend) * xgap);
//    plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);
//  }

  // Main loop time.
  if (is_steep) {
    // Calculate line width in the x direction TODO: Do this.
    double csc_theta =  std::sqrt(1. + gradient * gradient);
    double width = pixel_thickness * csc_theta;

    double mid_x = intery;
    for (int y = xpxl1 + 1; y < xpxl2; ++y) {
      // Starting pixel (y)
      int start_x = static_cast<int>(std::floor(mid_x - 0.5 * width));
      // Ending pixel (y)
      int end_x = static_cast<int>(std::floor(mid_x + 0.5 * width));

      // Antialias starting and ending pixels.
      double c_start = 1. - (mid_x - start_x - 0.5 * width);
      double c_end = 0.5 * width - (end_x - mid_x);
      plot(start_x, y, c_start);
      plot(end_x, y, c_end);
      // Treat central pixels normally.
      for (int x = start_x + 1; x < end_x; ++x) {
        plot(x, y, 1);
      }

      // Go to next pixel in the y direction.
      mid_x = mid_x + gradient;
    }
  }
  else {
    // Calculate line width in the y direction TODO: Do this.
    double sec_theta = std::sqrt(1. + gradient * gradient);
    double height = pixel_thickness * sec_theta;

    double mid_y = intery;
    for (int x = xpxl1 + 1; x < xpxl2; ++x) {
      // Starting pixel (y)
      int start_y = static_cast<int>(std::floor(mid_y - 0.5 * height));
      // Ending pixel (y)
      int end_y = static_cast<int>(std::floor(mid_y + 0.5 * height));

      // Antialias starting and ending pixels.
      double c_start = 1. - (mid_y - start_y - 0.5 * height);
      double c_end = 0.5 * height - (end_y - mid_y);
      plot(x, start_y, c_start);
      plot(x, end_y, c_end);
      // Treat central pixels normally.
      for (int y = start_y + 1; y < end_y; ++y) {
        plot(x, y, 1);
      }

      mid_y = mid_y + gradient;
    }
  }
}

void Ray::DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  auto base_pixels = canvas->PointToPixels(base_);
  auto ray_pixels = canvas->DisplacementToPixels(ray_);

  auto end_point = Point{ base_pixels.x + ray_pixels.dx,
                          base_pixels.y + ray_pixels.dy,
                          LocationType::Pixels,
                          LocationType::Pixels };
  XiaolinWuThickLine line(base_pixels, end_point, color_, thickness_);
  line.DrawOnBitmap(bitmap, canvas);
}

CoordinateBoundingBox Ray::GetBoundingBox() const {
  // Check if any point uses coordinates.
  double min_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double min_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_y = std::numeric_limits<double>::quiet_NaN();

  if (base_.type_x == LocationType::Coordinate) {
    min_coordinate_x = max_coordinate_x = base_.x;
    if (ray_.type_dx == LocationType::Coordinate) {
      min_coordinate_x = std::min(min_coordinate_x, base_.x + ray_.dx);
      max_coordinate_x = std::max(max_coordinate_x, base_.x + ray_.dx);
    }
  }
  if (base_.type_y == LocationType::Coordinate) {
    min_coordinate_y = max_coordinate_y = base_.y;
    if (ray_.type_dy == LocationType::Coordinate) {
      min_coordinate_y = std::min(min_coordinate_y, base_.y + ray_.dy);
      max_coordinate_y = std::max(max_coordinate_y, base_.y + ray_.dy);
    }
  }

  return { min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y };
}

void Circle::DrawOnBitmap(Bitmap& bitmap, const Canvas* canvas) const {
  auto pixel_center = canvas->PointToPixels(center);
  auto x0 = pixel_center.x, y0 = pixel_center.y;

  auto& background_color = canvas->GetBackgroundColor();

  auto plot = [&](int x, int y, double mult) {
    write(bitmap, x, y, color::Interpolate(background_color, color, mult));
  };

  Displacement corner{ radius.distance, radius.distance, radius.type, radius.type };
  auto corner_pixels = canvas->DisplacementToPixels(corner);

  double dx = corner_pixels.dx, dy = corner_pixels.dy;
  int x_start = static_cast<int>(std::floor(x0 - dx - 0.5)), x_end = static_cast<int>(std::ceil(x0 + dx));
  int y_start = static_cast<int>(std::floor(y0 - dy - 0.5)), y_end = static_cast<int>(std::ceil(y0 + dy));
  for (auto x = x_start; x <= x_end; ++x) {
    for (auto y = y_start; y <= y_end; ++y) {
      double rx = x - x0 + 0.5, ry = y - y0 + 0.5; // Displacement from the center of the pixel.
      double r_ellipse = std::sqrt(math::square(rx / dx) + math::square(ry / dy));
      if (r_ellipse < 1) {
        plot(x, y, 1);
      }
      else {
        // TODO: Proper antialiasing, this will only work for (rendered) circles, so not for ovals.
        double dr = std::sqrt(math::square(rx) + math::square(ry)) - radius.distance;
        if (dr < 1.) {
          plot(x, y, 1 - dr);
        }
      }
    }
  }
}

CoordinateBoundingBox Circle::GetBoundingBox() const {
  // Check if any point uses coordinates.
  double min_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_x = std::numeric_limits<double>::quiet_NaN();
  double min_coordinate_y = std::numeric_limits<double>::quiet_NaN();
  double max_coordinate_y = std::numeric_limits<double>::quiet_NaN();

  if (center.type_x == LocationType::Coordinate) {
    if (radius.type == LocationType::Coordinate) {
      min_coordinate_x = center.x - radius.distance;
      max_coordinate_x = center.x + radius.distance;
    }
    // Only consider the center.
    else {
      min_coordinate_x = center.x;
      max_coordinate_x = center.x;
    }
  }
  if (center.type_y == LocationType::Coordinate) {
    if (radius.type == LocationType::Coordinate) {
      min_coordinate_y = center.y - radius.distance;
      max_coordinate_y = center.y + radius.distance;
    }
    // Only consider the center.
    else {
      min_coordinate_y = center.y;
      max_coordinate_y = center.y;
    }
  }

  return { min_coordinate_x, max_coordinate_x, min_coordinate_y, max_coordinate_y };
}