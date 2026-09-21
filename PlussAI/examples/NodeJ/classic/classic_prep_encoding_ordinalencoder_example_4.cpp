#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Thread;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Thread train(pluss::make_series({30.0, 10.0, 40.0, 20.0}, {4}, f64));
  const Thread probe(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64));

  cls::prep::OrdinalEncoder via_thread;
  via_thread.learn(train);
  const pluss::Series thread_result = via_thread.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({30.0, 10.0, 40.0, 20.0}, {4}, f64).reshape({4, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});

  cls::prep::OrdinalEncoder via_batch;
  via_batch.learn(train_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (thread_result - batch_result).abs().amax(0, false).item<double>();
  std::cout << "max abs diff between Thread and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
