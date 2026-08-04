#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

void validate_non_negative_(std::int64_t value, const char* name) {
  PLUSS_CHECK(value >= 0, std::string("subsp::stats::ts: expected ") + name + " >= 0");
}

std::vector<std::int64_t> sort_unique_lags_(std::vector<std::int64_t> values, std::int64_t min_value, const char* name) {
  if (values.empty()) {
    return values;
  }
  const auto invalid = std::find_if(values.begin(), values.end(), [min_value](const std::int64_t v) {
    return v < min_value;
  });
  PLUSS_CHECK(invalid == values.end(),
              std::string("subsp::stats::ts: expected all values in ") + name + " >= " + std::to_string(min_value));
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

std::vector<std::vector<std::int64_t>> normalize_lag_matrix_(const std::vector<std::vector<std::int64_t>>& input,
                                                             std::int64_t expected_size,
                                                             std::int64_t min_value,
                                                             bool default_if_empty,
                                                             std::int64_t default_value,
                                                             const char* name) {
  if (expected_size <= 0) {
    return {};
  }

  if (input.empty()) {
    std::vector<std::vector<std::int64_t>> out(static_cast<std::size_t>(expected_size));
    if (default_if_empty) {
      std::vector<std::int64_t> defaults{default_value};
      defaults = sort_unique_lags_(std::move(defaults), min_value, name);
      std::fill(out.begin(), out.end(), defaults);
    }
    return out;
  }

  PLUSS_CHECK(static_cast<std::int64_t>(input.size()) == 1 || static_cast<std::int64_t>(input.size()) == expected_size,
              std::string("subsp::stats::ts: expected ") + name +
                  " to have size 1 or " + std::to_string(expected_size));

  std::vector<std::vector<std::int64_t>> out;
  if (static_cast<std::int64_t>(input.size()) == 1 && expected_size > 1) {
    const auto shared = sort_unique_lags_(input.front(), min_value, name);
    out.resize(static_cast<std::size_t>(expected_size), shared);
  } else {
    out.reserve(static_cast<std::size_t>(expected_size));
    std::transform(input.begin(), input.end(), std::back_inserter(out), [min_value, name](const auto& lags) {
      return sort_unique_lags_(lags, min_value, name);
    });
  }
  return out;
}

std::vector<std::int64_t> complement_indices_(std::int64_t size, const std::vector<std::int64_t>& picked) {
  std::vector<std::int64_t> out;
  out.reserve(static_cast<std::size_t>(size - static_cast<std::int64_t>(picked.size())));
  std::vector<bool> flags(static_cast<std::size_t>(size), false);
  for (const auto idx : picked) {
    flags[static_cast<std::size_t>(idx)] = true;
  }
  for (std::int64_t i = 0; i < size; ++i) {
    if (!flags[static_cast<std::size_t>(i)]) {
      out.push_back(i);
    }
  }
  return out;
}

Series bool_mask_matrix_(std::int64_t rows, std::int64_t cols, Config cfg) {
  const Config mask_cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return pluss::series::zeros({rows, cols}, mask_cfg);
}

Series bool_ones_(std::int64_t n, Config cfg) {
  const Config mask_cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return pluss::series::ones({n}, mask_cfg);
}

Series int_idx_(std::int64_t start, std::int64_t end, Config cfg) {
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return pluss::series::arange(start, end, idx_cfg);
}

Series matrix_with_shifted_source_(const Series& source, std::int64_t out_rows, std::int64_t shift, std::int64_t max_lead, Config cfg) {
  const Config out_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  Series out = pluss::series::full({out_rows}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  const std::int64_t nobs = source.numel();
  const std::int64_t start = shift + max_lead;
  PLUSS_CHECK(start >= 0, "subsp::stats::ts::build_lagged_predictors: negative start offset");
  PLUSS_CHECK(start + nobs <= out_rows, "subsp::stats::ts::build_lagged_predictors: shifted slice exceeds bounds");
  const Series idx = int_idx_(start, start + nobs, cfg);
  out = pluss::index_put(out, std::vector<Series>{idx}, source.to(out_cfg.platform, out_cfg.dtype), false);
  return out.reshape({out_rows, 1});
}

Series as_matrix_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0], 1});
  }
  PLUSS_CHECK(sizes.size() == 2, std::string("subsp::stats::ts::") + name + ": expected 1D or 2D Series");
  return x;
}

