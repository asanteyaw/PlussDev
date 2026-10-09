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

  // norm selects L1 (values sum to 1 in magnitude), L2 (Euclidean unit length),
  // or Linf (largest magnitude becomes 1). Standalone over a two-column frame.
  const pluss::Series a = pluss::make_series({3., 1., 4., 0.}, {4}, f64);
  const pluss::Series b = pluss::make_series({4., 1., 0., 5.}, {4}, f64);
  const DataFrame frame({{"a", Slate(a)}, {"b", Slate(b)}});

  for (auto [label, n] : {std::pair{"L1", lymph::prep::VectorNorm::L1},
                          std::pair{"L2", lymph::prep::VectorNorm::L2},
                          std::pair{"Linf", lymph::prep::VectorNorm::Linf}}) {
    lymph::options::VectorNormalizer o{};
    o.norm = n;
    std::cout << label << ":\n" << lymph::prep::VectorNormalizer{o}.learn_map(frame) << "\n";
  }

  return 0;
}
