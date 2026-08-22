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
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/StringViewNameRange.h"
#include "MSharp/IntrusivePtr.h"
#include "scribble/Slot.h"
#include "loader/Config.h"
#include "loader/CsvLoader.h"
#include "loader/ExcelLoader.h"

namespace pluss::kindle {
class Catalog;
struct TabelizePolicy;
}

namespace pluss::table {
class DataFrame;
}

namespace pluss::scribble {

namespace detail {
class RegistryImpl;
}

class RegistryFieldAxisAccessor;
class RegistryCastAccessor;
class RegistryNoteAccessor;
class RegistryTimelineAccessor;

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

  static Registry load(std::string_view source_type, std::string_view filepath);
  static Registry load(std::string_view source_type,
                       std::string_view filepath,
                       const pluss::loader::Config& config);
  static Registry load_csv(std::string_view filepath);
  static Registry load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options);
  static Registry load_csv(std::string_view filepath, const pluss::loader::Config& config);
  static Registry load_txt(std::string_view filepath);
  static Registry load_txt(std::string_view filepath, const pluss::loader::Config& config);
  static Registry load_excel(std::string_view filepath);
  static Registry load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options);
  static Registry load_excel(std::string_view filepath, const pluss::loader::Config& config);
  static Registry load_parquet(std::string_view filepath);
  static Registry load_parquet(std::string_view filepath, const pluss::loader::Config& config);
  static Registry load_json(std::string_view filepath);
  static Registry load_json(std::string_view filepath, const pluss::loader::Config& config);

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t nstate() const;
  std::int64_t minstate() const;
  std::int64_t maxstate() const;
  std::int64_t nfields() const;
  std::int64_t size() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  bool empty() const;
  const std::vector<std::string>& fields() const;
  Slot fields(std::string_view key) const;
  Registry fields(std::span<const std::string_view> keys) const;
  Registry fields(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Registry fields(const NameRange& keys) const;
  RegistryFieldAxisAccessor fieldaxis() const;
  RegistryCastAccessor cast() const;
  RegistryCastAccessor cast(const pluss::table::ThreadCastOptions& options) const;
  RegistryNoteAccessor nt() const;
  RegistryTimelineAccessor tl() const;
  bool has(std::string_view key) const;
  bool has_all(std::span<const std::string_view> keys) const;
  bool has_all(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  bool has_all(const NameRange& keys) const {
    return pluss::detail::with_name_span(keys, [&](auto names) { return has_all(names); });
  }
  bool has_any(std::span<const std::string_view> keys) const;
  bool has_any(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  bool has_any(const NameRange& keys) const {
    return pluss::detail::with_name_span(keys, [&](auto names) { return has_any(names); });
  }
  std::optional<Slot> get(std::string_view key) const;

  // Registry-level delegation over slots.
  Registry head(std::int64_t n) const;
  Registry tail(std::int64_t n) const;
  Registry slice(std::int64_t start, std::int64_t stop) const;
  Registry cloc(std::int64_t row) const;
  Registry cloc(std::int64_t start, std::int64_t stop) const;
  Registry take(std::span<const std::int64_t> indices) const;
  Registry take(const std::vector<std::int64_t>& indices) const;
  Registry sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const;

  Registry drop(std::span<const std::string_view> keys) const;
  Registry drop(std::initializer_list<std::string_view> keys) const;
  Registry drop(const std::vector<std::string>& keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Registry drop(const NameRange& keys) const {
    return pluss::detail::with_name_span(keys, [&](auto names) { return drop(names); });
  }
  Registry drop(std::string_view key) const;
  Registry rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Registry rename(const std::unordered_map<std::string, std::string>& mapping) const;
  Registry rename(std::string_view from, std::string_view to) const;
  Registry& rename_(std::string_view from, std::string_view to);
  Registry map(const std::function<Slot(const Slot&)>& fn) const;
  Registry& map_(const std::function<Slot(const Slot&)>& fn);
  std::unordered_map<std::string, std::int64_t> sizes() const;
  std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>> profiles() const;
  Registry combine(const Registry& other, std::string suffix = "_other") const;
  Registry clear() const;
  Registry& clear_();
  Registry prune() const;
  Registry& prune_();

  Registry isnull() const;
  Registry notnull() const;
  Registry invalid() const;
  Registry fillnull(double value) const;
  Registry fillnull(std::string_view value) const;
  Registry fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Registry fillnull(const pluss::Timeline& value) const;
  Registry fillnull(const pluss::TimeIndex& value) const;
  Registry dropnull() const;
  Registry distinct(pluss::kindle::DistinctOptions options = {}) const;
  Registry freq(pluss::kindle::FreqOptions options = {}) const;
  Registry ndistinct(pluss::kindle::DistinctOptions options = {}) const;

  Registry set(std::string_view key, const Slot& value) const;
  Registry& set_(std::string_view key, const Slot& value);
  Registry set(std::initializer_list<std::pair<std::string_view, Slot>> items) const;
  Registry& set_(std::initializer_list<std::pair<std::string_view, Slot>> items);
  Registry set(std::initializer_list<std::pair<std::string, Slot>> items) const;
  Registry& set_(std::initializer_list<std::pair<std::string, Slot>> items);
  Registry push(std::string_view key, const Slot::Value& value) const;
  Registry& push_(std::string_view key, const Slot::Value& value);
  Registry push(const Slot::Value& value) const;
  Registry& push_(const Slot::Value& value);
  Registry keep(std::string_view key,
                const std::function<bool(const Slot::Value&)>& predicate) const;
  Registry& keep_(std::string_view key,
                  const std::function<bool(const Slot::Value&)>& predicate);
  Registry morph(std::string_view key,
                 const std::function<Slot::Value(const Slot::Value&)>& fn) const;
  Registry& morph_(std::string_view key,
                   const std::function<Slot::Value(const Slot::Value&)>& fn);
  Registry take(std::string_view key, std::span<const std::int64_t> indices) const;
  Registry take(std::string_view key, const std::vector<std::int64_t>& indices) const;
  Registry& take_(std::string_view key, std::span<const std::int64_t> indices);
  Registry& take_(std::string_view key, const std::vector<std::int64_t>& indices);
  Registry concat(std::string_view key, const Slot& other) const;
  Registry& concat_(std::string_view key, const Slot& other);

  // Explicit bridge with kindle model.
  // Conversion may throw when slot heterogeneity is not stream-compatible.
  pluss::kindle::Catalog kindelize() const;
  pluss::table::DataFrame tabelize() const;
  pluss::table::DataFrame tabelize(const pluss::kindle::TabelizePolicy& policy) const;

  void save_csv(std::string_view filepath) const;
  void save_csv(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_txt(std::string_view filepath) const;
  void save_txt(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_excel(std::string_view filepath) const;
  void save_excel(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_json(std::string_view filepath) const;
  void save_json(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_parquet(std::string_view filepath) const;
  void save_parquet(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_as(std::string_view target_type,
               std::string_view filepath,
               const pluss::loader::Config& config = {}) const;

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

  const_iterator begin() const;
  const_iterator end() const;

private:
  explicit Registry(pluss::msharp::intrusive_ptr<detail::RegistryImpl> impl);
  std::int64_t nslots() const;
  const std::vector<std::string>& keys() const;
  const std::vector<std::string>& field_names() const;
  bool is_stream_compatible() const;
  void ensure_stream_compatible(std::string_view where = "Registry") const;
  bool has_slot(std::string_view key) const;
  Slot slot(std::string_view key) const;
  std::optional<Slot> get_slot(std::string_view key) const;
  std::vector<Slot> slots() const;
  std::unordered_map<std::string, std::int64_t> slot_sizes() const;
  Registry project(std::span<const std::string_view> fields) const;
  Registry project(const std::vector<std::string>& fields) const;
  Registry transform_fields(std::span<const std::string_view> fields,
                            const std::function<Slot(const Slot&)>& fn) const;
  Registry set_slot(std::string_view key, const Slot& value) const;
  Registry& set_slot_(std::string_view key, const Slot& value);
  Registry set_slots(std::initializer_list<std::pair<std::string_view, Slot>> items) const;
  Registry& set_slots_(std::initializer_list<std::pair<std::string_view, Slot>> items);
  Registry set_slots(std::initializer_list<std::pair<std::string, Slot>> items) const;
  Registry& set_slots_(std::initializer_list<std::pair<std::string, Slot>> items);
  const detail::RegistryImpl& impl_ref_() const;
  detail::RegistryImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::RegistryImpl> impl_{nullptr};
};

class RegistryFieldAxisAccessor {
public:
  explicit RegistryFieldAxisAccessor(Registry base);

  Registry like(std::string_view needle, bool case_sensitive = false) const;
  Registry match(std::string_view pattern) const;
  Registry prefix(std::string_view token) const;
  Registry suffix(std::string_view token) const;
  Registry kind(SlotKind kind) const;
  Registry series() const;
  Registry note() const;
  Registry timeline() const;
  Registry timeindex() const;
  Registry mixed() const;
  Registry nulls() const;
  Registry empty() const;

private:
  Registry base_{};
};

class RegistryCastAccessor {
public:
  explicit RegistryCastAccessor(Registry base,
                                std::vector<std::string> keys,
                                pluss::table::ThreadCastOptions options = {});

  Registry series() const;
  Registry note() const;
  Registry timeline() const;
  Registry timeindex() const;

private:
  Registry base_{};
  std::vector<std::string> keys_{};
  pluss::table::ThreadCastOptions options_{};
};

class RegistryNoteAccessor {
public:
  explicit RegistryNoteAccessor(Registry base, std::vector<std::string> keys);

  Registry strip() const;
  Registry strip(std::string_view chars) const;
  Registry lstrip() const;
  Registry lstrip(std::string_view chars) const;
  Registry rstrip() const;
  Registry rstrip(std::string_view chars) const;
  Registry lower() const;
  Registry upper() const;
  Registry casefold() const;
  Registry titlecase() const;
  Registry length() const;
  Registry starts(std::string_view prefix) const;
  Registry ends(std::string_view suffix) const;
  Registry has(std::string_view needle,
               const pluss::table::TextOptions& options = {}) const;
  Registry count(std::string_view pattern) const;
  Registry count(std::string_view pattern,
                 const pluss::table::TextOptions& options) const;
  Registry matches(std::string_view pattern, bool full = false) const;
  Registry fullmatch(std::string_view pattern) const;
  Registry pad(std::int64_t width,
               pluss::table::TextAlign align = pluss::table::TextAlign::Left,
               char fill = ' ') const;
  Registry chars(std::int64_t start,
                 std::optional<std::int64_t> stop = std::nullopt,
                 std::int64_t step = 1) const;
  Registry normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const;
  Registry encode(std::string_view encoding = "base64") const;
  Registry replace(std::string_view from, std::string_view to) const;
  Registry replace(std::string_view from,
                   std::string_view to,
                   const pluss::table::TextOptions& options) const;

private:
  Registry base_{};
  std::vector<std::string> keys_{};
};

class RegistryTimelineAccessor {
public:
  explicit RegistryTimelineAccessor(Registry base, std::vector<std::string> keys);

  Registry isnat() const;
  Registry valid() const;
  Registry year() const;
  Registry month() const;
  Registry day() const;
  Registry hour() const;
  Registry minute() const;
  Registry second() const;
  Registry weekday() const;
  Registry yearday() const;
  Registry quarter() const;
  Registry week() const;
  Registry weekend() const;
  Registry month_start() const;
  Registry month_end() const;
  Registry leap() const;
  Registry weekday_name() const;
  Registry month_name() const;
  Registry fmt(std::string_view pattern) const;
  Registry iso() const;
  Registry epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const;
  Registry floor(std::string_view unit) const;
  Registry ceil(std::string_view unit) const;
  Registry round(std::string_view unit) const;
  Registry date() const;
  Registry time() const;
  Registry shift(std::int64_t periods, std::string_view freq) const;
  Registry period(std::string_view freq) const;
  Registry tz(std::string_view name) const;
  Registry localize(const pluss::table::TimelineLocalizeOptions& options = {}) const;
  Registry utc() const;
  Registry index(bool require_sorted = true) const;

private:
  Registry base_{};
  std::vector<std::string> keys_{};
};

std::ostream& operator<<(std::ostream& os, const Registry& registry);

template <pluss::detail::StringViewNameRange NameRange>
Registry Registry::fields(const NameRange& keys) const {
  return pluss::detail::with_name_span(keys, [&](auto names) { return fields(names); });
}

} // namespace pluss::scribble
