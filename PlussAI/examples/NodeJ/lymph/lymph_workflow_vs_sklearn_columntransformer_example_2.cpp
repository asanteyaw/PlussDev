#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph   = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  namespace models  = lymph::models;
  namespace prep    = lymph::prep;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const std::vector<std::string> numeric_features     = {"income", "balance"};
  const std::vector<std::string> categorical_features = {"region", "tier"};

  // A cleanly separable toy set: the first five rows are class 0, the last
  // five class 1, and every feature (numeric and categorical) carries the
  // signal.
  const DataFrame raw({
      {"income",  Thread(pluss::make_series({1.0, 1.2, 1.4, 1.1, 1.3, 3.0, 3.2, 3.4, 3.1, 3.3}, {10}, f64))},
      {"balance", Thread(pluss::make_series({0.5, 0.7, 0.6, 0.8, 0.5, 2.5, 2.7, 2.6, 2.8, 2.5}, {10}, f64))},
      {"region",  Thread(pluss::make_note(std::vector<std::string>{
          "lo", "lo", "lo", "lo", "lo", "hi", "hi", "hi", "hi", "hi"}))},
      {"tier",    Thread(pluss::make_note(std::vector<std::string>{
          "a", "a", "a", "a", "a", "b", "b", "b", "b", "b"}))},
      {"target",  Thread(pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64))},
  });

  // final_pipe = Pipeline([("preprocessing", preprocessor), ("model", LogisticRegression())])
  //   -- the Workflow holds the one estimator the whole thing feeds into.
  workflow::Workflow final_pipe = workflow::make(models::logistic_regression());

  // ("num", numeric_pipeline, numeric_features)
  //   numeric_pipeline = Pipeline([("imputer", SimpleImputer("median")), ("scaler", StandardScaler())])
  final_pipe.on("num", numeric_features)
      .add(prep::stage::simple_imputer({.strategy = prep::SimpleImputerStrategy::Median}))
      .add(prep::stage::standard_scaler());

  // ("cat", categorical_pipeline, categorical_features)
  //   categorical_pipeline = Pipeline([("imputer", SimpleImputer("most_frequent")),
  //                                    ("encoder", OneHotEncoder(handle_unknown="ignore"))])
  //   include_unknown_bucket = true (the default) is the handle_unknown="ignore" behavior.
  final_pipe.on("cat", categorical_features)
      .add(prep::stage::simple_imputer({.strategy = prep::SimpleImputerStrategy::MostFrequent}))
      .add(prep::stage::one_hot_encoder({.include_unknown_bucket = true}));

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "region", "tier"};
  data_options.label_feature = "target";
  data_options.note_policy   = lymph::DataNotePolicy::Factorize;

  const workflow::Workflow fitted = final_pipe.learn(raw, data_options);

  std::cout << "branch_count = " << fitted.branch_count() << "\n";
  for (const auto& b : fitted.branches())
    std::cout << "  branch \"" << b.name << "\" stage_count=" << b.stage_count << "\n";
  std::cout << "total stage_count = " << fitted.stage_count() << "\n";
  std::cout << "pred = " << fitted.infer(raw, data_options) << "\n";

  return 0;
}
