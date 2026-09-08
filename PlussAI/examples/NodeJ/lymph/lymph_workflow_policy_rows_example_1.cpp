#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

// reject(names): the preferred, narrow row-admission surface -- a row with a
// null in any named column is dropped. Only the columns you name are
// checked, so feature columns meant for an imputer are left alone.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const double nan_val = std::numeric_limits<double>::quiet_NaN();
  const DataFrame raw({
      {"label", Thread(pluss::make_series({0.0, nan_val, 1.0, 1.0, nan_val, 0.0}, {6}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, nan_val, 2.2, 3.8, 4.2}, {6}, f64))},
  });

  // Reject rows missing the label only; the NaN in income survives (an
  // imputer stage would own it).
  workflow::Policy policy{};
  policy.reject({"label"});
  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw_rows      = " << raw.ncases() << "\n";
  std::cout << "admitted_rows = " << admitted.ncases() << " (2 null-label rows dropped)\n";

  return 0;
}
