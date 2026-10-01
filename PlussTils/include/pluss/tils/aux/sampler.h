#pragma once

// Index-sampling strategies for DataLoader, mirroring torch.utils.data's
// Sampler hierarchy (Sampler / SequentialSampler / RandomSampler /
// SubsetRandomSampler / WeightedRandomSampler). Unlike torch's lazy
// per-element __iter__ protocol, `sample()` returns the full epoch order up
// front as a single std::vector<int64_t> -- matching how DataLoader already
// builds its own `order_` vector, and avoiding a coroutine/generator design
// this codebase has no existing idiom for.

#include <CodeM/core/Errors.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace pluss::tils {

class Sampler {
public:
  virtual ~Sampler() = default;

  // Returns the full index order for one epoch over a dataset of `size`
  // samples. `epoch_seed` is the caller's seed already offset for the
  // current epoch, so the same Sampler instance reused across epochs still
  // produces a different (but reproducible) order each time.
  virtual std::vector<std::int64_t> sample(std::int64_t size, std::uint64_t epoch_seed) const = 0;
};

class SequentialSampler final : public Sampler {
public:
  std::vector<std::int64_t> sample(std::int64_t size, std::uint64_t /*epoch_seed*/) const override {
    PLUSS_CHECK(size >= 0, "pluss::tils::SequentialSampler: size must be >= 0");
    std::vector<std::int64_t> order(static_cast<std::size_t>(size));
    std::iota(order.begin(), order.end(), std::int64_t{0});
    return order;
  }
};

class RandomSampler final : public Sampler {
public:
  // `num_samples < 0` (the default) means "exactly one pass over the
  // dataset" -- `size` samples, without replacement. With `replacement`,
  // `num_samples` may exceed `size`; without it, it may not.
  explicit RandomSampler(bool replacement = false, std::int64_t num_samples = -1)
      : replacement_(replacement), num_samples_(num_samples) {}

  std::vector<std::int64_t> sample(std::int64_t size, std::uint64_t epoch_seed) const override {
    PLUSS_CHECK(size > 0, "pluss::tils::RandomSampler: size must be > 0");
    const std::int64_t n = num_samples_ >= 0 ? num_samples_ : size;
    std::mt19937_64 rng(epoch_seed);
    std::vector<std::int64_t> out(static_cast<std::size_t>(n));
    if (replacement_) {
      std::uniform_int_distribution<std::int64_t> dist(0, size - 1);
      for (auto& v : out) v = dist(rng);
    } else {
      PLUSS_CHECK(
          n <= size,
          "pluss::tils::RandomSampler: num_samples must be <= size when replacement=false");
      std::vector<std::int64_t> pool(static_cast<std::size_t>(size));
      std::iota(pool.begin(), pool.end(), std::int64_t{0});
      std::shuffle(pool.begin(), pool.end(), rng);
      std::copy_n(pool.begin(), static_cast<std::size_t>(n), out.begin());
    }
    return out;
  }

private:
  bool replacement_;
  std::int64_t num_samples_;
};

class SubsetRandomSampler final : public Sampler {
public:
  explicit SubsetRandomSampler(std::vector<std::int64_t> indices) : indices_(std::move(indices)) {}

  std::vector<std::int64_t> sample(std::int64_t /*size*/, std::uint64_t epoch_seed) const override {
    std::vector<std::int64_t> out = indices_;
    std::mt19937_64 rng(epoch_seed);
    std::shuffle(out.begin(), out.end(), rng);
    return out;
  }

private:
  std::vector<std::int64_t> indices_;
};

class WeightedRandomSampler final : public Sampler {
public:
  explicit WeightedRandomSampler(std::vector<double> weights, std::int64_t num_samples, bool replacement = true)
      : weights_(std::move(weights)), num_samples_(num_samples), replacement_(replacement) {
    PLUSS_CHECK(!weights_.empty(), "pluss::tils::WeightedRandomSampler: weights must not be empty");
    PLUSS_CHECK(num_samples_ > 0, "pluss::tils::WeightedRandomSampler: num_samples must be > 0");
  }

  std::vector<std::int64_t> sample(std::int64_t size, std::uint64_t epoch_seed) const override {
    PLUSS_CHECK(
        static_cast<std::int64_t>(weights_.size()) == size,
        "pluss::tils::WeightedRandomSampler: weights size must match dataset size");
    std::mt19937_64 rng(epoch_seed);
    std::vector<std::int64_t> out(static_cast<std::size_t>(num_samples_));
    if (replacement_) {
      std::discrete_distribution<std::size_t> dist(weights_.begin(), weights_.end());
      for (auto& v : out) v = static_cast<std::int64_t>(dist(rng));
      return out;
    }

    PLUSS_CHECK(
        num_samples_ <= size,
        "pluss::tils::WeightedRandomSampler: num_samples must be <= size when replacement=false");
    std::vector<double> remaining = weights_;
    std::vector<std::int64_t> pool(static_cast<std::size_t>(size));
    std::iota(pool.begin(), pool.end(), std::int64_t{0});
    for (std::int64_t i = 0; i < num_samples_; ++i) {
      std::discrete_distribution<std::size_t> dist(remaining.begin(), remaining.end());
      const std::size_t pick = dist(rng);
      out[static_cast<std::size_t>(i)] = pool[pick];
      remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(pick));
      pool.erase(pool.begin() + static_cast<std::ptrdiff_t>(pick));
    }
    return out;
  }

private:
  std::vector<double> weights_;
  std::int64_t num_samples_;
  bool replacement_;
};

} // namespace pluss::tils

namespace pluss::loader {
using tils::Sampler;
using tils::SequentialSampler;
using tils::RandomSampler;
using tils::SubsetRandomSampler;
using tils::WeightedRandomSampler;
} // namespace pluss::loader
