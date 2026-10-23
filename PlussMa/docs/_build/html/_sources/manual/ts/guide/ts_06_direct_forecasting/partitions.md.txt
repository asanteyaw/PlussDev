<!-- generated from partitions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Partitions

[Back to Direct Forecasting and Model Comparison](../ts_06_direct_forecasting.md)


```cpp
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
```

Output

```text
expanding window, 100 observations, 3 test sets
  train   1 -  25   test  26 -  50
  train   1 -  50   test  51 -  75
  train   1 -  75   test  76 - 100
sliding window
  train   1 -  25   test  26 -  50
  train  26 -  50   test  51 -  75
  train  51 -  75   test  76 - 100
expanding window with a gap of 2
  train   1 -  26   test  29 -  52
  train   1 -  50   test  53 -  76
  train   1 -  74   test  77 - 100
sliding window, test size 10 (step = 10, training takes the rest)
  train   1 -  60   test  61 -  70
  train  11 -  70   test  71 -  80
  train  21 -  80   test  81 -  90
  train  31 -  90   test  91 - 100
expanding window, forward from a training set of 40, step 20
  train   1 -  40   test  41 -  60
  train   1 -  60   test  61 -  80
  train   1 -  80   test  81 - 100
holdout, last 20%
  train   1 -  80   test  81 - 100
with max_lag = 5 the first test set 26 - 50 gets the buffer 21 - 25 (the lags its first rows need)
```
The default expanding window ends its last test set at the end of the series and gives the three tests and the first training set a length of a quarter each. A gap of 2 leaves the two observations before each test set unused, and the sizes are recomputed (24 instead of 25) so that the windows still fit. Giving only `test_size = 10` makes the step 10 and lets the sliding training window take the rest (60). The `Forward` window starts from a training set of 40 and moves by the step.
