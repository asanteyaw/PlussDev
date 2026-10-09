// Without max_train_size, each later fold's training window grows to include everything before it.
// With it set, the window is capped to a fixed size, so it slides forward instead of growing.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  loader::TemporalSplitOptions growing{};
  growing.folds = 3;
  growing.test_size = 3;
  const auto expanding = loader::temporal_split_indices(20, growing);

  loader::TemporalSplitOptions capped = growing;
  capped.max_train_size = 5;
  const auto fixed = loader::temporal_split_indices(20, capped);

  std::cout << "expanding window sizes: ";
  for (const auto& fold : expanding.folds) std::cout << fold.train_indices.size() << ' ';
  std::cout << '\n';
  std::cout << "capped window sizes: ";
  for (const auto& fold : fixed.folds) std::cout << fold.train_indices.size() << ' ';
  std::cout << '\n';
  std::cout << "capped windows never exceed 5: " << (fixed.folds.back().train_indices.size() <= 5) << '\n';
  return 0;
}
