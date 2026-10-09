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
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.4, 3.8, 4.2, 4.6}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // The non-const get returns a mutable reference, so you can train an entry
  // in place without ejecting and re-injecting it.
  builder.get("logreg").learn_(x, y);
  builder.get("tree").learn_(x, y);

  std::cout << "logreg trained in place = " << std::boolalpha
            << builder.get("logreg").trained() << "\n";
  std::cout << "tree trained in place   = " << builder.get("tree").trained() << "\n";

  return 0;
}
