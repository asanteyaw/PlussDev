#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/lymph.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main() {
  namespace fs = std::filesystem;
  using pluss::Series;
  using pluss::table::DataFrame;
  namespace reg = pluss::ma::reg;
  namespace lymph = pluss::ai::lymph;

  const auto f64_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto i64_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);

#ifdef PLUSSDEV_SOURCE_DIR
  const fs::path repo_root = fs::path(PLUSSDEV_SOURCE_DIR);
#else
  const fs::path repo_root = fs::current_path();
#endif
  const fs::path csv_path = repo_root / "PlussMic" / "data" / "hour.csv";
  const fs::path out_dir = repo_root / "outputs" / "bike_sharing_regression_notebook_demo";
  fs::create_directories(out_dir);
  fs::current_path(out_dir);

  //======= Cell One=======
  // Load the notebook dataset, apply the same core renames, and normalize dtypes for modeling and plotting.
  //======== End Cell One =====
  auto bike = DataFrame::load_csv(csv_path.string())
                  .rename("instant", "rec_id")
                  .rename("dteday", "datetime")
                  .rename("holiday", "is_holiday")
                  .rename("workingday", "is_workingday")
                  .rename("weathersit", "weather_condition")
                  .rename("hum", "humidity")
                  .rename("mnth", "month")
                  .rename("cnt", "total_count")
                  .rename("hr", "hour")
                  .rename("yr", "year");
  bike.set("datetime") = bike.features("datetime").cast().timeline();
  bike = bike.features(
      {"rec_id", "season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday",
       "weather_condition", "temp", "atemp", "humidity", "windspeed", "casual", "registered",
       "total_count"})
             .astype(pluss::ScalarType::Float64);
  bike.set({"season", "is_holiday", "weather_condition", "is_workingday"}) =
      bike.features({"season", "is_holiday", "weather_condition", "is_workingday"}).cast().note();

  std::cout << "[Cell 1] dataset shape: (" << bike.ncases() << ", " << bike.nfeatures() << ")\n";
  bike.head(8);

  //======= Cell Two=======
  // Run quick schema/stat checks like notebook info/describe.
  //======== End Cell Two =====
  std::cout << "\n[Cell 2] info + describe\n";
  bike.info();
  bike.describe();

  //======= Cell Three=======
  // Define notebook feature families and switch to NodeJ/Lymph prep APIs for encoding and scaling.
  //======== End Cell Three =====
  const std::vector<std::string> numeric_feature_cols = {
      "temp", "humidity", "windspeed", "hour", "weekday", "month", "year"};
  const std::vector<std::string> subset_cat_features = {
      "season", "is_holiday", "weather_condition", "is_workingday"};

  //======= Cell Four=======
  // Run notebook-parity holdout split then apply Lymph one-hot encoding + standard scaling.
  //======== End Cell Four =====
  auto model = bike.features(
                    {"temp", "humidity", "windspeed", "hour", "weekday", "month", "year",
                     "season", "is_holiday", "weather_condition", "is_workingday", "total_count"})
                   .dropnull();
  const auto split = lymph::prep::holdout_split(model,
                                          lymph::HoldoutSplitOptions{
                                              .test_size = 0.33,
                                              .random_state = 42,
                                              .deterministic = true,
                                              .shuffle = true,
                                          });
  auto train = split.train_set;
  auto test = split.test_set;
  const std::int64_t n_train = train.ncases();
  const std::int64_t n_test = test.ncases();

  std::cout << "[Cell 4] train rows: " << train.ncases() << ", test rows: " << test.ncases() << "\n";

  auto train_num_raw = train.features({"temp", "humidity", "windspeed", "hour", "weekday", "month", "year"});
  auto test_num_raw = test.features({"temp", "humidity", "windspeed", "hour", "weekday", "month", "year"});

  auto z_scaler = lymph::prep::standard_scaler(lymph::prep::StandardScalerOptions{
      .with_centering = true,
      .with_scaling = true,
      .variance_floor = 1e-12,
  });
  z_scaler.features({"temp", "humidity", "windspeed", "hour", "weekday", "month", "year"});
  auto train_num_scaled = z_scaler.learn_map(train_num_raw);
  auto test_num_scaled = z_scaler.map(test_num_raw);

  auto train_x_frame = train_num_scaled;
  auto test_x_frame = test_num_scaled;
  std::vector<std::string> predictor_names = numeric_feature_cols;

  for (const auto& cat_name : subset_cat_features) {
    auto ohe = lymph::prep::one_hot_encoder(lymph::prep::OneHotEncoderOptions{
        .max_categories = 32,
        .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .include_unknown_bucket = false,
    });
    ohe.features({cat_name});
    const auto train_hot = ohe.learn_map(train.features({cat_name}));
    const auto test_hot = ohe.map(test.features({cat_name}));
    const auto hot_names = train_hot.feataxis().series().features();
    for (const auto& hot_name : hot_names) {
      train_x_frame.set(hot_name) = train_hot.features(hot_name);
      test_x_frame.set(hot_name) = test_hot.features(hot_name);
      predictor_names.push_back(hot_name);
    }
  }

  const auto train_matrix_pack = train_x_frame.matrix();
  const auto test_matrix_pack = test_x_frame.matrix();
  const auto train_ext_matrix_pack = train_num_scaled.matrix();
  const auto test_ext_matrix_pack = test_num_scaled.matrix();
  const Series x_train = train_matrix_pack.as_series();
  const Series x_test = test_matrix_pack.as_series();
  const Series x_train_ext = train_ext_matrix_pack.as_series();
  const Series x_test_ext = test_ext_matrix_pack.as_series();
  const Series y_train = train.features("total_count").as_series().reshape({train.ncases()});
  const Series y_test = test.features("total_count").as_series().reshape({test.ncases()});

  //======= Cell Five=======
  // Plot notebook-style distribution/probability diagnostic for the response (QQ plot via flash).
  //======== End Cell Five =====
  const std::vector<double> y_train_values = y_train.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  auto fig_qq = pluss::pl::studio("cell05_response_qqplot");
  pluss::pl::flash::QqPlotOptions qq_opts;
  qq_opts.show_reference_line = true;
  pluss::pl::flash::qqplot(fig_qq, y_train_values, qq_opts, "Train total_count QQ plot");
  fig_qq.save_pdf("01_train_total_count_qqplot.pdf");

  //======= Cell Six=======
  // Run 10-fold CV diagnostics for OLS (manual loop because regression module has no cross_val helper).
  //======== End Cell Six =====
  reg::OLSOptions ols_cv_opts;
  ols_cv_opts.response_name = "total_count";
  ols_cv_opts.predictor_names = predictor_names;
  ols_cv_opts.add_intercept = true;
  ols_cv_opts.drop_missing = true;

  const std::int64_t k_folds = 10;
  std::vector<double> cv_r2_scores(static_cast<std::size_t>(k_folds), 0.0);
  std::vector<double> cv_mse_scores(static_cast<std::size_t>(k_folds), 0.0);
  std::vector<double> cv_pred_values(static_cast<std::size_t>(n_train), std::numeric_limits<double>::quiet_NaN());

  for (std::int64_t fold = 0; fold < k_folds; ++fold) {
    const std::int64_t fold_start = (fold * n_train) / k_folds;
    const std::int64_t fold_stop = ((fold + 1) * n_train) / k_folds;

    std::vector<std::int64_t> train_idx{};
    std::vector<std::int64_t> valid_idx{};
    train_idx.reserve(static_cast<std::size_t>(n_train - (fold_stop - fold_start)));
    valid_idx.reserve(static_cast<std::size_t>(fold_stop - fold_start));

    for (std::int64_t i = 0; i < n_train; ++i) {
      if (i >= fold_start && i < fold_stop) {
        valid_idx.push_back(i);
      } else {
        train_idx.push_back(i);
      }
    }

    const Series train_idx_series = pluss::make_series(
        train_idx, {static_cast<std::int64_t>(train_idx.size())}, i64_cfg);
    const Series valid_idx_series = pluss::make_series(
        valid_idx, {static_cast<std::int64_t>(valid_idx.size())}, i64_cfg);

    const Series x_fold_train = x_train.index_select(0, train_idx_series);
    const Series y_fold_train = y_train.index_select(0, train_idx_series);
    const Series x_fold_valid = x_train.index_select(0, valid_idx_series);
    const Series y_fold_valid = y_train.index_select(0, valid_idx_series);

    const auto fold_fit = reg::ols_fit(y_fold_train, x_fold_train, ols_cv_opts, f64_cfg);
    const Series fold_pred = reg::ols_predict(fold_fit, x_fold_valid, f64_cfg).reshape({y_fold_valid.numel()});
    const Series fold_resid = y_fold_valid.sub(fold_pred);

    const double fold_mse = fold_resid.square().mean().item<double>();
    const double fold_sse = fold_resid.square().sum().item<double>();
    const double fold_sst =
        y_fold_valid.sub(y_fold_valid.mean()).square().sum().item<double>();
    const double fold_r2 = (fold_sst > 0.0) ? (1.0 - (fold_sse / fold_sst)) : 0.0;

    cv_r2_scores[static_cast<std::size_t>(fold)] = fold_r2;
    cv_mse_scores[static_cast<std::size_t>(fold)] = -fold_mse;

    const auto fold_pred_host = fold_pred.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    for (std::size_t j = 0; j < valid_idx.size(); ++j) {
      cv_pred_values[static_cast<std::size_t>(valid_idx[j])] = fold_pred_host[j];
    }
  }

  const Series cv_pred = pluss::make_series(cv_pred_values, {n_train}, f64_cfg);
  const Series cv_resid = y_train.sub(cv_pred);

  std::cout << "[Cell 6] CV R-squared scores: ";
  for (std::size_t i = 0; i < cv_r2_scores.size(); ++i) {
    if (i > 0) {
      std::cout << ", ";
    }
    std::cout << cv_r2_scores[i];
  }
  std::cout << "\n";
  std::cout << "[Cell 6] CV neg-MSE scores: ";
  for (std::size_t i = 0; i < cv_mse_scores.size(); ++i) {
    if (i > 0) {
      std::cout << ", ";
    }
    std::cout << cv_mse_scores[i];
  }
  std::cout << "\n";

  //======= Cell Seven=======
  // Save notebook-equivalent CV residual and score plots using df.plot().
  //======== End Cell Seven =====
  auto cv_resid_df = DataFrame({
      {"observed", pluss::table::Slate(y_train)},
      {"residual", pluss::table::Slate(cv_resid)},
  });
  cv_resid_df.plot()
      .scatter("observed", "residual")
      .headline("CV residual plot: observed vs residual")
      .save_pdf("02_cv_observed_vs_residual.pdf");

  std::vector<double> fold_axis_values{};
  fold_axis_values.reserve(static_cast<std::size_t>(k_folds));
  for (std::int64_t i = 0; i < k_folds; ++i) {
    fold_axis_values.push_back(static_cast<double>(i));
  }
  auto cv_score_df = DataFrame({
      {"fold", pluss::table::Slate(pluss::make_series(fold_axis_values, {k_folds}, f64_cfg))},
      {"r2", pluss::table::Slate(pluss::make_series(cv_r2_scores, {k_folds}, f64_cfg))},
      {"neg_mse", pluss::table::Slate(pluss::make_series(cv_mse_scores, {k_folds}, f64_cfg))},
  }).label().set("fold", false);
  cv_score_df.plot().line("r2").headline("Cross-validation R-squared by fold").save_pdf("03_cv_r2_by_fold.pdf");

  //======= Cell Eight=======
  // Fit notebook-equivalent multiple OLS and print a statsmodels-style summary.
  //======== End Cell Eight =====
  auto ols_multiple_fit = reg::ols_fit(y_train, x_train, ols_cv_opts, f64_cfg);
  const std::string ols_multiple_summary = reg::ols_summary(ols_multiple_fit);
  std::cout << "\n================ OLS Multiple Regression Summary ================\n";
  std::cout << ols_multiple_summary << "\n";
  {
    std::ofstream out("ols_multiple_summary.txt");
    out << ols_multiple_summary << "\n";
  }

  const Series y_pred = reg::ols_predict(ols_multiple_fit, x_test, f64_cfg).reshape({y_test.numel()});
  const Series test_resid = y_test.sub(y_pred);
  const double test_mse = test_resid.square().mean().item<double>();
  const double test_sse = test_resid.square().sum().item<double>();
  const double test_sst = y_test.sub(y_test.mean()).square().sum().item<double>();
  const double test_r2 = (test_sst > 0.0) ? (1.0 - (test_sse / test_sst)) : 0.0;
  std::cout << "[Cell 8] Test R-squared: " << test_r2 << "\n";
  std::cout << "[Cell 8] Test MSE: " << test_mse << "\n";

  //======= Cell Nine=======
  // Save notebook-equivalent test residual and prediction-vs-observed plots.
  //======== End Cell Nine =====
  auto test_diag_df = DataFrame({
      {"observed", pluss::table::Slate(y_test)},
      {"predicted", pluss::table::Slate(y_pred)},
      {"residual", pluss::table::Slate(test_resid)},
  });
  test_diag_df.plot()
      .scatter("observed", "residual")
      .headline("Test residual plot: observed vs residual")
      .save_pdf("04_test_observed_vs_residual.pdf");
  test_diag_df.plot()
      .scatter("predicted", "observed")
      .headline("Test predictions vs observed")
      .save_pdf("05_test_predicted_vs_observed.pdf");
  test_diag_df.plot().hist("residual", 80).headline("Test residual histogram").save_pdf("06_test_residual_hist.pdf");

  //======= Cell Ten=======
  // Fit a simple linear regression baseline (single predictor) and print summary.
  //======== End Cell Ten =====
  auto simple_train_x_frame = train_num_scaled.features({"temp"});
  auto simple_test_x_frame = test_num_scaled.features({"temp"});
  const auto simple_train_pack = simple_train_x_frame.matrix();
  const auto simple_test_pack = simple_test_x_frame.matrix();

  reg::OLSOptions ols_simple_opts;
  ols_simple_opts.response_name = "total_count";
  ols_simple_opts.predictor_names = {"temp"};
  ols_simple_opts.add_intercept = true;
  ols_simple_opts.drop_missing = true;

  auto ols_simple_fit = reg::ols_fit(y_train, simple_train_pack.as_series(), ols_simple_opts, f64_cfg);
  const std::string ols_simple_summary = reg::ols_summary(ols_simple_fit);
  std::cout << "\n================ OLS Simple Linear Summary ================\n";
  std::cout << ols_simple_summary << "\n";
  {
    std::ofstream out("ols_simple_summary.txt");
    out << ols_simple_summary << "\n";
  }
  const Series y_simple_pred =
      reg::ols_predict(ols_simple_fit, simple_test_pack.as_series(), f64_cfg).reshape({y_test.numel()});
  const double simple_mse = y_test.sub(y_simple_pred).square().mean().item<double>();
  std::cout << "[Cell 10] Simple linear test MSE: " << simple_mse << "\n";

  //======= Cell Ten-B =======
  // Build a bounded training slice for iterative non-notebook extensions (quantile/poly/GLM/logit)
  // so the end-to-end demo remains fast and deterministic in CI and local runs.
  //======== End Cell Ten-B =====
  const std::int64_t aux_rows = std::min<std::int64_t>(2500, n_train);
  const std::vector<std::string> ext_predictor_names = numeric_feature_cols;
  const Series aux_idx = Series::arange(0, aux_rows, 1, pluss::ScalarType::Int64).to(pluss::pCPU, pluss::ScalarType::Int64);
  const Series x_aux = x_train_ext.index_select(0, aux_idx);
  const Series y_aux = y_train.index_select(0, aux_idx);

  //======= Cell Eleven=======
  // Fit median quantile regression on the bounded extension matrix and print summary.
  //======== End Cell Eleven =====
  reg::QuantileOptions q_opts;
  q_opts.tau = 0.5;
  q_opts.response_name = "total_count";
  q_opts.predictor_names = ext_predictor_names;
  q_opts.add_intercept = true;
  q_opts.drop_missing = true;
  q_opts.max_iter = 60;
  q_opts.tol = 1e-6;
  auto q_fit = reg::quantile_fit(y_aux, x_aux, q_opts, f64_cfg);
  const std::string q_summary = reg::quantile_summary(q_fit);
  std::cout << "\n================ Quantile Regression Summary (tau=0.5) ================\n";
  std::cout << q_summary << "\n";
  {
    std::ofstream out("quantile_summary.txt");
    out << q_summary << "\n";
  }

  //======= Cell Twelve=======
  // Fit polynomial regression (degree 2 with cross terms) on weather predictors and print summary.
  //======== End Cell Twelve =====
  auto train_poly_frame = train_num_scaled.features({"temp", "humidity", "windspeed"});
  const auto train_poly_pack = train_poly_frame.matrix();
  const Series x_aux_poly = train_poly_pack.as_series().index_select(0, aux_idx);

  reg::PolyOptions poly_opts;
  poly_opts.degree = 2;
  poly_opts.include_cross = true;
  poly_opts.response_name = "total_count";
  poly_opts.predictor_names = {"temp", "humidity", "windspeed"};
  poly_opts.add_intercept = true;
  poly_opts.drop_missing = true;
  auto poly_fit = reg::poly_fit(y_aux, x_aux_poly, poly_opts, f64_cfg);
  const std::string poly_summary = reg::poly_summary(poly_fit);
  std::cout << "\n================ Polynomial Regression Summary ================\n";
  std::cout << poly_summary << "\n";
  {
    std::ofstream out("poly_summary.txt");
    out << poly_summary << "\n";
  }

  //======= Cell Thirteen=======
  // Fit GLM Poisson for count modeling on the bounded extension matrix and print summary.
  //======== End Cell Thirteen =====
  reg::GLMOptions glm_poisson_opts;
  glm_poisson_opts.family = reg::GLMFamily::Poisson;
  glm_poisson_opts.link = reg::GLMLink::Canonical;
  glm_poisson_opts.response_name = "total_count";
  glm_poisson_opts.predictor_names = ext_predictor_names;
  glm_poisson_opts.add_intercept = true;
  glm_poisson_opts.drop_missing = true;
  glm_poisson_opts.max_iter = 20;
  glm_poisson_opts.tol = 1e-6;
  auto glm_poisson_fit = reg::glm_fit(y_aux, x_aux, glm_poisson_opts, f64_cfg);
  const std::string glm_poisson_summary = reg::glm_summary(glm_poisson_fit);
  std::cout << "\n================ GLM Poisson Summary ================\n";
  std::cout << glm_poisson_summary << "\n";
  {
    std::ofstream out("glm_poisson_summary.txt");
    out << glm_poisson_summary << "\n";
  }

  //======= Cell Fourteen=======
  // Fit logistic regression via GLM (Binomial + Logit) on the bounded extension matrix and print summary + test accuracy.
  //======== End Cell Fourteen =====
  const double train_median_count = y_aux.median().item<double>();
  const Series y_train_binary = y_aux.ge(train_median_count).to(pluss::ScalarType::Float64);
  const Series y_test_binary = y_test.ge(train_median_count).to(pluss::ScalarType::Float64);

  reg::GLMOptions glm_logit_opts;
  glm_logit_opts.family = reg::GLMFamily::Binomial;
  glm_logit_opts.link = reg::GLMLink::Logit;
  glm_logit_opts.response_name = "high_demand";
  glm_logit_opts.predictor_names = ext_predictor_names;
  glm_logit_opts.add_intercept = true;
  glm_logit_opts.drop_missing = true;
  glm_logit_opts.max_iter = 20;
  glm_logit_opts.tol = 1e-6;
  auto glm_logit_fit = reg::glm_fit(y_train_binary, x_aux, glm_logit_opts, f64_cfg);
  const std::string glm_logit_summary = reg::glm_summary(glm_logit_fit);
  std::cout << "\n================ Logistic Regression Summary (GLM Binomial-Logit) ================\n";
  std::cout << glm_logit_summary << "\n";
  {
    std::ofstream out("glm_logistic_summary.txt");
    out << glm_logit_summary << "\n";
  }

  const Series logit_prob = reg::glm_predict(glm_logit_fit, x_test_ext, reg::GLMPredictionType::Mean, f64_cfg)
                                .reshape({y_test_binary.numel()});
  const Series logit_class = logit_prob.ge(0.5).to(pluss::ScalarType::Float64);
  const double logit_accuracy = logit_class.eq(y_test_binary).to(pluss::ScalarType::Float64).mean().item<double>();
  std::cout << "[Cell 14] Logistic test accuracy: " << logit_accuracy << "\n";

  //======= Cell Fifteen=======
  // Save logistic probability diagnostics as PDF.
  //======== End Cell Fifteen =====
  auto logit_diag = DataFrame({
      {"pred_prob", pluss::table::Slate(logit_prob)},
      {"actual_class", pluss::table::Slate(y_test_binary)},
  });
  logit_diag.plot()
      .scatter("pred_prob", "actual_class")
      .headline("Logistic predicted probability vs actual class")
      .save_pdf("07_logistic_prob_vs_actual.pdf");
  logit_diag.plot().hist("pred_prob", 60).headline("Logistic predicted probability histogram").save_pdf(
      "08_logistic_prob_hist.pdf");

  std::cout << "\nSaved outputs under: " << out_dir << "\n";
  std::cout << "Notebook-style regression workflow demo completed.\n";
  return 0;
}
