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
#include <variant>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"

namespace pluss::kindle {
class Stream;
}

namespace pluss::bundle {

namespace detail {
class SlotImpl;
}

class Slot {
public:
  using Value = std::variant<
      std::monostate,
      bool,
      std::int64_t,
      double,
      std::string,
      pluss::Series,
      pluss::Note,
      pluss::Timeline,
      pluss::TimeIndex>;

  Slot();
  explicit Slot(std::vector<Value> values);
  Slot(std::initializer_list<Value> values);
  explicit Slot(Value value);
  Slot(const Slot& other);
  Slot(Slot&& other) noexcept;
  Slot& operator=(const Slot& other);
  Slot& operator=(Slot&& other) noexcept;
  ~Slot();

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t size() const;
  bool empty() const;
  const std::vector<Value>& values() const;
  const Value& at(std::int64_t i) const;

  Slot push(const Value& value) const;
  Slot& push_(const Value& value);
  Slot head(std::int64_t n) const;
  Slot tail(std::int64_t n) const;
  Slot slice(std::int64_t start, std::int64_t stop) const;
  Slot take(std::span<const std::int64_t> indices) const;
  Slot filter(const std::function<bool(const Value&)>& predicate) const;
  Slot transform(const std::function<Value(const Value&)>& fn) const;
  Slot concat(const Slot& other) const;
  std::unordered_map<std::string, std::int64_t> type_counts() const;

  // Conversion for bridge paths (throws when slot is not stream-compatible).
  pluss::kindle::Stream kindelize() const;
  static Slot from_kindle(const pluss::kindle::Stream& stream);

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

private:
  explicit Slot(pluss::msharp::intrusive_ptr<detail::SlotImpl> impl);
  const detail::SlotImpl& impl_ref_() const;
  detail::SlotImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::SlotImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Slot& slot);

} // namespace pluss::bundle
