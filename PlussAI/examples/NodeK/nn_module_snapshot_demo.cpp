#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  pluss::nn::Fusion layer(4, 2, true, cfg, 73ULL);
  const Series x = pluss::series::rand({3, 4}, cfg, 79ULL);

  const Series y_before = layer->forward(x);
  const auto snap = layer->snapshot();

  layer->weight() = pluss::series::zeros(layer->weight().sizes(), cfg);
  layer->bias() = pluss::series::zeros(layer->bias().sizes(), cfg);
  const Series y_mutated = layer->forward(x);

  const auto report = layer->restore(snap, true);
  const Series y_after = layer->forward(x);

  std::cout << "before_mean=" << y_before.mean().item<double>() << "\n";
  std::cout << "mutated_mean=" << y_mutated.mean().item<double>() << "\n";
  std::cout << "after_mean=" << y_after.mean().item<double>() << "\n";
  std::cout << "restore_missing=" << report.missing.size()
            << " restore_unexpected=" << report.unexpected.size() << "\n";
  return 0;
}
