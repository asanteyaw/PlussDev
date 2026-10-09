// RandomSampler without replacement visits every index exactly once, in a shuffled order. The
// same seed, passed through DataLoaderOptions, gives the same order every time.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::RandomSampler sampler(/*replacement=*/false);
  const auto order = sampler.sample(5, /*epoch_seed=*/7);
  std::cout << "visits: " << order.size() << '\n';
  auto sorted = order;
  std::sort(sorted.begin(), sorted.end());
  std::cout << "every index once: " << (sorted == std::vector<std::int64_t>{0, 1, 2, 3, 4}) << '\n';
  std::cout << "same seed, same order: " << (sampler.sample(5, 7) == order) << '\n';
  return 0;
}
