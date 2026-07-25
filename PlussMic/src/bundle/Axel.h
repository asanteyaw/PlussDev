#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "bundle/registry.h"

namespace pluss::kindle {
class Archive;
}

namespace pluss::bundle {

namespace detail {
class AxelImpl;
}

class Axel {
public:
  Axel();
  explicit Axel(std::vector<Registry> registries, std::optional<Slot> labels = std::nullopt);
  Axel(const Axel& other);
  Axel(Axel&& other) noexcept;
  Axel& operator=(const Axel& other);
  Axel& operator=(Axel&& other) noexcept;
  ~Axel();

  bool defined() const noexcept;
  bool unique() const noexcept;
  std::int64_t depth() const;
  std::int64_t size() const;
  bool empty() const;
  Registry registry(std::int64_t i) const;
  const std::vector<Registry>& registries() const;
  const std::optional<Slot>& labels() const;

  Axel head(std::int64_t n) const;
  Axel tail(std::int64_t n) const;
  Axel slice(std::int64_t start, std::int64_t stop) const;
  Axel map(const std::function<Registry(const Registry&)>& fn) const;
  Axel select(std::initializer_list<std::string_view> keys) const;
  Axel drop(std::initializer_list<std::string_view> fields) const;
  Axel rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Axel rename(std::string_view from, std::string_view to) const;
  Axel apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) const;
  Axel concat_depth(const Axel& other) const;

  bool has(std::string_view field) const;
  Axel isnull() const;
  Axel isnull(std::initializer_list<std::string_view> fields) const;
  Axel notnull() const;
  Axel notnull(std::initializer_list<std::string_view> fields) const;
  Axel invalid() const;
  Axel invalid(std::initializer_list<std::string_view> fields) const;
  Axel fillnull(double value) const;
  Axel fillnull(double value, std::initializer_list<std::string_view> fields) const;
  Axel fillnull(std::string_view value) const;
  Axel fillnull(std::string_view value, std::initializer_list<std::string_view> fields) const;
  Axel fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Axel fillnull(const char* value, std::initializer_list<std::string_view> fields) const {
    return fillnull(std::string_view{value != nullptr ? value : ""}, fields);
  }
  Axel fillnull(const pluss::Timeline& value) const;
  Axel fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> fields) const;
  Axel fillnull(const pluss::TimeIndex& value) const;
  Axel fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> fields) const;
  Axel dropnull() const;
  Axel dropnull(std::initializer_list<std::string_view> fields) const;
  Axel distinct(pluss::kindle::DistinctOptions options = {}) const;
  Axel distinct(std::initializer_list<std::string_view> fields,
                pluss::kindle::DistinctOptions options = {}) const;
  Axel freq(pluss::kindle::FreqOptions options = {}) const;
  Axel freq(std::initializer_list<std::string_view> fields,
            pluss::kindle::FreqOptions options = {}) const;
  Axel ndistinct(pluss::kindle::DistinctOptions options = {}) const;
  Axel ndistinct(std::initializer_list<std::string_view> fields,
                 pluss::kindle::DistinctOptions options = {}) const;
  Axel cast(std::initializer_list<std::string_view> fields,
            pluss::table::ThreadCastTarget target,
            const pluss::table::ThreadCastOptions& options = {}) const;
  Axel astype(std::initializer_list<std::string_view> fields,
              pluss::codem::ScalarType dtype) const;

  pluss::kindle::Archive to_kindle() const;
  // Semantic alias for explicit model switch.
  pluss::kindle::Archive as_kindle() const;
  // Backward-compatible alias for older call sites.
  pluss::kindle::Archive kindelize() const;
  static Axel from_kindle(const pluss::kindle::Archive& archive);

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

private:
  explicit Axel(pluss::msharp::intrusive_ptr<detail::AxelImpl> impl);
  const detail::AxelImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::AxelImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Axel& axel);

} // namespace pluss::bundle
