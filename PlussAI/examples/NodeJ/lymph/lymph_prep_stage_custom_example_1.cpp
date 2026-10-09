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

  // prep::stage::custom wraps any Series -> Series callable as a stateless stage.
  // It is attached with Model::add (no route -- it maps the whole feature frame).
  // Here: a log1p compressor for a positive, skewed column.
  const pluss::Series amount = pluss::make_series({0., 1., 9., 99., 999., 9999.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.add(lymph::prep::stage::custom("log1p", [](const pluss::Series& s) { return s.log1p(); }));
  model.learn_(x, y);
  std::cout << "trained = " << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"amount", Slate(amount)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
