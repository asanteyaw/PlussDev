#pragma once

#include "pluss/ai/lymph/common.h"

namespace pluss::ai::lymph::selection::validate {

struct CrossValOptions {
  CVPlanKind plan = CVPlanKind::CrossVal;
  double holdout_fraction = 0.2;
  std::int64_t folds = 5;
  std::int64_t repeats = 1;
  std::int64_t time_gap = 0;
  std::int64_t time_test_size = 0;
  std::int64_t time_max_train_size = 0;
  bool stratified = false;
  Series groups = Series::undefined();
  bool reliability_defaults = true;
  bool shuffle = true;
  std::int64_t max_parallel = 0;
  std::uint64_t random_state = 0;
  bool deterministic = true;
};

using TimeValOptions = CrossValOptions;

} // namespace pluss::ai::lymph::selection::validate

namespace pluss::ai::lymph::selection {

using CrossValOptions = validate::CrossValOptions;
using TimeValOptions = validate::TimeValOptions;

} // namespace pluss::ai::lymph::selection
