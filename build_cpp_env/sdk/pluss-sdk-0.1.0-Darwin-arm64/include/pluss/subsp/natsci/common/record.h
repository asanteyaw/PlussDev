#pragma once

#include "pluss/subsp/math/parameter_pack.h"
#include "pluss/subsp/natsci/common/quantity.h"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common {

namespace solve::detail {
struct RecordAccess;
}

struct DimensionSignature final {
  std::array<int, 7> powers{};

  template <DimensionType D>
  [[nodiscard]] static constexpr DimensionSignature of() noexcept {
    return {{D::length, D::mass, D::time, D::current, D::temperature, D::amount,
             D::luminous_intensity}};
  }

  [[nodiscard]] friend constexpr bool
  operator==(const DimensionSignature &, const DimensionSignature &) = default;
};

struct RecordMeta final {
  std::string name;
  DimensionSignature dimension;
  std::string symbol;
  std::string source;

  [[nodiscard]] friend bool operator==(const RecordMeta &,
                                       const RecordMeta &) = default;
};

namespace detail {

struct StringHash final {
  using is_transparent = void;

  [[nodiscard]] std::size_t operator()(std::string_view value) const noexcept {
    return std::hash<std::string_view>{}(value);
  }
  [[nodiscard]] std::size_t
  operator()(const std::string &value) const noexcept {
    return (*this)(std::string_view(value));
  }
  [[nodiscard]] std::size_t operator()(const char *value) const noexcept {
    return (*this)(std::string_view(value));
  }
};

inline bool same_shape(const Series &lhs, const Series &rhs) noexcept {
  if (lhs.dim() != rhs.dim())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis) {
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  }
  return true;
}

inline void require_numeric(const Series &value, std::string_view where,
                            bool allow_complex = true) {
  if (!value.defined())
    throw std::invalid_argument(std::string(where) +
                                ": expected a defined Series");
  if (!value.dtype().is_floating() &&
      !(allow_complex && value.dtype().is_complex())) {
    throw std::invalid_argument(std::string(where) +
                                ": expected a floating Series");
  }
}

inline void require_same_layout(const Series &value, const Series &peer,
                                std::string_view where) {
  if (!same_shape(value, peer))
    throw std::invalid_argument(std::string(where) + ": shape mismatch");
  if (value.scalar_type() != peer.scalar_type())
    throw std::invalid_argument(std::string(where) + ": dtype mismatch");
  if (value.platform() != peer.platform())
    throw std::invalid_argument(std::string(where) + ": platform mismatch");
}

class RecordStore final {
public:
  using Pack = pluss::subsp::math::optim::ParameterPack<Series>;

  RecordStore(std::vector<RecordMeta> meta, std::vector<Pack::Block> blocks)
      : pack_(std::move(blocks)) {
    if (meta.empty())
      throw std::invalid_argument(
          "natsci::common record: expected at least one field");
    if (meta.size() != pack_.size())
      throw std::invalid_argument(
          "natsci::common record: metadata size mismatch");

    auto schema = std::make_shared<RecordSchema>();
    schema->meta = std::move(meta);
    schema->layout = pack_.layout();
    schema->index.reserve(schema->meta.size());
    const auto &first = pack_[0].value;
    require_numeric(first, "natsci::common record");
    for (std::size_t i = 0; i < schema->meta.size(); ++i) {
      const auto &block = pack_[i];
      if (schema->meta[i].name != block.name)
        throw std::invalid_argument(
            "natsci::common record: field order mismatch");
      require_numeric(block.value, "natsci::common record");
      if (block.value.scalar_type() != first.scalar_type())
        throw std::invalid_argument(
            "natsci::common record: fields must share a dtype");
      if (block.value.platform() != first.platform())
        throw std::invalid_argument(
            "natsci::common record: fields must share a platform");
      if (!schema->index.emplace(schema->meta[i].name, i).second)
        throw std::invalid_argument(
            "natsci::common record: duplicate field name");
    }
    schema_ = std::move(schema);
  }

