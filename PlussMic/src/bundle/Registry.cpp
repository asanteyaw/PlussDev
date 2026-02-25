#include "bundle/registry.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <numeric>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include "CodeM/core/Errors.h"
#include "kindle/catalog.h"
#include <fmt/format.h>

namespace pluss::bundle {

namespace {

std::vector<std::string> normalize_names(std::initializer_list<std::string_view> keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (auto sv : keys) {
    std::string key(sv);
    PLUSS_CHECK(!key.empty(), "Registry: key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Registry: duplicate key '" + key + "'");
    out.push_back(std::move(key));
  }
  return out;
}

std::vector<std::string> normalize_names(const std::vector<std::string>& keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (const auto& key : keys) {
    PLUSS_CHECK(!key.empty(), "Registry: key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Registry: duplicate key '" + key + "'");
    out.push_back(key);
  }
  return out;
}

std::string trim_copy(std::string_view in) {
  std::size_t start = 0;
  std::size_t end = in.size();
  while (start < end && std::isspace(static_cast<unsigned char>(in[start]))) ++start;
  while (end > start && std::isspace(static_cast<unsigned char>(in[end - 1]))) --end;
  return std::string(in.substr(start, end - start));
}

std::string lowercase_copy(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  for (char c : in) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return out;
}

Registry bridge_registry_catalog(const Registry& registry,
                                 std::string_view where,
                                 const std::function<pluss::kindle::Catalog(const pluss::kindle::Catalog&)>& fn) {
  registry.ensure_stream_compatible(where);
  const auto catalog = registry.kindelize();
  return Registry::from_kindle(fn(catalog));
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
class RegistryImpl final : public pluss::msharp::RefCounted {
public:
  RegistryImpl() = default;
  explicit RegistryImpl(std::vector<std::pair<std::string, Slot>> fields) {
    keys_.reserve(fields.size());
    slots_.reserve(fields.size());
    for (auto& [k, s] : fields) {
      PLUSS_CHECK(!k.empty(), "Registry: key cannot be empty");
      PLUSS_CHECK(s.defined(), "Registry: slot is undefined");
      PLUSS_CHECK(name_to_pos_.find(k) == name_to_pos_.end(), "Registry: duplicate key '" + k + "'");
      const auto idx = static_cast<std::int64_t>(keys_.size());
      name_to_pos_.emplace(k, idx);
      keys_.push_back(std::move(k));
      slots_.push_back(std::move(s));
    }
  }

  std::vector<std::string> keys_{};
  std::vector<Slot> slots_{};
  std::unordered_map<std::string, std::int64_t> name_to_pos_{};
};
} // namespace detail


#include "bundle/ops/registry/RegistryLifecycleAndOps.inc"
#include "bundle/ops/registry/RegistryInteropAndRender.inc"

} // namespace pluss::bundle
