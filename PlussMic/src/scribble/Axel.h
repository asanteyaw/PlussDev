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
#include "scribble/registry.h"

namespace pluss::kindle {
class Archive;
struct TabelizePolicy;
}

namespace pluss::scribble {

namespace detail {
class AxelImpl;
}

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
  Axel fields(std::span<const std::string_view> keys) const;
  Axel fields(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Axel fields(const NameRange& keys) const;
  AxelFieldAxisAccessor fieldaxis() const;
  AxelCastAccessor cast() const;
  AxelCastAccessor cast(const pluss::table::ShelfCastOptions& options) const;
  AxelNoteAccessor nt() const;
  AxelTimelineAccessor tl() const;
  Registry registry(std::int64_t i) const;
  const std::vector<Registry>& registries() const;
  const std::optional<Slot>& labels() const;

  Axel head(std::int64_t n = 5) const;
  Axel tail(std::int64_t n = 5) const;
  Axel slice(std::int64_t start, std::int64_t stop) const;
  Axel map(const std::function<Registry(const Registry&)>& fn) const;
  Axel drop(std::span<const std::string_view> fields) const;
  Axel drop(std::initializer_list<std::string_view> fields) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Axel drop(const NameRange& fields) const {
    return pluss::detail::with_name_span(fields, [&](auto names) { return drop(names); });
  }
  Axel rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Axel rename(std::string_view from, std::string_view to) const;
  Axel concat_depth(const Axel& other) const;

  bool has(std::string_view field) const;
  Axel isnull() const;
  Axel notnull() const;
  Axel invalid() const;
  // NaN and infinity are values, not missing cells (see table/NonFinite.h): isnull / dropnull / fillnull never touch them and
  // these never touch a null.
  Axel isnan() const;
  Axel notnan() const;
  Axel isinf() const;
  Axel isposinf() const;
  Axel isneginf() const;
  Axel notinf() const;
  Axel dropnan() const;
  Axel dropinvalid() const;
  Axel dropinf() const;
  Axel dropposinf() const;
  Axel dropneginf() const;
  Axel fillnan(double value) const;
  Axel fillinvalid(double value) const;
  Axel fillinf(double value) const;
  Axel fillposinf(double value) const;
  Axel fillneginf(double value) const;
  Axel fillnull(double value) const;
  Axel fillnull(std::string_view value) const;
  Axel fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Axel fillnull(const pluss::Timeline& value) const;
  Axel fillnull(const pluss::Duration& value) const;
  Axel dropnull() const;
  Axel distinct(pluss::kindle::DistinctOptions options = {}) const;
  Axel freq(pluss::kindle::FreqOptions options = {}) const;
  Axel ndistinct(pluss::kindle::DistinctOptions options = {}) const;

  pluss::kindle::Archive kindelize() const;
  pluss::table::Atlas tabelize() const;
  pluss::table::Atlas tabelize(const pluss::kindle::TabelizePolicy& policy) const;

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;
  // Opens the axel in the PlussView window, one registry at a time as a ragged grid, or hands it to the notebook.
  void show() const;

private:
  explicit Axel(pluss::msharp::intrusive_ptr<detail::AxelImpl> impl);
  const detail::AxelImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::AxelImpl> impl_{nullptr};
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
  Axel duration() const;
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
                            pluss::table::ShelfCastOptions options = {});

  Axel series() const;
  Axel note() const;
  Axel timeline() const;
  Axel duration() const;

private:
  Axel base_{};
  std::vector<std::string> keys_{};
  pluss::table::ShelfCastOptions options_{};
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

private:
  Axel base_{};
  std::vector<std::string> keys_{};
};

std::ostream& operator<<(std::ostream& os, const Axel& axel);

template <pluss::detail::StringViewNameRange NameRange>
Axel Axel::fields(const NameRange& keys) const {
  return pluss::detail::with_name_span(keys, [&](auto names) { return fields(names); });
}

} // namespace pluss::scribble
