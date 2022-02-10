//
// Created by Nathaniel Rupprecht on 1/30/22.
//

#include "gemini/core/shapes/BezierCuve.h"
#include <map>

using namespace gemini;
using namespace gemini::core;
using namespace gemini::core::shapes;

NO_DISCARD BezierCurve BezierCurve::Copy() const {
  return BezierCurve{ contour_ends, points };
}

NO_DISCARD std::size_t BezierCurve::NumPoints() const {
  return points.size();
}

NO_DISCARD std::size_t BezierCurve::NumContours() const {
  return contour_ends.size();
}

BezierCurve& BezierCurve::Scale(double factor) {
  std::for_each(points.begin(), points.end(), [=](auto &p) {
    p.x *= factor;
    p.y *= factor;
  });
  return *this;
}

BezierCurve& BezierCurve::Translate(double dx, double dy) {
  std::for_each(points.begin(), points.end(), [=](auto &p) {
    p.x += dx;
    p.y += dy;
  });
  return *this;
}

BezierCurve& BezierCurve::Rotate(double theta) {
  double cth = std::cos(theta), sth = std::sin(theta);
  std::for_each(points.begin(), points.end(), [=](auto &p) {
    double nx = p.x * cth - p.y * sth, ny = p.x * sth + p.y * cth;
    p.x = nx, p.y = ny;
  });
  return *this;
}

BezierCurve& BezierCurve::SkewX(double theta) {

  GEMINI_REQUIRE (-0.5 * math::PI < theta && theta < 0.5 * math::PI,
                  "theta must be in the range (-π/2, +π/2)");

  double tth = std::tan(theta);
  std::for_each(points.begin(), points.end(), [=](auto &p) {
    p.x += p.y * tth;
  });
  return *this;
}

BezierCurve& BezierCurve::ScaleShifted(double factor, double dx, double dy) {
  std::for_each(points.begin(), points.end(), [=](auto &p) {
    p.x = factor * (p.x + dx);
    p.y = factor * (p.y + dy);
  });
  return *this;
}

BezierCurve& BezierCurve::ShiftScaled(double factor, double dx, double dy) {
  std::for_each(points.begin(), points.end(), [=](auto &p) {
    p.x = factor * p.x + dx;
    p.y = factor * p.y + dy;
  });
  return *this;
}

