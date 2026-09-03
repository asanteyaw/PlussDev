#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Not every stage has a dedicated branch:: builder. branch::compose is the
// escape hatch: pass any stage factory result (here stage::basen_encoder, which
// has no branch builder) and it composes into the transformer just the same.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"code",  Thread(pluss::make_series({10., 20., 30., 40., 10., 30.}, {6}, f64))},
      {"spend", Thread(pluss::make_series({1., 2., 3., 4., 5., 6.}, {6}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"code", "spend"}).feature_batch();
  };

  lymph::prep::FeatureTransformer t = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::compose(
              {lymph::prep::stage::basen_encoder({.base = 2})}, {"code"}),
          lymph::prep::branch::minmax_scaler({}, {"spend"}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const Series out = lymph::resolve(t.learn_map(batch()), "learn_map");
  std::cout << "basen(code) ++ minmax(spend), shape [" << out.sizes()[0] << ", "
            << out.sizes()[1] << "]:\n" << out << "\n";

  return 0;
}
