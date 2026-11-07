// pin_memory is accepted and calls pin_memory() on every field of every batch, but the header
// itself documents this as a no-op today: the underlying engine's pin_memory() returns its input
// unchanged, and is_pinned() always reports false. This example confirms that is still the case.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.pin_memory = true;
  loader::DataLoader data({x}, options);
  std::cout << "batch is reported pinned: " << data.batch_at(0).x().is_pinned() << '\n';
  return 0;
}
