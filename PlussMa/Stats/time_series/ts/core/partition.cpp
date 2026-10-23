#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kEps = 1e-12;

void require_observations_(std::int64_t n) {
  PLUSS_CHECK(n > 1, "subsp::stats::ts::TsPartition: num_observations must be > 1");
}

void require_non_negative_(std::int64_t v, const char* name) {
  PLUSS_CHECK(v >= 0, std::string("subsp::stats::ts::TsPartition: expected ") + name + " >= 0");
}

void require_positive_(std::int64_t v, const char* name) {
  PLUSS_CHECK(v > 0, std::string("subsp::stats::ts::TsPartition: expected ") + name + " > 0");
}

std::int64_t resolve_positive_size_(const std::optional<std::int64_t>& provided,
                                    std::int64_t fallback,
                                    const char* name) {
  const std::int64_t value = provided.has_value() ? *provided : fallback;
  require_positive_(value, name);
  return value;
}

struct FoldSizes {
  std::int64_t train = 1;
  std::int64_t test = 1;
  std::int64_t step = 1;
};

// Fills in whichever of the training, test and step sizes the caller left open. With k test sets, gap g and n observations the three satisfy
//   k - 1 = (n - train - test - g) / step,
// and an unknown value follows from that relation; with nothing given all three equal floor((n - g) / (k + 1)); when only one of test or step is given the
// other equals it.
FoldSizes resolve_fold_sizes_(const std::optional<std::int64_t>& train_opt,
                              const std::optional<std::int64_t>& test_opt,
                              const std::optional<std::int64_t>& step_opt,
                              std::int64_t n,
                              std::int64_t k,
                              std::int64_t gap) {
  const auto at_least_one = [](std::int64_t v) { return std::max<std::int64_t>(1, v); };
  FoldSizes out;
  if (train_opt) require_positive_(*train_opt, "train size");
  if (test_opt) require_positive_(*test_opt, "test_size");
  if (step_opt) require_positive_(*step_opt, "step_size");
  const bool has_train = train_opt.has_value(), has_test = test_opt.has_value(), has_step = step_opt.has_value();
  if (!has_train && !has_test && !has_step) {
    out.train = out.test = out.step = at_least_one((n - gap) / (k + 1));
  } else if (!has_train && !has_test) {
    out.test = out.step = *step_opt;
    out.train = at_least_one(n - gap - out.test - out.step * (k - 1));
  } else if (!has_train && !has_step) {
    out.test = out.step = *test_opt;
    out.train = at_least_one(n - gap - out.test - out.step * (k - 1));
  } else if (!has_test && !has_step) {
    out.train = *train_opt;
    out.test = out.step = at_least_one((n - gap - out.train) / k);
  } else if (!has_test) {
    out.train = *train_opt;
    out.step = *step_opt;
    out.test = at_least_one(n - gap - out.train - out.step * (k - 1));
  } else if (!has_train) {
    out.test = *test_opt;
    out.step = *step_opt;
    out.train = at_least_one(n - gap - out.test - out.step * (k - 1));
  } else if (!has_step) {
    out.train = *train_opt;
    out.test = *test_opt;
    out.step = (k > 1) ? at_least_one((n - gap - out.test - out.train) / (k - 1)) : out.test;
  } else {
    out = {*train_opt, *test_opt, *step_opt};
  }
  PLUSS_CHECK(n - (out.train + out.test + gap) - out.step * (k - 1) >= 0,
              "subsp::stats::ts::TsPartition: the requested sizes do not fit num_test_sets folds in num_observations");
  return out;
}

PartitionWindow make_window_(std::int64_t train_start,
                             std::int64_t train_end,
                             std::int64_t test_start,
                             std::int64_t test_end,
                             std::int64_t max_lag) {
  PartitionWindow window;
  window.train_start = train_start;
  window.train_end = train_end;
  window.test_start = test_start;
  window.test_end = test_end;
  if (max_lag > 0 && test_start > 0) {
    window.test_buffer_start = std::max<std::int64_t>(0, test_start - max_lag);
    window.test_buffer_end = test_start - 1;
  } else {
    window.test_buffer_start = 0;
    window.test_buffer_end = -1;
  }
  return window;
}

