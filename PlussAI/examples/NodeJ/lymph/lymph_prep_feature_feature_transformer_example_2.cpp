#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The remainder is every routed column no branch claimed. FeatureRemainderPolicy
// decides its fate: Drop removes it, PassThrough appends it unchanged after the
// branch outputs. Note the two defaults differ -- the feature_transformer()
// factory defaults to PassThrough, the FeatureTransformer constructor to Drop.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Only "income" is routed to a branch; "tenure" is the remainder.
  const DataFrame raw({
      {"income", Slate(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"tenure", Slate(pluss::make_series({7., 8., 9., 10.}, {4}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"income", "tenure"}).feature_batch();
  };

  for (const auto policy : {lymph::prep::FeatureRemainderPolicy::Drop,
                            lymph::prep::FeatureRemainderPolicy::PassThrough}) {
    lymph::prep::FeatureTransformer t = lymph::prep::feature_transformer(
        raw, {lymph::prep::branch::standard_scaler({}, {"income"})}, policy);
    const Series out = lymph::resolve(t.learn_map(batch()), "learn_map");
    std::cout << (policy == lymph::prep::FeatureRemainderPolicy::Drop ? "Drop" : "PassThrough")
              << " -> shape [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n"
              << out << "\n\n";
  }

  return 0;
}
