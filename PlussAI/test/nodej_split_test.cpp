#include <pluss/ai/ml.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::SplitOptions;
using pluss::ai::nodej::StatusCode;

Config i64() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

std::vector<std::int64_t> as_i64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
}

bool has_duplicates(const std::vector<std::int64_t>& indices) {
  std::unordered_set<std::int64_t> seen;
  for (std::int64_t idx : indices) {
    if (!seen.insert(idx).second) return true;
  }
  return false;
}

bool run_nodej_split_checks(std::ostream* err = nullptr) {
  {
    SplitOptions options{};
    options.holdout_fraction = 0.3;
    options.shuffle = true;
    options.random.seed = 17;
    options.random.deterministic = true;

    auto split_a = pluss::ai::nodej::holdout_split(20, options);
    auto split_b = pluss::ai::nodej::holdout_split(20, options);
    if (!split_a.ok() || !split_b.ok()) {
      if (err) *err << "holdout_split should succeed for valid options\n";
      return false;
    }

    const auto train_a = as_i64_vec(split_a.value().train_indices);
    const auto test_a = as_i64_vec(split_a.value().test_indices);
    const auto train_b = as_i64_vec(split_b.value().train_indices);
    const auto test_b = as_i64_vec(split_b.value().test_indices);

    if (train_a != train_b || test_a != test_b) {
      if (err) *err << "deterministic holdout_split must be reproducible\n";
      return false;
    }
    if (train_a.size() != 14 || test_a.size() != 6) {
      if (err) *err << "holdout sizes should match rounded fraction\n";
      return false;
    }
    if (std::is_sorted(train_a.begin(), train_a.end()) || std::is_sorted(test_a.begin(), test_a.end())) {
      if (err) *err << "shuffled holdout_split should preserve shuffled index order (not force-sort output)\n";
      return false;
    }
    if (has_duplicates(train_a) || has_duplicates(test_a)) {
      if (err) *err << "holdout indices should be unique\n";
      return false;
    }
  }

  {
    SplitOptions options{};
    options.holdout_fraction = 0.3;
    options.shuffle = false;
    options.random.seed = 17;
    options.random.deterministic = true;

    auto split_or = pluss::ai::nodej::holdout_split(10, options);
    if (!split_or.ok()) {
      if (err) *err << "holdout_split should succeed for deterministic unshuffled options\n";
      return false;
    }
    const auto train = as_i64_vec(split_or.value().train_indices);
    const auto test = as_i64_vec(split_or.value().test_indices);
    const std::vector<std::int64_t> expected_train{0, 1, 2, 3, 4, 5, 6};
    const std::vector<std::int64_t> expected_test{7, 8, 9};
    if (train != expected_train || test != expected_test) {
      if (err) *err << "unshuffled holdout_split should return train prefix and test suffix\n";
      return false;
    }
  }

  {
    SplitOptions options{};
    options.folds = 5;
    options.shuffle = false;

    auto plan_or = pluss::ai::nodej::fold_plan(11, options);
    if (!plan_or.ok()) {
      if (err) *err << "fold_plan should succeed for valid options\n";
      return false;
    }
    const auto& plan = plan_or.value();
    if (plan.folds.size() != 5) {
      if (err) *err << "fold_plan should emit requested fold count\n";
      return false;
    }

    std::vector<std::int64_t> all_test{};
    std::array<std::size_t, 5> expected_sizes{3, 2, 2, 2, 2};
    for (std::size_t i = 0; i < plan.folds.size(); ++i) {
      const auto test = as_i64_vec(plan.folds[i].test_indices);
      if (test.size() != expected_sizes[i]) {
        if (err) *err << "fold_plan test fold sizes should be balanced\n";
        return false;
      }
      all_test.insert(all_test.end(), test.begin(), test.end());
    }

    std::sort(all_test.begin(), all_test.end());
    if (all_test.size() != 11 || all_test.front() != 0 || all_test.back() != 10) {
      if (err) *err << "fold_plan should cover each observation exactly once in test folds\n";
      return false;
    }
    if (std::adjacent_find(all_test.begin(), all_test.end()) != all_test.end()) {
      if (err) *err << "fold_plan test folds should be disjoint\n";
      return false;
    }
  }

  {
    SplitOptions options{};
    options.folds = 3;
    options.shuffle = false;
    options.random.seed = 99;
    options.random.deterministic = true;

    const Series labels = pluss::make_series(std::vector<std::int64_t>{
                                                 0, 0, 0, 0, 0, 0,
                                                 1, 1, 1, 1, 1, 1},
                                             {12},
                                             i64());
    auto plan_or = pluss::ai::nodej::strat_fold_plan(labels, options);
    if (!plan_or.ok()) {
      if (err) *err << "strat_fold_plan should succeed for valid labels\n";
      return false;
    }

    const std::vector<std::int64_t> y = as_i64_vec(labels);
    for (const auto& fold : plan_or.value().folds) {
      const auto test = as_i64_vec(fold.test_indices);
      std::int64_t zeros = 0;
      std::int64_t ones = 0;
      for (std::int64_t idx : test) {
        if (y[static_cast<std::size_t>(idx)] == 0) ++zeros;
        if (y[static_cast<std::size_t>(idx)] == 1) ++ones;
      }
      if (!(zeros == 2 && ones == 2)) {
        if (err) *err << "strat_fold_plan should preserve class balance per fold in this setup\n";
        return false;
      }
    }
  }

  {
    SplitOptions options{};
    options.holdout_fraction = 0.3;
    options.shuffle = false;

    const Series labels = pluss::make_series(std::vector<std::int64_t>{
                                                 0, 0, 0, 0, 0, 0,
                                                 1, 1, 1, 1},
                                             {10},
                                             i64());
    auto split_or = pluss::ai::nodej::strat_holdout_split(labels, options);
    if (!split_or.ok()) {
      if (err) *err << "strat_holdout_split should succeed for valid labels\n";
      return false;
    }

    const auto test = as_i64_vec(split_or.value().test_indices);
    const auto y = as_i64_vec(labels);
    std::int64_t zeros = 0;
    std::int64_t ones = 0;
    for (std::int64_t idx : test) {
      if (y[static_cast<std::size_t>(idx)] == 0) ++zeros;
      if (y[static_cast<std::size_t>(idx)] == 1) ++ones;
    }
    if (!(test.size() == 3 && zeros == 2 && ones == 1)) {
      if (err) *err << "strat_holdout_split should preserve class proportion closely\n";
      return false;
    }
  }

  {
    SplitOptions options{};
    options.folds = 3;
    options.shuffle = false;
    options.time_test_size = 2;
    options.time_gap = 1;
    options.time_max_train_size = 4;

    auto plan_or = pluss::ai::nodej::time_series_fold_plan(12, options);
    if (!plan_or.ok()) {
      if (err) *err << "time_series_fold_plan should succeed for valid options\n";
      return false;
    }

    const auto& folds = plan_or.value().folds;
    if (folds.size() != 3) {
      if (err) *err << "time_series_fold_plan should emit requested fold count\n";
      return false;
    }

    const std::array<std::size_t, 3> expected_train_sizes{4, 4, 4};
    const std::array<std::size_t, 3> expected_test_sizes{2, 2, 2};
    const std::array<std::int64_t, 3> expected_test_start{6, 8, 10};
    for (std::size_t i = 0; i < folds.size(); ++i) {
      const auto train = as_i64_vec(folds[i].train_indices);
      const auto test = as_i64_vec(folds[i].test_indices);
      if (train.size() != expected_train_sizes[i] || test.size() != expected_test_sizes[i]) {
        if (err) *err << "time_series_fold_plan should respect train/test window sizes\n";
        return false;
      }
      if (test.front() != expected_test_start[i]) {
        if (err) *err << "time_series_fold_plan should place contiguous test windows at expected starts\n";
        return false;
      }
      if (!(train.back() + 1 < test.front())) {
        if (err) *err << "time_series_fold_plan should enforce configured gap\n";
        return false;
      }
    }
  }

  {
    SplitOptions bad{};
    bad.holdout_fraction = 1.0;
    auto split_or = pluss::ai::nodej::holdout_split(10, bad);
    if (split_or.ok() || split_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "holdout_split should reject invalid holdout fractions\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej split plans and holdout partitioning") {
  REQUIRE(run_nodej_split_checks());
}
#else
int main() {
  if (!run_nodej_split_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_split_test passed\n";
  return 0;
}
#endif
