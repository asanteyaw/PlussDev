#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

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

  // drop() strips columns before modeling; require() enforces a raw-frame
  // schema contract up front, before any drop is applied.
  workflow::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance", "default"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "== policy_apply ==\n";
  std::cout << "raw_rows=" << raw.ncases() << " admitted_rows=" << admitted.ncases() << "\n";
  std::cout << "raw_features=" << raw.nfeatures()
            << " admitted_features=" << admitted.nfeatures() << "\n";

  // require() throws before drop() ever runs -- a missing required column is
  // a hard schema-contract failure, not something drop() can paper over.
  workflow::Policy strict{};
  strict.require({"income", "balance", "does_not_exist"});
  try {
    (void)strict.apply(raw);
    std::cout << "strict.apply: no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "strict.apply threw: " << e.what() << "\n";
  }

  return 0;
}
