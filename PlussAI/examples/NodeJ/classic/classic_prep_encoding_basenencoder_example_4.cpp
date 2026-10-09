#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Slate train(pluss::make_series({10.0, 20.0, 30.0}, {3}, f64));
  const Slate probe(pluss::make_series({10.0, 20.0, 30.0}, {3}, f64));

  cls::prep::BaseNEncoder via_slate;  // base = 2 by default
  via_slate.learn(train);
  const pluss::Series slate_result = via_slate.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::BaseNEncoder via_batch;
  via_batch.learn(train_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (slate_result - batch_result).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between Slate and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