  [[nodiscard]] std::size_t size() const noexcept {
    return schema_->meta.size();
  }
  [[nodiscard]] bool contains(std::string_view name) const noexcept {
    return schema_->index.contains(name);
  }
  [[nodiscard]] const std::vector<RecordMeta> &meta() const noexcept {
    return schema_->meta;
  }
  [[nodiscard]] const pluss::subsp::math::optim::ParameterLayout &
  layout() const noexcept {
    return schema_->layout;
  }
  [[nodiscard]] Series flat() const { return pack_.flatten_values(); }

  [[nodiscard]] const Series &at(std::string_view name) const {
    return pack_[position(name)].value;
  }
  [[nodiscard]] const RecordMeta &info(std::string_view name) const {
    return schema_->meta[position(name)];
  }

  template <DimensionType D>
  [[nodiscard]] SeriesQuantity<D> get(std::string_view name) const {
    const auto pos = position(name);
    if (schema_->meta[pos].dimension != DimensionSignature::of<D>())
      throw std::invalid_argument(
          "natsci::common record: dimension mismatch for field '" +
          std::string(name) + "'");
    return SeriesQuantity<D>::from_si(pack_[pos].value);
  }

  [[nodiscard]] RecordStore repack(const Series &flat) const {
    auto rebound =
        pluss::subsp::math::optim::unflatten_values(schema_->layout, flat);
    return RecordStore(schema_, std::move(rebound));
  }

  [[nodiscard]] RecordStore
  stack(const std::vector<RecordStore> &sequence) const {
    if (sequence.empty())
      throw std::invalid_argument(
          "natsci::common record stack: expected at least one state");

    std::vector<Pack::Block> blocks;
    blocks.reserve(size());
    for (std::size_t field = 0; field < size(); ++field) {
      const auto &model = pack_[field];
      std::vector<Series> tail;
      tail.reserve(sequence.size() - 1);
      for (std::size_t step = 0; step < sequence.size(); ++step) {
        const auto &record = sequence[step];
        if (record.size() != size() ||
            record.schema_->meta[field] != schema_->meta[field])
          throw std::invalid_argument(
              "natsci::common record stack: state schema changed");
        const auto &value = record.pack_[field].value;
        require_same_layout(value, model.value, "natsci::common record stack");
        if (step != 0)
          tail.push_back(value.unsqueeze(0));
      }
      Series values = sequence.front().pack_[field].value.unsqueeze(0);
      if (!tail.empty())
        values = values.unite(tail, 0);
      blocks.push_back({model.name, std::move(values)});
    }
    return RecordStore(schema_->meta, std::move(blocks));
  }

private:
  struct RecordSchema final {
    std::vector<RecordMeta> meta;
    pluss::subsp::math::optim::ParameterLayout layout;
    std::unordered_map<std::string, std::size_t, StringHash, std::equal_to<>>
        index;
  };

  RecordStore(std::shared_ptr<const RecordSchema> schema, Pack pack)
      : pack_(std::move(pack)), schema_(std::move(schema)) {}

  [[nodiscard]] std::size_t position(std::string_view name) const {
    const auto found = schema_->index.find(name);
    if (found == schema_->index.end())
      throw std::out_of_range("natsci::common record: unknown field '" +
                              std::string(name) + "'");
    return found->second;
  }

  Pack pack_;
  std::shared_ptr<const RecordSchema> schema_;
};

struct RecordBuilder;

} // namespace detail

template <DimensionType D> struct Param final {
  using dimension = D;

  std::string name;
  SeriesQuantity<D> value;
  std::optional<SeriesQuantity<D>> lower;
  std::optional<SeriesQuantity<D>> upper;
  std::string symbol;
  std::string source;
};

template <DimensionType D>
[[nodiscard]] Param<D> param(std::string name, SeriesQuantity<D> value,
                             std::string symbol = {}, std::string source = {}) {
  return {std::move(name), std::move(value),  std::nullopt,
          std::nullopt,    std::move(symbol), std::move(source)};
}

