// Splitting a series for honest forecast evaluation. A `TsPartition` cuts the observations 0 ... n - 1 into consecutive training and test blocks so that the model is
// always tested on data that come after what it was trained on. `expanding_window(n, k)` grows the training set from the start; `sliding_window(n, k)` keeps its length
// fixed; `holdout(n, fraction)` makes one split. `PartitionOptions`: `train_size` (or `min_train_size` for expanding windows), `test_size`, `step_size` (distance between
// consecutive test sets), `gap_size` (observations left out between training and test), `max_train_size`, `max_lag` (a buffer of that many observations before each test
// set, to hold the lags the model needs), `direction` (Reverse, the default, ends the last test set at the end of the series; Forward starts from the beginning).
// Whatever is not given is derived from n - train - test - gap = step (k - 1), with test = step = train when nothing is given. Each window reports 0-based inclusive
// ranges; they are shown below 1-based.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;

void show(const char* title, const ts::TsPartition& p) {
  std::cout << title << "\n";
  for (const auto& w : p.windows())
    std::cout << "  train " << std::setw(3) << w.train_start + 1 << " - " << std::setw(3) << w.train_end + 1 << "   test " << std::setw(3) << w.test_start + 1 << " - " << std::setw(3) << w.test_end + 1 << "\n";
}
int main() {
  show("expanding window, 100 observations, 3 test sets", ts::TsPartition::expanding_window(100, 3));
  show("sliding window", ts::TsPartition::sliding_window(100, 3));
  ts::PartitionOptions gap;
  gap.gap_size = 2;
  show("expanding window with a gap of 2", ts::TsPartition::expanding_window(100, 3, gap));
  ts::PartitionOptions test10;
  test10.test_size = 10;
  show("sliding window, test size 10 (step = 10, training takes the rest)", ts::TsPartition::sliding_window(100, 4, test10));
  ts::PartitionOptions forward;
  forward.direction = ts::Direction::Forward;
  forward.min_train_size = 40;
  forward.step_size = 20;
  show("expanding window, forward from a training set of 40, step 20", ts::TsPartition::expanding_window(100, 3, forward));
  show("holdout, last 20%", ts::TsPartition::holdout(100, 0.2));
  ts::PartitionOptions lag;
  lag.max_lag = 5;
  const auto p = ts::TsPartition::expanding_window(100, 3, lag);
  const auto w = p.window(0);
  std::cout << "with max_lag = 5 the first test set " << w.test_start + 1 << " - " << w.test_end + 1 << " gets the buffer " << w.test_buffer_start + 1 << " - " << w.test_buffer_end + 1
            << " (the lags its first rows need)\n";
}
