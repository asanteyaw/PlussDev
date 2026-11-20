#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Shelf;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;

  const Shelf probe(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64));

  cls::prep::HashingEncoder via_shelf(options);
  via_shelf.learn(probe);
  const pluss::Series shelf_result = via_shelf.map(probe).value();

  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});

  cls::prep::HashingEncoder via_batch(options);
  via_batch.learn(probe_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (shelf_result - batch_result).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between Shelf and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
