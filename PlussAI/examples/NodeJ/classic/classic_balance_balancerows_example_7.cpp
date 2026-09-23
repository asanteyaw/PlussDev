#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch undefined_batch{};
  std::cout << "undefined batch ok = " << std::boolalpha << cls::balance_rows(undefined_batch, cls::BalanceTarget{}).ok() << "\n";

  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64).reshape({3, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64);
  cls::LabeledBatch three_classes{};
  three_classes.features.x = x;
  three_classes.y = y;
  cls::BalanceTarget ratio_target{};
  ratio_target.kind = cls::BalanceTargetKind::Ratio;
  const cls::StatusOr<cls::LabeledBatch> rejected = cls::balance_rows(three_classes, ratio_target);
  std::cout << "Ratio with 3 classes ok = " << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
