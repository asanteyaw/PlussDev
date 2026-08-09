#include "bundle/Axel.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "CodeM/core/Errors.h"
#include "kindle/Archive.h"
#include "table/Atlas.h"

namespace pluss::bundle {

namespace {

std::vector<std::string> normalize_names(std::span<const std::string_view> keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (auto sv : keys) {
    std::string key(sv);
    PLUSS_CHECK(!key.empty(), "Axel: field cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Axel: duplicate field '" + key + "'");
    out.push_back(std::move(key));
  }
  return out;
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) e = s;
  return {s, e};
}

std::string json_escape(std::string_view in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

} // namespace

namespace detail {
class AxelImpl final : public pluss::msharp::RefCounted {
public:
  AxelImpl() = default;
  AxelImpl(std::vector<Registry> registries, std::optional<Slot> labels)
      : registries_(std::move(registries)), labels_(std::move(labels)) {
    if (labels_.has_value()) {
      PLUSS_CHECK(labels_->defined(), "Axel: labels slot undefined");
      if (labels_->size() != static_cast<std::int64_t>(registries_.size())) {
        throw pluss::Error("Axel: labels size must match depth");
      }
    }
  }

  std::vector<Registry> registries_{};
  std::optional<Slot> labels_{std::nullopt};
};
} // namespace detail


#include "bundle/ops/axel/AxelLifecycleAndOps.inc"
#include "bundle/ops/axel/AxelAccessors.inc"
#include "bundle/ops/axel/AxelRenderAndInternals.inc"

} // namespace pluss::bundle
