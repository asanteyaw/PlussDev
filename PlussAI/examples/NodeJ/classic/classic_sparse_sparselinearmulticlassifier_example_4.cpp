#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      6, 3, {0, 1, 2, 3, 4, 5}, {0, 0, 1, 1, 2, 2}, {1.0, 2.0, 1.0, 2.0, 1.0, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SparseLinearMultiClassifier original;
  original.learn(x, y);
  const std::vector<pluss::Series> state = original.dump_state().value();
  std::cout << "dump_state() tensor count = " << state.size() << "\n";

  cls::SparseLinearMultiClassifier restored;
  const cls::Status loaded = restored.load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";

  const double diff = (original.infer_scores(x).value() - restored.infer_scores(x).value())
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff after restore = " << diff << "\n";

  return 0;
}
