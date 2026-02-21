#include "bundle/Aisle.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <utility>

#include "CodeM/core/Errors.h"
#include "signal/Archive.h"

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

Aisle::Aisle() = default;
Aisle::Aisle(std::vector<Registry> registries, std::optional<Slot> labels)
    : impl_(pluss::msharp::make_intrusive<detail::AisleImpl>(std::move(registries), std::move(labels))) {}
Aisle::Aisle(const Aisle& other) = default;
Aisle::Aisle(Aisle&& other) noexcept = default;
Aisle& Aisle::operator=(const Aisle& other) = default;
Aisle& Aisle::operator=(Aisle&& other) noexcept = default;
Aisle::~Aisle() = default;

Aisle::Aisle(pluss::msharp::intrusive_ptr<detail::AisleImpl> impl) : impl_(std::move(impl)) {}

bool Aisle::defined() const noexcept { return static_cast<bool>(impl_); }
bool Aisle::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Aisle::depth() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().registries_.size());
}

bool Aisle::empty() const { return depth() == 0; }

Registry Aisle::registry(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(i >= 0 && i < depth(), "Aisle::registry: index out of bounds");
  return impl_ref_().registries_[static_cast<std::size_t>(i)];
}

const std::vector<Registry>& Aisle::registries() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return impl_ref_().registries_;
}

const std::optional<Slot>& Aisle::labels() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return impl_ref_().labels_;
}

Aisle Aisle::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice(0, clipped);
}

Aisle Aisle::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, depth());
  return slice(depth() - clipped, depth());
}

Aisle Aisle::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  const auto [s, e] = normalize_slice(start, stop, depth());
  std::vector<Registry> out_regs;
  out_regs.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) out_regs.push_back(registry(i));

  std::optional<Slot> out_labels = std::nullopt;
  if (labels().has_value()) out_labels = labels()->slice(s, e);
  return Aisle(std::move(out_regs), std::move(out_labels));
}

Aisle Aisle::map(const std::function<Registry(const Registry&)>& fn) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Aisle::map: fn is empty");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    auto r = fn(registry(i));
    PLUSS_CHECK(r.defined(), "Aisle::map: function returned undefined Registry");
    out.push_back(std::move(r));
  }
  return Aisle(std::move(out), labels());
}

Aisle Aisle::select(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    out.push_back(registry(i).select(keys));
  }
  return Aisle(std::move(out), labels());
}

Aisle Aisle::apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Aisle::apply_slot: fn is empty");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    auto r = registry(i);
    if (r.has(key)) {
      r = r.apply(key, fn);
    }
    out.push_back(std::move(r));
  }
  return Aisle(std::move(out), labels());
}

Aisle Aisle::concat_depth(const Aisle& other) const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  PLUSS_CHECK(other.defined(), "Aisle::concat_depth: other aisle undefined");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(depth() + other.depth()));
  for (std::int64_t i = 0; i < depth(); ++i) out.push_back(registry(i));
  for (std::int64_t i = 0; i < other.depth(); ++i) out.push_back(other.registry(i));

  std::optional<Slot> merged_labels = std::nullopt;
  if (labels().has_value() && other.labels().has_value()) {
    merged_labels = labels()->concat(*other.labels());
  }
  return Aisle(std::move(out), std::move(merged_labels));
}

pluss::signal::Archive Aisle::signalize() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  std::vector<pluss::signal::Catalog> out;
  out.reserve(static_cast<std::size_t>(depth()));
  for (std::int64_t i = 0; i < depth(); ++i) {
    out.push_back(registry(i).signalize());
  }
  std::optional<pluss::signal::Stream> depth_labels = std::nullopt;
  if (labels().has_value()) depth_labels = labels()->signalize();
  return pluss::signal::Archive(std::move(out), std::move(depth_labels));
}

Aisle Aisle::from_signal(const pluss::signal::Archive& archive) {
  PLUSS_CHECK(archive.defined(), "Aisle::from_signal: archive undefined");
  std::vector<Registry> out;
  out.reserve(static_cast<std::size_t>(archive.depth()));
  for (std::int64_t i = 0; i < archive.depth(); ++i) {
    out.push_back(Registry::from_signal(archive.catalog(i)));
  }
  std::optional<Slot> labels = std::nullopt;
  if (archive.depth_labels().has_value()) labels = Slot::from_signal(*archive.depth_labels());
  return Aisle(std::move(out), std::move(labels));
}

std::string Aisle::to_string() const { return to_json(); }

std::string Aisle::to_json() const {
  if (!defined()) return "{ \"_type\": \"Aisle\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Aisle\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"depth\": " << depth() << ",\n";
  oss << "  \"labels\": " << (labels().has_value() ? "true" : "false") << ",\n";
  oss << "  \"registries\": [\n";
  for (std::int64_t i = 0; i < depth(); ++i) {
    oss << registry(i).to_json();
    if (i + 1 < depth()) oss << ",";
    oss << "\n";
  }
  oss << "  ]\n";
  oss << "}";
  return oss.str();
}

void Aisle::show(std::ostream& os) const { os << to_string(); }

const detail::AisleImpl& Aisle::impl_ref_() const {
  PLUSS_CHECK(defined(), "Aisle: undefined handle");
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Aisle& aisle) {
  aisle.show(os);
  return os;
}

} // namespace pluss::bundle
