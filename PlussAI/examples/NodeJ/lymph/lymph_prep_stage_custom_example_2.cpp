#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // The callable can close over configuration -- here the clip bounds -- so one
  // factory call produces a fully configured stage.
  const pluss::Series v = pluss::make_series({-100., -3., 0., 4., 50., 900.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"v", Slate(v)}});

  const double lo = -5.0, hi = 10.0;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.add(lymph::prep::stage::custom("clip", [lo, hi](const pluss::Series& s) {
    return s.clamp_min(lo).clamp_max(hi);
  }));
  model.learn_(x, y);
  std::cout << "trained = " << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"v", Slate(v)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
