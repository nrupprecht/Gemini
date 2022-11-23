//
// Created by Nathaniel Rupprecht on 11/17/22.
//

#include "internal/testing.h"
// Other files.
#include "gemini/plot/Figure.h"
#include "gemini/plot/renders/LinePlotRender.h"
#include "gemini/plot/renders/ScatterPlotRender.h"

using namespace gemini::plot;
using namespace gemini::core::color;
using namespace gemini::plot::renders;

namespace Testing {

TEST(Manager, CyclingColors) {
  Manager manager;
  std::vector colors{Black, Red, Green};

  EXPECT_NO_THROW(manager.SetDefaultColorPalette(colors));
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Black);
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Red);
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Green);
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Black);
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Red);

  manager.ResetColorCycle<LinePlotRender>();
  auto c6 = manager.RequestColor<LinePlotRender>();
  EXPECT_EQ(c6, Black);
}

TEST(Manager, CyclingMultipleTypes) {
  // Requesting colors for different types of renders should not mix between renders.
  //

  Manager manager;
  std::vector colors1{Black, Red, Green};
  std::vector colors2{White, Black};

  manager.SetColorPalette<LinePlotRender>(colors1);
  manager.SetColorPalette<ScatterPlotRender>(colors2);

  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Black);
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Red);
  EXPECT_EQ(manager.RequestColor<ScatterPlotRender>(), White);
  EXPECT_EQ(manager.RequestColor<LinePlotRender>(), Green);
  EXPECT_EQ(manager.RequestColor<ScatterPlotRender>(), Black);
}

} // namespace Testing
