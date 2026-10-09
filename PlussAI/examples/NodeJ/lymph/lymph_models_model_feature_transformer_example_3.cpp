#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A FeatureTransformer composes with append()'d stages: the transformer runs
// first, then the appended stages see its output. Here the transformer one-hot
// expands `region`, then an appended scaler standardises `spend` on the widened
// frame.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train({
      {"spend",  Slate(pluss::make_series({10., 20., 30., 40., 50., 60.}, {6}, f64))},
      {"region", Slate(pluss::make_series({0., 1., 0., 1., 0., 1.}, {6}, f64))},
      {"target", Slate(pluss::make_series({0., 0., 0., 1., 1., 1.}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"spend", "region"};
  data_options.label_feature = "target";

  lymph::prep::FeatureTransformer pre = lymph::prep::feature_transformer(
      train, data_options,
      {lymph::prep::branch::one_hot_encoder({}, {"region"})},
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.feature_transformer(std::move(pre));
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"spend"}));
  model.learn_(train, data_options);

  std::cout << "trained = " << std::boolalpha << model.trained() << "\n";
  std::cout << DataFrame({
      {"region",    Slate(train.features("region"))},
      {"actual",    Slate(train.features("target"))},
      {"predicted", Slate(model.infer(train, data_options))},
  });

  return 0;
}