template <DimensionType D>
[[nodiscard]] Param<D> param(std::string name, SeriesQuantity<D> value,
                             SeriesQuantity<D> lower, SeriesQuantity<D> upper,
                             std::string symbol = {}, std::string source = {}) {
  return {std::move(name),  std::move(value),  std::move(lower),
          std::move(upper), std::move(symbol), std::move(source)};
}

struct ParameterMeta final {
  std::optional<Series> lower;
  std::optional<Series> upper;
};

class ParameterSet final {
public:
  [[nodiscard]] std::size_t size() const noexcept { return store_.size(); }
  [[nodiscard]] bool contains(std::string_view name) const noexcept {
    return store_.contains(name);
  }
  [[nodiscard]] const std::vector<RecordMeta> &meta() const noexcept {
    return store_.meta();
  }
  [[nodiscard]] const pluss::subsp::math::optim::ParameterLayout &
  layout() const noexcept {
    return store_.layout();
  }
  [[nodiscard]] Series flat() const { return store_.flat(); }

  template <DimensionType D>
  [[nodiscard]] SeriesQuantity<D> get(std::string_view name) const {
    return store_.template get<D>(name);
  }

  [[nodiscard]] ParameterSet repack(const Series &flat) const {
    return ParameterSet(store_.repack(flat), bounds_, true);
  }

private:
  friend struct detail::RecordBuilder;
  friend struct solve::detail::RecordAccess;

  ParameterSet(detail::RecordStore store, std::vector<ParameterMeta> bounds)
      : ParameterSet(std::move(store),
                     std::make_shared<const std::vector<ParameterMeta>>(
                         std::move(bounds)),
                     true) {}

  ParameterSet(detail::RecordStore store,
               std::shared_ptr<const std::vector<ParameterMeta>> bounds,
               bool validate)
      : store_(std::move(store)), bounds_(std::move(bounds)) {
    if (validate)
      validate_bounds_();
  }

  void validate_bounds_() const {
    if (bounds_->size() != store_.size())
      throw std::invalid_argument(
          "natsci::common::ParameterSet: bounds size mismatch");
    for (std::size_t i = 0; i < bounds_->size(); ++i) {
      const auto &name = store_.meta()[i].name;
      const auto &value = store_.at(name);
      const auto &bound = (*bounds_)[i];
      if ((bound.lower || bound.upper) && value.dtype().is_complex())
        throw std::invalid_argument(
            "natsci::common::ParameterSet: bounded values must be real");
      if (bound.lower) {
        detail::require_same_layout(value, *bound.lower,
                                    "natsci::common::ParameterSet lower bound");
        if (!value.ge(*bound.lower).all().item<bool>())
          throw std::invalid_argument(
              "natsci::common::ParameterSet: value is below lower bound");
      }
      if (bound.upper) {
        detail::require_same_layout(value, *bound.upper,
                                    "natsci::common::ParameterSet upper bound");
        if (!value.le(*bound.upper).all().item<bool>())
          throw std::invalid_argument(
              "natsci::common::ParameterSet: value is above upper bound");
      }
      if (bound.lower && bound.upper &&
          !bound.lower->le(*bound.upper).all().item<bool>())
        throw std::invalid_argument(
            "natsci::common::ParameterSet: lower bound exceeds upper bound");
    }
  }

  detail::RecordStore store_;
  std::shared_ptr<const std::vector<ParameterMeta>> bounds_;
};

template <DimensionType D> struct StateField final {
  using dimension = D;

  std::string name;
  SeriesQuantity<D> value;
  std::string symbol;
  std::string source;
};

template <DimensionType D>
[[nodiscard]] StateField<D> field(std::string name, SeriesQuantity<D> value,
                                  std::string symbol = {},
                                  std::string source = {}) {
  return {std::move(name), std::move(value), std::move(symbol),
          std::move(source)};
}

