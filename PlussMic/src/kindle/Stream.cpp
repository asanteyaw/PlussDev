#include "kindle/Stream.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "kindle/display/Display.h"

namespace pluss::kindle {

namespace {
using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::TimeIndex>;

StreamKind to_kind(const Payload& payload) {
  return std::visit(
      [](const auto& v) -> StreamKind {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, pluss::Series>) return StreamKind::Series;
        if constexpr (std::is_same_v<T, pluss::Note>) return StreamKind::Note;
        if constexpr (std::is_same_v<T, pluss::Timeline>) return StreamKind::Timeline;
        return StreamKind::TimeIndex;
      },
      payload);
}

std::int64_t payload_size(const Payload& payload) {
  return std::visit(
      [](const auto& v) -> std::int64_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, pluss::TimeIndex>) {
          return v.size();
        } else {
          return v.numel();
        }
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
        return std::string("TimeIndex");
      },
      payload);
}

void validate_mask_size(const std::optional<pluss::Series>& mask, std::int64_t n) {
  if (!mask.has_value()) return;
  PLUSS_CHECK(mask->defined(), "Stream: validity mask is undefined");
  PLUSS_CHECK(mask->numel() == n, "Stream: validity mask length must match payload length");
}

void validate_cases_size(const std::optional<pluss::table::Thread>& cases, std::int64_t n) {
  if (!cases.has_value()) return;
  PLUSS_CHECK(cases->defined(), "Stream: cases axis is undefined");
  PLUSS_CHECK(cases->size() == n, "Stream: cases axis length must match payload length");
}

std::optional<pluss::table::Thread> copy_cases(const Stream& stream) {
  if (!stream.has_cases()) return std::nullopt;
  return *stream.cases();
}

template <typename Op>
Stream unary_stream_same_cases(const Stream& stream, Op&& op) {
  const auto out = std::forward<Op>(op)(stream.to_thread());
  return Stream::from_thread(out, copy_cases(stream));
}

template <typename Op>
Stream rowwise_stream_transform(const Stream& stream, Op&& op) {
  const auto source = stream.to_thread();
  const auto out = std::forward<Op>(op)(source);
  if (!stream.has_cases()) return Stream::from_thread(out);
  const auto cases_out = std::forward<Op>(op)(*stream.cases());
  return Stream::from_thread(out, cases_out);
}
} // namespace

namespace detail {
class StreamImpl final : public pluss::msharp::RefCounted {
public:
  using FieldValue = std::variant<
      Payload,
      pluss::Series,
      pluss::table::Thread,
      std::string,
      std::int64_t,
      std::uint8_t>;

  explicit StreamImpl(Payload payload,
                      std::optional<pluss::Series> validity_mask,
                      std::optional<pluss::table::Thread> cases)
      : payload_(std::move(payload)),
        validity_mask_(std::move(validity_mask)),
        cases_(std::move(cases)),
        size_(payload_size(payload_)),
        kind_(to_kind(payload_)),
        dtype_(payload_dtype(payload_)) {
    validate_mask_size(validity_mask_, size_);
    validate_cases_size(cases_, size_);
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
  std::optional<pluss::table::Thread> cases_{std::nullopt};
  std::int64_t size_{0};
  StreamKind kind_{StreamKind::Series};
  std::string dtype_{};
  std::unordered_map<std::string, FieldValue> fields_{};
  std::unordered_map<std::string, std::string> meta_{};
};
} // namespace detail


#include "kindle/ops/stream/StreamLifecycleAndMeta.inc"
#include "kindle/ops/stream/StreamCoreOps.inc"
#include "kindle/ops/stream/StreamOperators.inc"

} // namespace pluss::kindle
