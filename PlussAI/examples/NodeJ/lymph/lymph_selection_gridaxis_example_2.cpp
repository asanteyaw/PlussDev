#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// grid_points(axes) materializes the whole space in cross-product order -- the
// last axis varies fastest, matching a nested-loop expansion. Only reach for it
// when every point will actually be evaluated.
int main() {
  namespace lymph = pluss::ai::lymph;

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "depth", .values = {1.0, 2.0}},
      {.name = "leaf", .values = {5.0, 10.0, 20.0}},
  };

  const std::vector<lymph::selection::GridPoint> points =
      lymph::selection::grid_points(axes);

  std::cout << "points = " << points.size() << "\n";
  for (const auto& p : points) {
    std::cout << "  " << p.label() << "\n";
  }

  return 0;
}
