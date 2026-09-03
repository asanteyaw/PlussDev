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
  const DataFrame train(
      {{"code", Thread(pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8., 9.}, {9}, f64))}});

  // A larger base uses fewer, wider digit columns. base = 4 codes 9 categories
  // (codes 1..9) in ceil(log4(10)) = 2 digit columns, each 0..3.
  lymph::prep::BaseNEncoder encoder(lymph::prep::BaseNEncoderOptions{.base = 4});
  std::cout << "base = 4:\n"
            << encoder.features({"code"}).learn_map(train) << "\n";

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary.state tensors = " << s.state.size() << "\n";

  return 0;
}
