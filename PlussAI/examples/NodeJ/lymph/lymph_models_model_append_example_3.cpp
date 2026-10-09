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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});

  // prepare().scale(names) is exactly append(standard_scaler, route::names).
  // Build the same one-stage model both ways and confirm they agree.
  lymph::models::Model via_prepare = lymph::models::logistic_regression();
  via_prepare.prepare().scale({"amount", "ratio"});
  via_prepare.learn_(x, y);

  lymph::models::Model via_append = lymph::models::logistic_regression();
  via_append.append(lymph::prep::stage::standard_scaler(),
                    lymph::prep::route::names({"amount", "ratio"}));
  via_append.learn_(x, y);

  const DataFrame compare({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"pred_via_prepare", Slate(via_prepare.infer(x))},
      {"pred_via_append", Slate(via_append.infer(x))},
  });
  std::cout << "stage_count: prepare=" << via_prepare.stage_count()
            << ", append=" << via_append.stage_count() << "\n\n";
  std::cout << compare;

  return 0;
}
