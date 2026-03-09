#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace pluss::subsp::stat::ts {

enum class PartitionType : std::uint8_t {
  ExpandingWindow = 0,
  SlidingWindow = 1,
  Holdout = 2,
};

enum class Direction : std::uint8_t {
  Forward = 0,
  Reverse = 1,
};

struct PartitionOptions {
  Direction direction = Direction::Reverse;
  std::int64_t gap_size = 0;
  std::int64_t max_lag = 0;
  std::optional<std::int64_t> max_train_size{};
  std::optional<std::int64_t> min_train_size{};
  std::optional<std::int64_t> train_size{};
  std::optional<std::int64_t> test_size{};
  std::optional<std::int64_t> step_size{};
};

struct PartitionWindow {
  std::int64_t train_start = 0;
  std::int64_t train_end = -1;
  std::int64_t test_start = 0;
  std::int64_t test_end = -1;
  std::int64_t test_buffer_start = 0;
  std::int64_t test_buffer_end = -1;

  [[nodiscard]] std::int64_t train_size() const noexcept {
    return (train_end >= train_start) ? (train_end - train_start + 1) : 0;
  }

  [[nodiscard]] std::int64_t test_size() const noexcept {
    return (test_end >= test_start) ? (test_end - test_start + 1) : 0;
  }

  [[nodiscard]] std::int64_t test_buffer_size() const noexcept {
    return (test_buffer_end >= test_buffer_start) ? (test_buffer_end - test_buffer_start + 1) : 0;
  }
};

class CODEM_API TsPartition {
 public:
  static TsPartition expanding_window(std::int64_t num_observations,
                                      std::int64_t num_test_sets,
                                      const PartitionOptions& options = {});

  static TsPartition sliding_window(std::int64_t num_observations,
                                    std::int64_t num_test_sets,
                                    const PartitionOptions& options = {});

  static TsPartition holdout(std::int64_t num_observations,
                             double holdout = 0.1,
                             const PartitionOptions& options = {});

  [[nodiscard]] PartitionType type() const noexcept { return type_; }
  [[nodiscard]] Direction direction() const noexcept { return direction_; }
  [[nodiscard]] std::int64_t num_observations() const noexcept { return num_observations_; }
  [[nodiscard]] std::int64_t num_test_sets() const noexcept { return static_cast<std::int64_t>(windows_.size()); }
  [[nodiscard]] std::int64_t gap_size() const noexcept { return gap_size_; }
  [[nodiscard]] std::int64_t max_lag() const noexcept { return max_lag_; }
  [[nodiscard]] std::int64_t step_size() const noexcept { return step_size_; }

  [[nodiscard]] const PartitionWindow& window(std::int64_t i) const;
  [[nodiscard]] std::span<const PartitionWindow> windows() const noexcept { return windows_; }

