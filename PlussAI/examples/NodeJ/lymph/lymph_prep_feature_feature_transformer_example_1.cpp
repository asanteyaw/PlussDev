#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Two numeric columns and one categorical column. Each branch routes to its
  // own column subset and runs its own stage pipeline; the results are
  // concatenated left to right in branch order.
  const DataFrame raw({
      {"income",  Thread(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"region",  Thread(pluss::make_series({0., 1., 0., 1.}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10., 20., 30., 40.}, {4}, f64))},
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
  std::cout << "stage_count = " << transformer.stage_count() << "\n";
  std::cout << "output shape = [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n";
  std::cout << "columns: income(scaled) balance(scaled) region=0 region=1 region_unknown\n\n";
  std::cout << out << "\n";

  return 0;
}
