// Prepare a feature matrix for a model: put every column on a common scale, flag outliers and check how the columns relate.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Eight households: income (thousands), household size, and floor area (square metres). One row per household.
  auto features = pluss::make_series({{52.0, 3.0, 85.0},
                                      {61.0, 4.0, 110.0},
                                      {48.0, 2.0, 62.0},
                                      {75.0, 5.0, 140.0},
                                      {43.0, 1.0, 45.0},
                                      {58.0, 3.0, 95.0},
                                      {390.0, 4.0, 105.0},
                                      {66.0, 4.0, 120.0}});
  std::cout << "rows " << features.size(0) << ", columns " << features.size(1) << "\n";

  // 1. Standardise each column: subtract its mean and divide by its standard deviation (dimension 0 runs down the rows).
  auto z = features.standardize(0);
  show("z-scores", z);

  // 2. A value more than two standard deviations from its column mean is an outlier candidate.
  auto outlier = z.abs().gt(2.0);
  show("outlier flags per column", outlier.sum(0));
  show("row of the outlier", outlier.any(1).nonzero());

  // 3. Min-max scale to [0, 1], the other common choice when you must keep a bounded range.
  auto lo = features.amin(0, true);
  auto hi = features.amax(0, true);
  auto scaled = (features - lo) / (hi - lo);
  show("min-max scaled", scaled);

  // 4. Correlation between the columns (variables are rows of the transposed matrix).
  show("correlation of the columns", features.t().corrcoef());
}