namespace gemini::core::shapes {

void RasterBezierCurve(const BezierCurve& spline,
                       Bitmap& bmp,
                       color::PixelColor color,
                       double z,
                       bool color_by_spline) {
  using Direction = bool;

  std::map<uint16_t, std::vector<std::tuple<double, Direction, uint16_t>>> y_divisions;

  uint16_t spline_begin = 0, spline_end;
  for (int nspline = 0; nspline < spline.NumContours(); ++nspline) {
    // Contour ends are the *last* point in each contour.
    spline_end = spline.contour_ends[nspline];
    for (auto i = spline_begin; i <= spline_end; ++i) {
      auto& p0 = spline.points[i];
      auto p1_index = i == spline_end ? spline_begin : i + 1;
      auto& p1 = spline.points[p1_index];

      // Straight line.
      if (p0.is_on_curve && p1.is_on_curve) {
        double cx = p1.x - p0.x;
        double cy = p1.y - p0.y;
        double dx = p0.x, dy = p0.y;

        for (int Y = 0; Y < bmp.GetHeight(); ++Y) {
          double t = (Y - dy) / cy;
          double x = cx * t + dx;

          if (0 <= t && t <= 1) {
            Direction dir = 0 < cy;
            y_divisions[Y].emplace_back(x, dir, i);
          }
        }
      }
      // Spline.
      else {
        auto p2_index = p1_index == spline_end ? spline_begin : p1_index + 1;
        auto& p2 = spline.points[p2_index];

        double p0x = p0.x, p0y = p0.y, p2x = p2.x, p2y = p2.y;

        // If p0 is a spline point, interpolate its effective position.
        if (!p0.is_on_curve) {
          p0x = 0.5 * (p0x + p1.x), p0y = 0.5 * (p0y + p1.y);
        }

        // If p2 is a spline point, interpolate its effective position.
        if (!p2.is_on_curve) {
          p2x = 0.5 * (p2x + p1.x), p2y = 0.5 * (p2y + p1.y);
        }

        double bx = p2x - 2 * p1.x + p0x;
        double by = p2y - 2 * p1.y + p0y;
        double cx = 2 * p1.x - 2 * p0x;
        double cy = 2 * p1.y - 2 * p0y;
        double dx = p0x, dy = p0y;

        // Degenerate case.
        if (std::abs(by) < 1.e-5) {
          for (int Y = 0; Y < bmp.GetHeight(); ++Y) {
            auto t = (Y - dy) / cy;
            auto x = bx * t * t + cx * t + dx;
            if (0 <= t && t <= 1) {
              Direction dir = 0 < cy;
              y_divisions[Y].emplace_back(x, dir, i);
            }
          }
        }
        else {
          for (int Y = 0; Y < bmp.GetHeight(); ++Y) {
            auto argument = cy * cy - 4 * by * (dy - Y);
            if (0 < argument) {
              auto t_plus = 0.5 * (-cy + std::sqrt(argument)) / by;
              auto t_minus = 0.5 * (-cy - std::sqrt(argument)) / by;

              if (0 <= t_plus && t_plus <= 1) {
                auto x = bx * t_plus * t_plus + cx * t_plus + dx;
                Direction dir = 0 < 2 * by * t_plus + cy;
                y_divisions[Y].emplace_back(x, dir, i);
              }
              if (0 <= t_minus && t_minus <= 1) {
                auto x = bx * t_minus * t_minus + cx * t_minus + dx;
                Direction dir = 0 < 2 * by * t_minus + cy;
                y_divisions[Y].emplace_back(x, dir, i);
              }
            }
          }
        }

        // If p2 is the end of the spline segment, advance i so p2 will be the next p0.
        if (p2.is_on_curve) {
          ++i;
        }
      }
    }
    spline_begin = spline_end + 1;
  }

  std::vector<color::PixelColor> colors;
  if (color_by_spline) {
    colors.reserve(spline.NumPoints());
    for (int i = 0; i < spline.NumPoints(); ++i) {
      colors.push_back(color::RandomUniformColor());
    }
  }

  for (auto& [y, crossings] : y_divisions) {
    int count_wrapping = 0;
    std::sort(crossings.begin(), crossings.end());
    for (int i = 0; i < crossings.size() - 1; ) {
      // Keep track of winding number.

      // This point is the end of one spline segment and the beginning of the next. Skip it.
      int j = i + 1;
      if (std::get<0>(crossings[i]) == std::get<0>(crossings[i + 1])
          && std::get<1>(crossings[i]) == std::get<1>(crossings[i + 1])) {
        ++j;
      }

      count_wrapping += std::get<1>(crossings[i]) ? 1 : -1;
      if (count_wrapping != 0) {
        uint16_t x1 = std::floor(std::get<0>(crossings[i])), x2 = std::ceil(std::get<0>(crossings[j]));

        for (auto x = x1; x <= x2; ++x) {
          bmp.SetPixel(x, y, color, z);
        }
        if (color_by_spline) {
          bmp.SetPixel(x1, y, colors[std::get<2>(crossings[i])], z);
          bmp.SetPixel(x2, y, colors[std::get<2>(crossings[i + 1])], z);
        }
      }

      i = j;
    }
  }

}

} // namespace gemini::shapes


void QuadraticBezierCurve::DrawOnBitmap(Bitmap &bitmap, const Canvas *canvas) const {
  RasterBezierCurve(spline_, bitmap, color_, false);
}

CoordinateBoundingBox QuadraticBezierCurve::GetBoundingBox() const {
  return {};
}