void validate_window_bounds_(const PartitionWindow& window,
                             std::int64_t num_observations,
                             std::int64_t min_train_size,
                             std::int64_t gap_size,
                             const char* family) {
  PLUSS_CHECK(window.train_start >= 0,
              std::string("subsp::stats::ts::TsPartition: invalid train_start for ") + family);
  PLUSS_CHECK(window.train_end < num_observations,
              std::string("subsp::stats::ts::TsPartition: invalid train_end for ") + family);
  PLUSS_CHECK(window.test_start >= 0,
              std::string("subsp::stats::ts::TsPartition: invalid test_start for ") + family);
  PLUSS_CHECK(window.test_end < num_observations,
              std::string("subsp::stats::ts::TsPartition: invalid test_end for ") + family);
  PLUSS_CHECK(window.train_end >= window.train_start,
              std::string("subsp::stats::ts::TsPartition: empty training set for ") + family);
  PLUSS_CHECK(window.test_end >= window.test_start,
              std::string("subsp::stats::ts::TsPartition: empty test set for ") + family);
  PLUSS_CHECK(window.train_size() >= min_train_size,
              std::string("subsp::stats::ts::TsPartition: training window too short for ") + family);
  PLUSS_CHECK(window.test_start - window.train_end - 1 >= gap_size,
              std::string("subsp::stats::ts::TsPartition: gap constraint violated for ") + family);
}

std::vector<PartitionWindow> build_expanding_windows_(std::int64_t num_observations,
                                                       std::int64_t num_test_sets,
                                                       const PartitionOptions& options) {
  require_positive_(num_test_sets, "num_test_sets");

  const std::int64_t gap_size = options.gap_size;
  const std::int64_t max_lag = options.max_lag;
  const FoldSizes sizes = resolve_fold_sizes_(options.min_train_size, options.test_size, options.step_size, num_observations, num_test_sets, gap_size);
  const std::int64_t test_size = sizes.test;
  const std::int64_t step_size = sizes.step;
  const std::int64_t min_train_size = sizes.train;
  const std::int64_t max_train_size = resolve_positive_size_(
      options.max_train_size,
      std::max<std::int64_t>(1, num_observations - 1),
      "max_train_size");

  PLUSS_CHECK(max_train_size >= min_train_size,
              "subsp::stats::ts::TsPartition: expected max_train_size >= min_train_size");
  PLUSS_CHECK(max_lag < min_train_size,
              "subsp::stats::ts::TsPartition: expected max_lag < min_train_size");

  std::vector<PartitionWindow> windows(static_cast<std::size_t>(num_test_sets));
  if (options.direction == Direction::Forward) {
    const std::int64_t first_train_end = min_train_size - 1;
    for (std::int64_t i = 0; i < num_test_sets; ++i) {
      const std::int64_t train_end = first_train_end + i * step_size;
      const std::int64_t train_start = std::max<std::int64_t>(0, train_end - max_train_size + 1);
      const std::int64_t test_start = train_end + gap_size + 1;
      const std::int64_t test_end = test_start + test_size - 1;
      windows[static_cast<std::size_t>(i)] = make_window_(train_start, train_end, test_start, test_end, max_lag);
      validate_window_bounds_(windows[static_cast<std::size_t>(i)],
                              num_observations,
                              min_train_size,
                              gap_size,
                              "expanding_window");
    }
  } else {
    const std::int64_t last_train_end = num_observations - test_size - gap_size - 1;
    for (std::int64_t i = 0; i < num_test_sets; ++i) {
      const std::int64_t shift = (num_test_sets - 1 - i) * step_size;
      const std::int64_t train_end = last_train_end - shift;
      const std::int64_t train_start = std::max<std::int64_t>(0, train_end - max_train_size + 1);
      const std::int64_t test_start = train_end + gap_size + 1;
      const std::int64_t test_end = test_start + test_size - 1;
      windows[static_cast<std::size_t>(i)] = make_window_(train_start, train_end, test_start, test_end, max_lag);
      validate_window_bounds_(windows[static_cast<std::size_t>(i)],
                              num_observations,
                              min_train_size,
                              gap_size,
                              "expanding_window");
    }
  }

  return windows;
}

