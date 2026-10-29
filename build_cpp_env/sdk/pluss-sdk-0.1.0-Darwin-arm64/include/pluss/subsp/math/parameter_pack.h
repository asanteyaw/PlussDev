#pragma once

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optim {

namespace detail {

inline bool same_shape_(const Series &a, const Series &b) {
  if (!a.defined() || !b.defined())
    return false;
  if (a.sizes().size() != b.sizes().size())
    return false;
  for (std::size_t i = 0; i < a.sizes().size(); ++i) {
    if (a.sizes()[i] != b.sizes()[i])
      return false;
  }
  return true;
}

inline void require_defined_(const Series &x, const char *op) {
  if (!x.defined()) {
    ::pluss::throw_error(std::string("optim::") + op +
                         ": expected defined Series");
  }
}

inline void require_same_platform_(const Series &a, const Series &b,
                                   const char *op) {
  if (a.platform() != b.platform()) {
    ::pluss::throw_error(std::string("optim::") + op +
                         ": expected matching platforms");
  }
}

inline void require_float_dtype_(const Series &x, const char *op) {
  if (!x.dtype().is_floating() && !x.dtype().is_complex()) {
    ::pluss::throw_error(std::string("optim::") + op +
                         ": expected floating or complex dtype");
  }
}

} // namespace detail

struct ParameterBlockLayout {
  std::string name;
  std::vector<std::int64_t> sizes;
  std::int64_t numel{0};
  ScalarType scalar_type{ScalarType::Float32};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
};

class ParameterLayout {
public:
  ParameterLayout() = default;
  explicit ParameterLayout(std::vector<ParameterBlockLayout> blocks)
      : blocks_(std::move(blocks)) {}

  [[nodiscard]] const std::vector<ParameterBlockLayout> &
  blocks() const noexcept {
    return blocks_;
  }
  [[nodiscard]] std::size_t size() const noexcept { return blocks_.size(); }
  [[nodiscard]] bool empty() const noexcept { return blocks_.empty(); }

  [[nodiscard]] std::int64_t total_numel() const noexcept {
    std::int64_t total = 0;
    for (const auto &block : blocks_)
      total += block.numel;
    return total;
  }

  [[nodiscard]] bool contains(std::string_view name) const noexcept {
    return std::any_of(blocks_.begin(), blocks_.end(),
                       [&](const auto &block) { return block.name == name; });
  }

  [[nodiscard]] const ParameterBlockLayout &at(std::string_view name) const {
    for (const auto &block : blocks_) {
      if (block.name == name)
        return block;
    }
    ::pluss::throw_error(
        "optim::ParameterLayout::at: unknown parameter block");
  }

private:
  std::vector<ParameterBlockLayout> blocks_{};
};

template <class T> class ParameterPack {
public:
  struct Block {
    std::string name;
    T value;
  };

  ParameterPack() = default;
  explicit ParameterPack(std::vector<Block> blocks)
      : blocks_(std::move(blocks)) {
    validate_names_();
  }

  [[nodiscard]] std::size_t size() const noexcept { return blocks_.size(); }
  [[nodiscard]] bool empty() const noexcept { return blocks_.empty(); }

  [[nodiscard]] const Block &operator[](std::size_t index) const {
    return blocks_.at(index);
  }
  [[nodiscard]] Block &operator[](std::size_t index) {
    return blocks_.at(index);
  }

  [[nodiscard]] const std::vector<Block> &blocks() const noexcept {
    return blocks_;
  }
  [[nodiscard]] std::vector<Block> take_blocks() && noexcept {
    return std::move(blocks_);
  }

  [[nodiscard]] std::int64_t total_numel() const noexcept {
    std::int64_t total = 0;
    for (const auto &block : blocks_)
      total += block.value.numel();
    return total;
  }

  [[nodiscard]] bool contains(std::string_view name) const noexcept {
    return std::any_of(blocks_.begin(), blocks_.end(),
                       [&](const Block &block) { return block.name == name; });
  }

  [[nodiscard]] const T &at(std::string_view name) const {
    for (const auto &block : blocks_) {
      if (block.name == name)
        return block.value;
    }
    ::pluss::throw_error(
        "optim::ParameterPack::at: unknown parameter block");
  }

  [[nodiscard]] T &at(std::string_view name) {
    for (auto &block : blocks_) {
      if (block.name == name)
        return block.value;
    }
    ::pluss::throw_error(
        "optim::ParameterPack::at: unknown parameter block");
  }

  [[nodiscard]] std::vector<T> values() const {
    std::vector<T> out;
    out.reserve(blocks_.size());
    for (const auto &block : blocks_)
      out.push_back(block.value);
    return out;
  }

  template <class U>
  [[nodiscard]] ParameterPack<U> rebind(std::vector<U> values) const {
    if (values.size() != blocks_.size()) {
      ::pluss::throw_error(
          "optim::ParameterPack::rebind: size mismatch");
    }

    std::vector<typename ParameterPack<U>::Block> rebound;
    rebound.reserve(blocks_.size());
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      rebound.push_back(typename ParameterPack<U>::Block{blocks_[i].name,
                                                         std::move(values[i])});
    }
    return ParameterPack<U>(std::move(rebound));
  }

  [[nodiscard]] ParameterLayout layout() const
    requires std::is_same_v<T, Series>
  {
    std::vector<ParameterBlockLayout> out;
    out.reserve(blocks_.size());
    for (const auto &block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::layout");
      const auto sizes = block.value.sizes();
      out.push_back(ParameterBlockLayout{
          block.name, std::vector<std::int64_t>(sizes.begin(), sizes.end()),
          block.value.numel(), block.value.scalar_type(),
          block.value.platform()});
    }
    return ParameterLayout(std::move(out));
  }

  [[nodiscard]] bool same_layout(const ParameterPack<Series> &other) const
    requires std::is_same_v<T, Series>
  {
    if (blocks_.size() != other.blocks_.size())
      return false;
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      const auto &lhs = blocks_[i];
      const auto &rhs = other.blocks_[i];
      if (lhs.name != rhs.name ||
          lhs.value.scalar_type() != rhs.value.scalar_type() ||
          lhs.value.platform() != rhs.value.platform() ||
          !detail::same_shape_(lhs.value, rhs.value)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] Series flatten_values() const
    requires std::is_same_v<T, Series>
  {
    if (blocks_.empty()) {
      ::pluss::throw_error(
          "optim::ParameterPack::flatten_values: empty pack");
    }

    std::vector<Series> flat;
    flat.reserve(blocks_.size());
    const auto scalar_type = blocks_.front().value.scalar_type();
    for (const auto &block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::flatten_values");
      detail::require_same_platform_(blocks_.front().value, block.value,
                                     "ParameterPack::flatten_values");
      if (block.value.scalar_type() != scalar_type) {
        ::pluss::throw_error("optim::ParameterPack::flatten_"
                             "values: expected matching dtypes");
      }
      flat.push_back(block.value.reshape({block.value.numel()}));
    }

    if (flat.size() == 1)
      return flat.front();
    return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()),
                              0);
  }

  [[nodiscard]] ParameterPack<Series> zeros_like() const
    requires std::is_same_v<T, Series>
  {
    std::vector<typename ParameterPack<Series>::Block> out;
    out.reserve(blocks_.size());
    for (const auto &block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::zeros_like");
      out.push_back({block.name, pluss::series::zeros_like(
                                     block.value, block.value.scalar_type())});
    }
    return ParameterPack<Series>(std::move(out));
  }

