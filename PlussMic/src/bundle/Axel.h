#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/StringViewNameRange.h"
#include "MSharp/IntrusivePtr.h"
#include "bundle/registry.h"

namespace pluss::kindle {
class Archive;
}

namespace pluss::bundle {

namespace detail {
class AxelImpl;
}

class AxelFieldAccessor;
class AxelFieldAxisAccessor;
class AxelCastAccessor;
class AxelNoteAccessor;
class AxelTimelineAccessor;

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
  AxelFieldAccessor fields(std::span<const std::string_view> keys) const;
  AxelFieldAccessor fields(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  AxelFieldAccessor fields(const NameRange& keys) const;
  AxelFieldAxisAccessor fieldaxis() const;
  Registry registry(std::int64_t i) const;
  const std::vector<Registry>& registries() const;
  const std::optional<Slot>& labels() const;

  Axel head(std::int64_t n) const;
  Axel tail(std::int64_t n) const;
  Axel slice(std::int64_t start, std::int64_t stop) const;
  Axel map(const std::function<Registry(const Registry&)>& fn) const;
  Axel select(std::span<const std::string_view> keys) const;
  Axel select(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Axel select(const NameRange& keys) const {
    return pluss::detail::with_name_span(keys, [&](auto names) { return select(names); });
  }
  Axel drop(std::span<const std::string_view> fields) const;
  Axel drop(std::initializer_list<std::string_view> fields) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Axel drop(const NameRange& fields) const {
    return pluss::detail::with_name_span(fields, [&](auto names) { return drop(names); });
  }
  Axel rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Axel rename(std::string_view from, std::string_view to) const;
  Axel apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) const;
  Axel concat_depth(const Axel& other) const;

  bool has(std::string_view field) const;
  Axel isnull() const;
  Axel notnull() const;
  Axel invalid() const;
  Axel fillnull(double value) const;
  Axel fillnull(std::string_view value) const;
  Axel fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Axel fillnull(const pluss::Timeline& value) const;
  Axel fillnull(const pluss::TimeIndex& value) const;
  Axel dropnull() const;
  Axel distinct(pluss::kindle::DistinctOptions options = {}) const;
  Axel freq(pluss::kindle::FreqOptions options = {}) const;
  Axel ndistinct(pluss::kindle::DistinctOptions options = {}) const;

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
  Axel isnull(std::span<const std::string_view> fields) const;
  Axel isnull(std::initializer_list<std::string_view> fields) const;
  Axel notnull(std::span<const std::string_view> fields) const;
  Axel notnull(std::initializer_list<std::string_view> fields) const;
  Axel invalid(std::span<const std::string_view> fields) const;
  Axel invalid(std::initializer_list<std::string_view> fields) const;
  Axel fillnull(double value, std::span<const std::string_view> fields) const;
  Axel fillnull(double value, std::initializer_list<std::string_view> fields) const;
  Axel fillnull(std::string_view value, std::span<const std::string_view> fields) const;
  Axel fillnull(std::string_view value, std::initializer_list<std::string_view> fields) const;
  Axel fillnull(const pluss::Timeline& value, std::span<const std::string_view> fields) const;
  Axel fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> fields) const;
  Axel fillnull(const pluss::TimeIndex& value, std::span<const std::string_view> fields) const;
  Axel fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> fields) const;
  Axel dropnull(std::span<const std::string_view> fields) const;
  Axel dropnull(std::initializer_list<std::string_view> fields) const;
  Axel distinct(std::span<const std::string_view> fields,
                pluss::kindle::DistinctOptions options = {}) const;
  Axel distinct(std::initializer_list<std::string_view> fields,
                pluss::kindle::DistinctOptions options = {}) const;
  Axel freq(std::span<const std::string_view> fields,
            pluss::kindle::FreqOptions options = {}) const;
  Axel freq(std::initializer_list<std::string_view> fields,
            pluss::kindle::FreqOptions options = {}) const;
  Axel ndistinct(std::span<const std::string_view> fields,
                 pluss::kindle::DistinctOptions options = {}) const;
  Axel ndistinct(std::initializer_list<std::string_view> fields,
                 pluss::kindle::DistinctOptions options = {}) const;
  Axel cast(std::span<const std::string_view> fields,
            pluss::table::ThreadCastTarget target,
            const pluss::table::ThreadCastOptions& options = {}) const;
  Axel cast(std::initializer_list<std::string_view> fields,
            pluss::table::ThreadCastTarget target,
            const pluss::table::ThreadCastOptions& options = {}) const;
  Axel astype(std::span<const std::string_view> fields,
              pluss::codem::ScalarType dtype) const;
  Axel astype(std::initializer_list<std::string_view> fields,
              pluss::codem::ScalarType dtype) const;
  const detail::AxelImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::AxelImpl> impl_{nullptr};
};

class AxelFieldAccessor {
public:
  explicit AxelFieldAccessor(Axel base, std::vector<std::string> keys);

  AxelCastAccessor cast() const;
  AxelCastAccessor cast(const pluss::table::ThreadCastOptions& options) const;
  AxelNoteAccessor nt() const;
  AxelTimelineAccessor tl() const;

