// materialize_fields returns the whole dataset as its original fields, which is what a DataLoader
// built from this dataset reads in one shot, rather than calling get() once per sample.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg);
  const loader::SeriesDataset dataset({x});
  const auto materialized = dataset.materialize_fields();
  std::cout << "materialized field count: " << materialized.size() << '\n';
  std::cout << "values survive: " << (materialized[0].host<double>() == std::vector<double>{10, 20, 30}) << '\n';
  return 0;
}
