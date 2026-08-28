#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"x1", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"x2", Thread(pluss::make_series({1000, 2000, 500, 4000, 1500, 3000, 800, 3500}, {8}, f64))},
      {"category", Thread(pluss::make_note({"a", "b", "a", "b", "a", "b", "a", "b"}))},
      {"y", Thread(pluss::make_series({0, 0, 0, 1, 0, 1, 0, 1}, {8}, f64))},
  });

  const lymph::prep::Route categorical = lymph::prep::route::names({"category"});
  const lymph::prep::Route numeric = lymph::prep::route::remaining_of(categorical);

  lymph::models::Model model = lymph::models::tree::decision_tree_classifier({.max_depth = 3});
  model.append(lymph::prep::stage::standard_scaler(), numeric);
  model.append(
      lymph::prep::stage::one_hot_encoder({
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      }),
      categorical);

  std::cout << "== append_before_learn ==\n";
  std::cout << "trained=" << std::boolalpha << model.trained()
            << " stage_count=" << model.stage_count() << "\n";

  lymph::DataFrameBatchOptions data_options{};
  data_options.label_feature = "y";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  model.learn_(raw, data_options);
  const Series predicted = model.infer(raw.drop({"y"}), data_options);

  std::cout << "== append_after_learn ==\n";
  std::cout << "trained=" << model.trained()
            << " stage_count=" << model.stage_count() << "\n";
  std::cout << "predictions=" << predicted << "\n";

  return 0;
}
