// Train, validation and test sets of different sizes in an Archive (unlike an Atlas, an Archive's catalogs may have different record counts),
// scaled with the training statistics only.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

static Catalog part(const std::vector<double> &x) {
  std::vector<double> y;
  for (double v : x) y.push_back(5.0 + 1.5 * v);
  return Catalog({"x"_k = Stream(pluss::make_series(x)), "y"_k = Stream(pluss::make_series(y))});
}

int main() {
  Archive splits({part({10, 12, 14, 16, 18, 20, 22, 24}), part({26, 28}), part({30, 32, 34})}, Stream(pluss::make_note({"train", "valid", "test"})));
  std::cout << "catalogs " << splits.depth() << ", records per catalog from " << splits.minrecords() << " to " << splits.maxrecords() << "\n";

  // 1. Fit the scaler on the training catalog alone.
  auto train_x = splits.loc("train").keys("x");
  const double mean = train_x.mean().as_series().items<double>()[0];
  const double sd = train_x.std().as_series().items<double>()[0];
  std::cout << "training mean " << mean << ", std " << sd << "\n";

  // 2. Apply that scaler to every catalog with map_catalogs.
  auto scaled = splits.map_catalogs([&](const Catalog &c) { return c.set("x_scaled", (c.keys("x") - mean) / sd); });
  for (const char *name : {"train", "valid", "test"}) {
    auto c = scaled.loc(name);
    std::cout << name << ": " << c.nrows() << " records, mean of x_scaled " << c.keys("x_scaled").mean().as_series().items<double>()[0] << "\n";
  }

  // 3. A leak check: the validation and test values are expressed in units of the training spread, so they lie outside the training range.
  std::cout << "largest validation x_scaled " << scaled.loc("valid").keys("x_scaled").max().as_series().items<double>()[0] << " (training max "
            << scaled.loc("train").keys("x_scaled").max().as_series().items<double>()[0] << ")\n";
}
