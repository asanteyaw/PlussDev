#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.learning_rate = -0.1;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
