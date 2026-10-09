#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

namespace {

namespace lymph = pluss::ai::lymph;
using pluss::ScalarType;
using pluss::table::DataFrame;
using pluss::table::Slate;

const pluss::Config kF64 =
    pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

DataFrame features() {
  return DataFrame({
      {"x1", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, kF64))},
      {"x2", Slate(pluss::make_series({2, 2, 3, 3, 7, 7, 8, 8}, {8}, kF64))},
  });
}

pluss::Series labels() {
  return pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, kF64);
}

DataFrame labelled() {
  return DataFrame({
      {"x1", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, kF64))},
      {"x2", Slate(pluss::make_series({2, 2, 3, 3, 7, 7, 8, 8}, {8}, kF64))},
      {"label", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, kF64))},
  });
}

lymph::models::Model tree() {
  return lymph::models::tree::decision_tree_classifier({.max_depth = 3});
}

lymph::options::DFBatch frame_options() {
  lymph::options::DFBatch options{};
  options.feature_names = {"x1", "x2"};
  options.label_feature = "label";
  return options;
}

void print_metric(const char* heading, const lymph::reports::Metric& report) {
  std::cout << heading << " accuracy=" << report.accuracy.item<double>() << '\n';
}

}  // namespace

int main() {
  const DataFrame x = features();
  const pluss::Series y = labels();
  const DataFrame frame = labelled();
  const lymph::options::DFBatch options = frame_options();

  // APPEND
  auto append_numeric = tree();
  append_numeric.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"x1"}));
  std::cout << "append_named trained=" << std::boolalpha << append_numeric.trained() << '\n';

  auto append_remaining = tree();
  const auto x1 = lymph::prep::route::names({"x1"});
  append_remaining.append(lymph::prep::stage::standard_scaler(), x1)
      .append(lymph::prep::stage::minmax_scaler(), lymph::prep::route::remaining_of(x1));
  std::cout << "append_chain trained=" << append_remaining.trained() << '\n';

  auto append_learned = tree();
  append_learned.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"x1", "x2"}));
  append_learned.learn_(x, y);
  std::cout << "append_learned trained=" << append_learned.trained()
            << " stages=" << append_learned.stage_count() << '\n';

  // PREPARE
  auto prepared_named = tree();
  prepared_named.prepare().standardize(lymph::prep::route::names({"x1"}));
  prepared_named.learn_(x, y);
  std::cout << "prepare_named stages=" << prepared_named.stage_count() << '\n';

  auto prepared_remaining = tree();
  auto prep = prepared_remaining.prepare();
  const auto first = prep.names({"x1"});
  prep.standardize(prep.remaining(first));
  prepared_remaining.learn_(x, y);
  std::cout << "prepare_remaining stages=" << prepared_remaining.stage_count() << '\n';

  auto prepared_empty = tree();
  [[maybe_unused]] const auto empty_prep = prepared_empty.prepare();
  std::cout << "prepare_accessor trained=" << prepared_empty.trained() << '\n';

  // LEARN
  auto learned_mutating = tree();
  learned_mutating.learn_(x, y);
  std::cout << "learn_mutating trained=" << learned_mutating.trained() << '\n';

  const auto template_model = tree();
  const auto learned_copy = template_model.learn(x, y);
  std::cout << "learn_copy template=" << template_model.trained()
            << " copy=" << learned_copy.trained() << '\n';

  auto learned_frame = tree();
  learned_frame.learn_(frame, options);
  std::cout << "learn_frame trained=" << learned_frame.trained() << '\n';

  // INFER
  const auto inference_model = tree().learn(x, y);
  std::cout << "infer_frame cases=" << inference_model.infer(x).size().item<std::int64_t>() << '\n';
  std::cout << "infer_options cases=" << inference_model.infer(frame, options).size().item<std::int64_t>() << '\n';
  std::cout << "infer_scores cases=" << inference_model.infer_scores(x).size().item<std::int64_t>() << '\n';

  // ASSESS
  print_metric("assess_xy", inference_model.assess(x, y));
  print_metric("assess_frame", inference_model.assess(frame, options));
  std::cout << "assess_series cases=" << inference_model.assess_series(x, y).size().item<std::int64_t>() << '\n';

  // SAVE AND LOAD
  const std::filesystem::path plain_path = "/tmp/pluss_lymph_model_methods_plain.flow";
  const std::filesystem::path trained_path = "/tmp/pluss_lymph_model_methods_trained.flow";
  const std::filesystem::path frame_path = "/tmp/pluss_lymph_model_methods_frame.flow";
  inference_model.save(plain_path.string());
  const auto loaded_plain = lymph::models::Model::load(plain_path.string());
  std::cout << "save_plain loaded=" << loaded_plain.trained() << '\n';

  tree().save(trained_path.string(), x, y);
  const auto loaded_training = lymph::models::Model::load(trained_path.string());
  std::cout << "save_training loaded=" << loaded_training.trained() << '\n';

  tree().save(frame_path.string(), frame, options);
  const auto loaded_frame = lymph::models::Model::load(frame_path.string());
  std::cout << "save_frame loaded=" << loaded_frame.trained() << '\n';

  const auto restored_prediction = loaded_plain.infer(x);
  std::cout << "load_prediction cases=" << restored_prediction.size().item<std::int64_t>() << '\n';
  std::cout << "load_state stages=" << loaded_training.stage_count() << '\n';

  std::filesystem::remove(plain_path);
  std::filesystem::remove(trained_path);
  std::filesystem::remove(frame_path);
  std::cout << "save_cleanup=" << (!std::filesystem::exists(plain_path) && !std::filesystem::exists(trained_path)
                                       && !std::filesystem::exists(frame_path)) << '\n';
  return 0;
}
