#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// drop(names): strip columns before modeling -- IDs, leakage columns,
// audit-only fields. drop never removes rows, only columns.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}, {6}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw_features      = " << raw.nfeatures()
            << "  raw_rows      = " << raw.ncases() << "\n";
  std::cout << "admitted_features = " << admitted.nfeatures()
            << "  admitted_rows = " << admitted.ncases() << "\n";

  return 0;
}
