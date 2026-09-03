#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// A GridPoint feeds a search's make_model closure: the closure reads
// point.at("axis") to build one Model per grid cell. Here the same pattern,
// driven directly by grid_point_at over a two-axis space.
int main() {
  namespace lymph = pluss::ai::lymph;

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };

  const auto make_model = [](const lymph::selection::GridPoint& point) {
    return lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = point.at("fit_intercept") != 0.0,
    });
  };

  const std::int64_t total = lymph::selection::grid_size(axes);
  for (std::int64_t i = 0; i < total; ++i) {
    const lymph::selection::GridPoint point = lymph::selection::grid_point_at(i, axes);
    const lymph::models::Model m = make_model(point);
    std::cout << "cell " << i << " [" << point.label()
              << "] -> task " << static_cast<int>(m.task_kind()) << "\n";
  }

  return 0;
}
