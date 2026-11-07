#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

namespace {

// A map-style dataset that computes each sample on demand instead of storing a table.
// Sample i has feature i and label i modulo 2.
class SquaresDataset final : public pluss::loader::Dataset {
public:
  explicit SquaresDataset(std::int64_t count) : count_(count) {}

  [[nodiscard]] std::int64_t size() const override { return count_; }

  [[nodiscard]] pluss::loader::DataBatch get(std::int64_t index) const override {
    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
    pluss::loader::DataBatch sample{};
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index * index)}, {1}, cfg));
    sample.fields.push_back(pluss::make_series(std::vector<double>{static_cast<double>(index % 2)}, {1}, cfg));
    return sample;
  }

private:
  std::int64_t count_;
};

} // namespace

int main() {
  namespace loader = pluss::loader;
  const SquaresDataset dataset(5);

  // Without a collate function, the loader stacks samples into batches.
  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader plain(dataset, options);
  std::cout << "samples: " << plain.sample_count() << '\n';
  std::cout << "batches: " << plain.batch_count() << '\n';
  std::cout << "last batch squares: " << plain.batch_at(2).x().sum().item<double>() << '\n';

  // A collate function receives each assembled batch and may transform it. Here it
  // keeps only the labels, so every delivered batch has one field.
  loader::DataLoaderOptions collated = options;
  collated.collate_fn = [](loader::DataBatch batch) {
    loader::DataBatch out{};
    out.fields.push_back(batch.y());
    return out;
  };
  loader::DataLoader labels(dataset, collated);
  std::cout << "collated fields per batch: " << labels.batch_at(0).size() << '\n';
  std::cout << "label sum in batch 0: " << labels.batch_at(0).x().sum().item<double>() << '\n';
}
