#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  // rounds controls how many refinement passes run; ridge is the regression
  // regulariser. b is exactly 2*a, so the fill should recover 6 for row 2.
  const pluss::Series a = pluss::make_series({1.,2.,3.,4.,5.,6.}, {6}, f64);
  const pluss::Series b = pluss::make_series({2., 4., NaN, 8., 10., 12.}, {6}, f64);
  const DataFrame frame({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::options::IterativeImputer few{};
  few.rounds = 1;
  lymph::options::IterativeImputer many{};
  many.rounds = 20;

  std::cout << "rounds = 1:\n"  << lymph::prep::IterativeImputer{few}.learn_map(frame) << "\n";
  std::cout << "rounds = 20:\n" << lymph::prep::IterativeImputer{many}.learn_map(frame);

  return 0;
}
