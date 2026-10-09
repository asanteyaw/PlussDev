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

  // A custom sqrt compressor combined with a built-in stage. Route the scaler at
  // the column; the custom stage runs first over the whole (one-column) frame.
  const pluss::Series amount = pluss::make_series({1., 4., 16., 64., 256., 1024.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.add(lymph::prep::stage::custom("sqrt", [](const pluss::Series& s) { return s.sqrt(); }))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"amount", Slate(amount)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
