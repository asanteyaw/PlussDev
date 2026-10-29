#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace pluss::subsp::stats::regression {

enum class OLSCovarianceType : std::uint8_t {
  Nonrobust = 0,
};

struct OLSOptions {
  double alpha = 0.05;
  bool add_intercept = true;
  bool drop_missing = true;
  double rcond = 1e-12;
  std::string response_name = "y";
  std::vector<std::string> predictor_names{};
  OLSCovarianceType covariance = OLSCovarianceType::Nonrobust;
};

enum class GLMFamily : std::uint8_t {
  Normal = 0,
  Binomial = 1,
  Poisson = 2,
};

enum class GLMLink : std::uint8_t {
  Canonical = 0,
  Identity = 1,
  Log = 2,
  Logit = 3,
};

enum class GLMPredictionType : std::uint8_t {
  Mean = 0,
  Linear = 1,
};

struct GLMOptions {
  GLMFamily family = GLMFamily::Normal;
  GLMLink link = GLMLink::Canonical;
  double alpha = 0.05;
  bool add_intercept = true;
  bool drop_missing = true;
  double rcond = 1e-12;
  std::int64_t max_iter = 200;
  double tol = 1e-8;
  std::string response_name = "y";
  std::vector<std::string> predictor_names{};
};

enum class QuantileCovariance : std::uint8_t {
  Robust = 0,  // heteroskedasticity-robust sandwich (default)
  Iid = 1,     // errors assumed independent of the predictors
};

struct QuantileOptions {
  double tau = 0.5;
  QuantileCovariance covariance = QuantileCovariance::Robust;
  double alpha = 0.05;
  bool add_intercept = true;
  bool drop_missing = true;
  double rcond = 1e-12;
  std::int64_t max_iter = 500;
  double tol = 1e-7;
  double eps = 1e-6;
  std::string response_name = "y";
  std::vector<std::string> predictor_names{};
};

struct PolyOptions {
  std::int64_t degree = 2;
  bool add_intercept = true;
  bool include_cross = false;
  bool drop_missing = true;
  double rcond = 1e-12;
  double alpha = 0.05;
  std::string response_name = "y";
  std::vector<std::string> predictor_names{};
};

struct CODEM_API OLSFitResult {
  pluss::codem::Series coefficients = pluss::series::undefined();   // [k]
  pluss::codem::Series std_errors = pluss::series::undefined();     // [k]
  pluss::codem::Series t_values = pluss::series::undefined();       // [k]
  pluss::codem::Series p_values = pluss::series::undefined();       // [k]
  pluss::codem::Series conf_int = pluss::series::undefined();       // [k,2]
  pluss::codem::Series covariance = pluss::series::undefined();     // [k,k]
  pluss::codem::Series fitted_values = pluss::series::undefined();  // [n]
  pluss::codem::Series residuals = pluss::series::undefined();      // [n]

  std::int64_t nobs = 0;
  std::int64_t rank = 0;
  std::int64_t k_params = 0;
  std::int64_t num_predictors = 0;
  std::int64_t dropped_observations = 0;
  std::int64_t df_model = 0;
  std::int64_t df_resid = 0;

  double alpha = 0.05;
  double mse = 0.0;
  double rmse = 0.0;
  double sse = 0.0;
  double ssr = 0.0;
  double sst = 0.0;
  double rsquared = 0.0;
  double adj_rsquared = 0.0;
  double f_stat = 0.0;
  double f_pvalue = 1.0;
  double log_likelihood = 0.0;
  double aic = 0.0;
  double bic = 0.0;
  double durbin_watson = 0.0;
  double omnibus = 0.0;
  double omnibus_pvalue = 1.0;
  double jarque_bera = 0.0;
  double jarque_bera_pvalue = 1.0;
  double skewness = 0.0;
  double kurtosis = 3.0;
  double condition_number = 0.0;
  double smallest_eig = 0.0;

  bool has_intercept = true;
  OLSCovarianceType covariance_type = OLSCovarianceType::Nonrobust;
  std::string response_name = "y";
  std::vector<std::string> coefficient_names{};
  std::vector<std::string> warnings{};
};

struct CODEM_API GLMFitResult {
  pluss::codem::Series coefficients = pluss::series::undefined();   // [k]
  pluss::codem::Series std_errors = pluss::series::undefined();     // [k]
  pluss::codem::Series z_values = pluss::series::undefined();       // [k]
  pluss::codem::Series p_values = pluss::series::undefined();       // [k]
  pluss::codem::Series conf_int = pluss::series::undefined();       // [k,2]
  pluss::codem::Series covariance = pluss::series::undefined();     // [k,k]
  pluss::codem::Series fitted_values = pluss::series::undefined();  // [n]
  pluss::codem::Series linear_predictor = pluss::series::undefined(); // [n]
  pluss::codem::Series residuals = pluss::series::undefined();      // [n]

