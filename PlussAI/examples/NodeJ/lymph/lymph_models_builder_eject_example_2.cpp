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

  lymph::models::builder::Builder builder = lymph::models::builder::build({
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // eject returns the Model as a value you can keep working with -- here,
  // train it and put a fitted version back under the same name.
  lymph::models::Model tree = builder.eject("tree");
  tree.learn_(x, y);
  builder.inject("tree", tree);

  std::cout << "size            = " << builder.size() << "\n";
  std::cout << "re-injected trained = " << std::boolalpha
            << builder.get("tree").trained() << "\n";

  return 0;
}
