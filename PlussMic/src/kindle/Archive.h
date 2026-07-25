#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "kindle/catalog.h"

namespace pluss::table {
class Atlas;
}

namespace pluss::kindle {

namespace detail {
class ArchiveImpl;
}

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

  const std::optional<Stream>& depth_labels() const;
  Catalog catalog(std::int64_t i) const;
  Catalog dloc(std::int64_t i) const;
  Catalog loc(std::string_view label) const;
  Catalog loc(std::int64_t label) const;
  Catalog loc(const pluss::Timeline& label) const;
  const std::vector<Catalog>& catalogs() const;

  Archive head(std::int64_t n = 5) const;
  Archive tail(std::int64_t n = 5) const;
  Archive slice_depth(std::int64_t start, std::int64_t stop) const;
  Archive sort_depth(bool ascending = true, bool nulls_last = true) const;
  Archive relabel_depth(const Stream& labels) const;
  Archive align_depth(const Stream& labels) const;
  Archive map_catalogs(const std::function<Catalog(const Catalog&)>& fn) const;
  Catalog aggregate_over_catalogs(const std::function<Catalog(const std::vector<Catalog>&)>& fn) const;

  bool has(std::string_view key) const;
  Archive select(std::initializer_list<std::string_view> keys) const;
  Archive select(const std::vector<std::string>& keys) const;
  Archive drop(std::initializer_list<std::string_view> keys) const;
  Archive drop(const std::vector<std::string>& keys) const;
  Archive rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Archive rename(std::string_view from, std::string_view to) const;
  Archive fillnull(double value, std::initializer_list<std::string_view> keys = {}) const;
  Archive fillnull(std::string_view value, std::initializer_list<std::string_view> keys = {}) const;
  Archive fillnull(const char* value, std::initializer_list<std::string_view> keys = {}) const {
    return fillnull(std::string_view{value != nullptr ? value : ""}, keys);
  }
  Archive fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const;
  Archive fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const;
  Archive dropnull(std::initializer_list<std::string_view> keys = {}) const;
  Archive ffill(std::initializer_list<std::string_view> keys = {}) const;
  Archive bfill(std::initializer_list<std::string_view> keys = {}) const;
  Archive isnull(std::initializer_list<std::string_view> keys = {}) const;
  Archive notnull(std::initializer_list<std::string_view> keys = {}) const;
  Archive invalid(std::initializer_list<std::string_view> keys = {}) const;
  Archive cast(std::initializer_list<std::string_view> keys,
               pluss::table::ThreadCastTarget target,
               const pluss::table::ThreadCastOptions& options = {}) const;
  Archive astype(std::initializer_list<std::string_view> keys,
                 pluss::codem::ScalarType dtype) const;
  Archive distinct(DistinctOptions options = {}) const;
  Archive distinct(std::initializer_list<std::string_view> keys, DistinctOptions options = {}) const;
  Archive freq(FreqOptions options = {}) const;
  Archive freq(std::initializer_list<std::string_view> keys, FreqOptions options = {}) const;
  Archive ndistinct(DistinctOptions options = {}) const;
  Archive ndistinct(std::initializer_list<std::string_view> keys, DistinctOptions options = {}) const;

  pluss::table::Atlas tabelize(const TabelizePolicy& policy = {}) const;
  pluss::table::Atlas to_table(const TabelizePolicy& policy = {}, bool render = false) const;

  std::string to_string() const;
  void show(std::ostream& os) const;

private:
  explicit Archive(pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl);
  const detail::ArchiveImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Archive& archive);

} // namespace pluss::kindle