std::vector<PartitionWindow> build_sliding_windows_(std::int64_t num_observations,
                                                     std::int64_t num_test_sets,
                                                     const PartitionOptions& options) {
  require_positive_(num_test_sets, "num_test_sets");

  const std::int64_t gap_size = options.gap_size;
  const std::int64_t max_lag = options.max_lag;
  const FoldSizes sizes = resolve_fold_sizes_(options.train_size, options.test_size, options.step_size, num_observations, num_test_sets, gap_size);
  const std::int64_t train_size = sizes.train;
  const std::int64_t test_size = sizes.test;
  const std::int64_t step_size = sizes.step;

  PLUSS_CHECK(max_lag < train_size,
              "subsp::stats::ts::TsPartition: expected max_lag < train_size for sliding_window");

  std::vector<PartitionWindow> windows(static_cast<std::size_t>(num_test_sets));
  if (options.direction == Direction::Forward) {
    for (std::int64_t i = 0; i < num_test_sets; ++i) {
      const std::int64_t train_start = i * step_size;
      const std::int64_t train_end = train_start + train_size - 1;
      const std::int64_t test_start = train_end + gap_size + 1;
      const std::int64_t test_end = test_start + test_size - 1;
      windows[static_cast<std::size_t>(i)] = make_window_(train_start, train_end, test_start, test_end, max_lag);
      validate_window_bounds_(windows[static_cast<std::size_t>(i)],
                              num_observations,
                              train_size,
                              gap_size,
                              "sliding_window");
    }
  } else {
    const std::int64_t last_train_end = num_observations - test_size - gap_size - 1;
    const std::int64_t last_train_start = last_train_end - train_size + 1;
    for (std::int64_t i = 0; i < num_test_sets; ++i) {
      const std::int64_t shift = (num_test_sets - 1 - i) * step_size;
      const std::int64_t train_start = last_train_start - shift;
      const std::int64_t train_end = train_start + train_size - 1;
      const std::int64_t test_start = train_end + gap_size + 1;
      const std::int64_t test_end = test_start + test_size - 1;
      windows[static_cast<std::size_t>(i)] = make_window_(train_start, train_end, test_start, test_end, max_lag);
      validate_window_bounds_(windows[static_cast<std::size_t>(i)],
                              num_observations,
                              train_size,
                              gap_size,
                              "sliding_window");
    }
  }

  return windows;
}

std::vector<PartitionWindow> build_holdout_window_(std::int64_t num_observations,
                                                    double holdout,
                                                    const PartitionOptions& options) {
  const std::int64_t gap_size = options.gap_size;
  const std::int64_t max_lag = options.max_lag;
  PLUSS_CHECK(holdout > 0.0, "subsp::stats::ts::TsPartition: holdout must be > 0");

  std::int64_t test_size = 0;
  if (holdout < 1.0) {
    test_size = static_cast<std::int64_t>(std::floor(holdout * static_cast<double>(num_observations)));
  } else {
    const auto rounded = static_cast<std::int64_t>(std::llround(holdout));
    PLUSS_CHECK(std::abs(holdout - static_cast<double>(rounded)) <= kEps,
                "subsp::stats::ts::TsPartition: holdout >= 1 must be an integer count");
    test_size = rounded;
  }
  test_size = std::max<std::int64_t>(1, test_size);

  const std::int64_t train_start = 0;
  const std::int64_t train_end = num_observations - test_size - gap_size - 1;
  const std::int64_t test_start = train_end + gap_size + 1;
  const std::int64_t test_end = test_start + test_size - 1;
  PartitionWindow window = make_window_(train_start, train_end, test_start, test_end, max_lag);
  validate_window_bounds_(window, num_observations, 1, gap_size, "holdout");

  PLUSS_CHECK(max_lag < window.train_size(),
              "subsp::stats::ts::TsPartition: expected max_lag < train_size for holdout");

  return {window};
}

Series range_indices_(std::int64_t start, std::int64_t end, Config cfg) {
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  if (end < start) {
    return pluss::series::empty({0}, idx_cfg);
  }
  return pluss::series::arange(start, end + 1, idx_cfg);
}

