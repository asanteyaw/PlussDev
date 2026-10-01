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
#include <variant>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "kindle/Stream.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"

namespace pluss::scribble {

namespace detail {
class SlotImpl;
}

class SlotCastAccessor;
class SlotNoteAccessor;
class SlotTimelineAccessor;

enum class SlotKind : std::uint8_t {
  Empty = 0,
  Null,
  Series,
  Note,
  Timeline,
  Duration,
  Mixed
};

class Slot {
public:
  // Scribble is built on pluss domain types (not native C++ scalars).
  using Value = std::variant<
      std::monostate,
      pluss::Series,
      pluss::Note,
      pluss::Timeline,
      pluss::Duration>;

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
  std::pair<std::int64_t, std::int64_t> shape() const;
  SlotKind kind() const;
  std::string ptype() const;
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

  Slot isnull() const;
  Slot notnull() const;
  Slot invalid() const;
  Slot fillnull(double value) const;
  Slot fillnull(std::string_view value) const;
  Slot fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Slot fillnull(const pluss::Timeline& value) const;
  Slot fillnull(const pluss::Duration& value) const;
  Slot dropnull() const;
  Slot distinct(pluss::kindle::DistinctOptions options = {}) const;
  Slot freq(pluss::kindle::FreqOptions options = {}) const;
  Slot ndistinct(pluss::kindle::DistinctOptions options = {}) const;
  SlotCastAccessor cast() const;
  SlotCastAccessor cast(const pluss::table::ThreadCastOptions& options) const;
  Slot astype(pluss::codem::ScalarType dtype) const;
  SlotNoteAccessor nt() const;
  SlotTimelineAccessor tl() const;
  Slot sum() const;
  Slot mean() const;
  Slot min() const;
  Slot max() const;

  // Conversion for bridge paths (throws when slot is not stream-compatible).
  pluss::kindle::Stream kindelize() const;
  // 1D counterpart to Registry::tabelize()/Axel::tabelize().
  pluss::table::Thread tabelize() const;

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

private:
  friend class Registry;
  friend class SlotCastAccessor;
  explicit Slot(pluss::msharp::intrusive_ptr<detail::SlotImpl> impl);
  bool is_stream_compatible() const;
  void ensure_stream_compatible(std::string_view where = "Slot") const;
  Slot cast(pluss::table::ThreadCastTarget target,
            const pluss::table::ThreadCastOptions& options) const;
  const detail::SlotImpl& impl_ref_() const;
  detail::SlotImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::SlotImpl> impl_{nullptr};
};

class SlotCastAccessor {
public:
  explicit SlotCastAccessor(Slot base, pluss::table::ThreadCastOptions options = {});

  Slot series() const;
  Slot note() const;
  Slot timeline() const;
  Slot duration() const;

private:
  Slot base_{};
  pluss::table::ThreadCastOptions options_{};
};

class SlotNoteAccessor {
public:
  explicit SlotNoteAccessor(Slot base);

  Slot strip() const;
  Slot strip(std::string_view chars) const;
  Slot lstrip() const;
  Slot lstrip(std::string_view chars) const;
  Slot rstrip() const;
  Slot rstrip(std::string_view chars) const;
  Slot lower() const;
  Slot upper() const;
  Slot casefold() const;
  Slot titlecase() const;
  Slot length() const;
  Slot starts(std::string_view prefix) const;
  Slot ends(std::string_view suffix) const;
  Slot has(std::string_view needle,
           const pluss::table::TextOptions& options = {}) const;
  Slot count(std::string_view pattern) const;
  Slot count(std::string_view pattern,
             const pluss::table::TextOptions& options) const;
  Slot matches(std::string_view pattern, bool full = false) const;
  Slot fullmatch(std::string_view pattern) const;
  pluss::table::DataFrame findall(std::string_view pattern) const;
  pluss::table::DataFrame findall(std::string_view pattern,
                                  const pluss::table::TextOptions& options) const;
  pluss::table::DataFrame split(std::string_view delimiter,
                                std::int64_t limit = -1,
                                std::string prefix = "part") const;
  pluss::table::DataFrame split(std::string_view pattern,
                                const pluss::table::TextOptions& options) const;
  pluss::table::DataFrame partition(std::string_view separator,
                                    std::string prefix = "part") const;
  Slot pad(std::int64_t width,
           pluss::table::TextAlign align = pluss::table::TextAlign::Left,
           char fill = ' ') const;
  Slot chars(std::int64_t start,
             std::optional<std::int64_t> stop = std::nullopt,
             std::int64_t step = 1) const;
  Slot normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const;
  Slot encode(std::string_view encoding = "base64") const;
  pluss::table::DataFrame indicate(std::string_view delimiter = "|",
                                   std::string prefix = "") const;
  Slot replace(std::string_view from, std::string_view to) const;
  Slot replace(std::string_view from,
               std::string_view to,
               const pluss::table::TextOptions& options) const;

private:
  Slot base_{};
};

class SlotTimelineAccessor {
public:
  explicit SlotTimelineAccessor(Slot base);

  Slot isnat() const;
  Slot valid() const;
  Slot year() const;
  Slot month() const;
  Slot day() const;
  Slot hour() const;
  Slot minute() const;
  Slot second() const;
  Slot weekday() const;
  Slot yearday() const;
  Slot quarter() const;
  Slot week() const;
  Slot weekend() const;
  Slot month_start() const;
  Slot month_end() const;
  Slot leap() const;
  Slot weekday_name() const;
  Slot month_name() const;
  Slot fmt(std::string_view pattern) const;
  Slot iso() const;
  Slot epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const;
  Slot floor(std::string_view unit) const;
  Slot ceil(std::string_view unit) const;
  Slot round(std::string_view unit) const;
  Slot date() const;
  Slot time() const;
  Slot shift(std::int64_t periods, std::string_view freq) const;
  Slot period(std::string_view freq) const;
  Slot tz(std::string_view name) const;
  Slot localize(const pluss::table::TimelineLocalizeOptions& options = {}) const;
  Slot utc() const;

private:
  Slot base_{};
};

std::ostream& operator<<(std::ostream& os, const Slot& slot);

} // namespace pluss::scribble
