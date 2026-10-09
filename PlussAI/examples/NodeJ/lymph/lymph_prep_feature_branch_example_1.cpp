#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A branch is a route (feature_names) plus an ordered stage pipeline.
// branch::compose takes an explicit stage list; the per-transform builders
// (branch::standard_scaler, branch::one_hot_encoder, ...) are the one-stage
// shorthand for the common case.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Column with a gap (NaN) so the impute -> scale pipeline in the composed
  // branch has something to do.
  const DataFrame raw({
      {"x", Slate(pluss::make_series({10., 20., std::nan(""), 40., 50.}, {5}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"x"}).feature_batch();
  };

  // compose: mean-impute, then standard-scale, on column x.
  lymph::prep::FeatureTransformer composed = lymph::prep::feature_transformer(
      raw,
      {lymph::prep::branch::compose(
          {
              lymph::prep::stage::simple_imputer(
                  {.strategy = lymph::prep::SimpleImputerStrategy::Mean}),
              lymph::prep::stage::standard_scaler(),
          },
          {"x"})},
      lymph::prep::FeatureRemainderPolicy::Drop);

  std::cout << "compose(impute -> scale):\n"
            << lymph::resolve(composed.learn_map(batch()), "composed") << "\n";

  return 0;
}
