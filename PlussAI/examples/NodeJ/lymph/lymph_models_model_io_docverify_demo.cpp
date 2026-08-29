#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <vector>

namespace {

bool same_values(const pluss::Series& left, const pluss::Series& right) {
  const std::vector<double> left_values = left.to(pluss::pCPU, pluss::ScalarType::Float64)
                                              .reshape({left.numel()})
                                              .to_vec<double>();
  const std::vector<double> right_values = right.to(pluss::pCPU, pluss::ScalarType::Float64)
                                               .reshape({right.numel()})
                                               .to_vec<double>();
  return left_values == right_values;
}

} // namespace

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace options = lymph::options;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  options::Linear logistic_options{};
  logistic_options.regularization = lymph::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::models::Model model = lymph::models::linear::logistic_regression(logistic_options);
  lymph::models::ModelPrep prep = model.prepare();
  const lymph::prep::Route categorical = prep.names({"region"});
  prep.standardize(prep.remaining(categorical));
  prep.encode(
      categorical,
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });

  options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  options::Train train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  model.learn_(raw, data_options, train_options);
  const pluss::Series original = model.infer(raw, data_options);

  const std::filesystem::path model_path =
      std::filesystem::temp_directory_path() / "plussai_lymph_models_model_io_docverify.bundle";
  model.save(model_path.string());
  const lymph::models::Model loaded = lymph::models::Model::load(model_path.string());
  const pluss::Series reloaded = loaded.infer(raw, data_options);

  const DataFrame original_frame({
      {"original", Thread(original)},
  });
  const DataFrame reloaded_frame({
      {"reloaded", Thread(reloaded)},
  });

  std::cout << "== save_load ==\n";
  std::cout << "original_trained = " << std::boolalpha << model.trained()
            << ", loaded_trained = " << loaded.trained() << "\n";
  std::cout << "original_stage_count = " << model.stage_count()
            << ", loaded_stage_count = " << loaded.stage_count() << "\n\n";
  std::cout << original_frame << "\n";
  std::cout << reloaded_frame << "\n";
  std::cout << "roundtrip_match = " << same_values(original, reloaded) << "\n";

  std::filesystem::remove(model_path);

  return 0;
}
