#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A FeatureTransformer does not need a model. The same object can be driven
// directly: build a FeatureBatch from a workflow::Payload and call learn_map to
// get the transformed feature matrix back.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income",  Slate(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"region",  Slate(pluss::make_series({0., 1., 0., 1.}, {4}, f64))},
      {"balance", Slate(pluss::make_series({10., 20., 30., 40.}, {4}, f64))},
  });

  lymph::prep::FeatureTransformer transformer = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::standard_scaler({}, {"income", "balance"}),
          lymph::prep::branch::one_hot_encoder({}, {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const lymph::FeatureBatch batch =
      lymph::workflow::payload::make(raw).features({"income", "region", "balance"}).feature_batch();
  const Series out = lymph::resolve(transformer.learn_map(batch),
                                    "feature_transformer.learn_map");

  std::cout << "trained     = " << std::boolalpha << transformer.trained() << "\n";
  std::cout << "stage_count = " << transformer.stage_count() << " (branches)\n";
  std::cout << "output shape = [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n";
  std::cout << "columns: income(scaled) balance(scaled) region=0 region=1 region_unknown\n\n";
  std::cout << out << "\n";

  return 0;
}
