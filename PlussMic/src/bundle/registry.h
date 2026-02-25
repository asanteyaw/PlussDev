#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "bundle/Slot.h"

namespace pluss::kindle {
class Catalog;
}

namespace pluss::bundle {

namespace detail {
class RegistryImpl;
}

class Registry {
public:
  using const_iterator = std::vector<std::string>::const_iterator;

  Registry();
  explicit Registry(std::vector<std::pair<std::string, Slot>> fields);
  Registry(std::initializer_list<std::pair<std::string, Slot>> fields);
  Registry(const Registry& other);
  Registry(Registry&& other) noexcept;
  Registry& operator=(const Registry& other);
  Registry& operator=(Registry&& other) noexcept;
  ~Registry();

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t nslots() const;
  std::int64_t nfields() const;
  bool empty() const;
  const std::vector<std::string>& keys() const;
  const std::vector<std::string>& field_names() const;
  bool has(std::string_view key) const;
  bool has_slot(std::string_view key) const;
  bool has_all(std::initializer_list<std::string_view> keys) const;
  bool has_any(std::initializer_list<std::string_view> keys) const;
  Slot slot(std::string_view key) const;
  Slot at(std::string_view key) const;
  std::optional<Slot> get(std::string_view key) const;
  std::optional<Slot> get_slot(std::string_view key) const;
  std::vector<Slot> slots() const;
  bool is_stream_compatible() const;
  void ensure_stream_compatible(std::string_view where = "Registry") const;

  // Registry-level delegation over slots.
  Registry head(std::int64_t n) const;
  Registry tail(std::int64_t n) const;
  Registry slice(std::int64_t start, std::int64_t stop) const;
  Registry iloc(std::int64_t row) const;
  Registry iloc(std::int64_t start, std::int64_t stop) const;
  Registry take(std::span<const std::int64_t> indices) const;
  Registry take(const std::vector<std::int64_t>& indices) const;
  Registry sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const;

  Registry select(std::initializer_list<std::string_view> keys) const;
  Registry select(const std::vector<std::string>& keys) const;
  Registry select(std::string_view key) const;
  Registry drop(std::initializer_list<std::string_view> keys) const;
  Registry drop(const std::vector<std::string>& keys) const;
  Registry drop(std::string_view key) const;
  Registry rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Registry rename(const std::unordered_map<std::string, std::string>& mapping) const;
  Registry rename(std::string_view from, std::string_view to) const;
  Registry& rename_(std::string_view from, std::string_view to);
  Registry apply(std::string_view key, const std::function<Slot(const Slot&)>& fn) const;
  Registry apply(std::initializer_list<std::string_view> keys,
                 const std::function<Slot(const Slot&)>& fn) const;
  Registry& apply_(std::string_view key, const std::function<Slot(const Slot&)>& fn);
  Registry& apply_(std::initializer_list<std::string_view> keys,
                   const std::function<Slot(const Slot&)>& fn);
  Registry apply_all(const std::function<Slot(const Slot&)>& fn) const;
  Registry map(const std::function<Slot(const Slot&)>& fn) const;
  Registry& map_(const std::function<Slot(const Slot&)>& fn);
  std::unordered_map<std::string, std::int64_t> slot_sizes() const;
  std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>> type_profiles() const;
  Registry combine(const Registry& other, std::string suffix = "_other") const;
  Registry clear() const;
  Registry& clear_();
  Registry drop_empty_slots() const;
  Registry& drop_empty_slots_();

  Registry set_slot(std::string_view key, const Slot& value) const;
  Registry& set_slot_(std::string_view key, const Slot& value);
  Registry set_slots(std::initializer_list<std::pair<std::string_view, Slot>> items) const;
  Registry& set_slots_(std::initializer_list<std::pair<std::string_view, Slot>> items);
  Registry set_slots(std::initializer_list<std::pair<std::string, Slot>> items) const;
  Registry& set_slots_(std::initializer_list<std::pair<std::string, Slot>> items);
  Registry push(std::string_view key, const Slot::Value& value) const;
  Registry& push_(std::string_view key, const Slot::Value& value);
  Registry push_all(const Slot::Value& value) const;
  Registry& push_all_(const Slot::Value& value);
  Registry filter_values(std::string_view key,
                         const std::function<bool(const Slot::Value&)>& predicate) const;
  Registry& filter_values_(std::string_view key,
                           const std::function<bool(const Slot::Value&)>& predicate);
  Registry transform_values(std::string_view key,
                            const std::function<Slot::Value(const Slot::Value&)>& fn) const;
  Registry& transform_values_(std::string_view key,
                              const std::function<Slot::Value(const Slot::Value&)>& fn);
  Registry take_values(std::string_view key, std::span<const std::int64_t> indices) const;
  Registry& take_values_(std::string_view key, std::span<const std::int64_t> indices);
  Registry concat_values(std::string_view key, const Slot& other) const;
  Registry& concat_values_(std::string_view key, const Slot& other);

  // Stream-compatible delegated APIs (Slot -> Stream -> Catalog bridge).
  Registry series() const;
  Registry note() const;
  Registry timeline() const;
  Registry timeindex() const;
  Registry sort(std::string_view by_field, bool ascending = true) const;
  Registry sort_by(const std::vector<std::string>& fields,
                   const std::vector<bool>& ascending = {}) const;
  std::unordered_map<std::string, Registry> group_by(std::string_view key_field) const;
  Registry fillna(double value, std::initializer_list<std::string_view> keys = {}) const;
  Registry dropna(std::initializer_list<std::string_view> keys = {}) const;
  Registry ffill(std::initializer_list<std::string_view> keys = {}) const;
  Registry bfill(std::initializer_list<std::string_view> keys = {}) const;
  Registry isnull(std::initializer_list<std::string_view> keys = {}) const;
  Registry notnull(std::initializer_list<std::string_view> keys = {}) const;
  Registry cast_series(std::initializer_list<std::string_view> keys,
                       std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const;
  Registry cast_note(std::initializer_list<std::string_view> keys) const;
  Registry cast_timeline(std::initializer_list<std::string_view> keys) const;
  Registry cast_timeindex(std::initializer_list<std::string_view> keys) const;
  Registry astype(std::initializer_list<std::string_view> keys, pluss::codem::ScalarType dtype) const;
  Slot sum(std::string_view field) const;
  Slot mean(std::string_view field) const;
  Slot min(std::string_view field) const;
  Slot max(std::string_view field) const;
  std::int64_t count() const;
  Registry merge(const Registry& other, std::string_view on, std::string_view how = "inner") const;
  Registry text_preprocess(std::string_view field) const;
  Registry extract_keywords(std::string_view field, int top_n = 10) const;
  Registry compute_similarity(std::string_view field1,
                              std::string_view field2,
                              std::string_view method = "cosine") const;
  Registry filter_by_text(std::string_view field, std::string_view pattern) const;
  Registry add_text_features(std::string_view field) const;
  Registry aggregate_text_stats(std::string_view field, std::string_view stat_type) const;
  Registry tfidf(std::string_view field) const;

  // Bridge with kindle model (may throw if slot heterogeneity is not stream-compatible).
  pluss::kindle::Catalog kindelize() const;
  static Registry from_kindle(const pluss::kindle::Catalog& catalog);

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

  const_iterator begin() const;
  const_iterator end() const;

private:
  explicit Registry(pluss::msharp::intrusive_ptr<detail::RegistryImpl> impl);
  const detail::RegistryImpl& impl_ref_() const;
  detail::RegistryImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::RegistryImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Registry& registry);

} // namespace pluss::bundle
