#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Shelf;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Shelf train(pluss::make_series({10.0, 20.0, 30.0}, {3}, f64));
  const Shelf probe(pluss::make_series({10.0, 20.0, 30.0}, {3}, f64));

  cls::prep::BaseNEncoder via_shelf;  // base = 2 by default
  via_shelf.learn(train);
  const pluss::Series shelf_result = via_shelf.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::BaseNEncoder via_batch;
  via_batch.learn(train_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (shelf_result - batch_result).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between Shelf and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
