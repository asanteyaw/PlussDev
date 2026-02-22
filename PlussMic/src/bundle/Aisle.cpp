#include "bundle/Aisle.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <utility>

#include "CodeM/core/Errors.h"
#include "kindle/Archive.h"

namespace pluss::bundle {

namespace {

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) e = s;
  return {s, e};
}

} // namespace

namespace detail {
class AisleImpl final : public pluss::msharp::RefCounted {
public:
  AisleImpl() = default;
  AisleImpl(std::vector<Registry> registries, std::optional<Slot> labels)
      : registries_(std::move(registries)), labels_(std::move(labels)) {
    if (labels_.has_value()) {
      PLUSS_CHECK(labels_->defined(), "Aisle: labels slot undefined");
      PLUSS_CHECK(labels_->size() == static_cast<std::int64_t>(registries_.size()),
                  "Aisle: labels size must match depth");
    }
  }

  std::vector<Registry> registries_{};
  std::optional<Slot> labels_{std::nullopt};
};
} // namespace detail


#include "bundle/ops/aisle/AisleLifecycleAndOps.inc"
#include "bundle/ops/aisle/AisleRenderAndInternals.inc"

} // namespace pluss::bundle
