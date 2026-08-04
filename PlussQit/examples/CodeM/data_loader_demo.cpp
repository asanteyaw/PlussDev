#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const auto x = pluss::series::randn({64, 3}, cfg, 7ULL);
  const auto y = x.select(1, 0).reshape({64, 1}).gt(0.0).to(pluss::ScalarType::Float32);

  pluss::DataLoader loader(
      {x, y},
      pluss::loader::DataLoaderOptions{
          .batch_size = 16,
          .shuffle = true,
          .drop_last = false,
          .seed = 2026ULL,
      });

  std::size_t batches = 0;
  double total = 0.0;
  for (const auto batch : loader) {
    const auto loss_like = batch.x().mean().abs() + batch.y().mean();
    total += pluss::as_f64(loss_like);
    ++batches;
  }

  std::cout << "data_loader_demo\n";
  std::cout << "samples=" << loader.sample_count() << " batches=" << batches << "\n";
  std::cout << "aggregate=" << total << "\n";
  return 0;
}
