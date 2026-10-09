#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;

  const Slate probe(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64));

  cls::prep::HashingEncoder via_slate(options);
  via_slate.learn(probe);
  const pluss::Series slate_result = via_slate.map(probe).value();

  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});

  cls::prep::HashingEncoder via_batch(options);
  via_batch.learn(probe_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (slate_result - batch_result).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between Slate and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
