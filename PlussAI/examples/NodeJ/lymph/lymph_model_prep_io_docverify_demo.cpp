#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string join_i64(const std::vector<std::int64_t>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ", ";
    out << values[i];
  }
  out << "]";
  return out.str();
}

std::vector<double> to_host_values(const pluss::Series& values) {
  return values.to(pluss::pCPU, pluss::ScalarType::Float64)
      .reshape({values.numel()})
      .to_vec<double>();
}

bool same_values(const pluss::Series& left, const pluss::Series& right) {
  return to_host_values(left) == to_host_values(right);
}

} // namespace

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;
  using pluss::ai::nodej::AssessOptions;
  using pluss::ai::nodej::LinearFamilyOptions;
  using pluss::ai::nodej::MetricKind;
  using pluss::ai::nodej::RegularizationKind;
  using pluss::ai::nodej::TrainOptions;

  const pluss::Config f64 = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  lymph::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance", "late", "region", "observed_at"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "== policy_apply ==\n";
  std::cout << "raw_rows=" << raw.ncases() << " admitted_rows=" << admitted.ncases() << "\n";
  std::cout << "admitted_features=" << admitted.nfeatures() << "\n";

  lymph::Payload payload = lymph::payload::make(raw)
      .features({"income", "balance", "late", "region"})
      .label("default")
      .time("observed_at")
      .note(lymph::DataNotePolicy::Factorize);

  const lymph::LabeledBatch labeled = payload.labeled_batch();
  std::cout << "== payload ==\n";
  std::cout << "has_frame=" << payload.has_frame()
            << " has_label_feature=" << payload.has_label_feature()
            << " has_time=" << payload.has_time()
            << " time_feature=" << payload.time_feature() << "\n";
  std::cout << "batch_rows=" << labeled.features.sample_count()
            << " feature_width=" << labeled.features.feature_count() << "\n";

  const std::vector<std::string> ordered_names = {"income", "balance", "late", "region"};
  const lymph::ColumnRouter numeric = lymph::columnrouter::where(
      [](std::string_view, std::string_view dtype) {
        return dtype == "float64";
      });
  const lymph::ColumnRouter categorical =
      lymph::columnrouter::names(std::vector<std::string>{"region"});
  const lymph::ColumnRouter remainder = lymph::columnrouter::remaining_of(categorical);

  std::cout << "== column_router ==\n";
  std::cout << "numeric=" << join_i64(numeric.resolve(raw, ordered_names)) << "\n";
  std::cout << "categorical=" << join_i64(categorical.resolve(raw, ordered_names)) << "\n";
  std::cout << "remainder=" << join_i64(remainder.resolve(raw, ordered_names)) << "\n";

  lymph::prep::StandardScaler standalone_scaler(lymph::prep::StandardScalerOptions{
      .with_centering = true,
      .with_scaling = true,
      .variance_floor = 1e-12,
  });
  const DataFrame scaled = standalone_scaler.learn_map(raw.features({"income", "balance", "late"}));
  const lymph::prep::StandardScalerSummary scaler_summary = standalone_scaler.summary();
  std::cout << "== standard_scaler ==\n";
  std::cout << "rows=" << scaled.ncases()
            << " features=" << scaled.nfeatures()
            << " summary_features=" << scaler_summary.feature_names.size() << "\n";
  std::cout << scaled.head(2) << "\n";

  lymph::prep::OneHotEncoder standalone_encoder(lymph::prep::OneHotEncoderOptions{
      .max_categories = 8,
      .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
  });
  standalone_encoder.features({"region"});
  const DataFrame encoded = standalone_encoder.learn_map(raw.features({"region"}));
  const lymph::prep::TransformerSummary encoder_summary = standalone_encoder.summary();
  std::cout << "== one_hot_encoder ==\n";
  std::cout << "rows=" << encoded.ncases()
            << " features=" << encoded.nfeatures()
            << " summary_features=" << encoder_summary.feature_names.size() << "\n";
  std::cout << encoded.head(3) << "\n";

  lymph::DataFrameBatchOptions transform_options{};
  transform_options.feature_names = {"income", "region", "balance"};
  transform_options.note_policy = lymph::DataNotePolicy::Factorize;
  transform_options.layout = lymph::ObservationLayout::Rows;
  lymph::FeatureTransformer feature_transformer = lymph::make_feature_transformer(
      raw,
      transform_options,
      {
          lymph::workflow::feature::compose(
              {
                  lymph::workflow::stage::standard_scaler(),
              },
              {"income"}),
          lymph::workflow::feature::one_hot_encoder(
              lymph::prep::OneHotEncoderOptions{
                  .max_categories = 8,
                  .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
                  .include_unknown_bucket = true,
              },
              {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);
  const lymph::Payload transform_payload =
      lymph::payload::make(raw).features({"income", "region", "balance"}).note(lymph::DataNotePolicy::Factorize);
  const lymph::FeatureBatch transform_batch = transform_payload.feature_batch();
  const Series transformed = lymph::resolve(
      feature_transformer.learn_map(transform_batch), "feature_transformer.learn_map");
  std::cout << "== feature_transformer ==\n";
  std::cout << "trained=" << feature_transformer.trained()
            << " stage_count=" << feature_transformer.stage_count()
            << " rows=" << transformed.sizes()[0]
            << " features=" << transformed.sizes()[1] << "\n";
  std::cout << transformed << "\n";

  LinearFamilyOptions logistic_options{};
  logistic_options.regularization = RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::Model model = lymph::model::logistic_regression(logistic_options);
  lymph::ModelPrep prep = model.prepare();
  prep.standardize({"income", "balance", "late"});
  prep.encode(categorical, lymph::prep::OneHotEncoderOptions{
      .max_categories = 8,
      .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
  });

  TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = ordered_names;
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  lymph::Model staged_model = lymph::model::logistic_regression(logistic_options);
  staged_model.append(
      lymph::stage::standard_scaler(),
      lymph::columnrouter::names({"income", "balance", "late"}));
  staged_model.append(
      lymph::stage::one_hot_encoder(lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      }),
      lymph::columnrouter::names({"region"}));
  staged_model.learn_(raw, data_options, train_options);
  std::cout << "== stage ==\n";
  std::cout << "stage_count=" << staged_model.stage_count()
            << " trained=" << staged_model.trained()
            << " task_kind=" << static_cast<int>(staged_model.task_kind()) << "\n";

  model.learn_(raw, data_options, train_options);
  const Series model_pred = model.infer(raw, data_options);
  const lymph::MetricReport model_score = model.assess(raw, data_options, AssessOptions{
      .metrics = {MetricKind::Accuracy, MetricKind::F1},
  });

  std::cout << "== model ==\n";
  std::cout << "stage_count=" << model.stage_count()
            << " trained=" << model.trained()
            << " task_kind=" << static_cast<int>(model.task_kind()) << "\n";
  std::cout << "predictions=" << model_pred << "\n";
  std::cout << "accuracy=" << model_score.accuracy
            << " f1=" << model_score.f1 << "\n";

  lymph::Workflow workflow = lymph::workflow::make(model, policy);
  const DataFrame incoming({
      {"customer_id", Thread(pluss::make_series({201, 202, 203}, {3}, f64))},
      {"income", Thread(pluss::make_series({1.2, 4.4, 1.6}, {3}, f64))},
      {"balance", Thread(pluss::make_series({0.3, 2.3, 0.5}, {3}, f64))},
      {"late", Thread(pluss::make_series({0, 1, 0}, {3}, f64))},
      {"region", Thread(pluss::make_note({"north", "south", "west"}))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-02-01T00:00:00Z",
          "2026-02-02T00:00:00Z",
          "2026-02-03T00:00:00Z",
      }))},
  });
  const Series workflow_pred = workflow.infer(incoming, data_options);

  const std::filesystem::path bundle_dir = std::filesystem::temp_directory_path();
  const std::filesystem::path model_path = bundle_dir / "plussai_lymph_model_docverify.bundle";
  const std::filesystem::path workflow_path = bundle_dir / "plussai_lymph_workflow_docverify.bundle";
  model.save(model_path.string());
  workflow.save(workflow_path.string());

  const lymph::Model loaded_model = lymph::Model::load(model_path.string());
  const lymph::Workflow loaded_workflow = lymph::workflow::load(workflow_path.string());
  const Series loaded_model_pred = loaded_model.infer(raw, data_options);
  const Series loaded_workflow_pred = loaded_workflow.infer(incoming, data_options);

  std::cout << "== workflow ==\n";
  std::cout << "predictions=" << workflow_pred << "\n";
  std::cout << "roundtrip_match=" << same_values(workflow_pred, loaded_workflow_pred) << "\n";

  std::cout << "== io ==\n";
  std::cout << "model_roundtrip=" << same_values(model_pred, loaded_model_pred) << "\n";
  std::cout << "workflow_roundtrip=" << same_values(workflow_pred, loaded_workflow_pred) << "\n";
  std::cout << "loaded_workflow_trained=" << loaded_workflow.trained() << "\n";

  std::filesystem::remove(model_path);
  std::filesystem::remove(workflow_path);
  std::filesystem::remove(workflow_path.string() + ".policy");

  return 0;
}