private:
  void validate_names_() const {
    std::unordered_set<std::string> seen;
    seen.reserve(blocks_.size());
    for (const auto &block : blocks_) {
      if (block.name.empty()) {
        ::pluss::throw_error(
            "optim::ParameterPack: block names must be non-empty");
      }
      if (!seen.insert(block.name).second) {
        ::pluss::throw_error(
            "optim::ParameterPack: duplicate block name");
      }
    }
  }

  std::vector<Block> blocks_{};
};

[[nodiscard]] inline ParameterPack<Series>
unflatten_values(const ParameterLayout &layout, const Series &flat_values) {
  detail::require_defined_(flat_values, "unflatten_values");

  Series flat = flat_values.reshape({flat_values.numel()});
  if (flat.numel() != layout.total_numel()) {
    ::pluss::throw_error(
        "optim::unflatten_values: flat size mismatch");
  }

  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(layout.size());

  std::int64_t offset = 0;
  for (const auto &block : layout.blocks()) {
    Series piece = flat.slice(0, offset, offset + block.numel, 1);
    if (!piece.is_contiguous())
      piece = piece.contiguous();
    piece =
        block.sizes.empty() ? piece.reshape({}) : piece.reshape(block.sizes);
    if (piece.scalar_type() != block.scalar_type)
      piece = piece.to(block.scalar_type);
    if (piece.platform() != block.platform)
      piece = piece.to(block.platform);
    blocks.push_back({block.name, std::move(piece)});
    offset += block.numel;
  }

  return ParameterPack<Series>(std::move(blocks));
}

[[nodiscard]] inline Series
flatten_values(const ParameterPack<Series> &params) {
  return params.flatten_values();
}

} // namespace pluss::subsp::math::optim