Series as_vector_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::stats::ts::") + name + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

Series concat_columns_(const std::vector<Series>& columns, std::int64_t rows, Config cfg) {
  const Config out_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  if (columns.empty()) {
    return pluss::series::empty({rows, 0}, out_cfg);
  }
  if (columns.size() == 1) {
    return columns.front();
  }
  return columns.front().unite(std::vector<Series>(columns.begin() + 1, columns.end()), 1);
}

Series build_shifted_block_(const Series& matrix,
                            std::span<const std::int64_t> source_columns,
                            const std::vector<std::vector<std::int64_t>>& shifts,
                            std::int64_t out_rows,
                            std::int64_t max_lead,
                            Config cfg) {
  PLUSS_CHECK(static_cast<std::size_t>(source_columns.size()) == shifts.size(),
              "subsp::stats::ts::build_lagged_predictors: source_columns and shifts size mismatch");
  std::vector<Series> lagged_columns;
  const std::size_t total_lags =
      std::accumulate(shifts.begin(), shifts.end(), static_cast<std::size_t>(0), [](std::size_t acc, const auto& s) {
        return acc + s.size();
      });
  lagged_columns.reserve(total_lags);

  for (std::size_t i = 0; i < source_columns.size(); ++i) {
    const Series source =
        matrix.select(1, source_columns[i]).reshape({matrix.sizes()[0]}).to(cfg.platform, ScalarType::Float64);
    for (const auto shift : shifts[i]) {
      lagged_columns.push_back(matrix_with_shifted_source_(source, out_rows, shift, max_lead, cfg));
    }
  }

  return concat_columns_(lagged_columns, out_rows, cfg);
}

} // namespace

