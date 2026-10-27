// in_order set to false allows a worker to deliver a batch as soon as it finishes, rather than
// waiting for strict index order. Every batch still arrives exactly once; only the arrival order
// may differ from a run with in_order left at its default of true.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  std::vector<double> values(40);
  for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i);
  const pluss::Series x = pluss::make_series(values, {40}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.num_workers = 3;
  options.in_order = false;
  loader::DataLoader data({x}, options);

  std::vector<double> seen;
  for (auto batch : data) {
    const auto v = batch.x().host<double>();
    seen.insert(seen.end(), v.begin(), v.end());
  }
  std::sort(seen.begin(), seen.end());
  std::cout << "every value delivered exactly once: " << (seen == values) << '\n';
  return 0;
}
