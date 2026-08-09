#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>

#include "common/StringViewNameRange.h"
#include "MSharp/IntrusivePtr.h"
#include "kindle/catalog.h"

namespace pluss::table {
class Atlas;
}
namespace pluss::bundle {
class Axel;
}

namespace pluss::kindle {

namespace detail {
class ArchiveImpl;
}

class ArchiveLabelAccessor;
class ArchiveKeyAccessor;
class ArchiveKeyAxisAccessor;
class ArchiveCastAccessor;

class Archive {
public:
  Archive();
  explicit Archive(std::vector<Catalog> catalogs,
                   std::optional<Stream> depth_labels = std::nullopt);
  Archive(const Archive& other);
  Archive(Archive&& other) noexcept;
  Archive& operator=(const Archive& other);
  Archive& operator=(Archive&& other) noexcept;
  ~Archive();

  static Archive kindelize(const pluss::table::Atlas& atlas,
                           const KindelizePolicy& policy = {});

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t depth() const;
  std::int64_t size() const;
  std::int64_t nkeys() const;
  std::int64_t minkeys() const;
  std::int64_t maxkeys() const;
  std::int64_t nrecords() const;
  std::int64_t minrecords() const;
  std::int64_t maxrecords() const;
  std::tuple<std::int64_t, std::int64_t, std::int64_t> shape() const;
  bool empty() const;

  ArchiveLabelAccessor label() const;
  Archive keys(std::span<const std::string_view> keys) const;
  Archive keys(std::initializer_list<std::string_view> keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Archive keys(const NameRange& keys) const;
  ArchiveKeyAxisAccessor keyaxis() const;
  Catalog catalog(std::int64_t i) const;
  Catalog dloc(std::int64_t i) const;
  Catalog loc(std::string_view label) const;
  Catalog loc(std::int64_t label) const;
  Catalog loc(const pluss::Timeline& label) const;
  const std::vector<Catalog>& catalogs() const;

  Archive head(std::int64_t n = 5) const;
  Archive tail(std::int64_t n = 5) const;
  Archive slice_depth(std::int64_t start, std::int64_t stop) const;
  Archive map_catalogs(const std::function<Catalog(const Catalog&)>& fn) const;
  Catalog aggregate_over_catalogs(const std::function<Catalog(const std::vector<Catalog>&)>& fn) const;

  bool has(std::string_view key) const;
  Archive drop(std::span<const std::string_view> keys) const;
  Archive drop(std::initializer_list<std::string_view> keys) const;
  Archive drop(const std::vector<std::string>& keys) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Archive drop(const NameRange& keys) const {
    return pluss::detail::with_name_span(keys, [&](auto names) { return drop(names); });
  }
  Archive rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Archive rename(std::string_view from, std::string_view to) const;
  Archive fillnull(double value) const;
  Archive fillnull(std::string_view value) const;
  Archive fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Archive fillnull(const pluss::Timeline& value) const;
  Archive fillnull(const pluss::TimeIndex& value) const;
  Archive dropnull() const;
  Archive ffill() const;
  Archive bfill() const;
  Archive isnull() const;
  Archive notnull() const;
  Archive invalid() const;
  ArchiveCastAccessor cast() const;
  ArchiveCastAccessor cast(const pluss::table::ThreadCastOptions& options) const;
  Archive cast(pluss::table::ThreadCastTarget target,
               const pluss::table::ThreadCastOptions& options = {}) const;
  Archive astype(pluss::codem::ScalarType dtype) const;
  Archive distinct(DistinctOptions options = {}) const;
  Archive freq(FreqOptions options = {}) const;
  Archive ndistinct(DistinctOptions options = {}) const;

  pluss::table::Atlas tabelize(const TabelizePolicy& policy = {}) const;
  pluss::table::Atlas to_table(const TabelizePolicy& policy = {}, bool render = false) const;
  pluss::bundle::Axel bundelize() const;

  std::string to_string() const;
  void show(std::ostream& os) const;

private:
  explicit Archive(pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl);
  const detail::ArchiveImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl_{nullptr};

  friend class ArchiveLabelAccessor;
  friend class ArchiveKeyAccessor;
  friend class ArchiveKeyAxisAccessor;
};

class ArchiveLabelAccessor {
public:
  explicit ArchiveLabelAccessor(Archive archive);

  bool has() const;
  Stream values() const;
  Archive set(const Stream& labels) const;
  Archive reset() const;
  Archive sort(bool ascending = true, bool nulls_last = true) const;
  Archive align(const Stream& labels) const;

private:
  Archive archive_{};
};

class ArchiveKeyAxisAccessor {
public:
  explicit ArchiveKeyAxisAccessor(Archive archive);

  Archive like(std::string_view needle, bool case_sensitive = true) const;
  Archive match(std::string_view pattern) const;
  Archive series() const;
  Archive note() const;
  Archive timeline() const;
  Archive timeindex() const;
  Archive prefix(std::string_view prefix) const;
  Archive suffix(std::string_view suffix) const;

private:
  Archive archive_{};
};

class ArchiveKeyAccessor {
public:
  ArchiveKeyAccessor(Archive archive, std::vector<std::string> keys);

  ArchiveCastAccessor cast() const;
  ArchiveCastAccessor cast(const pluss::table::ThreadCastOptions& options) const;
  Archive fillnull(double value) const;
  Archive fillnull(std::string_view value) const;
  Archive fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Archive fillnull(const pluss::Timeline& value) const;
  Archive fillnull(const pluss::TimeIndex& value) const;
  Archive dropnull() const;
  Archive ffill() const;
  Archive bfill() const;
  Archive isnull() const;
  Archive notnull() const;
  Archive invalid() const;
  Archive astype(pluss::codem::ScalarType dtype) const;
  Archive distinct(DistinctOptions options = {}) const;
  Archive freq(FreqOptions options = {}) const;
  Archive ndistinct(DistinctOptions options = {}) const;

private:
  template <class Fn>
  Archive map_selected_(Fn&& fn) const;

  Archive archive_{};
  std::vector<std::string> keys_{};

  friend class ArchiveCastAccessor;
};

class ArchiveCastAccessor {
public:
  ArchiveCastAccessor(Archive archive,
                      std::vector<std::string> keys,
                      pluss::table::ThreadCastOptions options = {});

  Archive series() const;
  Archive note() const;
  Archive timeline() const;
  Archive timeindex() const;

private:
  ArchiveKeyAccessor keysel_;
  pluss::table::ThreadCastOptions options_{};
};

template <class Fn>
Archive ArchiveKeyAccessor::map_selected_(Fn&& fn) const {
  return archive_.map_catalogs([&](const Catalog& catalog) { return std::forward<Fn>(fn)(catalog); });
}

template <pluss::detail::StringViewNameRange NameRange>
Archive Archive::keys(const NameRange& keys) const {
  return pluss::detail::with_name_span(keys, [&](auto names) { return this->keys(names); });
}

std::ostream& operator<<(std::ostream& os, const Archive& archive);

} // namespace pluss::kindle
