#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <limits>

// apply(frame): the one method that runs the whole contract, in a fixed
// order -- validate presence (require/drop/reject/allow targets must exist),
// check forbid, then apply drop (columns), reject_missing (rows), then the
// legacy filter_nulls sweep. This example combines all three visible effects.
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
      {"customer_id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6}, {6}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1}, {6}, f64))},
      {"default", Thread(pluss::make_series({0.0, nan_val, 0.0, 1.0, 1.0, nan_val}, {6}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});                 // column rule
  policy.require({"income", "balance", "default"});  // schema contract
  policy.reject({"default"});                   // row rule

  const DataFrame admitted = policy.apply(raw);
  std::cout << "raw:      " << raw.nfeatures() << " features x " << raw.ncases() << " rows\n";
  std::cout << "admitted: " << admitted.nfeatures() << " features x " << admitted.ncases()
            << " rows  (customer_id dropped; 2 null-default rows dropped)\n";

  return 0;
}
