#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({-1.0, 1.0, 2.0, 4.0}, {4}, f64).reshape({4, 1});

  cls::prep::PowerMapOptions box_cox_opts{};
  box_cox_opts.mode = cls::prep::PowerMapMode::BoxCox;
  cls::prep::PowerMap box_cox(box_cox_opts);
  const cls::Status box_cox_fit = box_cox.learn(train);
  std::cout << "BoxCox ok = " << std::boolalpha << box_cox_fit.ok() << "\n";
  std::cout << "message = \"" << box_cox_fit.message << "\"\n";

  cls::prep::PowerMap yeo_johnson;  // YeoJohnson is the default mode
  const cls::Status yeo_fit = yeo_johnson.learn(train);
  std::cout << "YeoJohnson ok = " << yeo_fit.ok() << "\n";

  return 0;
}
