// A custom Dataset computes each sample on demand, rather than storing a table. The loader
// materializes it once, by calling get() for every index, when no faster path is overridden.
#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>

namespace {
class SquaresDataset final : public pluss::loader::Dataset {
public:
  explicit SquaresDataset(std::int64_t count) : count_(count) {}
  [[nodiscard]] std::int64_t size() const override { return count_; }
  [[nodiscard]] pluss::loader::DataBatch get(std::int64_t index) const override {
    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
    pluss::loader::DataBatch sample{};
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index * index)}, {1}, cfg));
    return sample;
  }

private:
  std::int64_t count_;
};
} // namespace

int main() {
  const SquaresDataset dataset(4);
  std::cout << "size: " << dataset.size() << '\n';
  std::cout << "sample 3: " << dataset.get(3).x().item<double>() << '\n';
  return 0;
}