  [[nodiscard]] pluss::codem::Series training(std::int64_t i, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series test(std::int64_t i, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series test_buffer(std::int64_t i, pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series training_mask(std::int64_t i, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series test_mask(std::int64_t i, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series test_buffer_mask(std::int64_t i, pluss::codem::Config cfg = {}) const;

 private:
  TsPartition(PartitionType type,
              Direction direction,
              std::int64_t num_observations,
              std::int64_t gap_size,
              std::int64_t max_lag,
              std::int64_t step_size,
              std::vector<PartitionWindow> windows);

  PartitionType type_ = PartitionType::Holdout;
  Direction direction_ = Direction::Reverse;
  std::int64_t num_observations_ = 0;
  std::int64_t gap_size_ = 0;
  std::int64_t max_lag_ = 0;
  std::int64_t step_size_ = 0;
  std::vector<PartitionWindow> windows_{};
};

struct LagPlanOptions {
  std::vector<std::int64_t> horizon{1};
  std::vector<std::int64_t> response_lags{1};
  std::vector<std::int64_t> leading_predictors{};
  std::vector<std::vector<std::int64_t>> predictor_lags{};
  std::vector<std::vector<std::int64_t>> leading_predictor_lags{};
};

struct LagPlan {
  std::vector<std::int64_t> horizon{};
  std::vector<std::int64_t> response_lags{};
  std::vector<std::int64_t> nonleading_predictor_indices{};
  std::vector<std::int64_t> leading_predictor_indices{};
  std::vector<std::vector<std::int64_t>> predictor_lags{};
  std::vector<std::vector<std::int64_t>> leading_unique_lags{};
  pluss::codem::Series prepared_predictors_per_horizon = pluss::codem::Series::undefined();
  std::int64_t max_lag = 0;
  std::int64_t max_lead = 0;
  std::int64_t feature_count = 0;
};

CODEM_API LagPlan build_lag_plan(std::int64_t num_predictors, const LagPlanOptions& options);

CODEM_API pluss::codem::Series build_lagged_predictors(const pluss::codem::Series& x,
                                                       const pluss::codem::Series& y,
                                                       const LagPlan& plan,
                                                       pluss::codem::Config cfg = {});

enum class DirectLearnerKind : std::uint8_t {
  OLS = 0,
  Ridge = 1,
};

enum class RegressionLoss : std::uint8_t {
  MSE = 0,
  MAE = 1,
};

struct DirectForecasterOptions {
  LagPlanOptions lag{};
  DirectLearnerKind learner = DirectLearnerKind::OLS;
  bool fit_intercept = true;
  double ridge_lambda = 1e-6;
  double pinv_rcond = 1e-12;
};

struct DirectStepModel {
  std::int64_t horizon = 1;
  std::vector<std::int64_t> active_columns{};
  pluss::codem::Series beta = pluss::codem::Series::undefined();
};

class CODEM_API DirectForecaster {
 public:
  static DirectForecaster fit(const pluss::codem::Series& x,
                              const pluss::codem::Series& y,
                              const DirectForecasterOptions& options = {},
                              pluss::codem::Config cfg = {});

  [[nodiscard]] pluss::codem::Series predict(const pluss::codem::Series& x,
                                             const pluss::codem::Series& y,
                                             pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series prepared_predictors(const pluss::codem::Series& x,
                                                         const pluss::codem::Series& y,
                                                         std::int64_t horizon_step,
                                                         pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series forecast(const pluss::codem::Series& x,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series loss(const pluss::codem::Series& x,
                                          const pluss::codem::Series& y,
                                          RegressionLoss kind = RegressionLoss::MSE,
                                          pluss::codem::Config cfg = {}) const;

  [[nodiscard]] const LagPlan& lag_plan() const noexcept { return lag_plan_; }
  [[nodiscard]] const DirectForecasterOptions& options() const noexcept { return options_; }
  [[nodiscard]] std::int64_t num_steps() const noexcept { return static_cast<std::int64_t>(steps_.size()); }
  [[nodiscard]] const DirectStepModel& step(std::int64_t i) const;
  [[nodiscard]] std::span<const DirectStepModel> steps() const noexcept { return steps_; }

 private:
  DirectForecaster(LagPlan lag_plan, DirectForecasterOptions options, std::vector<DirectStepModel> steps);

  LagPlan lag_plan_{};
  DirectForecasterOptions options_{};
  std::vector<DirectStepModel> steps_{};
};

enum class CVLossMode : std::uint8_t {
  Average = 0,
  Individual = 1,
};

class CODEM_API DirectCVForecaster {
 public:
  static DirectCVForecaster fit(const pluss::codem::Series& x,
                                const pluss::codem::Series& y,
                                const TsPartition& partition,
                                const DirectForecasterOptions& options = {},
                                pluss::codem::Config cfg = {});

  [[nodiscard]] pluss::codem::Series cvpredict(const pluss::codem::Series& x,
                                               const pluss::codem::Series& y,
                                               pluss::codem::Config cfg = {}) const;

  [[nodiscard]] pluss::codem::Series cvloss(const pluss::codem::Series& x,
                                            const pluss::codem::Series& y,
                                            CVLossMode mode = CVLossMode::Average,
                                            RegressionLoss kind = RegressionLoss::MSE,
                                            pluss::codem::Config cfg = {}) const;

  [[nodiscard]] const TsPartition& partition() const noexcept { return partition_; }
  [[nodiscard]] const DirectForecasterOptions& options() const noexcept { return options_; }
  [[nodiscard]] std::int64_t num_folds() const noexcept { return static_cast<std::int64_t>(models_.size()); }
  [[nodiscard]] std::int64_t num_steps() const noexcept;
  [[nodiscard]] const DirectForecaster& fold_model(std::int64_t i) const;
  [[nodiscard]] std::span<const DirectForecaster> fold_models() const noexcept { return models_; }

 private:
  DirectCVForecaster(TsPartition partition, DirectForecasterOptions options, std::vector<DirectForecaster> models);

  TsPartition partition_;
  DirectForecasterOptions options_{};
  std::vector<DirectForecaster> models_{};
};

enum class AcfNorm : std::uint8_t {
  Biased = 0,
  Unbiased = 1,
};

struct AcfOptions {
  std::int64_t max_lag = 20;
  AcfNorm norm = AcfNorm::Biased;
  bool demean = true;
};

struct PacfOptions {
  std::int64_t max_lag = 20;
  AcfNorm norm = AcfNorm::Biased;
  bool demean = true;
};

struct LjungBoxOptions {
  std::vector<std::int64_t> lags{10};
  std::int64_t model_df = 0;
  AcfNorm norm = AcfNorm::Biased;
  bool demean = true;
};

struct LjungBoxResult {
  pluss::codem::Series lags = pluss::codem::Series::undefined();
  pluss::codem::Series stat = pluss::codem::Series::undefined();
  pluss::codem::Series pvalue = pluss::codem::Series::undefined();
  pluss::codem::Series dof = pluss::codem::Series::undefined();
};

CODEM_API pluss::codem::Series acf(const pluss::codem::Series& x,
                                   const AcfOptions& options = {},
                                   pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series pacf(const pluss::codem::Series& x,
                                    const PacfOptions& options = {},
                                    pluss::codem::Config cfg = {});

CODEM_API LjungBoxResult ljung_box(const pluss::codem::Series& x,
                                   const LjungBoxOptions& options = {},
                                   pluss::codem::Config cfg = {});

} // namespace pluss::subsp::stat::ts
