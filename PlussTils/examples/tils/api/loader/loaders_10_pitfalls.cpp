#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

// Returns true if the callable throws. The message itself is not printed, so this
// example's output does not depend on library wording.
template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, cfg);

  // Prefetching depth is a worker setting. Without workers it is an error, not a no-op.
  loader::DataLoaderOptions prefetch_without_workers{};
  prefetch_without_workers.prefetch_factor = 4;
  std::cout << "prefetch without workers rejected: "
            << throws([&] { loader::DataLoader bad({x}, prefetch_without_workers); }) << '\n';

  // Reading past the last batch is an error. Batch indices start at zero.
  loader::DataLoader two_batches({x}, [] {
    loader::DataLoaderOptions o{};
    o.batch_size = 2;
    return o;
  }());
  std::cout << "batch count: " << two_batches.batch_count() << '\n';
  std::cout << "reading batch two rejected: "
            << throws([&] { (void)two_batches.batch_at(two_batches.batch_count()); }) << '\n';

  // A zero batch size cannot describe any batch.
  loader::DataLoaderOptions zero{};
  zero.batch_size = 0;
  std::cout << "zero batch size rejected: "
            << throws([&] { loader::DataLoader bad({x}, zero); }) << '\n';
}
