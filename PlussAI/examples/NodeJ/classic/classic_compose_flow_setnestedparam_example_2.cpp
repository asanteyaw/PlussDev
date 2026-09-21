#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // transform-only
  flow.add_standard_scaler();
  flow.set_stage_key(0, "scale");

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  flow.learn_map(batch);
  std::cout << "trained() after learn_map        = " << std::boolalpha << flow.trained() << "\n";

  const cls::Status set = flow.set_nested_param("scale::opt.with_centering", 0.0);  // false
  std::cout << "set_nested_param ok               = " << set.ok() << "\n";
  std::cout << "trained() after set_nested_param  = " << flow.trained() << "\n";

  return 0;
}
