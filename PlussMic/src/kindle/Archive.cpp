#include "kindle/Archive.h"

#include <algorithm>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "CodeM/core/Errors.h"
#include "kindle/display/Display.h"
#include "table/Atlas.h"

namespace pluss::kindle {

namespace detail {
class ArchiveImpl final : public pluss::msharp::RefCounted {
public:
  ArchiveImpl() = default;
  ArchiveImpl(std::vector<Catalog> catalogs, std::optional<Stream> depth_labels)
      : catalogs_(std::move(catalogs)), depth_labels_(std::move(depth_labels)) {
    if (depth_labels_.has_value()) {
      PLUSS_CHECK(depth_labels_->defined(), "Archive: depth labels stream is undefined");
      PLUSS_CHECK(depth_labels_->size() == static_cast<std::int64_t>(catalogs_.size()),
                  "Archive: depth labels length must match catalog depth");
    }
  }

  std::vector<Catalog> catalogs_{};
  std::optional<Stream> depth_labels_{std::nullopt};
};
} // namespace detail


#include "kindle/ops/archive/ArchiveLifecycleAndOps.inc"
#include "kindle/ops/archive/ArchiveInternals.inc"

} // namespace pluss::kindle
