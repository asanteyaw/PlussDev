// Without replacement, the sampler cannot deliver more samples than there are indices to draw, and
// each index is picked at most once.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::WeightedRandomSampler sampler(std::vector<double>{1.0, 1.0, 1.0, 1.0}, /*num_samples=*/4, /*replacement=*/false);
  const auto order = sampler.sample(4, /*epoch_seed=*/5);
  std::cout << "visits: " << order.size() << '\n';
  auto sorted = order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every index at most once: " << (sorted == std::vector<std::int64_t>{0, 1, 2, 3}) << '\n';
  return 0;
}