  std::int64_t nobs = 0;
  std::int64_t rank = 0;
  std::int64_t k_params = 0;
  std::int64_t num_predictors = 0;
  std::int64_t dropped_observations = 0;
  std::int64_t df_model = 0;
  std::int64_t df_resid = 0;
  std::int64_t iterations = 0;

  double alpha = 0.05;
  double dispersion = 1.0;
  double deviance = 0.0;
  double null_deviance = 0.0;
  double log_likelihood = 0.0;
  double aic = 0.0;
  double bic = 0.0;
  double pseudo_rsquared = 0.0;
  double condition_number = 0.0;
  bool converged = false;

  bool has_intercept = true;
  GLMFamily family = GLMFamily::Normal;
  GLMLink link = GLMLink::Canonical;
  std::string response_name = "y";
  std::vector<std::string> coefficient_names{};
  std::vector<std::string> warnings{};
};

struct CODEM_API QuantileFitResult {
  pluss::codem::Series coefficients = pluss::series::undefined();   // [k]
  pluss::codem::Series std_errors = pluss::series::undefined();     // [k]
  pluss::codem::Series t_values = pluss::series::undefined();       // [k]
  pluss::codem::Series p_values = pluss::series::undefined();       // [k]
  pluss::codem::Series conf_int = pluss::series::undefined();       // [k,2]
  pluss::codem::Series covariance = pluss::series::undefined();     // [k,k]
  pluss::codem::Series fitted_values = pluss::series::undefined();  // [n]
  pluss::codem::Series residuals = pluss::series::undefined();      // [n]

  std::int64_t nobs = 0;
  std::int64_t rank = 0;
  std::int64_t k_params = 0;
  std::int64_t num_predictors = 0;
  std::int64_t dropped_observations = 0;
  std::int64_t df_model = 0;
  std::int64_t df_resid = 0;
  std::int64_t iterations = 0;

  double tau = 0.5;
  double alpha = 0.05;
  double objective = 0.0;
  double objective_null = 0.0;
  double pseudo_rsquared = 0.0;
  double residual_density0 = 0.0;
  bool converged = false;

  bool has_intercept = true;
  std::string response_name = "y";
  std::vector<std::string> coefficient_names{};
  std::vector<std::string> warnings{};
};

struct CODEM_API PolyFitResult {
  OLSFitResult ols{};
  std::int64_t degree = 2;
  bool include_cross = false;
  std::vector<std::string> base_predictor_names{};
};

CODEM_API OLSFitResult ols_fit(const pluss::codem::Series& y,
                               const pluss::codem::Series& x,
                               const OLSOptions& options = {},
                               pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series ols_predict(const OLSFitResult& fit,
                                           const pluss::codem::Series& x,
                                           pluss::codem::Config cfg = {});

CODEM_API std::string ols_summary(const OLSFitResult& fit);

CODEM_API std::ostream& print_ols_summary(std::ostream& os, const OLSFitResult& fit);

CODEM_API GLMFitResult glm_fit(const pluss::codem::Series& y,
                               const pluss::codem::Series& x,
                               const GLMOptions& options = {},
                               pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series glm_predict(const GLMFitResult& fit,
                                           const pluss::codem::Series& x,
                                           GLMPredictionType type = GLMPredictionType::Mean,
                                           pluss::codem::Config cfg = {});

CODEM_API std::string glm_summary(const GLMFitResult& fit);

CODEM_API std::ostream& print_glm_summary(std::ostream& os, const GLMFitResult& fit);

CODEM_API QuantileFitResult quantile_fit(const pluss::codem::Series& y,
                                         const pluss::codem::Series& x,
                                         const QuantileOptions& options = {},
                                         pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series quantile_predict(const QuantileFitResult& fit,
                                                const pluss::codem::Series& x,
                                                pluss::codem::Config cfg = {});

CODEM_API std::string quantile_summary(const QuantileFitResult& fit);

CODEM_API std::ostream& print_quantile_summary(std::ostream& os, const QuantileFitResult& fit);

CODEM_API pluss::codem::Series poly_expand(const pluss::codem::Series& x,
                                           std::int64_t degree,
                                           bool include_cross = false,
                                           pluss::codem::Config cfg = {});

CODEM_API PolyFitResult poly_fit(const pluss::codem::Series& y,
                                 const pluss::codem::Series& x,
                                 const PolyOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series poly_predict(const PolyFitResult& fit,
                                            const pluss::codem::Series& x,
                                            pluss::codem::Config cfg = {});

CODEM_API std::string poly_summary(const PolyFitResult& fit);

CODEM_API std::ostream& print_poly_summary(std::ostream& os, const PolyFitResult& fit);

} // namespace pluss::subsp::stats::regression
