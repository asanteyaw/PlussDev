#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The lymph equivalent of sklearn's ColumnTransformer([("num", numeric_pipeline,
// numeric_features), ("cat", categorical_pipeline, categorical_features)]) +
// Pipeline([("preprocessing", preprocessor), ("model", LogisticRegression())]):
// each Workflow::on(...) branch IS one sub-pipeline (its own ordered .add(stage)
// chain, routed at its own columns); the branches together plus the held Model
// are the whole "preprocessing -> model" pipeline, in one object instead of
// four nested ones.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8}, {10}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4}, {10}, f64))},
      {"region", Slate(pluss::make_note(std::vector<std::string>{
          "a", "b", "a", "b", "a", "b", "a", "b", "a", "b"}))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64))},
  });

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());

  // "numeric_pipeline" -- imputer, then scaler, routed at the numeric columns.
  wf.on("numeric", {"income", "balance"})
      .add(lymph::prep::stage::simple_imputer())
      .add(lymph::prep::stage::standard_scaler());

  // "categorical_pipeline" -- imputer, then encoder, routed at the categorical
  // column. Two branches on the same Workflow == ColumnTransformer's num/cat
  // split.
  wf.on("categorical", {"region"})
      .add(lymph::prep::stage::simple_imputer())
      .add(lymph::prep::stage::one_hot_encoder());

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  const workflow::Workflow trained = wf.learn(raw, data_options);
  std::cout << "branch_count = " << trained.branch_count() << "\n";
  for (const auto& b : trained.branches()) {
    std::cout << "  branch \"" << b.name << "\" stage_count=" << b.stage_count << "\n";
  }
  std::cout << "pred = " << trained.infer(raw, data_options) << "\n";

  return 0;
}
