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

std::string payload_dtype_name(const Payload& payload) {
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
        dtype_name_(payload_dtype_name(payload_)) {
    validate_mask_size(validity_mask_, size_);
    validate_cases_size(cases_, size_);
    sync_fields();
  }

  void sync_fields() {
    fields_.clear();
    fields_.emplace("payload", payload_);
    fields_.emplace("size", size_);
    fields_.emplace("kind", static_cast<std::uint8_t>(kind_));
    fields_.emplace("dtype", dtype_name_);
    if (validity_mask_.has_value()) fields_.insert_or_assign("validity_mask", *validity_mask_);
    if (cases_.has_value()) fields_.insert_or_assign("cases", *cases_);
  }

  Payload payload_;
  std::optional<pluss::Series> validity_mask_{std::nullopt};
  std::optional<pluss::table::Thread> cases_{std::nullopt};
  std::int64_t size_{0};
  StreamKind kind_{StreamKind::Series};
  std::string dtype_name_{};
  std::unordered_map<std::string, FieldValue> fields_{};
  std::unordered_map<std::string, std::string> meta_{};
};
} // namespace detail

Stream::Stream(pluss::msharp::intrusive_ptr<detail::StreamImpl> impl) : impl_(std::move(impl)) {}

Stream::Stream() = default;

Stream::Stream(const pluss::Series& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const pluss::Note& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const pluss::Timeline& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const pluss::TimeIndex& data,
               std::optional<pluss::Series> validity_mask,
               std::optional<pluss::table::Thread> cases)
    : impl_(pluss::msharp::make_intrusive<detail::StreamImpl>(Payload{data}, std::move(validity_mask), std::move(cases))) {}

Stream::Stream(const Stream& other) = default;
Stream::Stream(Stream&& other) noexcept = default;
Stream& Stream::operator=(const Stream& other) = default;
Stream& Stream::operator=(Stream&& other) noexcept = default;
Stream::~Stream() = default;

Stream Stream::from_thread(const pluss::table::Thread& thread, std::optional<pluss::table::Thread> cases) {
  PLUSS_CHECK(thread.defined(), "Stream::from_thread requires a defined thread");
  switch (thread.kind()) {
    case pluss::table::ThreadKind::Series:
      return Stream(thread.as_series(), thread.validity_mask(), std::move(cases));
    case pluss::table::ThreadKind::Note:
      return Stream(thread.as_note(), thread.validity_mask(), std::move(cases));
    case pluss::table::ThreadKind::Timeline:
      return Stream(thread.as_timeline(), thread.validity_mask(), std::move(cases));
    case pluss::table::ThreadKind::TimeIndex:
      return Stream(thread.as_timeindex(), thread.validity_mask(), std::move(cases));
  }
  pluss::codem::throw_error("Stream::from_thread: unsupported thread kind");
}

bool Stream::defined() const noexcept { return static_cast<bool>(impl_); }
bool Stream::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Stream::size() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().size_;
}

StreamKind Stream::kind() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().kind_;
}

std::string Stream::dtype_name() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().dtype_name_;
}

Stream Stream::isnull() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(pluss::table::Thread(to_thread().isnull()), copy_cases(*this));
}

Stream Stream::notnull() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream::from_thread(pluss::table::Thread(to_thread().notnull()), copy_cases(*this));
}

Stream Stream::fillna(double value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [&](const pluss::table::Thread& t) { return t.fillna(value); });
}

Stream Stream::abs() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.abs(); });
}

Stream Stream::clip(double min_value, double max_value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [&](const pluss::table::Thread& t) { return t.clip(min_value, max_value); });
}

Stream Stream::log() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.log(); });
}

Stream Stream::exp() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.exp(); });
}

bool Stream::has_validity_mask() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().validity_mask_.has_value();
}

const std::optional<pluss::Series>& Stream::validity_mask() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().validity_mask_;
}

Stream Stream::with_validity_mask(const pluss::Series& mask) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_validity_mask_(mask);
  return out;
}

