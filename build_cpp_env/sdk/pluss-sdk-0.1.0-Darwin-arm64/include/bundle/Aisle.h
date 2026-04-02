#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "bundle/registry.h"

namespace pluss::kindle {
class Archive;
}

namespace pluss::bundle {

namespace detail {
class AisleImpl;
}

class Aisle {
public:
  Aisle();
  explicit Aisle(std::vector<Registry> registries, std::optional<Slot> labels = std::nullopt);
  Aisle(const Aisle& other);
  Aisle(Aisle&& other) noexcept;
  Aisle& operator=(const Aisle& other);
  Aisle& operator=(Aisle&& other) noexcept;
  ~Aisle();

  bool defined() const noexcept;
  bool unique() const noexcept;
  std::int64_t depth() const;
  bool empty() const;
  Registry registry(std::int64_t i) const;
  const std::vector<Registry>& registries() const;
  const std::optional<Slot>& labels() const;

  Aisle head(std::int64_t n) const;
  Aisle tail(std::int64_t n) const;
  Aisle slice(std::int64_t start, std::int64_t stop) const;
  Aisle map(const std::function<Registry(const Registry&)>& fn) const;
  Aisle select(std::initializer_list<std::string_view> keys) const;
  Aisle apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) const;
  Aisle concat_depth(const Aisle& other) const;

  pluss::kindle::Archive to_kindle() const;
  // Semantic alias for explicit model switch.
  pluss::kindle::Archive as_kindle() const;
  // Backward-compatible alias for older call sites.
  pluss::kindle::Archive kindelize() const;
  static Aisle from_kindle(const pluss::kindle::Archive& archive);

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

private:
  explicit Aisle(pluss::msharp::intrusive_ptr<detail::AisleImpl> impl);
  const detail::AisleImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::AisleImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Aisle& aisle);

} // namespace pluss::bundle
