// reset() moves to the next epoch and rewinds the cursor, the same way begin() does automatically
// when reshuffle_each_epoch is on. set_epoch jumps to a chosen epoch instead of the next one.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  loader::DataLoader data({x}, options);

  std::cout << "epoch before reset: " << data.epoch() << '\n';
  data.reset();
  std::cout << "epoch after reset: " << data.epoch() << '\n';
  data.reset();
  std::cout << "epoch after a second reset: " << data.epoch() << '\n';
  return 0;
}