Stream& Stream::with_validity_mask_(const pluss::Series& mask) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  validate_mask_size(mask, w.size_);
  w.validity_mask_ = mask;
  w.sync_fields();
  return *this;
}

Stream Stream::clear_validity_mask() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.clear_validity_mask_();
  return out;
}

Stream& Stream::clear_validity_mask_() {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  w.sync_fields();
  return *this;
}

bool Stream::has_cases() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().cases_.has_value();
}

const std::optional<pluss::table::Thread>& Stream::cases() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().cases_;
}

Stream Stream::with_cases(const pluss::table::Thread& cases) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_cases_(cases);
  return out;
}

Stream& Stream::with_cases_(const pluss::table::Thread& cases) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  validate_cases_size(cases, w.size_);
  w.cases_ = cases;
  w.sync_fields();
  return *this;
}

Stream Stream::clear_cases() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.clear_cases_();
  return out;
}

Stream& Stream::clear_cases_() {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto& w = mut_impl_();
  w.cases_.reset();
  w.sync_fields();
  return *this;
}

bool Stream::has_meta(std::string_view key) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().meta_.find(std::string(key)) != impl_ref_().meta_.end();
}

std::optional<std::string> Stream::meta(std::string_view key) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto it = impl_ref_().meta_.find(std::string(key));
  if (it == impl_ref_().meta_.end()) return std::nullopt;
  return it->second;
}

const std::unordered_map<std::string, std::string>& Stream::meta() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return impl_ref_().meta_;
}

Stream Stream::with_meta(std::string key, std::string value) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_meta_(std::move(key), std::move(value));
  return out;
}

Stream& Stream::with_meta_(std::string key, std::string value) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  PLUSS_CHECK(!key.empty(), "Stream::with_meta_: key cannot be empty");
  mut_impl_().meta_[std::move(key)] = std::move(value);
  return *this;
}

Stream Stream::with_meta(std::initializer_list<std::pair<std::string_view, std::string_view>> items) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.with_meta_(items);
  return out;
}

Stream& Stream::with_meta_(std::initializer_list<std::pair<std::string_view, std::string_view>> items) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  for (const auto& [k, v] : items) {
    with_meta_(std::string(k), std::string(v));
  }
  return *this;
}

Stream Stream::drop_meta(std::string_view key) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  auto out = *this;
  out.drop_meta_(key);
  return out;
}

Stream& Stream::drop_meta_(std::string_view key) {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  mut_impl_().meta_.erase(std::string(key));
  return *this;
}

pluss::table::Thread Stream::to_thread() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return std::visit(
      [&](const auto& payload) {
        return pluss::table::Thread(payload, impl_ref_().validity_mask_);
      },
      impl_ref_().payload_);
}

pluss::table::Thread Stream::to_thread(std::int64_t start, std::int64_t stop) const {
  const auto t = to_thread();
  return t.slice(start, stop);
}

Stream Stream::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.head(n); });
}

Stream Stream::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.tail(n); });
}

Stream Stream::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.slice(start, stop); });
}

Stream Stream::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.take(indices); });
}

Stream Stream::filter(const pluss::Series& bool_mask) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return rowwise_stream_transform(*this, [&](const pluss::table::Thread& t) { return t.filter(bool_mask); });
}

Stream Stream::dropna() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto source = to_thread();
  const auto out = source.dropna();
  if (!has_cases()) return Stream::from_thread(out);
  const auto cases_out = cases()->filter(source.notnull());
  return Stream::from_thread(out, cases_out);
}

Stream Stream::ffill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.ffill(); });
}

Stream Stream::bfill() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [](const pluss::table::Thread& t) { return t.bfill(); });
}

Stream Stream::cast(pluss::table::ThreadCastTarget target,
                    std::optional<pluss::codem::ScalarType> series_dtype) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [&](const pluss::table::Thread& t) { return t.cast(target, series_dtype); });
}

Stream Stream::astype(pluss::codem::ScalarType dtype) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return unary_stream_same_cases(*this, [&](const pluss::table::Thread& t) { return t.astype(dtype); });
}

Stream Stream::sum() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream(to_thread().sum());
}

