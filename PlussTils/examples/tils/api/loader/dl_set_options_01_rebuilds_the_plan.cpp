// set_options replaces the whole configuration after construction and rebuilds the batch plan, the
// same validation and ordering the constructor runs.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6}, cfg);

  loader::DataLoaderOptions start{};
  start.batch_size = 2;
  loader::DataLoader data({x}, start);
  std::cout << "batches with size 2: " << data.batch_count() << '\n';

  loader::DataLoaderOptions changed{};
  changed.batch_size = 3;
  data.set_options(changed);
  std::cout << "batches with size 3: " << data.batch_count() << '\n';
  std::cout << "options reflects the change: " << (data.options().batch_size == 3) << '\n';
  return 0;
}