  Axel fillnull(double value) const;
  Axel fillnull(std::string_view value) const;
  Axel fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Axel fillnull(const pluss::Timeline& value) const;
  Axel fillnull(const pluss::TimeIndex& value) const;
  Axel dropnull() const;
  Axel isnull() const;
  Axel notnull() const;
  Axel invalid() const;
  Axel distinct(pluss::kindle::DistinctOptions options = {}) const;
  Axel freq(pluss::kindle::FreqOptions options = {}) const;
  Axel ndistinct(pluss::kindle::DistinctOptions options = {}) const;
  Axel astype(pluss::codem::ScalarType dtype) const;

private:
  friend class AxelCastAccessor;
  friend class AxelNoteAccessor;
  friend class AxelTimelineAccessor;
  template <class Fn>
  Axel map_selected_(Fn&& fn) const;

  Axel base_{};
  std::vector<std::string> keys_{};
};

class AxelFieldAxisAccessor {
public:
  explicit AxelFieldAxisAccessor(Axel base);

  Axel like(std::string_view needle, bool case_sensitive = false) const;
  Axel match(std::string_view pattern) const;
  Axel prefix(std::string_view token) const;
  Axel suffix(std::string_view token) const;
  Axel kind(SlotKind kind) const;
  Axel series() const;
  Axel note() const;
  Axel timeline() const;
  Axel timeindex() const;
  Axel mixed() const;
  Axel nulls() const;
  Axel empty() const;

private:
  Axel base_{};
};

class AxelCastAccessor {
public:
  explicit AxelCastAccessor(Axel base,
                            std::vector<std::string> keys,
                            pluss::table::ThreadCastOptions options = {});

  Axel series() const;
  Axel note() const;
  Axel timeline() const;
  Axel timeindex() const;

private:
  Axel base_{};
  std::vector<std::string> keys_{};
  pluss::table::ThreadCastOptions options_{};
};

class AxelNoteAccessor {
public:
  explicit AxelNoteAccessor(Axel base, std::vector<std::string> keys);

  Axel strip() const;
  Axel strip(std::string_view chars) const;
  Axel lstrip() const;
  Axel lstrip(std::string_view chars) const;
  Axel rstrip() const;
  Axel rstrip(std::string_view chars) const;
  Axel lower() const;
  Axel upper() const;
  Axel casefold() const;
  Axel titlecase() const;
  Axel length() const;
  Axel starts(std::string_view prefix) const;
  Axel ends(std::string_view suffix) const;
  Axel has(std::string_view needle,
           const pluss::table::TextOptions& options = {}) const;
  Axel count(std::string_view pattern) const;
  Axel count(std::string_view pattern,
             const pluss::table::TextOptions& options) const;
  Axel matches(std::string_view pattern, bool full = false) const;
  Axel fullmatch(std::string_view pattern) const;
  Axel pad(std::int64_t width,
           pluss::table::TextAlign align = pluss::table::TextAlign::Left,
           char fill = ' ') const;
  Axel chars(std::int64_t start,
             std::optional<std::int64_t> stop = std::nullopt,
             std::int64_t step = 1) const;
  Axel normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const;
  Axel encode(std::string_view encoding = "base64") const;
  Axel replace(std::string_view from, std::string_view to) const;
  Axel replace(std::string_view from,
               std::string_view to,
               const pluss::table::TextOptions& options) const;

private:
  Axel base_{};
  std::vector<std::string> keys_{};
};

class AxelTimelineAccessor {
public:
  explicit AxelTimelineAccessor(Axel base, std::vector<std::string> keys);

  Axel isnat() const;
  Axel valid() const;
  Axel year() const;
  Axel month() const;
  Axel day() const;
  Axel hour() const;
  Axel minute() const;
  Axel second() const;
  Axel weekday() const;
  Axel yearday() const;
  Axel quarter() const;
  Axel week() const;
  Axel weekend() const;
  Axel month_start() const;
  Axel month_end() const;
  Axel leap() const;
  Axel weekday_name() const;
  Axel month_name() const;
  Axel fmt(std::string_view pattern) const;
  Axel iso() const;
  Axel epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const;
  Axel floor(std::string_view unit) const;
  Axel ceil(std::string_view unit) const;
  Axel round(std::string_view unit) const;
  Axel date() const;
  Axel time() const;
  Axel shift(std::int64_t periods, std::string_view freq) const;
  Axel period(std::string_view freq) const;
  Axel tz(std::string_view name) const;
  Axel localize(const pluss::table::TimelineLocalizeOptions& options = {}) const;
  Axel utc() const;
  Axel index(bool require_sorted = true) const;

private:
  Axel base_{};
  std::vector<std::string> keys_{};
};

std::ostream& operator<<(std::ostream& os, const Axel& axel);

template <pluss::detail::StringViewNameRange NameRange>
AxelFieldAccessor Axel::fields(const NameRange& keys) const {
  return pluss::detail::with_name_span(keys, [&](auto names) { return fields(names); });
}

} // namespace pluss::bundle
