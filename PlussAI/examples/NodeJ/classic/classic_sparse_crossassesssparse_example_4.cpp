#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // Class 2.0 has exactly one row (row 8) -- a plain (non-stratified)
  // holdout can easily place it entirely in the test partition.
  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      9, 1, {0, 1, 2, 3, 4, 5, 6, 7, 8}, std::vector<std::int64_t>(9, 0),
      {1.0, 1.1, 1.2, 2.0, 2.1, 2.2, 3.0, 3.1, 3.2}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0},
      {9}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions unstratified_holdout{};
  unstratified_holdout.plan = cls::CVPlanKind::Holdout;
  unstratified_holdout.reliability_defaults = false;  // keep it unstratified
  unstratified_holdout.holdout_fraction = 0.2;
  unstratified_holdout.shuffle = false;  // deterministic: last ~20% of rows become the test split

  const cls::StatusOr<cls::SparseCrossReport> report =
      cls::cross_assess_sparse_multiclass(x, y, unstratified_holdout);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
