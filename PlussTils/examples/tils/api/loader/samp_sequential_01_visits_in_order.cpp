// SequentialSampler visits every index in order, ignoring the seed entirely. It is the sampler a
// loader with shuffle left off would behave like, if you wanted one as an explicit object.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::SequentialSampler sampler;
  const auto order = sampler.sample(4, /*epoch_seed=*/999);
  std::cout << "order: ";
  for (auto index : order) std::cout << index << ' ';
  std::cout << '\n';
  std::cout << "in order: " << (order == std::vector<std::int64_t>{0, 1, 2, 3}) << '\n';
  return 0;
}