class StateVector final {
public:
  [[nodiscard]] std::size_t size() const noexcept { return store_.size(); }
  [[nodiscard]] bool contains(std::string_view name) const noexcept {
    return store_.contains(name);
  }
  [[nodiscard]] const std::vector<RecordMeta> &meta() const noexcept {
    return store_.meta();
  }
  [[nodiscard]] const pluss::subsp::math::optim::ParameterLayout &
  layout() const noexcept {
    return store_.layout();
  }
  [[nodiscard]] Series flat() const { return store_.flat(); }

  template <DimensionType D>
  [[nodiscard]] SeriesQuantity<D> get(std::string_view name) const {
    return store_.template get<D>(name);
  }

  [[nodiscard]] StateVector repack(const Series &flat) const {
    return StateVector(store_.repack(flat));
  }

private:
  friend struct detail::RecordBuilder;
  friend struct solve::detail::RecordAccess;
  explicit StateVector(detail::RecordStore store) : store_(std::move(store)) {}

  detail::RecordStore store_;
};

namespace detail {

struct RecordBuilder final {
  template <DimensionType D>
  static void append(const Param<D> &entry, std::vector<RecordMeta> &meta,
                     std::vector<RecordStore::Pack::Block> &blocks,
                     std::vector<ParameterMeta> &bounds) {
    meta.push_back(
        {entry.name, DimensionSignature::of<D>(), entry.symbol, entry.source});
    blocks.push_back({entry.name, entry.value.si_value()});
    bounds.push_back(
        {entry.lower ? std::optional<Series>(entry.lower->si_value())
                     : std::nullopt,
         entry.upper ? std::optional<Series>(entry.upper->si_value())
                     : std::nullopt});
  }

  template <DimensionType D>
  static void append(const StateField<D> &entry, std::vector<RecordMeta> &meta,
                     std::vector<RecordStore::Pack::Block> &blocks) {
    meta.push_back(
        {entry.name, DimensionSignature::of<D>(), entry.symbol, entry.source});
    blocks.push_back({entry.name, entry.value.si_value()});
  }

  template <class... Entries>
  static ParameterSet parameters(const Entries &...entries) {
    std::vector<RecordMeta> meta;
    std::vector<RecordStore::Pack::Block> blocks;
    std::vector<ParameterMeta> bounds;
    meta.reserve(sizeof...(Entries));
    blocks.reserve(sizeof...(Entries));
    bounds.reserve(sizeof...(Entries));
    (append(entries, meta, blocks, bounds), ...);
    return ParameterSet(RecordStore(std::move(meta), std::move(blocks)),
                        std::move(bounds));
  }

  template <class... Entries>
  static StateVector states(const Entries &...entries) {
    std::vector<RecordMeta> meta;
    std::vector<RecordStore::Pack::Block> blocks;
    meta.reserve(sizeof...(Entries));
    blocks.reserve(sizeof...(Entries));
    (append(entries, meta, blocks), ...);
    return StateVector(RecordStore(std::move(meta), std::move(blocks)));
  }
};

template <class T> struct is_param : std::false_type {};
template <DimensionType D> struct is_param<Param<D>> : std::true_type {};
template <class T>
inline constexpr bool is_param_v = is_param<std::remove_cvref_t<T>>::value;

template <class T> struct is_field : std::false_type {};
template <DimensionType D> struct is_field<StateField<D>> : std::true_type {};
template <class T>
inline constexpr bool is_field_v = is_field<std::remove_cvref_t<T>>::value;

} // namespace detail

template <class... Entries>
  requires(sizeof...(Entries) > 0 && (... && detail::is_param_v<Entries>))
[[nodiscard]] ParameterSet params(Entries &&...entries) {
  return detail::RecordBuilder::parameters(entries...);
}

template <class... Entries>
  requires(sizeof...(Entries) > 0 && (... && detail::is_field_v<Entries>))
[[nodiscard]] StateVector states(Entries &&...entries) {
  return detail::RecordBuilder::states(entries...);
}

} // namespace pluss::subsp::natsci::common
