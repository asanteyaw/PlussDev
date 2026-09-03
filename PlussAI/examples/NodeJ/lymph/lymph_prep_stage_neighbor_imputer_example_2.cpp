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

  // neighbors sets how many similar rows are averaged; weighting chooses uniform
  // or distance-weighted averaging. Here column b tracks a with some scatter, so
  // the fill depends on which rows count as "near".
  const pluss::Series a = pluss::make_series({1., 2., 3., 4., 10., 11., 12., 13.}, {8}, f64);
  const pluss::Series b = pluss::make_series({2., 5., NaN, 9., 21., 24., 23., 27.}, {8}, f64);
  const DataFrame frame({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::options::NeighborImputer k2{};
  k2.neighbors = 2;
  lymph::options::NeighborImputer k4_dist{};
  k4_dist.neighbors = 4;
  k4_dist.weighting = lymph::prep::NeighborImputerWeighting::Distance;

  std::cout << "neighbors=2, uniform:\n" << lymph::prep::NeighborImputer{k2}.learn_map(frame) << "\n";
  std::cout << "neighbors=4, distance-weighted:\n" << lymph::prep::NeighborImputer{k4_dist}.learn_map(frame);

  return 0;
}
