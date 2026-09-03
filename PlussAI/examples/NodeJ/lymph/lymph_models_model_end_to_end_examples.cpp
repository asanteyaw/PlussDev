#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

namespace {
namespace lymph = pluss::ai::lymph;
using pluss::ScalarType;
using pluss::table::DataFrame;
using pluss::table::Thread;

// BEGIN APPEND 1
void append_named_route() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"income", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))},
                     {"late", Thread(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"income"}));
  model.learn_(x, y);
  std::cout << "append_named trained=" << std::boolalpha << model.trained()
            << " predictions=" << model.infer(x).size().item<std::int64_t>() << '\n';
}
// END APPEND 1

// BEGIN APPEND 2
void append_chained_routes() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"amount", Thread(pluss::make_series({10, 20, 80, 90}, {4}, cfg))},
                     {"age", Thread(pluss::make_series({20, 30, 50, 60}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  const auto amount = lymph::prep::route::names({"amount"});
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.append(lymph::prep::stage::standard_scaler(), amount)
      .append(lymph::prep::stage::minmax_scaler(), lymph::prep::route::remaining_of(amount));
  model.learn_(x, y);
  std::cout << "append_chain trained=" << std::boolalpha << model.trained()
            << " stages=" << model.stage_count() << '\n';
}
// END APPEND 2

// BEGIN APPEND 3
void append_categorical_encoder() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame rows({{"income", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))},
                        {"tier", Thread(pluss::make_note({"basic", "basic", "gold", "gold"}))},
                        {"defaulted", Thread(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  lymph::options::DFBatch contract{};
  contract.feature_names = {"income", "tier"};
  contract.label_feature = "defaulted";
  contract.note_policy = lymph::DataNotePolicy::Factorize;
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"tier"}));
  model.learn_(rows, contract);
  std::cout << "append_encoder trained=" << std::boolalpha << model.trained()
            << " predictions=" << model.infer(rows, contract).size().item<std::int64_t>() << '\n';
}
// END APPEND 3

// BEGIN PREPARE 1
void prepare_named_standardization() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"income", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))},
                     {"late", Thread(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.prepare().standardize(lymph::prep::route::names({"income"}));
  model.learn_(x, y);
  std::cout << "prepare_named trained=" << std::boolalpha << model.trained()
            << " stages=" << model.stage_count() << '\n';
}
// END PREPARE 1

// BEGIN PREPARE 2
void prepare_remainder_route() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"amount", Thread(pluss::make_series({10, 20, 80, 90}, {4}, cfg))},
                     {"age", Thread(pluss::make_series({20, 30, 50, 60}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  auto prep = model.prepare();
  const auto amount = prep.names({"amount"});
  prep.standardize(prep.remaining(amount));
  model.learn_(x, y);
  std::cout << "prepare_remaining trained=" << std::boolalpha << model.trained()
            << " stages=" << model.stage_count() << '\n';
}
// END PREPARE 2

// BEGIN PREPARE 3
void prepare_categorical_encoding() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame rows({{"income", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))},
                        {"tier", Thread(pluss::make_note({"basic", "basic", "gold", "gold"}))},
                        {"defaulted", Thread(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  lymph::options::DFBatch contract{};
  contract.feature_names = {"income", "tier"};
  contract.label_feature = "defaulted";
  contract.note_policy = lymph::DataNotePolicy::Factorize;
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.prepare().encode(lymph::prep::route::names({"tier"}));
  model.learn_(rows, contract);
  std::cout << "prepare_encode trained=" << std::boolalpha << model.trained()
            << " predictions=" << model.infer(rows, contract).size().item<std::int64_t>() << '\n';
}
// END PREPARE 3

// BEGIN LEARN 1
void learn_mutates_current_model() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"score", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.learn_(x, y);
  std::cout << "learn_mutating trained=" << std::boolalpha << model.trained()
            << " predictions=" << model.infer(x).size().item<std::int64_t>() << '\n';
}
// END LEARN 1

// BEGIN LEARN 2
void learn_returns_a_trained_copy() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"score", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  const auto template_model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  const auto trained_model = template_model.learn(x, y);
  std::cout << "learn_copy template=" << std::boolalpha << template_model.trained()
            << " copy=" << trained_model.trained() << '\n';
}
// END LEARN 2

// BEGIN LEARN 3
void learn_from_a_labelled_dataframe() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame rows({{"score", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))},
                        {"defaulted", Thread(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  lymph::options::DFBatch contract{};
  contract.feature_names = {"score"};
  contract.label_feature = "defaulted";
  auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  model.learn_(rows, contract);
  std::cout << "learn_dataframe trained=" << std::boolalpha << model.trained()
            << " predictions=" << model.infer(rows, contract).size().item<std::int64_t>() << '\n';
}
// END LEARN 3

// BEGIN INFER 1
void infer_feature_dataframe() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"score", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2}).learn(x, y);
  std::cout << "infer_features cases=" << model.infer(x).size().item<std::int64_t>() << '\n';
}
// END INFER 1

// BEGIN INFER 2
void infer_labelled_dataframe_contract() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame rows({{"score", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))},
                        {"defaulted", Thread(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  lymph::options::DFBatch contract{};
  contract.feature_names = {"score"};
  contract.label_feature = "defaulted";
  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2}).learn(rows, contract);
  std::cout << "infer_dataframe cases=" << model.infer(rows, contract).size().item<std::int64_t>() << '\n';
}
// END INFER 2

// BEGIN INFER 3
void infer_scores_instead_of_labels() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"score", Thread(pluss::make_series({1, 2, 8, 9}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2}).learn(x, y);
  std::cout << "infer_scores cases=" << model.infer_scores(x).size().item<std::int64_t>() << '\n';
}
// END INFER 3

}  // namespace

int main() {
  append_named_route(); append_chained_routes(); append_categorical_encoder();
  prepare_named_standardization(); prepare_remainder_route(); prepare_categorical_encoding();
  learn_mutates_current_model(); learn_returns_a_trained_copy(); learn_from_a_labelled_dataframe();
  infer_feature_dataframe(); infer_labelled_dataframe_contract(); infer_scores_instead_of_labels();
  return 0;
}
