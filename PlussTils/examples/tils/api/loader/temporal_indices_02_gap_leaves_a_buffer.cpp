// gap leaves a buffer of untouched indices between the end of the training window and the start of
// the test window, so a value right at the boundary never leaks between the two.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;
  loader::TemporalSplitOptions no_gap{};
  no_gap.folds = 2;
  no_gap.test_size = 3;
  const auto without = loader::temporal_split_indices(20, no_gap);

  loader::TemporalSplitOptions with_gap = no_gap;
  with_gap.gap = 2;
  const auto with = loader::temporal_split_indices(20, with_gap);

  std::cout << "without gap, fold 0 train size: " << without.folds[0].train_indices.size() << '\n';
  std::cout << "with gap, fold 0 train size: " << with.folds[0].train_indices.size() << '\n';
  std::cout << "both folds' test windows start the same: "
            << (without.folds[0].test_indices.front() == with.folds[0].test_indices.front()) << '\n';
  return 0;
}
