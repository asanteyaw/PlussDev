#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// A branch can be routed by integer column positions instead of names -- the
// third argument of branch::compose. Positions index the transformer's ordered
// feature space.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"a", Slate(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"b", Slate(pluss::make_series({10., 20., 10., 20.}, {4}, f64))},
      {"c", Slate(pluss::make_series({5., 5., 6., 6.}, {4}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"a", "b", "c"}).feature_batch();
  };

  // Route by position: scale columns 0 and 2 (a and c), one-hot column 1 (b).
  lymph::prep::FeatureTransformer t = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::compose(
              {lymph::prep::stage::standard_scaler()}, {}, std::vector<std::int64_t>{0, 2}),
          lymph::prep::branch::compose(
              {lymph::prep::stage::one_hot_encoder()}, {}, std::vector<std::int64_t>{1}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const Series out = lymph::resolve(t.learn_map(batch()), "learn_map");
  std::cout << "scale(a,c) ++ one_hot(b), shape [" << out.sizes()[0] << ", "
            << out.sizes()[1] << "]:\n" << out << "\n";

  return 0;
}
