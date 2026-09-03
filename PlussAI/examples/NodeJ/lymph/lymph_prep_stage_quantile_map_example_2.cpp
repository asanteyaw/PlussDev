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

  // output = Uniform spreads values evenly on [0, 1]; output = Gaussian maps
  // them onto a standard normal shape. Standalone over a skewed column.
  const pluss::Series v = pluss::make_series({1., 2., 4., 8., 16., 32., 64., 128.}, {8}, f64);
  const DataFrame frame({{"v", Thread(v)}});

  lymph::options::QuantileMap uni{};                                   // default: Uniform
  lymph::options::QuantileMap gauss{};
  gauss.output = lymph::prep::QuantileMapOutput::Gaussian;

  std::cout << "Uniform:\n"  << lymph::prep::QuantileMap{uni}.learn_map(frame) << "\n";
  std::cout << "Gaussian:\n" << lymph::prep::QuantileMap{gauss}.learn_map(frame);

  return 0;
}
