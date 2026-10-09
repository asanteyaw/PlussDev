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

  // threshold is the cut; low_value / high_value are what a below / above value
  // becomes. Default is 0 / 1; here a second binarizer emits -1 / +1.
  const pluss::Series v = pluss::make_series({-2., -1., 0., 1., 2., 3.}, {6}, f64);
  const DataFrame frame({{"v", Slate(v)}});

  lymph::options::ThresholdBinarizer zero_one{};                       // threshold 0, 0/1
  lymph::options::ThresholdBinarizer signed_out{};
  signed_out.low_value = -1.0;
  signed_out.high_value = 1.0;

  std::cout << "0 / 1 about 0:\n"  << lymph::prep::ThresholdBinarizer{zero_one}.learn_map(frame) << "\n";
  std::cout << "-1 / +1 about 0:\n" << lymph::prep::ThresholdBinarizer{signed_out}.learn_map(frame);

  return 0;
}
