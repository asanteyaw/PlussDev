#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;

  // The source stores Float32 values. Nothing about the source changes.
  const auto f32 = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<float>{0.5F, 1.5F, 2.5F, 3.5F}, {4}, f32);

  // Without a target, each batch keeps the source dtype.
  loader::DataLoaderOptions keep{};
  keep.batch_size = 2;
  loader::DataLoader unchanged({x}, keep);
  std::cout << "source float32: " << (x.scalar_type() == pluss::ScalarType::Float32) << '\n';
  std::cout << "unchanged batches float32: "
            << (unchanged.batch_at(0).x().scalar_type() == pluss::ScalarType::Float32) << '\n';

  // With a target dtype, every delivered batch is converted on the way out.
  loader::DataLoaderOptions widen = keep;
  widen.target_dtype = pluss::ScalarType::Float64;
  loader::DataLoader converted({x}, widen);
  std::cout << "converted batches float64: "
            << (converted.batch_at(0).x().scalar_type() == pluss::ScalarType::Float64) << '\n';
  std::cout << "values survive conversion: "
            << (converted.batch_at(1).x().host<double>() == std::vector<double>{2.5, 3.5}) << '\n';
}
