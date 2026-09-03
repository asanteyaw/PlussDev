#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Thread(pluss::make_series({0., 1., 2., 0., 1., 2.}, {6}, f64))}});

  lymph::prep::OneHotEncoder encoder;
  encoder.features({"region"}).learn(train);

  // summary() reports the generic TransformerSummary: the pinned feature_names
  // plus a state vector of raw fitted tensors.
  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary.feature_names[0] = " << s.feature_names[0] << "\n";
  std::cout << "summary.state tensors    = " << s.state.size() << "\n\n";

  // The public attributes carry the human-readable layout: categories_ is the
  // learned category set per feature, mapped_names_ the output column labels
  // (one per category, plus the reserved unknown bucket).
  std::cout << "categories_[0] :";
  for (const auto& c : encoder.categories_[0]) std::cout << " " << c;
  std::cout << "\nmapped_names_  :";
  for (const auto& n : encoder.mapped_names_) std::cout << " " << n;
  std::cout << "\n";

  return 0;
}
