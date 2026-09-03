#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// A GridAxis is {name, values}. grid_size(axes) is the cross-product size.
// grid_point_at(i, axes) decodes index i into one GridPoint, which offers
// name-keyed access (point.at("name")) and an auto-generated label().
int main() {
  namespace lymph = pluss::ai::lymph;

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };

  std::cout << "grid_size = " << lymph::selection::grid_size(axes) << "\n";

  const lymph::selection::GridPoint p = lymph::selection::grid_point_at(2, axes);
  std::cout << "point 2: reg=" << p.at("regularization_strength")
            << ", intercept=" << p.at("fit_intercept")
            << ", size=" << p.size() << "\n";
  std::cout << "point 2 label: " << p.label() << "\n";

  return 0;
}
