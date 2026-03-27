#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::InferOptions;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::TrainOptions;
using pluss::ai::nodej::UnknownCategoryPolicy;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_lymph_quick_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0},
                                        {4, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    TrainOptions options{};
    options.max_iterations = 4000;
    options.tolerance = 1e-8;
    auto model_or = pluss::ai::lymph::quick::linear_classifier(train, LinearFamilyOptions{}, options);
    if (!model_or.ok()) {
      if (err) *err << "lymph::linear_classifier should return learned model\n";
      return false;
    }

    auto pred_or = model_or.value()->infer(train.features);
    if (!pred_or.ok()) {
      if (err) *err << "lymph linear classifier model should infer\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "lymph linear classifier should fit separable data\n";
        return false;
      }
    }
  }

  {
    const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
    const Series x1 = pluss::make_series(std::vector<double>{1.0, 0.0, 2.0, 1.0, 3.0, 2.0}, {6}, f64());
    const Series y = x0.mul(2.0).sub(x1.mul(0.5)).add(1.0);
    const DataFrame frame({
        {"x0", Thread(x0)},
        {"x1", Thread(x1)},
        {"target", Thread(y)},
    });

    pluss::ai::nodej::DataFrameBatchOptions data_options{};
    data_options.feature_names = {"x0", "x1", "target"};
    data_options.label_feature = "target";
    data_options.layout = ObservationLayout::Rows;

    const std::string save_path = "/tmp/lymph_quick_dataframe_flow.bundle";
    auto artifact_or = pluss::ai::lymph::quick::fit_evaluate_save(
        pluss::ai::nodej::ModelSpec{
            .task = pluss::ai::nodej::TaskKind::Regression,
            .family = pluss::ai::nodej::ModelFamily::Linear,
            .options = pluss::ai::nodej::LinearFamilyOptions{
                .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
                .regularization = pluss::ai::nodej::RegularizationKind::L2,
                .regularization_strength = 1e-8,
                .fit_intercept = true,
            },
        },
        frame,
        data_options,
        TrainOptions{},
        pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Mse, pluss::ai::nodej::MetricKind::R2}},
        save_path);
    if (!artifact_or.ok()) {
      if (err) *err << "lymph::fit_evaluate_save should train/assess/save from DataFrame\n";
      return false;
    }
    auto metrics = artifact_or.value().metrics.to(pluss::pCPU, ScalarType::Float64).reshape({2}).to_vec<double>();
    if (metrics.size() != 2 || metrics[0] > 1e-9 || metrics[1] < 0.999999) {
      if (err) *err << "lymph::fit_evaluate_save should recover exact synthetic DataFrame signal\n";
      return false;
    }
    auto restored_or = pluss::ai::nodej::io::load_and_restore(save_path);
    if (!restored_or.ok()) {
      if (err) *err << "flow saved via lymph DataFrame convenience should restore\n";
      return false;
    }
    auto pred_or = restored_or.value().infer(frame, data_options);
    if (!pred_or.ok() || pred_or.value().numel() != 6) {
      if (err) *err << "restored DataFrame flow should infer\n";
      return false;
    }
  }

  {
    const Thread category = Thread(pluss::make_note({"red", "blue", "red", "green"})).with_display_name("ocean_proximity");
    auto one_hot = pluss::ai::lymph::prep::OneHotEncoder({
        .max_categories = 32,
        .overflow = pluss::ai::nodej::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .include_unknown_bucket = true,
    });
    const Series mapped = one_hot.learn_map(category);
    const auto sizes = mapped.sizes();
    if (sizes.size() != 2 || sizes[0] != 4 || sizes[1] != 4) {
      if (err) *err << "lymph OneHotEncoder Thread learn_map should return [n, encoded_features]\n";
      return false;
    }
    if (one_hot.categories_.size() != 1 || one_hot.categories_[0].size() != 3) {
      if (err) *err << "lymph OneHotEncoder should expose learned categories_\n";
      return false;
    }
    bool seen_red = false;
    bool seen_blue = false;
    bool seen_green = false;
    for (const auto& label : one_hot.categories_[0]) {
      seen_red = seen_red || (label == "red");
      seen_blue = seen_blue || (label == "blue");
      seen_green = seen_green || (label == "green");
    }
    if (!(seen_red && seen_blue && seen_green)) {
      if (err) *err << "lymph OneHotEncoder categories_ should preserve Note labels\n";
      return false;
    }
    if (one_hot.category_keys_.size() != 1 || one_hot.category_keys_[0].size() != 3) {
      if (err) *err << "lymph OneHotEncoder should expose raw category_keys_\n";
      return false;
    }
    if (one_hot.mapped_names_.size() != 4 || one_hot.mapped_names_[3] != "ocean_proximity_unknown") {
      if (err) *err << "lymph OneHotEncoder should expose mapped_names_\n";
      return false;
    }

    auto unknown_or_error = false;
    try {
      (void)one_hot.map(Thread(pluss::make_note({"yellow"})));
    } catch (...) {
      unknown_or_error = true;
    }
    if (!unknown_or_error) {
      if (err) *err << "lymph OneHotEncoder Thread map should error on unknown category by default\n";
      return false;
    }

    InferOptions infer{};
    infer.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    const Series mapped_unknown = one_hot.map(Thread(pluss::make_note({"yellow"})), infer);
    const auto unknown_vec = mapped_unknown.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (unknown_vec.size() != 4 || !approx(unknown_vec[3], 1.0)) {
      if (err) *err << "lymph OneHotEncoder Thread should activate unknown bucket\n";
      return false;
    }
  }

  {
    const DataFrame cat_df({
        {"feat1", Thread(pluss::make_note({"red", "blue", "green", "red"})).with_display_name("feat1")},
        {"feat2", Thread(pluss::make_note({"low", "high", "low", "high"})).with_display_name("feat2")},
    });
    {
      auto one_hot_no_features = pluss::ai::lymph::prep::OneHotEncoder({});
      bool threw = false;
      try {
        (void)one_hot_no_features.learn_map(cat_df);
      } catch (...) {
        threw = true;
      }
      if (!threw) {
        if (err) *err << "lymph OneHotEncoder DataFrame path should require explicit .features({...})\n";
        return false;
      }
    }
    auto one_hot = pluss::ai::lymph::prep::OneHotEncoder({
        .max_categories = 32,
        .overflow = pluss::ai::nodej::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .include_unknown_bucket = true,
    });
    one_hot.features({"feat1", "feat2"});
    const DataFrame mapped = one_hot.learn_map(cat_df);
    if (mapped.ncases() != cat_df.ncases() || mapped.nfeats() < cat_df.nfeats()) {
      if (err) *err << "lymph OneHotEncoder DataFrame learn_map should support Note-backed DataFrame inputs\n";
      return false;
    }
    if (one_hot.categories_.size() != 2 || one_hot.mapped_names_.size() < 5) {
      if (err) *err << "lymph OneHotEncoder DataFrame path should expose categories_ and mapped_names_\n";
      return false;
    }

    const DataFrame cat_unknown({
        {"feat1", Thread(pluss::make_note({"yellow", "blue", "green", "red"})).with_display_name("feat1")},
        {"feat2", Thread(pluss::make_note({"low", "high", "low", "high"})).with_display_name("feat2")},
    });
    InferOptions infer{};
    infer.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    const DataFrame mapped_unknown = one_hot.map(cat_unknown, infer);
    if (mapped_unknown.ncases() != cat_unknown.ncases() || mapped_unknown.nfeats() != mapped.nfeats()) {
      if (err) *err << "lymph OneHotEncoder DataFrame map should handle unknown categories via MissingBucket\n";
      return false;
    }
  }

  {
    const Thread category = Thread(pluss::make_note({"red", "blue", "red", "green"})).with_display_name("ocean_proximity");
    auto freq = pluss::ai::lymph::prep::FrequencyEncoder({
        .max_categories = 32,
        .overflow = pluss::ai::nodej::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .unknown_frequency = 0.125,
    });
    const Series mapped = freq.learn_map(category);
    const auto mapped_vec = mapped.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (mapped_vec.size() != 4 || !approx(mapped_vec[0], 0.5) || !approx(mapped_vec[1], 0.25) || !approx(mapped_vec[3], 0.25)) {
      if (err) *err << "lymph FrequencyEncoder Thread learn_map should encode learned frequencies\n";
      return false;
    }
    if (freq.categories_.size() != 1 || freq.categories_[0].size() != 3) {
      if (err) *err << "lymph FrequencyEncoder should expose learned categories_\n";
      return false;
    }
    bool seen_red = false;
    bool seen_blue = false;
    bool seen_green = false;
    for (const auto& label : freq.categories_[0]) {
      seen_red = seen_red || (label == "red");
      seen_blue = seen_blue || (label == "blue");
      seen_green = seen_green || (label == "green");
    }
    if (!(seen_red && seen_blue && seen_green)) {
      if (err) *err << "lymph FrequencyEncoder categories_ should preserve Note labels\n";
      return false;
    }
    if (freq.category_keys_.size() != 1 || freq.category_keys_[0].size() != 3) {
      if (err) *err << "lymph FrequencyEncoder should expose raw category_keys_\n";
      return false;
    }
    if (freq.frequencies_.size() != 1 || freq.frequencies_[0].size() != 3) {
      if (err) *err << "lymph FrequencyEncoder should expose learned frequencies_\n";
      return false;
    }
    if (freq.mapped_names_.size() != 1 || freq.mapped_names_[0] != "ocean_proximity") {
      if (err) *err << "lymph FrequencyEncoder should expose mapped_names_\n";
      return false;
    }

    auto unknown_or_error = false;
    try {
      (void)freq.map(Thread(pluss::make_note({"yellow"})));
    } catch (...) {
      unknown_or_error = true;
    }
    if (!unknown_or_error) {
      if (err) *err << "lymph FrequencyEncoder Thread map should error on unknown category by default\n";
      return false;
    }

    InferOptions infer{};
    infer.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    const Series mapped_unknown = freq.map(Thread(pluss::make_note({"yellow"})), infer);
    const auto unknown_vec = mapped_unknown.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (unknown_vec.size() != 1 || !approx(unknown_vec[0], 0.125)) {
      if (err) *err << "lymph FrequencyEncoder Thread should use unknown_frequency for unknown category\n";
      return false;
    }
  }

  {
    const DataFrame cat_df({
        {"feat1", Thread(pluss::make_note({"red", "blue", "green", "red"})).with_display_name("feat1")},
        {"feat2", Thread(pluss::make_note({"low", "high", "low", "high"})).with_display_name("feat2")},
    });
    {
      auto freq_no_features = pluss::ai::lymph::prep::FrequencyEncoder({});
      bool threw = false;
      try {
        (void)freq_no_features.learn_map(cat_df);
      } catch (...) {
        threw = true;
      }
      if (!threw) {
        if (err) *err << "lymph FrequencyEncoder DataFrame path should require explicit .features({...})\n";
        return false;
      }
    }
    auto freq = pluss::ai::lymph::prep::FrequencyEncoder({
        .max_categories = 32,
        .overflow = pluss::ai::nodej::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .unknown_frequency = 0.125,
    });
    freq.features({"feat1", "feat2"});
    const DataFrame mapped = freq.learn_map(cat_df);
    if (mapped.ncases() != cat_df.ncases() || mapped.nfeats() != cat_df.nfeats()) {
      if (err) *err << "lymph FrequencyEncoder DataFrame learn_map should support Note-backed DataFrame inputs\n";
      return false;
    }
    if (freq.categories_.size() != 2 || freq.frequencies_.size() != 2) {
      if (err) *err << "lymph FrequencyEncoder DataFrame path should expose categories_ and frequencies_\n";
      return false;
    }
  }

  {
    const Thread category = Thread(pluss::make_note({"red", "blue", "red", "green"})).with_display_name("segment");
    auto ord = pluss::ai::lymph::prep::OrdinalEncoder({
        .max_categories = 32,
        .overflow = pluss::ai::nodej::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .include_unknown_bucket = true,
        .one_based = true,
    });
    const Series mapped = ord.learn_map(category);
    const auto mapped_vec = mapped.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (mapped_vec.size() != 4 || !approx(mapped_vec[0], mapped_vec[2])) {
      if (err) *err << "lymph OrdinalEncoder Thread should assign stable ordinal indices\n";
      return false;
    }
    if (ord.categories_.size() != 1 || ord.categories_[0].size() != 3) {
      if (err) *err << "lymph OrdinalEncoder should expose categories_\n";
      return false;
    }
    if (ord.mapped_names_.size() != 1 || ord.mapped_names_[0] != "segment_ordinal") {
      if (err) *err << "lymph OrdinalEncoder should expose mapped_names_\n";
      return false;
    }

    InferOptions infer{};
    infer.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    const Series unknown = ord.map(Thread(pluss::make_note({"yellow"})), infer);
    const auto unknown_vec = unknown.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (unknown_vec.size() != 1 || !approx(unknown_vec[0], 4.0)) {
      if (err) *err << "lymph OrdinalEncoder unknown category should map to unknown bucket index\n";
      return false;
    }
  }

  {
    const DataFrame cat_df({
        {"feat1", Thread(pluss::make_note({"red", "blue", "green", "red"})).with_display_name("feat1")},
        {"feat2", Thread(pluss::make_note({"low", "high", "low", "high"})).with_display_name("feat2")},
    });
    auto lbl = pluss::ai::lymph::prep::LabelEncoder({
        .max_categories = 32,
        .overflow = pluss::ai::nodej::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .include_unknown_bucket = true,
        .one_based = false,
    });
    lbl.features({"feat1", "feat2"});
    const DataFrame mapped = lbl.learn_map(cat_df);
    if (mapped.ncases() != cat_df.ncases() || mapped.nfeats() != 2) {
      if (err) *err << "lymph LabelEncoder DataFrame learn_map should return one encoded column per selected feature\n";
      return false;
    }
    if (lbl.categories_.size() != 2 || lbl.mapped_names_.size() != 2) {
      if (err) *err << "lymph LabelEncoder DataFrame should expose categories_ and mapped_names_\n";
      return false;
    }
    if (lbl.mapped_names_[0] != "feat1_label" || lbl.mapped_names_[1] != "feat2_label") {
      if (err) *err << "lymph LabelEncoder DataFrame should emit *_label mapped names\n";
      return false;
    }
  }

  {
    const Thread category = Thread(pluss::make_note({"red", "blue", "red", "green"})).with_display_name("segment");
    auto indexer = pluss::ai::lymph::prep::CategoryIndexer({});
    const Series code = indexer.learn_map(category);
    const auto code_vec = code.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (code_vec.size() != 4 || !approx(code_vec[0], code_vec[2])) {
      if (err) *err << "CategoryIndexer Thread path should assign consistent indices per repeated category\n";
      return false;
    }
    if (indexer.categories_.size() != 1 || indexer.categories_[0].size() != 3) {
      if (err) *err << "CategoryIndexer should expose learned category labels\n";
      return false;
    }

    InferOptions infer{};
    infer.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    const Series unknown = indexer.map(Thread(pluss::make_note({"yellow"})), infer);
    const auto unknown_vec = unknown.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const double expected_unknown = static_cast<double>(indexer.categories_[0].size() + 1);
    if (unknown_vec.size() != 1 || !approx(unknown_vec[0], expected_unknown)) {
      if (err) *err << "CategoryIndexer unknown category should map to unknown bucket index with MissingBucket policy\n";
      return false;
    }
  }

  {
    const DataFrame cat_df({
        {"feat1", Thread(pluss::make_note({"red", "blue", "green", "red"})).with_display_name("feat1")},
        {"feat2", Thread(pluss::make_note({"low", "high", "low", "high"})).with_display_name("feat2")},
    });
    auto indexer = pluss::ai::lymph::prep::CategoryIndexer({});
    bool threw_without_features = false;
    try {
      (void)indexer.learn_map(cat_df);
    } catch (...) {
      threw_without_features = true;
    }
    if (!threw_without_features) {
      if (err) *err << "CategoryIndexer DataFrame path should require explicit .features({...})\n";
      return false;
    }

    indexer.features({"feat1", "feat2"});
    const DataFrame mapped = indexer.learn_map(cat_df);
    if (mapped.ncases() != cat_df.ncases() || mapped.nfeats() != 2) {
      if (err) *err << "CategoryIndexer DataFrame path should return one indexed column per selected feature\n";
      return false;
    }
    const auto mapped_names = mapped.feature_names();
    if (mapped_names.size() != 2 || mapped_names[0] != "feat1_index" || mapped_names[1] != "feat2_index") {
      if (err) *err << "CategoryIndexer DataFrame path should emit *_index output names\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64());
    const Thread cat = Thread(pluss::make_note({"red", "blue", "red", "blue", "red", "blue"}));
    const Series y = x.mul(2.0).add(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0, 0.0, 1.0}, {6}, f64()));
    const DataFrame frame({
        {"x", Thread(x)},
        {"cat", cat},
        {"target", Thread(y)},
    });

    const auto num_cols = frame.series_features().feature_names();
    const auto cat_cols = frame.note_features().feature_names();
    if (num_cols.size() < 2 || cat_cols.size() != 1 || cat_cols.front() != "cat") {
      if (err) *err << "lymph feature selectors should detect numeric and categorical features\n";
      return false;
    }
    const std::vector<std::string> model_numeric_features{"x"};

    pluss::ai::nodej::DataFrameBatchOptions data_options{};
    data_options.feature_names = {"x", "cat", "target"};
    data_options.label_feature = "target";
    data_options.layout = ObservationLayout::Rows;
    data_options.note_policy = pluss::ai::nodej::DataNotePolicy::Hash;
    data_options.thread_policy = pluss::ai::nodej::DataThreadPolicy::CastTemporalToSeries;

    auto feature_transformer = pluss::ai::lymph::make_feature_transformer(
        frame,
        data_options,
        std::vector<pluss::ai::lymph::workflow::feature::Branch>{
            pluss::ai::lymph::workflow::feature::compose(
                {
                    pluss::ai::lymph::workflow::stage::simple_imputer(
                        pluss::ai::lymph::prep::SimpleImputerOptions{
                            .strategy = pluss::ai::lymph::prep::SimpleImputerStrategy::Mean,
                        }),
                    pluss::ai::lymph::workflow::stage::standard_scaler(),
                },
                model_numeric_features),
            pluss::ai::lymph::workflow::feature::one_hot_encoder(
                pluss::ai::lymph::prep::OneHotEncoderOptions{.include_unknown_bucket = true},
                cat_cols),
        },
        pluss::ai::lymph::prep::FeatureRemainderPolicy::PassThrough);

    auto model = pluss::ai::lymph::linear_factory::linear_regression(LinearFamilyOptions{
        .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
        .regularization = pluss::ai::nodej::RegularizationKind::None,
        .regularization_strength = 0.0,
        .fit_intercept = true,
    });
    model.feature_transformer(std::move(feature_transformer));

    TrainOptions train_options{};
    train_options.max_iterations = 4000;
    train_options.tolerance = 1e-8;
    model.learn_(frame, data_options, train_options);

    auto metrics = model.assess(frame, data_options, pluss::ai::nodej::AssessOptions{
                                                        .metrics = {pluss::ai::nodej::MetricKind::Mse, pluss::ai::nodej::MetricKind::R2},
                                                    });
    if (!metrics.mse.defined() || !metrics.r2.defined()) {
      if (err) *err << "lymph feature-transformer workflow assess should expose mse/r2\n";
      return false;
    }
    if (metrics.r2.item<double>() < 0.95) {
      if (err) *err << "lymph feature-transformer workflow should fit simple mixed-schema regression\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0},
                                        {4, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto logistic_or = pluss::ai::lymph::quick::linear_logistic_classifier(train, 1e-6);
    if (!logistic_or.ok()) {
      if (err) *err << "lymph::linear_logistic_classifier should return learned model\n";
      return false;
    }
    auto hinge_or = pluss::ai::lymph::quick::linear_hinge_classifier(train, 1e-6);
    if (!hinge_or.ok()) {
      if (err) *err << "lymph::linear_hinge_classifier should return learned model\n";
      return false;
    }

    auto logistic_pred_or = logistic_or.value()->infer(train.features);
    auto hinge_pred_or = hinge_or.value()->infer(train.features);
    if (!logistic_pred_or.ok() || !hinge_pred_or.ok()) {
      if (err) *err << "lymph linear preset models should infer\n";
      return false;
    }

    const auto logistic_pred = logistic_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto hinge_pred = hinge_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < truth.size(); ++i) {
      if (!approx(logistic_pred[i], truth[i]) || !approx(hinge_pred[i], truth[i])) {
        if (err) *err << "lymph linear preset classifiers should fit this separable dataset\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.5, -1.0,
                                            -2.0, -2.0,
                                            -1.0, -1.5,
                                             1.0,  1.0,
                                             2.0,  1.0,
                                             1.0,  2.0,
                                             1.5,  1.0},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto cls_or = pluss::ai::lymph::quick::forest_classifier(train, 21, 1.0);
    if (!cls_or.ok()) {
      if (err) *err << "lymph::forest_classifier should return learned model\n";
      return false;
    }

    auto metrics_or = cls_or.value()->assess(train);
    if (!metrics_or.ok()) {
      if (err) *err << "lymph forest classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.empty() || metrics[0] < 0.95) {
      if (err) *err << "lymph forest classifier should produce high train accuracy\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, {6, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto reg_or = pluss::ai::lymph::quick::rbf_kernel_regressor(train, 50.0, 0.7, 0.0);
    if (!reg_or.ok()) {
      if (err) *err << "lymph::rbf_kernel_regressor should return learned model\n";
      return false;
    }

    auto pred_or = reg_or.value()->infer(train.features);
    if (!pred_or.ok()) {
      if (err) *err << "lymph kernel regressor model should infer\n";
      return false;
    }

    auto metrics_or = reg_or.value()->assess(train);
    if (!metrics_or.ok()) {
      if (err) *err << "lymph kernel regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() < 3 || metrics[2] < 0.9) {
      if (err) *err << "lymph kernel regressor should produce strong fit quality\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.0,
                                            -1.0, 1.0,
                                             0.0, 0.0,
                                             1.0, 1.0,
                                             2.0, 0.0,
                                             3.0, 1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.5, 1.0, 2.5, 5.0, 6.5}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto ridge_or = pluss::ai::lymph::quick::ridge_regressor(train, 1e-8);
    auto bayes_or = pluss::ai::lymph::quick::gaussian_bayes_regressor(train, 1e-6);
    if (!ridge_or.ok() || !bayes_or.ok()) {
      if (err) *err << "lymph regression presets should return learned models\n";
      return false;
    }

    auto ridge_metrics_or = ridge_or.value()->assess(train);
    auto bayes_pred_or = bayes_or.value()->infer(train.features);
    if (!ridge_metrics_or.ok() || !bayes_pred_or.ok()) {
      if (err) *err << "lymph regression presets should assess/infer\n";
      return false;
    }

    const auto ridge_metrics = ridge_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (ridge_metrics.size() < 3 || ridge_metrics[2] < 0.95) {
      if (err) *err << "lymph ridge preset should produce high training R2\n";
      return false;
    }

    const auto pred = bayes_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-3)) {
        if (err) *err << "lymph gaussian bayes regressor should recover near-linear training relation\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0, -1.0,
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0,
                                             3.0,  1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto flow_or = pluss::ai::lymph::quick::linear_classifier_flow(train, 1e-6, false, false);
    if (!flow_or.ok()) {
      if (err) *err << "lymph::linear_classifier_flow should return a learned flow\n";
      return false;
    }
    auto metrics_or =
        flow_or.value().assess(train, pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Accuracy}});
    if (!metrics_or.ok()) {
      if (err) *err << "lymph linear flow assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.empty() || metrics[0] < 0.9) {
      if (err) *err << "lymph linear flow should fit separable data\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.0,
                                            -1.0, 1.0,
                                             0.0, 0.0,
                                             1.0, 1.0,
                                             2.0, 0.0,
                                             3.0, 1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0, 7.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto flow_or = pluss::ai::lymph::quick::ridge_regressor_flow(train, 1e-8, false, false);
    if (!flow_or.ok()) {
      if (err) *err << "lymph::ridge_regressor_flow should return a learned flow\n";
      return false;
    }
    auto metrics_or = flow_or.value().assess(
        train,
        pluss::ai::nodej::AssessOptions{
            .metrics = {pluss::ai::nodej::MetricKind::Mse, pluss::ai::nodej::MetricKind::R2},
        });
    if (!metrics_or.ok()) {
      if (err) *err << "lymph ridge flow assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() < 2 || metrics[1] < 0.95) {
      if (err) *err << "lymph ridge flow should produce strong fit quality\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.0,
                                            -1.0, 1.0,
                                             0.0, 0.0,
                                             1.0, 1.0,
                                             2.0, 0.0,
                                             3.0, 1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0, 7.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto linear_flow_or = pluss::ai::lymph::quick::linear_regressor_flow(
        train,
        pluss::ai::nodej::LinearFamilyOptions{
            .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
            .regularization = pluss::ai::nodej::RegularizationKind::None,
            .regularization_strength = 0.0,
            .fit_intercept = true,
        },
        false,
        false);
    if (!linear_flow_or.ok()) {
      if (err) *err << "lymph::linear_regressor_flow should return a learned flow\n";
      return false;
    }
    auto linear_metrics_or = linear_flow_or.value().assess(
        train,
        pluss::ai::nodej::AssessOptions{
            .metrics = {pluss::ai::nodej::MetricKind::Mse, pluss::ai::nodej::MetricKind::R2},
        });
    if (!linear_metrics_or.ok()) {
      if (err) *err << "lymph linear regressor flow assess should succeed\n";
      return false;
    }
    const auto linear_metrics = linear_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (linear_metrics.size() < 2 || linear_metrics[1] < 0.95) {
      if (err) *err << "lymph linear regressor flow should fit linear signal\n";
      return false;
    }

    auto tree_flow_or = pluss::ai::lymph::quick::tree_regressor_flow(
        train,
        pluss::ai::nodej::TreeFamilyOptions{
            .criterion = pluss::ai::nodej::TreeCriterion::Mse,
            .max_depth = 8,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
        false);
    if (!tree_flow_or.ok()) {
      if (err) *err << "lymph::tree_regressor_flow should return a learned flow\n";
      return false;
    }
    auto tree_metrics_or = tree_flow_or.value().assess(
        train,
        pluss::ai::nodej::AssessOptions{
            .metrics = {pluss::ai::nodej::MetricKind::Mse, pluss::ai::nodej::MetricKind::R2},
        });
    if (!tree_metrics_or.ok()) {
      if (err) *err << "lymph tree regressor flow assess should succeed\n";
      return false;
    }
    const auto tree_metrics = tree_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (tree_metrics.size() < 2 || tree_metrics[1] < 0.95) {
      if (err) *err << "lymph tree regressor flow should fit this simple signal\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.9, -0.9,
                                            -1.8, -0.8,
                                             1.1,  1.0,
                                             1.2,  1.1,
                                             1.3,  1.2},
                                        {6, 2},
                                        f64());
    const FeatureBatch fb{
        .x = x,
        .layout = ObservationLayout::Rows,
    };
    auto kmeans_or = pluss::ai::lymph::quick::kmeans_clusterer(fb, 2, 3, 150, 1e-5);
    if (!kmeans_or.ok()) {
      if (err) *err << "lymph::kmeans_clusterer should return learned transformer\n";
      return false;
    }
    auto labels_or = kmeans_or.value()->map(fb);
    if (!labels_or.ok()) {
      if (err) *err << "lymph kmeans map should succeed\n";
      return false;
    }
    const auto labels = labels_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({6}).to_vec<double>();
    bool seen0 = false;
    bool seen1 = false;
    for (double v : labels) {
      if (std::llround(v) == 0) seen0 = true;
      if (std::llround(v) == 1) seen1 = true;
    }
    if (!(seen0 && seen1)) {
      if (err) *err << "lymph kmeans should discover two clusters\n";
      return false;
    }
  }

  {
    std::vector<double> values{};
    for (int i = 0; i < 24; ++i) {
      const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 23.0;
      const double x1 = 0.7 * x0;
      const double x2 = -0.5 * x0 + 0.6 * x1;
      values.push_back(x0);
      values.push_back(x1);
      values.push_back(x2);
    }
    const FeatureBatch fb{
        .x = pluss::make_series(values, {24, 3}, f64()),
        .layout = ObservationLayout::Rows,
    };
    auto pca_or = pluss::ai::lymph::quick::pca_mapper(fb, 2, false, 300, 1e-6);
    if (!pca_or.ok()) {
      if (err) *err << "lymph::pca_mapper should return learned transformer\n";
      return false;
    }
    auto mapped_or = pca_or.value()->map(fb);
    if (!mapped_or.ok()) {
      if (err) *err << "lymph pca map should succeed\n";
      return false;
    }
    const auto sz = mapped_or.value().sizes();
    if (!(sz.size() == 2 && sz[0] == 24 && sz[1] == 2)) {
      if (err) *err << "lymph pca output should have [samples, components] shape\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph quick wrappers") {
  REQUIRE(run_lymph_quick_checks(&std::cerr));
}
#else
int main() {
  if (!run_lymph_quick_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph_quick_test passed\n";
  return 0;
}
#endif
