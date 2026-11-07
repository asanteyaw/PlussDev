// epoch() reads back the epoch set_epoch moved to, so a caller can tell which pass through the data
// a loader is currently on.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.shuffle = true;
  options.seed = 1;
  loader::DataLoader data({x}, options);

  std::cout << "epoch at construction: " << data.epoch() << '\n';
  data.set_epoch(5);
  std::cout << "epoch after set_epoch(5): " << data.epoch() << '\n';
  return 0;
}
