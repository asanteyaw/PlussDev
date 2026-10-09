#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  // vector_normalizer scales each ROW of the routed columns to unit norm (L2 by
  // default), so only the direction of the feature vector matters -- its length
  // is discarded. Rows that point the same way become identical.
  const pluss::Series a = pluss::make_series({3., 1., 0., 6., 8., 2.}, {6}, f64);
  const pluss::Series b = pluss::make_series({4., 3., 5., 8., 1., 7.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 0, 1}, {6}, f64);
  const DataFrame x({{"a", Slate(a)}, {"b", Slate(b)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::vector_normalizer(), lymph::prep::route::names({"a", "b"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"a", Slate(a)}, {"b", Slate(b)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
