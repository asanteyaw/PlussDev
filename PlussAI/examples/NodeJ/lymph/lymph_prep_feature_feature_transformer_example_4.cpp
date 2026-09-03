#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The common home for a FeatureTransformer is a model: build it, hand it over
// with Model::feature_transformer (it is move-only), and the model fits and
// replays it as the first step of learn / infer.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train({
      {"income",  Thread(pluss::make_series({1.0, 1.5, 2.0, 3.5, 4.0, 4.5}, {6}, f64))},
      {"region",  Thread(pluss::make_series({0., 1., 0., 1., 0., 1.}, {6}, f64))},
      {"target",  Thread(pluss::make_series({0., 0., 0., 1., 1., 1.}, {6}, f64))},
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
      {"income",    Thread(train.features("income"))},
      {"region",    Thread(train.features("region"))},
      {"actual",    Thread(train.features("target"))},
      {"predicted", Thread(model.infer(train, data_options))},
  });

  return 0;
}
