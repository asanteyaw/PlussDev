// empty, field_count, sample_count, batch_size, and options read back what the loader was built
// with, without changing anything.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6}, {7}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 1, 0, 1, 0, 1, 0}, {7}, cfg);
  loader::DataLoaderOptions options{};
  options.batch_size = 3;
  loader::DataLoader data({x, y}, options);

  std::cout << "empty: " << data.empty() << '\n';
  std::cout << "field_count: " << data.field_count() << '\n';
  std::cout << "sample_count: " << data.sample_count() << '\n';
  std::cout << "batch_size: " << data.batch_size() << '\n';
  std::cout << "options().batch_size matches: " << (data.options().batch_size == 3) << '\n';

  const loader::DataLoader empty_loader;
  std::cout << "default-constructed loader is empty: " << empty_loader.empty() << '\n';
  return 0;
}
