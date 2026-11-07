// next() pulls one batch and advances the loader's own cursor, so repeated calls walk through the
// epoch without a range-for. It returns nullopt once every batch has been delivered.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data({x}, options);

  std::size_t pulled = 0;
  while (const auto batch = data.next()) {
    ++pulled;
    std::cout << "batch " << pulled << " rows: " << batch->x().sizes()[0] << '\n';
  }
  std::cout << "batches pulled: " << pulled << '\n';
  std::cout << "next past the end: " << data.next().has_value() << '\n';
  return 0;
}
