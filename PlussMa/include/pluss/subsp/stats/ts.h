#pragma once

#include "pluss/codem/codem.h"
#include "pluss/codeh/codeh.h"
#include "pluss/config.h"
#include "pluss/export.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::pl {
class Studio;
}

namespace pluss::subsp::stats::ts {

enum class GapFillMode : std::uint8_t {
  Forward = 0,
  Backward = 1,
  Zero = 2,
  Constant = 3,
};

enum class ResampleOp : std::uint8_t {
  Mean = 0,
  Sum = 1,
  Min = 2,
  Max = 3,
  First = 4,
  Last = 5,
  Count = 6,
};

struct ResampleResult {
  std::optional<pluss::Timeline> labels{};
  pluss::codem::Series values = pluss::series::undefined();
};

CODEM_API pluss::codem::Series returns(const pluss::codem::Series& x, std::int64_t periods = 1);
CODEM_API pluss::codem::Series log_returns(const pluss::codem::Series& x, std::int64_t periods = 1);
CODEM_API pluss::codem::Series diff(const pluss::codem::Series& x, std::int64_t periods = 1);
CODEM_API pluss::codem::Series lag(const pluss::codem::Series& x, std::int64_t periods = 1);
CODEM_API pluss::codem::Series lead(const pluss::codem::Series& x, std::int64_t periods = 1);
CODEM_API pluss::codem::Series rolling_mean(const pluss::codem::Series& x,
                                            std::int64_t window,
                                            std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series rolling_std(const pluss::codem::Series& x,
                                           std::int64_t window,
                                           std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series rolling_sum(const pluss::codem::Series& x,
                                           std::int64_t window,
                                           std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series rolling_var(const pluss::codem::Series& x,
                                           std::int64_t window,
                                           std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series rolling_min(const pluss::codem::Series& x,
                                           std::int64_t window,
                                           std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series rolling_max(const pluss::codem::Series& x,
                                           std::int64_t window,
                                           std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series ema(const pluss::codem::Series& x, double alpha);
CODEM_API pluss::codem::Series ewm_mean(const pluss::codem::Series& x, double alpha);
CODEM_API pluss::codem::Series ewm_std(const pluss::codem::Series& x,
                                       double alpha,
                                       double epsilon = 1e-12);
CODEM_API pluss::codem::Series expanding_mean(const pluss::codem::Series& x,
                                              std::int64_t min_periods = 1);
CODEM_API pluss::codem::Series expanding_std(const pluss::codem::Series& x,
                                             std::int64_t min_periods = 1,
                                             double epsilon = 1e-12);
CODEM_API pluss::codem::Series zscore_rolling(const pluss::codem::Series& x,
                                              std::int64_t window,
                                              std::int64_t min_periods = 1,
                                              double epsilon = 1e-12);
CODEM_API pluss::codem::Series fill_gaps(const pluss::codem::Series& x,
                                         GapFillMode mode = GapFillMode::Forward,
                                         std::optional<double> constant = std::nullopt);
CODEM_API ResampleResult resample(const pluss::Timeline& timeline,
                                  const pluss::codem::Series& values,
                                  std::string_view freq,
                                  ResampleOp op = ResampleOp::Mean,
                                  std::int64_t step = 1);

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
  pluss::codem::Series prepared_predictors_per_horizon = pluss::series::undefined();
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
  pluss::codem::Series active_columns_index = pluss::series::undefined();
  pluss::codem::Series beta = pluss::series::undefined();
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
  pluss::codem::Series lags = pluss::series::undefined();
  pluss::codem::Series stat = pluss::series::undefined();
  pluss::codem::Series pvalue = pluss::series::undefined();
  pluss::codem::Series dof = pluss::series::undefined();
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

struct ArchLmOptions {
  std::int64_t lags = 1;
  bool demean = true;
  double alpha = 0.05;
};

struct ArchLmResult {
  double stat = 0.0;
  double pvalue = 1.0;
  double critical = 0.0;
  bool reject = false;
  std::int64_t dof = 0;
  std::int64_t effective_n = 0;
};

CODEM_API ArchLmResult arch_lm(const pluss::codem::Series& x,
                               const ArchLmOptions& options = {},
                               pluss::codem::Config cfg = {});

enum class AdfModel : std::uint8_t {
  AR = 0,
  ARD = 1,
  TS = 2,
};

struct AdfOptions {
  std::int64_t lags = 0;
  AdfModel model = AdfModel::ARD;
  double alpha = 0.05;
};

struct AdfResult {
  double stat = 0.0;
  double pvalue = 1.0;
  double critical = 0.0;
  bool reject = false;
  std::int64_t lags = 0;
  std::int64_t effective_n = 0;
};

CODEM_API AdfResult adf_test(const pluss::codem::Series& y,
                             const AdfOptions& options = {},
                             pluss::codem::Config cfg = {});

struct KpssOptions {
  std::int64_t lags = 0;
  bool trend = true;
  double alpha = 0.05;
};

struct KpssResult {
  double stat = 0.0;
  double pvalue = 1.0;
  double critical = 0.0;
  bool reject = false;
  double long_run_var = 0.0;
  std::int64_t lags = 0;
  std::int64_t effective_n = 0;
};

CODEM_API KpssResult kpss_test(const pluss::codem::Series& y,
                               const KpssOptions& options = {},
                               pluss::codem::Config cfg = {});

enum class DmLoss : std::uint8_t {
  Squared = 0,
  Absolute = 1,
  Power = 2,
};

enum class DmTail : std::uint8_t {
  Both = 0,
  Left = 1,
  Right = 2,
};

struct DmOptions {
  std::int64_t horizon = 1;
  std::int64_t max_lag = -1; // -1 uses horizon-1
  DmLoss loss = DmLoss::Squared;
  double loss_power = 2.0;
  bool hln_adjust = true;
  DmTail tail = DmTail::Both;
  double alpha = 0.05;
};

struct DmResult {
  double stat = 0.0;
  double pvalue = 1.0;
  double critical = 0.0;
  bool reject = false;
  std::int64_t horizon = 1;
  std::int64_t lags = 0;
  std::int64_t effective_n = 0;
  double mean_diff = 0.0;
  double lrv = 0.0;
};

CODEM_API DmResult dm_test(const pluss::codem::Series& err_a,
                           const pluss::codem::Series& err_b,
                           const DmOptions& options = {},
                           pluss::codem::Config cfg = {});

namespace plot {

struct CorrPlotOptions {
  std::int64_t max_lag = 20;
  AcfNorm norm = AcfNorm::Biased;
  bool demean = true;
  bool include_zero_lag = false;
  bool show_confidence_band = true;
  double confidence_level = 0.95;
  float trace_width = 1.2F;
};

struct CorrPlotPayload {
  pluss::codem::Series lags = pluss::series::undefined();
  pluss::codem::Series values = pluss::series::undefined();
  double conf_low = 0.0;
  double conf_high = 0.0;
};

struct ResidualPlotOptions {
  bool show_zero_line = true;
  float trace_width = 1.2F;
};

struct ResidualPlotPayload {
  pluss::codem::Series index = pluss::series::undefined();
  pluss::codem::Series residuals = pluss::series::undefined();
};

CODEM_API CorrPlotPayload acf_payload(const pluss::codem::Series& x,
                                      const CorrPlotOptions& options = {},
                                      pluss::codem::Config cfg = {});

CODEM_API CorrPlotPayload pacf_payload(const pluss::codem::Series& x,
                                       const CorrPlotOptions& options = {},
                                       pluss::codem::Config cfg = {});

CODEM_API ResidualPlotPayload residuals_payload(const pluss::codem::Series& residuals,
                                                pluss::codem::Config cfg = {});

CODEM_API pluss::pl::Studio& acf(pluss::pl::Studio& studio,
                                 const pluss::codem::Series& x,
                                 const CorrPlotOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API pluss::pl::Studio& pacf(pluss::pl::Studio& studio,
                                  const pluss::codem::Series& x,
                                  const CorrPlotOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API pluss::pl::Studio& residuals(pluss::pl::Studio& studio,
                                       const pluss::codem::Series& residuals,
                                       const ResidualPlotOptions& options = {},
                                       pluss::codem::Config cfg = {});

} // namespace plot

struct GarchOptions {
  std::int64_t arch_order = 1;
  std::int64_t garch_order = 1;
  bool demean = true;
  double omega0 = 0.02;
  std::vector<double> arch0{0.08};
  std::vector<double> garch0{0.90};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;
  std::int64_t max_iterations = 800;
  std::int64_t max_evaluations = 6000;
  double simplex_delta = 0.05;
  std::int64_t nm_restarts = 3;
};

struct GarchFit {
  double mean = 0.0;
  double omega = 0.02;
  std::vector<double> arch{0.08};
  std::vector<double> garch{0.90};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;

  [[nodiscard]] double persistence() const;
  [[nodiscard]] double unconditional_var() const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Garch {
 public:
  static GarchFit fit_mle(const pluss::codem::Series& y,
                          const GarchOptions& options = {},
                          pluss::codem::Config cfg = {});
};

struct Garch11Options {
  bool demean = true;
  double omega0 = 0.02;
  double alpha0 = 0.08;
  double beta0 = 0.90;
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;
  std::int64_t max_iterations = 800;
  std::int64_t max_evaluations = 6000;
  double simplex_delta = 0.05;
  std::int64_t nm_restarts = 3;
};

struct Garch11Fit {
  double mean = 0.0;
  double omega = 0.02;
  double alpha = 0.08;
  double beta = 0.90;
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;

  [[nodiscard]] double unconditional_var() const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Garch11 {
 public:
  static Garch11Fit fit_mle(const pluss::codem::Series& y,
                            const Garch11Options& options = {},
                            pluss::codem::Config cfg = {});
};

struct EgarchOptions {
  std::int64_t arch_order = 1;
  std::int64_t garch_order = 1;
  bool demean = true;
  double omega0 = -0.1;
  std::vector<double> arch0{0.10};
  std::vector<double> asym0{0.00};
  std::vector<double> garch0{0.90};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;
  std::int64_t max_iterations = 800;
  std::int64_t max_evaluations = 6000;
  double simplex_delta = 0.05;
  std::int64_t nm_restarts = 3;
};

struct EgarchFit {
  double mean = 0.0;
  double omega = -0.1;
  std::vector<double> arch{0.10};
  std::vector<double> asym{0.00};
  std::vector<double> garch{0.90};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;

  [[nodiscard]] double persistence() const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Egarch {
 public:
  static EgarchFit fit_mle(const pluss::codem::Series& y,
                           const EgarchOptions& options = {},
                           pluss::codem::Config cfg = {});
};

struct GjrOptions {
  std::int64_t arch_order = 1;
  std::int64_t garch_order = 1;
  bool demean = true;
  double omega0 = 0.02;
  std::vector<double> arch0{0.08};
  std::vector<double> asym0{0.04};
  std::vector<double> garch0{0.86};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;
  std::int64_t max_iterations = 800;
  std::int64_t max_evaluations = 6000;
  double simplex_delta = 0.05;
  std::int64_t nm_restarts = 3;
};

struct GjrFit {
  double mean = 0.0;
  double omega = 0.02;
  std::vector<double> arch{0.08};
  std::vector<double> asym{0.04};
  std::vector<double> garch{0.86};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;

  [[nodiscard]] double persistence() const;
  [[nodiscard]] double unconditional_var() const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Gjr {
 public:
  static GjrFit fit_mle(const pluss::codem::Series& y,
                        const GjrOptions& options = {},
                        pluss::codem::Config cfg = {});
};

struct MvGarchOptions {
  GarchOptions marginal{};
  double corr_shrinkage = 1e-6;
  double corr_jitter = 1e-9;
};

struct MvGarchFit {
  std::vector<GarchFit> marginals{};
  pluss::codem::Series mean = pluss::series::undefined();
  pluss::codem::Series corr = pluss::series::undefined();

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
};

class CODEM_API MvGarch {
 public:
  static MvGarchFit fit_mle(const pluss::codem::Series& y,
                            const MvGarchOptions& options = {},
                            pluss::codem::Config cfg = {});
};

struct MvEgarchOptions {
  EgarchOptions marginal{};
  double corr_shrinkage = 1e-6;
  double corr_jitter = 1e-9;
};

struct MvEgarchFit {
  std::vector<EgarchFit> marginals{};
  pluss::codem::Series mean = pluss::series::undefined();
  pluss::codem::Series corr = pluss::series::undefined();

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
};

class CODEM_API MvEgarch {
 public:
  static MvEgarchFit fit_mle(const pluss::codem::Series& y,
                             const MvEgarchOptions& options = {},
                             pluss::codem::Config cfg = {});
};

struct MvGjrOptions {
  GjrOptions marginal{};
  double corr_shrinkage = 1e-6;
  double corr_jitter = 1e-9;
};

struct MvGjrFit {
  std::vector<GjrFit> marginals{};
  pluss::codem::Series mean = pluss::series::undefined();
  pluss::codem::Series corr = pluss::series::undefined();

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
};

class CODEM_API MvGjr {
 public:
  static MvGjrFit fit_mle(const pluss::codem::Series& y,
                          const MvGjrOptions& options = {},
                          pluss::codem::Config cfg = {});
};

struct ArimaOptions {
  std::int64_t ar_order = 1;
  std::int64_t diff_order = 0;
  std::int64_t ma_order = 0;
  std::int64_t seasonal_ar_order = 0;
  std::int64_t seasonal_diff_order = 0;
  std::int64_t seasonal_ma_order = 0;
  std::int64_t seasonal_period = 0;
  bool demean = true;
  double intercept0 = 0.0;
  std::vector<double> ar0{};
  std::vector<double> ma0{};
  std::vector<double> sar0{};
  std::vector<double> sma0{};
  std::vector<double> beta0{};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;
  std::int64_t max_iterations = 800;
  std::int64_t max_evaluations = 6000;
  double simplex_delta = 0.05;
};

struct ArimaFit {
  double mean = 0.0;
  double intercept = 0.0;
  double sigma = 1.0;
  std::vector<double> ar{};
  std::vector<double> ma{};
  std::vector<double> sar{};
  std::vector<double> sma{};
  std::vector<double> beta{};
  std::int64_t diff_order = 0;
  std::int64_t seasonal_diff_order = 0;
  std::int64_t seasonal_period = 0;
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y,
                                           const pluss::codem::Series& x,
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y, const pluss::codem::Series& x) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y,
                                                    const pluss::codem::Series& x,
                                                    pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y,
                                           const pluss::codem::Series& x,
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y,
                                                  const pluss::codem::Series& x,
                                                  pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              const pluss::codem::Series& x_hist,
                                              const pluss::codem::Series& x_future,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
  [[nodiscard]] pluss::codem::Series simulate(const pluss::codem::Series& x,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Arima {
 public:
  static ArimaFit fit_mle(const pluss::codem::Series& y,
                          const ArimaOptions& options = {},
                          pluss::codem::Config cfg = {});
  static ArimaFit fit_mle(const pluss::codem::Series& y,
                          const pluss::codem::Series& x,
                          const ArimaOptions& options = {},
                          pluss::codem::Config cfg = {});
};

struct SarimaOptions {
  std::int64_t p = 1;
  std::int64_t d = 0;
  std::int64_t q = 0;
  std::int64_t P = 0;
  std::int64_t D = 0;
  std::int64_t Q = 0;
  std::int64_t s = 0;
  bool demean = true;
  double intercept0 = 0.0;
  std::vector<double> ar0{};
  std::vector<double> ma0{};
  std::vector<double> sar0{};
  std::vector<double> sma0{};
  std::vector<double> beta0{};
  double variance_floor = 1e-12;
  double stationarity_margin = 1e-6;
  double finite_diff_eps = 1e-5;
  std::int64_t max_iterations = 800;
  std::int64_t max_evaluations = 6000;
  double simplex_delta = 0.05;
};

struct SarimaFit {
  ArimaFit core{};
  std::int64_t p = 0;
  std::int64_t d = 0;
  std::int64_t q = 0;
  std::int64_t P = 0;
  std::int64_t D = 0;
  std::int64_t Q = 0;
  std::int64_t s = 0;

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y,
                                           const pluss::codem::Series& x,
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y, const pluss::codem::Series& x) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y,
                                                    const pluss::codem::Series& x,
                                                    pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series score(const pluss::codem::Series& y,
                                           const pluss::codem::Series& x,
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series observed_inf(const pluss::codem::Series& y,
                                                  const pluss::codem::Series& x,
                                                  pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              const pluss::codem::Series& x_hist,
                                              const pluss::codem::Series& x_future,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
  [[nodiscard]] pluss::codem::Series simulate(const pluss::codem::Series& x,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Sarima {
 public:
  static SarimaFit fit_mle(const pluss::codem::Series& y,
                           const SarimaOptions& options = {},
                           pluss::codem::Config cfg = {});
  static SarimaFit fit_mle(const pluss::codem::Series& y,
                           const pluss::codem::Series& x,
                           const SarimaOptions& options = {},
                           pluss::codem::Config cfg = {});
};

struct SsmModel {
  pluss::codem::Series transition = pluss::series::undefined();        // A: [m,m]
  pluss::codem::Series state_loading = pluss::series::undefined();     // B: [m,k]
  pluss::codem::Series observation = pluss::series::undefined();       // C: [n,m]
  pluss::codem::Series innovation_loading = pluss::series::undefined(); // D: [n,h]
  pluss::codem::Series mean0 = pluss::series::undefined();             // [m] or [m,1]
  pluss::codem::Series cov0 = pluss::series::undefined();              // [m,m]
};

struct SsmOptions {
  double tolerance = 0.0;
};

struct SsmFilterResult {
  pluss::codem::Series filtered_states = pluss::series::undefined();      // [T,m]
  pluss::codem::Series filtered_states_cov = pluss::series::undefined();  // [T,m,m]
  pluss::codem::Series forecasted_states = pluss::series::undefined();    // [T,m]
  pluss::codem::Series forecasted_states_cov = pluss::series::undefined(); // [T,m,m]
  pluss::codem::Series forecasted_obs = pluss::series::undefined();       // [T,n]
  pluss::codem::Series forecasted_obs_cov = pluss::series::undefined();   // [T,n,n]
  pluss::codem::Series forecast_obs_error = pluss::series::undefined();   // [T,n]
  pluss::codem::Series kalman_gain = pluss::series::undefined();          // [T,m,n]
  pluss::codem::Series data_used = pluss::series::undefined();            // [T,n], 1 if used else 0
  pluss::codem::Series loglik_per_obs = pluss::series::undefined();       // [T]
  double loglik = 0.0;
};

struct SsmSmoothResult {
  SsmFilterResult filter{};
  pluss::codem::Series smoothed_states = pluss::series::undefined();      // [T,m]
  pluss::codem::Series smoothed_states_cov = pluss::series::undefined();  // [T,m,m]
};

struct SsmForecastResult {
  pluss::codem::Series observations = pluss::series::undefined();        // [H,n]
  pluss::codem::Series observations_cov = pluss::series::undefined();    // [H,n,n]
  pluss::codem::Series states = pluss::series::undefined();              // [H,m]
  pluss::codem::Series states_cov = pluss::series::undefined();          // [H,m,m]
};

struct SsmSimResult {
  pluss::codem::Series observations = pluss::series::undefined();       // [T,n] or [P,T,n]
  pluss::codem::Series states = pluss::series::undefined();             // [T,m] or [P,T,m]
  pluss::codem::Series state_disturbance = pluss::series::undefined();  // [T,k] or [P,T,k]
  pluss::codem::Series obs_innovation = pluss::series::undefined();     // [T,h] or [P,T,h]
};

class CODEM_API Ssm {
 public:
  static SsmFilterResult filter(const pluss::codem::Series& y,
                                const SsmModel& model,
                                const SsmOptions& options = {},
                                pluss::codem::Config cfg = {});

  static SsmSmoothResult smooth(const pluss::codem::Series& y,
                                const SsmModel& model,
                                const SsmOptions& options = {},
                                pluss::codem::Config cfg = {});

  static SsmForecastResult forecast(std::int64_t horizon,
                                    const SsmModel& model,
                                    pluss::codem::Config cfg = {});

  static SsmForecastResult forecast(std::int64_t horizon,
                                    const pluss::codem::Series& y_history,
                                    const SsmModel& model,
                                    const SsmOptions& options = {},
                                    pluss::codem::Config cfg = {});

  static SsmSimResult simulate(std::int64_t num_observations,
                               const SsmModel& model,
                               std::int64_t num_paths = 1,
                               pluss::codem::Config cfg = {},
                               std::uint64_t seed = 0);
};

struct VarOptions {
  std::int64_t lag_order = 1;
  bool include_intercept = true;
  bool include_trend = false;
  double ridge = 0.0;
  double pinv_rcond = 1e-12;
};

enum class IrfMethod : std::uint8_t {
  Orthogonalized = 0,
  Generalized = 1,
};

struct IrfOptions {
  std::int64_t horizon = 20;
  IrfMethod method = IrfMethod::Orthogonalized;
};

struct FevdOptions {
  std::int64_t horizon = 20;
  IrfMethod method = IrfMethod::Orthogonalized;
};

enum class GrangerType : std::uint8_t {
  LeaveOneOut = 0,
  ExcludeAll = 1,
  BlockWise = 2,
};

enum class GrangerStat : std::uint8_t {
  ChiSquare = 0,
  F = 1,
};

struct GrangerOptions {
  GrangerType type = GrangerType::LeaveOneOut;
  GrangerStat stat = GrangerStat::ChiSquare;
  double alpha = 0.05;
  std::vector<std::int64_t> cause{};
  std::vector<std::int64_t> effect{};
};

struct GrangerResult {
  pluss::codem::Series cause = pluss::series::undefined();
  pluss::codem::Series effect = pluss::series::undefined();
  pluss::codem::Series stat = pluss::series::undefined();
  pluss::codem::Series pvalue = pluss::series::undefined();
  pluss::codem::Series critical = pluss::series::undefined();
  pluss::codem::Series dof1 = pluss::series::undefined();
  pluss::codem::Series dof2 = pluss::series::undefined();
  pluss::codem::Series reject = pluss::series::undefined();
};

struct VarFit {
  std::int64_t lag_order = 1;
  bool include_intercept = true;
  bool include_trend = false;
  pluss::codem::Series constant = pluss::series::undefined();
  pluss::codem::Series trend = pluss::series::undefined();
  pluss::codem::Series ar = pluss::series::undefined();
  pluss::codem::Series beta = pluss::series::undefined();
  pluss::codem::Series covariance = pluss::series::undefined();
  std::int64_t sample_size = 0;

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y, pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y,
                                           const pluss::codem::Series& x,
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y, const pluss::codem::Series& x) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y,
                                                    pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y,
                                                    const pluss::codem::Series& x,
                                                    pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              const pluss::codem::Series& x_future,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series irf(const IrfOptions& options = {},
                                         pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series fevd(const FevdOptions& options = {},
                                          pluss::codem::Config cfg = {}) const;
  [[nodiscard]] GrangerResult granger_test(const pluss::codem::Series& y,
                                           const GrangerOptions& options = {},
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] GrangerResult granger_test(const pluss::codem::Series& y,
                                           const pluss::codem::Series& x,
                                           const GrangerOptions& options = {},
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
  [[nodiscard]] pluss::codem::Series simulate(const pluss::codem::Series& x,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Var {
 public:
  static VarFit fit_mle(const pluss::codem::Series& y,
                        const VarOptions& options = {},
                        pluss::codem::Config cfg = {});
  static VarFit fit_mle(const pluss::codem::Series& y,
                        const pluss::codem::Series& x,
                        const VarOptions& options = {},
                        pluss::codem::Config cfg = {});
};

enum class JohansenStat : std::uint8_t {
  Trace = 0,
  MaxEigen = 1,
};

struct JohansenOptions {
  std::int64_t lag_order = 1;
  bool include_intercept = true;
  bool include_trend = false;
  JohansenStat statistic = JohansenStat::Trace;
  double alpha = 0.05;
};

struct JohansenResult {
  pluss::codem::Series eigenvalues = pluss::series::undefined();
  pluss::codem::Series stat = pluss::series::undefined();
  pluss::codem::Series pvalue = pluss::series::undefined();
  pluss::codem::Series critical = pluss::series::undefined();
  pluss::codem::Series reject = pluss::series::undefined();
  pluss::codem::Series dof = pluss::series::undefined();
  pluss::codem::Series alpha = pluss::series::undefined();
  pluss::codem::Series beta = pluss::series::undefined();
  std::int64_t selected_rank = 0;
};

CODEM_API JohansenResult johansen_test(const pluss::codem::Series& y,
                                       const JohansenOptions& options = {},
                                       pluss::codem::Config cfg = {});

struct VecmOptions {
  std::int64_t lag_order = 1;
  std::int64_t rank = -1;
  bool include_intercept = true;
  bool include_trend = false;
  double alpha = 0.05;
  double ridge = 0.0;
  double pinv_rcond = 1e-12;
};

struct VecmFit {
  std::int64_t lag_order = 1;
  std::int64_t rank = 0;
  bool include_intercept = true;
  bool include_trend = false;
  pluss::codem::Series alpha = pluss::series::undefined();
  pluss::codem::Series beta = pluss::series::undefined();
  pluss::codem::Series impact = pluss::series::undefined();
  pluss::codem::Series gamma = pluss::series::undefined();
  pluss::codem::Series constant = pluss::series::undefined();
  pluss::codem::Series trend = pluss::series::undefined();
  pluss::codem::Series covariance = pluss::series::undefined();
  std::int64_t sample_size = 0;

  [[nodiscard]] pluss::codem::Series infer(const pluss::codem::Series& y,
                                           pluss::codem::Config cfg = {}) const;
  [[nodiscard]] double loglik(const pluss::codem::Series& y) const;
  [[nodiscard]] pluss::codem::Series loglik_per_obs(const pluss::codem::Series& y,
                                                    pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series forecast(std::int64_t horizon,
                                              const pluss::codem::Series& y,
                                              pluss::codem::Config cfg = {}) const;
  [[nodiscard]] pluss::codem::Series simulate(std::int64_t num_observations,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0) const;
};

class CODEM_API Vecm {
 public:
  static VecmFit fit_mle(const pluss::codem::Series& y,
                         const VecmOptions& options = {},
                         pluss::codem::Config cfg = {});
};

CODEM_API std::pair<pluss::codem::Series, pluss::codem::Series> var_to_vecm(const pluss::codem::Series& ar,
                                                                             pluss::codem::Config cfg = {});
CODEM_API pluss::codem::Series vecm_to_var(const pluss::codem::Series& short_run,
                                           const pluss::codem::Series& impact,
                                           pluss::codem::Config cfg = {});

} // namespace pluss::subsp::stats::ts
