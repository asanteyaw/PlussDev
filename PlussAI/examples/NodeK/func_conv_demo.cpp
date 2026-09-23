#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

namespace {

pluss::Config f64() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

} // namespace

int main() {
  using namespace pluss;
  using namespace pluss::nn;

  const Config cfg = f64();

  const Series image = make_series(std::vector<double>{
                                       1.0, 2.0, 3.0, 4.0,
                                       5.0, 6.0, 7.0, 8.0,
                                       9.0, 10.0, 11.0, 12.0,
                                       13.0, 14.0, 15.0, 16.0},
                                   {1, 1, 4, 4},
                                   cfg);
  const Series edge_kernel = make_series(std::vector<double>{
                                             1.0, 0.0,
                                             0.0, -1.0},
                                         {1, 1, 2, 2},
                                         cfg);
  const Series edge_bias = make_series(std::vector<double>{0.25}, {1}, cfg);
  const Series image_features = conv(image, edge_kernel, edge_bias, 1, 0, 1, 1);

  const Series signal = make_series(std::vector<double>{0.0, 0.4, 0.9, 0.5, -0.2, -0.8, -0.3, 0.6, 1.0, 0.2},
                                    {1, 1, 10},
                                    cfg);
  const Series temporal_kernel1 = make_series(std::vector<double>{0.2, 0.6, 0.2}, {1, 1, 3}, cfg);
  const Series temporal_kernel2 = make_series(std::vector<double>{1.0, 0.0, -1.0}, {1, 1, 3}, cfg);
  const Series tcn_stage1 = conv1d(signal, temporal_kernel1, Series(), 1, 1, 1, 1).relu();
  const Series tcn_stage2 = conv1d(tcn_stage1, temporal_kernel2, Series(), 1, 2, 2, 1);

  std::cout << "func_conv_demo\n";
  std::cout << "surface: pluss::nn::conv / pluss::nn::conv1d via #include <pluss/pluss.h> + <pluss/ai.h>\n";
  std::cout << "classical 2d conv feature map: " << image_features << "\n";
  std::cout << "temporal signal: " << signal << "\n";
  std::cout << "TCN-style stage 1: " << tcn_stage1 << "\n";
  std::cout << "TCN-style dilated stage 2: " << tcn_stage2 << "\n";

  return 0;
}
