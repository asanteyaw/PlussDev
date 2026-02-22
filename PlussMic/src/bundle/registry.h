#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
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
  bool empty() const;
  const std::vector<std::string>& keys() const;
  bool has(std::string_view key) const;
  Slot slot(std::string_view key) const;
  std::optional<Slot> get(std::string_view key) const;

  // Registry-level delegation over slots.
  Registry head(std::int64_t n) const;
  Registry tail(std::int64_t n) const;
  Registry slice(std::int64_t start, std::int64_t stop) const;

  Registry select(std::initializer_list<std::string_view> keys) const;
  Registry drop(std::initializer_list<std::string_view> keys) const;
  Registry rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Registry apply(std::string_view key, const std::function<Slot(const Slot&)>& fn) const;
  Registry& apply_(std::string_view key, const std::function<Slot(const Slot&)>& fn);
  Registry apply_all(const std::function<Slot(const Slot&)>& fn) const;
  std::unordered_map<std::string, std::int64_t> slot_sizes() const;
  Registry combine(const Registry& other, std::string suffix = "_other") const;

  Registry set_slot(std::string_view key, const Slot& value) const;
  Registry& set_slot_(std::string_view key, const Slot& value);

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
