#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor();
  std::cout << "default OnlineLinearOptions ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "trained() on a fresh regressor = " << made.value().trained() << "\n";

  return 0;
}