Stream Stream::mean() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream(to_thread().mean());
}

Stream Stream::min() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream(to_thread().min());
}

Stream Stream::max() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream(to_thread().max());
}

Stream Stream::var(bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream(to_thread().var(unbiased));
}

Stream Stream::std(bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return Stream(to_thread().std(unbiased));
}

const pluss::Series& Stream::as_series() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Series>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Series");
  return *p;
}

const pluss::Note& Stream::as_note() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Note>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Note");
  return *p;
}

const pluss::Timeline& Stream::as_timeline() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::Timeline>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not Timeline");
  return *p;
}

const pluss::TimeIndex& Stream::as_timeindex() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const auto* p = std::get_if<pluss::TimeIndex>(&impl_ref_().payload_);
  PLUSS_CHECK(p != nullptr, "Stream payload is not TimeIndex");
  return *p;
}

std::string Stream::to_string() const {
  return display::render(*this);
}

std::string Stream::to_json() const {
  return display::render(*this);
}

void Stream::show(std::ostream& os) const { display::print(os, *this); }

detail::StreamImpl& Stream::mut_impl_() {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  if (impl_.use_count() > 1) {
    auto cloned = pluss::msharp::make_intrusive<detail::StreamImpl>(
        impl_ref_().payload_, impl_ref_().validity_mask_, impl_ref_().cases_);
    cloned->meta_ = impl_ref_().meta_;
    cloned->sync_fields();
    impl_ = std::move(cloned);
  }
  return *impl_;
}

const detail::StreamImpl& Stream::impl_ref_() const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Stream& stream) {
  stream.show(os);
  return os;
}

namespace {
Stream wrap_binary_with_cases(const Stream& lhs,
                              const pluss::table::Thread& out,
                              const std::optional<pluss::table::Thread>& rhs_cases = std::nullopt) {
  if (lhs.has_cases()) return Stream::from_thread(out, *lhs.cases());
  if (rhs_cases.has_value()) return Stream::from_thread(out, rhs_cases);
  return Stream::from_thread(out);
}
} // namespace

#define PLUSS_SIGNAL_DEFINE_MATH_OP(OP)                                                     \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(rhs, lhs OP rhs.to_thread());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs.to_thread(), copy_cases(rhs)); \
  }

#define PLUSS_SIGNAL_DEFINE_CMP_OP(OP)                                                      \
  Stream operator OP(const Stream& lhs, double rhs) {                                       \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs);                             \
  }                                                                                          \
  Stream operator OP(double lhs, const Stream& rhs) {                                       \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(rhs, lhs OP rhs.to_thread());                             \
  }                                                                                          \
  Stream operator OP(const Stream& lhs, const Stream& rhs) {                                \
    PLUSS_CHECK(lhs.defined(), "Stream operator" #OP ": lhs undefined");                    \
    PLUSS_CHECK(rhs.defined(), "Stream operator" #OP ": rhs undefined");                    \
    return wrap_binary_with_cases(lhs, lhs.to_thread() OP rhs.to_thread(), copy_cases(rhs)); \
  }

PLUSS_SIGNAL_DEFINE_MATH_OP(+)
PLUSS_SIGNAL_DEFINE_MATH_OP(-)
PLUSS_SIGNAL_DEFINE_MATH_OP(*)
PLUSS_SIGNAL_DEFINE_MATH_OP(/)
PLUSS_SIGNAL_DEFINE_MATH_OP(%)

PLUSS_SIGNAL_DEFINE_CMP_OP(==)
PLUSS_SIGNAL_DEFINE_CMP_OP(!=)
PLUSS_SIGNAL_DEFINE_CMP_OP(<)
PLUSS_SIGNAL_DEFINE_CMP_OP(<=)
PLUSS_SIGNAL_DEFINE_CMP_OP(>)
PLUSS_SIGNAL_DEFINE_CMP_OP(>=)

#undef PLUSS_SIGNAL_DEFINE_MATH_OP
#undef PLUSS_SIGNAL_DEFINE_CMP_OP

} // namespace pluss::kindle