LagPlan build_lag_plan(std::int64_t num_predictors, const LagPlanOptions& options) {
  validate_non_negative_(num_predictors, "num_predictors");

  LagPlan plan;
  plan.horizon = sort_unique_lags_(options.horizon, 1, "horizon");
  PLUSS_CHECK(!plan.horizon.empty(), "subsp::stats::ts::build_lag_plan: horizon must be non-empty");
  plan.response_lags = sort_unique_lags_(options.response_lags, 1, "response_lags");

  plan.leading_predictor_indices = sort_unique_lags_(options.leading_predictors, 0, "leading_predictors");
  const auto invalid_leading = std::find_if(plan.leading_predictor_indices.begin(),
                                            plan.leading_predictor_indices.end(),
                                            [num_predictors](const std::int64_t idx) { return idx >= num_predictors; });
  PLUSS_CHECK(invalid_leading == plan.leading_predictor_indices.end(),
              "subsp::stats::ts::build_lag_plan: leading predictor index out of range");

  plan.nonleading_predictor_indices = complement_indices_(num_predictors, plan.leading_predictor_indices);

  plan.predictor_lags = normalize_lag_matrix_(options.predictor_lags,
                                              static_cast<std::int64_t>(plan.nonleading_predictor_indices.size()),
                                              1,
                                              true,
                                              1,
                                              "predictor_lags");

  const auto leading_lags = normalize_lag_matrix_(options.leading_predictor_lags,
                                                  static_cast<std::int64_t>(plan.leading_predictor_indices.size()),
                                                  0,
                                                  true,
                                                  0,
                                                  "leading_predictor_lags");

  plan.max_lead = std::max<std::int64_t>(0, *std::max_element(plan.horizon.begin(), plan.horizon.end()) - 1);
  plan.max_lag = 0;
  if (!plan.response_lags.empty()) {
    plan.max_lag = std::max<std::int64_t>(plan.max_lag, *std::max_element(plan.response_lags.begin(), plan.response_lags.end()));
  }
  for (const auto& lags : plan.predictor_lags) {
    if (!lags.empty()) {
      plan.max_lag = std::max<std::int64_t>(plan.max_lag, *std::max_element(lags.begin(), lags.end()));
    }
  }

  plan.leading_unique_lags.reserve(leading_lags.size());
  for (const auto& raw_lags : leading_lags) {
    std::vector<std::int64_t> actual;
    actual.reserve(raw_lags.size() * plan.horizon.size());
    for (const auto h : plan.horizon) {
      for (const auto lag : raw_lags) {
        actual.push_back(-(h - 1 - lag));
      }
    }
    auto unique_actual = sort_unique_lags_(std::move(actual), std::numeric_limits<std::int64_t>::min(), "leading_actual_lags");
    const auto positive = std::find_if(unique_actual.begin(), unique_actual.end(), [](const std::int64_t v) { return v > 0; });
    if (positive != unique_actual.end()) {
      plan.max_lag = std::max<std::int64_t>(plan.max_lag, *std::max_element(positive, unique_actual.end()));
    }
    plan.leading_unique_lags.push_back(std::move(unique_actual));
  }

  const std::int64_t nonleading_count = std::accumulate(plan.predictor_lags.begin(),
                                                        plan.predictor_lags.end(),
                                                        static_cast<std::int64_t>(0),
                                                        [](const std::int64_t acc, const auto& lags) {
                                                          return acc + static_cast<std::int64_t>(lags.size());
                                                        });
  const std::int64_t response_count = static_cast<std::int64_t>(plan.response_lags.size());
  const std::int64_t leading_count = std::accumulate(plan.leading_unique_lags.begin(),
                                                     plan.leading_unique_lags.end(),
                                                     static_cast<std::int64_t>(0),
                                                     [](const std::int64_t acc, const auto& lags) {
                                                       return acc + static_cast<std::int64_t>(lags.size());
                                                     });
  plan.feature_count = nonleading_count + response_count + leading_count;

  const Config mask_cfg = Config{}.with_dtype(ScalarType::Bool);
  Series mask = bool_mask_matrix_(static_cast<std::int64_t>(plan.horizon.size()), plan.feature_count, mask_cfg);
  const std::int64_t always_on_prefix = nonleading_count + response_count;

  if (always_on_prefix > 0) {
    const std::int64_t rows = static_cast<std::int64_t>(plan.horizon.size());
    const Series row_idx = int_idx_(0, rows, mask_cfg).reshape({rows, 1}).expand(std::vector<std::int64_t>{rows, always_on_prefix}).reshape({rows * always_on_prefix});
    const Series col_idx = int_idx_(0, always_on_prefix, mask_cfg).reshape({1, always_on_prefix}).expand(std::vector<std::int64_t>{rows, always_on_prefix}).reshape({rows * always_on_prefix});
    mask = pluss::index_put(mask, std::vector<Series>{row_idx, col_idx}, bool_ones_(rows * always_on_prefix, mask_cfg), false);
  }

  std::int64_t leading_col_offset = always_on_prefix;
  for (std::size_t p = 0; p < leading_lags.size(); ++p) {
    const auto& raw = leading_lags[p];
    const auto& unique_actual = plan.leading_unique_lags[p];
    for (std::size_t hidx = 0; hidx < plan.horizon.size(); ++hidx) {
      const auto h = plan.horizon[hidx];
      std::vector<std::int64_t> active_cols;
      active_cols.reserve(unique_actual.size());
      for (std::size_t uidx = 0; uidx < unique_actual.size(); ++uidx) {
        const auto use_this_lag = std::any_of(raw.begin(), raw.end(), [h, target = unique_actual[uidx]](const std::int64_t lag) {
          return (-(h - 1 - lag)) == target;
        });
        if (use_this_lag) {
          active_cols.push_back(leading_col_offset + static_cast<std::int64_t>(uidx));
        }
      }
      if (!active_cols.empty()) {
        const Series row_idx = pluss::series::full({static_cast<std::int64_t>(active_cols.size())},
                                           static_cast<double>(hidx),
                                           Config{}.with_dtype(ScalarType::Int64).with_platform(mask_cfg.platform));
        const Series col_idx = pluss::make_series(active_cols,
                                                  {static_cast<std::int64_t>(active_cols.size())},
                                                  Config{}.with_dtype(ScalarType::Int64).with_platform(mask_cfg.platform));
        mask = pluss::index_put(mask, std::vector<Series>{row_idx, col_idx}, bool_ones_(static_cast<std::int64_t>(active_cols.size()), mask_cfg), false);
      }
    }
    leading_col_offset += static_cast<std::int64_t>(unique_actual.size());
  }

  plan.prepared_predictors_per_horizon = mask;
  return plan;
}