Series range_mask_(std::int64_t n, std::int64_t start, std::int64_t end, Config cfg) {
  const Config mask_cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(cfg.platform).with_pinned(cfg.pinned);
  Series mask = pluss::series::zeros({n}, mask_cfg);
  if (end < start) {
    return mask;
  }
  const Series idx = range_indices_(start, end, cfg);
  const Series values = pluss::series::ones({idx.numel()}, mask_cfg);
  return pluss::index_put(mask, std::vector<Series>{idx}, values, false);
}

} // namespace

TsPartition::TsPartition(PartitionType type,
                         Direction direction,
                         std::int64_t num_observations,
                         std::int64_t gap_size,
                         std::int64_t max_lag,
                         std::int64_t step_size,
                         std::vector<PartitionWindow> windows)
    : type_(type),
      direction_(direction),
      num_observations_(num_observations),
      gap_size_(gap_size),
      max_lag_(max_lag),
      step_size_(step_size),
      windows_(std::move(windows)) {}

TsPartition TsPartition::expanding_window(std::int64_t num_observations,
                                          std::int64_t num_test_sets,
                                          const PartitionOptions& options) {
  require_observations_(num_observations);
  require_non_negative_(options.gap_size, "gap_size");
  require_non_negative_(options.max_lag, "max_lag");
  const auto windows = build_expanding_windows_(num_observations, num_test_sets, options);
  const std::int64_t step_size = resolve_fold_sizes_(
      options.min_train_size, options.test_size, options.step_size, num_observations, num_test_sets, options.gap_size).step;
  return TsPartition(PartitionType::ExpandingWindow,
                     options.direction,
                     num_observations,
                     options.gap_size,
                     options.max_lag,
                     step_size,
                     windows);
}

TsPartition TsPartition::sliding_window(std::int64_t num_observations,
                                        std::int64_t num_test_sets,
                                        const PartitionOptions& options) {
  require_observations_(num_observations);
  require_non_negative_(options.gap_size, "gap_size");
  require_non_negative_(options.max_lag, "max_lag");
  const auto windows = build_sliding_windows_(num_observations, num_test_sets, options);
  const std::int64_t step_size = resolve_fold_sizes_(
      options.train_size, options.test_size, options.step_size, num_observations, num_test_sets, options.gap_size).step;
  return TsPartition(PartitionType::SlidingWindow,
                     options.direction,
                     num_observations,
                     options.gap_size,
                     options.max_lag,
                     step_size,
                     windows);
}

TsPartition TsPartition::holdout(std::int64_t num_observations, double holdout, const PartitionOptions& options) {
  require_observations_(num_observations);
  require_non_negative_(options.gap_size, "gap_size");
  require_non_negative_(options.max_lag, "max_lag");
  const auto windows = build_holdout_window_(num_observations, holdout, options);
  return TsPartition(PartitionType::Holdout,
                     options.direction,
                     num_observations,
                     options.gap_size,
                     options.max_lag,
                     0,
                     windows);
}

const PartitionWindow& TsPartition::window(std::int64_t i) const {
  PLUSS_CHECK(i >= 0 && i < num_test_sets(),
              "subsp::stats::ts::TsPartition: window index out of bounds");
  return windows_[static_cast<std::size_t>(i)];
}

Series TsPartition::training(std::int64_t i, Config cfg) const {
  const auto& w = window(i);
  return range_indices_(w.train_start, w.train_end, cfg);
}

Series TsPartition::test(std::int64_t i, Config cfg) const {
  const auto& w = window(i);
  return range_indices_(w.test_start, w.test_end, cfg);
}

Series TsPartition::test_buffer(std::int64_t i, Config cfg) const {
  const auto& w = window(i);
  return range_indices_(w.test_buffer_start, w.test_buffer_end, cfg);
}

Series TsPartition::training_mask(std::int64_t i, Config cfg) const {
  const auto& w = window(i);
  return range_mask_(num_observations_, w.train_start, w.train_end, cfg);
}

Series TsPartition::test_mask(std::int64_t i, Config cfg) const {
  const auto& w = window(i);
  return range_mask_(num_observations_, w.test_start, w.test_end, cfg);
}

Series TsPartition::test_buffer_mask(std::int64_t i, Config cfg) const {
  const auto& w = window(i);
  return range_mask_(num_observations_, w.test_buffer_start, w.test_buffer_end, cfg);
}

} // namespace pluss::subsp::stats::ts
