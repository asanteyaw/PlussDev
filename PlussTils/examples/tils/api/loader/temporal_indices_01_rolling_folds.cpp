// temporal_split_indices produces several train/test folds that each move forward in time: the
// train indices for a later fold include everything an earlier fold's train and test together
// covered, and the test window always comes immediately after the training window.
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  loader::TemporalSplitOptions options{};
  options.folds = 4;
  options.test_size = 3;

  const loader::TemporalIndexSplit split = loader::temporal_split_indices(20, options);
  std::cout << "fold count: " << split.folds.size() << '\n';
  for (std::size_t i = 0; i < split.folds.size(); ++i) {
    const auto& fold = split.folds[i];
    std::cout << "fold " << i << ": train 0.." << fold.train_indices.size()
              << " test " << fold.test_indices.front() << ".." << fold.test_indices.back() << '\n';
  }
  return 0;
}
