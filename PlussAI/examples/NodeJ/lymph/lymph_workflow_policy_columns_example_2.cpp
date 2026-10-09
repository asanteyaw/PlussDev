#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// require(names): a raw-schema contract, checked before any drop is applied.
// A missing required column fails loudly, naming the column -- it never
// silently proceeds with a partial frame.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1}, {6}, f64))},
  });

  workflow::Policy ok{};
  ok.require({"income", "balance", "default"});
  std::cout << "require(present columns): admitted_rows=" << ok.apply(raw).ncases() << "\n";

  workflow::Policy missing{};
  missing.require({"income", "does_not_exist"});
  try {
    (void)missing.apply(raw);
    std::cout << "require(missing): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "require(missing) threw: " << e.what() << "\n";
  }

  return 0;
}