Series build_lagged_predictors(const Series& x, const Series& y, const LagPlan& plan, Config cfg) {
  const bool needs_x = !plan.nonleading_predictor_indices.empty() || !plan.leading_predictor_indices.empty();
  const bool needs_y = !plan.response_lags.empty();

  Series x2 = Series::undefined();
  if (needs_x) {
    x2 = as_matrix_(x, "build_lagged_predictors");
    PLUSS_CHECK(x2.sizes()[1] > 0, "subsp::stats::ts::build_lagged_predictors: expected X with at least one predictor column");
    const std::int64_t max_index = std::max(
        plan.leading_predictor_indices.empty() ? static_cast<std::int64_t>(-1)
                                               : *std::max_element(plan.leading_predictor_indices.begin(), plan.leading_predictor_indices.end()),
        plan.nonleading_predictor_indices.empty() ? static_cast<std::int64_t>(-1)
                                                  : *std::max_element(plan.nonleading_predictor_indices.begin(),
                                                                      plan.nonleading_predictor_indices.end()));
    PLUSS_CHECK(max_index < x2.sizes()[1],
                "subsp::stats::ts::build_lagged_predictors: lag plan references predictor index outside X");
  }

  Series y1 = Series::undefined();
  if (needs_y) {
    y1 = as_vector_(y, "build_lagged_predictors");
  }

  const std::int64_t nobs = needs_y ? y1.numel() : x2.sizes()[0];
  PLUSS_CHECK(nobs > 0, "subsp::stats::ts::build_lagged_predictors: expected positive observation count");
  if (needs_x && needs_y) {
    PLUSS_CHECK(x2.sizes()[0] == y1.numel(),
                "subsp::stats::ts::build_lagged_predictors: X and Y must have equal observation counts");
  }

  const std::int64_t out_rows = nobs + plan.max_lag + plan.max_lead;
  const Config out_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);

  std::vector<Series> blocks;
  blocks.reserve(3);
  if (!plan.nonleading_predictor_indices.empty()) {
    blocks.push_back(build_shifted_block_(x2,
                                          std::span<const std::int64_t>(plan.nonleading_predictor_indices.data(),
                                                                         plan.nonleading_predictor_indices.size()),
                                          plan.predictor_lags,
                                          out_rows,
                                          plan.max_lead,
                                          out_cfg));
  } else {
    blocks.push_back(pluss::series::empty({out_rows, 0}, out_cfg));
  }

  if (!plan.response_lags.empty()) {
    const Series y2 = y1.reshape({nobs, 1});
    const std::int64_t response_col = 0;
    blocks.push_back(build_shifted_block_(y2,
                                          std::span<const std::int64_t>(&response_col, 1),
                                          std::vector<std::vector<std::int64_t>>{plan.response_lags},
                                          out_rows,
                                          plan.max_lead,
                                          out_cfg));
  } else {
    blocks.push_back(pluss::series::empty({out_rows, 0}, out_cfg));
  }

  if (!plan.leading_predictor_indices.empty()) {
    blocks.push_back(build_shifted_block_(x2,
                                          std::span<const std::int64_t>(plan.leading_predictor_indices.data(),
                                                                         plan.leading_predictor_indices.size()),
                                          plan.leading_unique_lags,
                                          out_rows,
                                          plan.max_lead,
                                          out_cfg));
  } else {
    blocks.push_back(pluss::series::empty({out_rows, 0}, out_cfg));
  }

  std::vector<Series> nonempty;
  nonempty.reserve(3);
  for (const auto& block : blocks) {
    if (block.sizes()[1] > 0) {
      nonempty.push_back(block);
    }
  }
  Series out = concat_columns_(nonempty, out_rows, out_cfg);
  PLUSS_CHECK(out.sizes()[1] == plan.feature_count,
              "subsp::stats::ts::build_lagged_predictors: output column count does not match lag plan");
  return out;
}

} // namespace pluss::subsp::stats::ts
