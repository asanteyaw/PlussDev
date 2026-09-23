#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      3, 1, {0, 1, 2}, {0, 0, 0}, {1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 1.0, 5.0}, {3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  cls::SparseLinearBinaryClassifier model;
  model.learn(x, y);

  cls::InferOptions bad_threshold{};
  bad_threshold.positive_threshold = 1.0;
  const cls::StatusOr<pluss::Series> rejected = model.infer(x, bad_threshold);
  std::cout << "positive_threshold=1.0 ok = " << std::boolalpha << rejected.ok() << "\n";

  // assess's own convention: y=1.0 counts as positive (y > 0.0) even
  // though learn() folded it into the negative class above.
  cls::AssessOptions recall_only{};
  recall_only.metrics = {cls::MetricKind::Recall};
  std::cout << "assess Recall (y=1.0 treated as positive) = " << model.assess(x, y, recall_only).value() << "\n";

  return 0;
}
