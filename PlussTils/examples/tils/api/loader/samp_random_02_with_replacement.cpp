// With replacement, num_samples may exceed the dataset size, and the same index can appear more
// than once.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  const loader::RandomSampler sampler(/*replacement=*/true, /*num_samples=*/8);
  const auto order = sampler.sample(3, /*epoch_seed=*/1);
  std::cout << "visits: " << order.size() << '\n';
  bool in_range = true;
  for (auto index : order) in_range = in_range && index >= 0 && index < 3;
  std::cout << "every index in range: " << in_range << '\n';
  return 0;
}
