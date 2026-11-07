// SeriesDataset wraps a list of fields as a map-style Dataset, the same way the loader's own
// constructor does internally when you pass it a plain vector of fields. Building one directly
// gives access to the fields and the batch dimension on their own.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0}, {5}, cfg);

  const loader::SeriesDataset dataset({x, y});
  std::cout << "size: " << dataset.size() << '\n';
  std::cout << "field count: " << dataset.fields().size() << '\n';
  std::cout << "batch_dim: " << dataset.batch_dim() << '\n';

  const loader::DataBatch sample = dataset.get(2);
  std::cout << "sample 2 x: " << sample.x().item<double>() << '\n';
  std::cout << "sample 2 y: " << sample.y().item<double>() << '\n';
  return 0;
}
