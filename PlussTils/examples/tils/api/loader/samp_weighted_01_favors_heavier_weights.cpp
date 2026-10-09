// WeightedRandomSampler picks indices with probability proportional to their weight. A weight of
// zero is never picked, and the sample count is independent of the dataset size.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::WeightedRandomSampler sampler(std::vector<double>{0.0, 1.0, 0.0}, /*num_samples=*/10);
  const auto order = sampler.sample(3, /*epoch_seed=*/3);
  std::cout << "visits: " << order.size() << '\n';
  bool only_index_one = true;
  for (auto index : order) only_index_one = only_index_one && index == 1;
  std::cout << "only the weighted index is picked: " << only_index_one << '\n';
  return 0;
}
