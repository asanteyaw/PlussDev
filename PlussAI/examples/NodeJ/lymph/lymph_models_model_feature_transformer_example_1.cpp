#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Model::feature_transformer attaches a pre-built prep::FeatureTransformer --
// a multi-branch, column-routed preprocessing contract -- to the model. The
// model fits it and replays it as the first step of learn / infer. The
// argument is move-only.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train({
      {"income",  Slate(pluss::make_series({1.0, 1.5, 2.0, 3.5, 4.0, 4.5}, {6}, f64))},
      {"region",  Slate(pluss::make_series({0., 1., 0., 1., 0., 1.}, {6}, f64))},
      {"target",  Slate(pluss::make_series({0., 0., 0., 1., 1., 1.}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "region"};
  data_options.label_feature = "target";

  lymph::prep::FeatureTransformer preprocessing = lymph::prep::feature_transformer(
      train, data_options,
      {
          lymph::prep::branch::standard_scaler({}, {"income"}),
          lymph::prep::branch::one_hot_encoder({}, {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.feature_transformer(std::move(preprocessing));
  model.learn_(train, data_options);

  std::cout << "trained = " << std::boolalpha << model.trained() << "\n";
  std::cout << DataFrame({
      {"income",    Slate(train.features("income"))},
      {"region",    Slate(train.features("region"))},
      {"actual",    Slate(train.features("target"))},
      {"predicted", Slate(model.infer(train, data_options))},
  });

  return 0;
}
