#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// The order of samples for one full pass, read through the batches the loader delivers.
std::vector<double> one_pass(pluss::loader::DataLoader& loader) {
  std::vector<double> order;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).host<double>();
    order.insert(order.end(), values.begin(), values.end());
  }
  return order;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, {8}, cfg);

  loader::DataLoaderOptions options{};
  options.batch_size = 4;
  options.shuffle = true;
  options.seed = 42;

  // Two loaders built from the same seed produce the same first pass.
  loader::DataLoader first(std::vector<pluss::Series>{x}, options);
  loader::DataLoader second(std::vector<pluss::Series>{x}, options);
  const auto first_order = one_pass(first);
  std::cout << "same seed, same order: " << (first_order == one_pass(second)) << '\n';

  // A shuffled pass still visits every sample exactly once.
  auto sorted = first_order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every sample once: " << (sorted == std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}) << '\n';

  // Moving to a new epoch reshuffles by default. Returning to epoch zero replays it.
  first.set_epoch(1);
  const auto epoch_one = one_pass(first);
  std::cout << "epoch one differs from epoch zero: " << (epoch_one != first_order) << '\n';
  first.set_epoch(0);
  std::cout << "epoch zero replays: " << (one_pass(first) == first_order) << '\n';
}
