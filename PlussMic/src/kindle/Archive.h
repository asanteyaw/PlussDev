#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
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
  bool empty() const;

  const std::optional<Stream>& depth_labels() const;
  Catalog catalog(std::int64_t i) const;
  const std::vector<Catalog>& catalogs() const;

  pluss::table::Atlas tabelize(const TabelizePolicy& policy = {}) const;

  std::string to_string() const;
  void show(std::ostream& os) const;

private:
  explicit Archive(pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl);
  const detail::ArchiveImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ArchiveImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Archive& archive);

} // namespace pluss::kindle
