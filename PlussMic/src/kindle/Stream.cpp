#include "kindle/Stream.h"

#include <cstring>

#include <algorithm>
#include <unordered_set>
#include <cmath>
#include <ostream>
#include <sstream>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>


#include "CodeM/core/Errors.h"
#include "MSharp/Parallel.h"
#include "scribble/Slot.h"
#include "kindle/display/Display.h"
#include "pluss/codeh/timeline_factory.h"
#include <pluss/tils/kit/output.h>

namespace pluss::kindle {

namespace {
using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::Duration>;

StreamKind to_kind(const Payload& payload) {
  return std::visit(
      [](const auto& v) -> StreamKind {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, pluss::Series>) return StreamKind::Series;
        if constexpr (std::is_same_v<T, pluss::Note>) return StreamKind::Note;
        if constexpr (std::is_same_v<T, pluss::Timeline>) return StreamKind::Timeline;
        return StreamKind::Duration;
      },
      payload);
}

std::int64_t payload_size(const Payload& payload) {
  return std::visit(
      [](const auto& v) -> std::int64_t {
        return v.numel();
      },
      payload);
}

std::string payload_dtype(const Payload& payload) {
  return std::visit(
      [](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, pluss::Series>) return std::string("Series[") + std::string(v.dtype().name) + "]";
        if constexpr (std::is_same_v<T, pluss::Note>) return std::string("Note");
        if constexpr (std::is_same_v<T, pluss::Timeline>) return std::string("Timeline");
        return std::string("duration64[ns]");
      },
      payload);
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

void validate_mask_size(const std::optional<pluss::Series>& mask, std::int64_t n) {
  if (!mask.has_value()) return;
  PLUSS_CHECK(mask->defined(), "Stream: validity mask is undefined");
  PLUSS_CHECK(mask->numel() == n, "Stream: validity mask length must match payload length");
}

void validate_label_size(const std::optional<pluss::table::Shelf>& label, std::int64_t n) {
  if (!label.has_value()) return;
  PLUSS_CHECK(label->defined(), "Stream: label axis is undefined");
  PLUSS_CHECK(label->size() == n, "Stream: label axis length must match payload length");
}

std::optional<pluss::table::Shelf> copy_label(const Stream& stream) {
  if (!stream.has_label()) return std::nullopt;
  return *stream.label();
}

std::optional<pluss::table::Shelf> shelf_output_labels(const pluss::table::Shelf& shelf) {
  if (shelf.label().has()) return shelf.label().values();
  return std::nullopt;
}

// A stream derived from `source` by an operation that keeps the meaning of the values (clip, fill, cast, slice, filter, arithmetic)
// keeps the source's metadata: "unit" and "sensor" still describe it. Flags (isnull, comparisons) and reductions do not use this.
Stream carry_meta(Stream out, const Stream& source) {
  for (const auto& [key, value] : source.meta()) out.with_meta_(key, value);
  return out;
}

template <typename Op>
Stream unary_stream_same_label(const Stream& stream, Op&& op) {
  const auto out = std::forward<Op>(op)(stream.to_shelf());
  return carry_meta(Stream::from_shelf(out, copy_label(stream)), stream);
}

template <typename Op>
Stream shelf_accessor_stream_result(const Stream& stream, Op&& op) {
  const auto out = std::forward<Op>(op)(stream.to_shelf());
  auto labels = shelf_output_labels(out);
  if (!labels.has_value() && out.size() == stream.size() && stream.has_label()) {
    labels = copy_label(stream);
  }
  return Stream::from_shelf(out, std::move(labels));
}

template <typename Op>
Stream rowwise_stream_transform(const Stream& stream, Op&& op) {
  const auto source = stream.to_shelf();
  const auto out = std::forward<Op>(op)(source);
  if (!stream.has_label()) return carry_meta(Stream::from_shelf(out), stream);
  const auto label_out = std::forward<Op>(op)(*stream.label());
  return carry_meta(Stream::from_shelf(out, label_out), stream);
}
} // namespace

namespace detail {
class StreamImpl final : public pluss::msharp::RefCounted {
public:
  using FieldValue = std::variant<
      Payload,
      pluss::Series,
      pluss::table::Shelf,
      std::string,
      std::int64_t,
      std::uint8_t>;

  explicit StreamImpl(Payload payload,
                      std::optional<pluss::Series> validity_mask,
                      std::optional<pluss::table::Shelf> cases)
      : payload_(std::move(payload)),
        validity_mask_(std::move(validity_mask)),
        cases_(std::move(cases)),
        size_(payload_size(payload_)),
        kind_(to_kind(payload_)),
        dtype_(payload_dtype(payload_)) {
    validate_mask_size(validity_mask_, size_);
    validate_label_size(cases_, size_);
    sync_fields();
  }

  void sync_fields() {
    fields_.clear();
    fields_.emplace("payload", payload_);
    fields_.emplace("size", size_);
    fields_.emplace("kind", static_cast<std::uint8_t>(kind_));
    fields_.emplace("dtype", dtype_);
    if (validity_mask_.has_value()) fields_.insert_or_assign("validity_mask", *validity_mask_);
    if (cases_.has_value()) fields_.insert_or_assign("cases", *cases_);
  }

  Payload payload_;
  std::optional<pluss::Series> validity_mask_{std::nullopt};
  std::optional<pluss::table::Shelf> cases_{std::nullopt};
  std::int64_t size_{0};
  StreamKind kind_{StreamKind::Series};
  std::string dtype_{};
  std::unordered_map<std::string, FieldValue> fields_{};
  std::unordered_map<std::string, std::string> meta_{};
};
} // namespace detail


#include "kindle/ops/stream/StreamLifecycleAndMeta.inc"
#include "kindle/ops/stream/StreamCoreOps.inc"
#include "kindle/ops/stream/StreamGather.inc"
#include "kindle/ops/stream/StreamReduceGroups.inc"
#include "kindle/ops/stream/StreamAccessors.inc"
#include "kindle/ops/stream/StreamOperators.inc"
#include "kindle/ops/stream/StreamRaggedBinary.inc"

} // namespace pluss::kindle
