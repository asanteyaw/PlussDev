#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1000.0}, {1}, f64).reshape({1, 1});  // far beyond the fitted range

  cls::prep::QuantileMapOptions tight_clip{};
  tight_clip.clip = 1e-7;  // the default
  cls::prep::QuantileMap default_map(tight_clip);
  default_map.learn(train);
  std::cout << "clip=1e-7: " << default_map.map(probe).value() << "\n";

  cls::prep::QuantileMapOptions loose_clip{};
  loose_clip.clip = 0.1;
  cls::prep::QuantileMap wide_map(loose_clip);
  wide_map.learn(train);
  std::cout << "clip=0.1:  " << wide_map.map(probe).value() << "\n";

  return 0;
}
