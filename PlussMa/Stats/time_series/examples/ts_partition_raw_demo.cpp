#include <pluss/ma.h>

#include <cstdint>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);

  ts::PartitionOptions expanding_opts;
  expanding_opts.direction = ts::Direction::Reverse;
  expanding_opts.gap_size = 1;
  expanding_opts.max_lag = 2;
  expanding_opts.min_train_size = 6;
  expanding_opts.test_size = 3;
  expanding_opts.step_size = 2;
  const auto expanding = ts::TsPartition::expanding_window(32, 4, expanding_opts);

  std::cout << "ExpandingWindow\n";
  for (std::int64_t i = 0; i < expanding.num_test_sets(); ++i) {
    const auto& w = expanding.window(i);
    std::cout << "fold=" << i
              << " train=[" << w.train_start << "," << w.train_end << "]"
              << " test=[" << w.test_start << "," << w.test_end << "]"
              << " buffer=[" << w.test_buffer_start << "," << w.test_buffer_end << "]\n";
    std::cout << "train_idx=" << expanding.training(i, cfg) << "\n";
    std::cout << "test_idx=" << expanding.test(i, cfg) << "\n";
    std::cout << "test_buffer_idx=" << expanding.test_buffer(i, cfg) << "\n";
    std::cout << "train_mask=" << expanding.training_mask(i, cfg) << "\n";
    std::cout << "test_mask=" << expanding.test_mask(i, cfg) << "\n";
    std::cout << "test_buffer_mask=" << expanding.test_buffer_mask(i, cfg) << "\n\n";
  }

  ts::PartitionOptions sliding_opts;
  sliding_opts.direction = ts::Direction::Forward;
  sliding_opts.train_size = 8;
  sliding_opts.test_size = 2;
  sliding_opts.step_size = 2;
  const auto sliding = ts::TsPartition::sliding_window(30, 4, sliding_opts);

  std::cout << "SlidingWindow\n";
  for (std::int64_t i = 0; i < sliding.num_test_sets(); ++i) {
    const auto& w = sliding.window(i);
    std::cout << "fold=" << i
              << " train=[" << w.train_start << "," << w.train_end << "]"
              << " test=[" << w.test_start << "," << w.test_end << "]\n";
  }

  ts::PartitionOptions holdout_opts;
  holdout_opts.gap_size = 1;
  holdout_opts.max_lag = 3;
  const auto holdout = ts::TsPartition::holdout(40, 0.2, holdout_opts);
  const auto& hw = holdout.window(0);
  std::cout << "\nHoldout\n";
  std::cout << "train=[" << hw.train_start << "," << hw.train_end << "]"
            << " test=[" << hw.test_start << "," << hw.test_end << "]"
            << " buffer=[" << hw.test_buffer_start << "," << hw.test_buffer_end << "]\n";
  std::cout << "train_idx=" << holdout.training(0, cfg) << "\n";
  std::cout << "test_idx=" << holdout.test(0, cfg) << "\n";
  std::cout << "test_buffer_idx=" << holdout.test_buffer(0, cfg) << "\n";
  return 0;
}
