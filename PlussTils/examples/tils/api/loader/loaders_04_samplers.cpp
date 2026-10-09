#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void print_values(const std::vector<double>& values) {
  for (std::size_t i = 0; i < values.size(); ++i) std::cout << (i ? ", " : "") << values[i];
}

std::vector<double> visited(pluss::loader::DataLoader& loader) {
  std::vector<double> out;
  for (auto batch : loader) {
    const auto values = batch.x().reshape({batch.x().numel()}).to_vec<double>();
    out.insert(out.end(), values.begin(), values.end());
  }
  return out;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, {8}, cfg);

  // A sequential sampler visits every sample in index order.
  loader::DataLoaderOptions sequential{};
  sequential.batch_size = 4;
  sequential.sampler = std::make_shared<loader::SequentialSampler>();
  loader::DataLoader in_order({x}, sequential);
  std::cout << "sequential: ";
  print_values(visited(in_order));
  std::cout << '\n';

  // A subset sampler visits only the listed indices, in a random order.
  loader::DataLoaderOptions subset{};
  subset.batch_size = 2;
  subset.sampler = std::make_shared<loader::SubsetRandomSampler>(std::vector<std::int64_t>{4, 5, 6});
  loader::DataLoader only_subset({x}, subset);
  auto picked = visited(only_subset);
  std::sort(picked.begin(), picked.end());
  std::cout << "subset visits exactly 4, 5, 6: " << (picked == std::vector<double>{4, 5, 6}) << '\n';

  // A sampler and the shuffle flag are mutually exclusive. The loader rejects the pair.
  loader::DataLoaderOptions both{};
  both.shuffle = true;
  both.sampler = std::make_shared<loader::SequentialSampler>();
  bool rejected = false;
  try {
    loader::DataLoader conflicting({x}, both);
  } catch (const std::exception&) {
    rejected = true;
  }
  std::cout << "shuffle and sampler together rejected: " << rejected << '\n';
}
