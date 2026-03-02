#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

pluss::Config f64() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

} // namespace

int main() {
  using namespace pluss;

  const Config cfg = f64();

  nn::Conv image_conv(nn::ConvOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = {3, 3},
      .stride = {1, 1},
      .padding = {1, 1},
      .dilation = {1, 1},
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 11ULL,
  });
  const Series image = make_series(std::vector<double>{
                                       1.0, 2.0, 3.0, 4.0,
                                       2.0, 3.0, 4.0, 5.0,
                                       3.0, 4.0, 5.0, 6.0,
                                       4.0, 5.0, 6.0, 7.0},
                                   {1, 1, 4, 4},
                                   cfg);
  const Series image_map = image_conv->forward(image);

  nn::Conv1d tcn_layer1(nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 4,
      .kernel_size = 3,
      .stride = 1,
      .padding = 1,
      .dilation = 1,
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 19ULL,
  });
  nn::Conv1d tcn_layer2(nn::Conv1dOptions{
      .in_channels = 4,
      .out_channels = 2,
      .kernel_size = 3,
      .stride = 1,
      .padding = 2,
      .dilation = 2,
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 23ULL,
  });
  const Series signal = make_series(std::vector<double>{0.0, 0.3, 0.9, 0.6, -0.1, -0.7, -0.4, 0.2, 0.8, 1.0, 0.5, -0.2},
                                    {2, 1, 6},
                                    cfg);
  const Series tcn_features = tcn_layer1->forward(signal).relu();
  const Series tcn_output = tcn_layer2->forward(tcn_features);

  std::cout << "nn_conv_demo\n";
  std::cout << "surface: pluss::nn::Conv / pluss::nn::Conv1d via #include <pluss/pluss.h>\n";
  std::cout << "2d conv module output: " << image_map << "\n";
  std::cout << "TCN layer 1 features: " << tcn_features << "\n";
  std::cout << "TCN layer 2 output: " << tcn_output << "\n";

  return 0;
}
