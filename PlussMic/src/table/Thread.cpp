#include "table/Thread.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace detail {

using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::TimeIndex>;

class ThreadImpl final : public pluss::msharp::RefCounted {
public:
  ThreadImpl(Payload payload, std::optional<pluss::Series> validity_mask);

  Payload payload_;
  ThreadKind kind_{ThreadKind::Series};
  std::int64_t size_{0};
  std::string dtype_name_;
  std::optional<pluss::Series> validity_mask_;
};

} // namespace detail

namespace {

using Payload = detail::Payload;

std::int64_t normalize_index(std::int64_t idx, std::int64_t size) {
  std::int64_t out = idx;
  if (out < 0) {
    out += size;
  }
  PLUSS_CHECK(out >= 0 && out < size, "Thread: index out of bounds");
  return out;
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) {
    e = s;
  }
  return {s, e};
}

pluss::Series bool_series_zeros(std::int64_t n) {
  PLUSS_CHECK(n >= 0, "Thread: bool mask length must be non-negative");
  return pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
}

pluss::Series bool_series_from_u8(const std::vector<std::uint8_t>& values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    p[i] = values[static_cast<std::size_t>(i)] != 0;
  }
  return out;
}

pluss::Series int64_index_series(const std::vector<std::int64_t>& values) {
  auto out = pluss::Series::empty({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return out;
}

std::vector<bool> bool_vec_from_series(const pluss::Series& s, std::int64_t expected_size, std::string_view where) {
  PLUSS_CHECK(s.defined(), std::string(where) + ": bool mask is undefined");
  PLUSS_CHECK(s.dim() == 1, std::string(where) + ": bool mask must be 1D");
  PLUSS_CHECK(s.numel() == expected_size, std::string(where) + ": bool mask length mismatch");
  PLUSS_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool, std::string(where) + ": bool mask dtype must be Bool");
  return s.to_vec<bool>();
}

void validate_validity_mask(const std::optional<pluss::Series>& mask, std::int64_t size) {
  if (!mask.has_value()) {
    return;
  }
  const auto& m = *mask;
  PLUSS_CHECK(m.defined(), "Thread: validity mask is undefined");
  PLUSS_CHECK(m.dim() == 1, "Thread: validity mask must be 1D");
  PLUSS_CHECK(m.scalar_type() == pluss::codem::ScalarType::Bool, "Thread: validity mask must use Bool dtype");
  PLUSS_CHECK(m.numel() == size, "Thread: validity mask length mismatch");
}

std::int64_t infer_size(const pluss::Series& s) {
  PLUSS_CHECK(s.defined(), "Thread: Series payload is undefined");
  PLUSS_CHECK(s.dim() == 1, "Thread: Series payload must be strictly 1D");
  return s.numel();
}

std::int64_t infer_size(const pluss::Note& n) {
  PLUSS_CHECK(n.defined(), "Thread: Note payload is undefined");
  PLUSS_CHECK(n.dim() == 1, "Thread: Note payload must be strictly 1D");
  return n.numel();
}

std::int64_t infer_size(const pluss::Timeline& t) {
  PLUSS_CHECK(t.defined(), "Thread: Timeline payload is undefined");
  PLUSS_CHECK(t.dim() == 1, "Thread: Timeline payload must be strictly 1D");
  return t.numel();
}

std::int64_t infer_size(const pluss::TimeIndex& ti) {
  PLUSS_CHECK(ti.timeline().defined(), "Thread: TimeIndex payload is undefined");
  PLUSS_CHECK(ti.timeline().dim() == 1, "Thread: TimeIndex payload must be strictly 1D");
  return ti.size();
}

ThreadKind infer_kind(const Payload& payload) {
  if (std::holds_alternative<pluss::Series>(payload)) return ThreadKind::Series;
  if (std::holds_alternative<pluss::Note>(payload)) return ThreadKind::Note;
  if (std::holds_alternative<pluss::Timeline>(payload)) return ThreadKind::Timeline;
  return ThreadKind::TimeIndex;
}

std::int64_t infer_size(const Payload& payload) {
  return std::visit([](const auto& v) { return infer_size(v); }, payload);
}

bool is_numeric_series_type(const pluss::Series& s) {
  const auto dt = s.dtype();
  return dt.scalar == pluss::codem::ScalarType::Bool || dt.is_integral() || dt.is_floating();
}

std::string infer_dtype_name(const Payload& payload) {
  return std::visit([](const auto& v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, pluss::Series>) {
      return std::string(v.dtype().name);
    } else if constexpr (std::is_same_v<T, pluss::Note>) {
      return static_cast<const pluss::coden::NoteBase&>(v).is_bytes() ? "bytes" : "utf8";
    } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
      return std::string("datetime64[ns, ") + std::string(v.tz()) + "]";
    } else {
      return "timeindex";
    }
  }, payload);
}

pluss::msharp::intrusive_ptr<detail::ThreadImpl> build_impl(
    Payload payload,
    std::optional<pluss::Series> validity_mask);

std::vector<std::int64_t> true_indices_from_mask(const pluss::Series& mask, std::int64_t expected_size) {
  const auto keep = bool_vec_from_series(mask, expected_size, "Thread::filter");
  std::vector<std::int64_t> out;
  out.reserve(static_cast<std::size_t>(expected_size));
  for (std::int64_t i = 0; i < expected_size; ++i) {
    if (keep[static_cast<std::size_t>(i)]) {
      out.push_back(i);
    }
  }
  return out;
}

std::vector<std::string> note_take_strings(const pluss::Note& note, std::span<const std::int64_t> indices, std::int64_t size) {
  std::vector<std::string> out;
  out.reserve(indices.size());
  for (const auto raw_idx : indices) {
    const auto idx = normalize_index(raw_idx, size);
    out.emplace_back(note.at(idx));
  }
  return out;
}

pluss::Timeline timeline_take(const pluss::Timeline& t, std::span<const std::int64_t> indices, std::int64_t size) {
  const auto ticks = t.to_epoch_nanos_vector();
  std::vector<std::int64_t> out;
  out.reserve(indices.size());
  for (const auto raw_idx : indices) {
    const auto idx = normalize_index(raw_idx, size);
    out.push_back(ticks[static_cast<std::size_t>(idx)]);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(out.size())};
  auto result = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(out.data(), out.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  result.unsafe_tz_name() = std::string(t.tz());
  return result;
}

} // namespace

detail::ThreadImpl::ThreadImpl(Payload payload, std::optional<pluss::Series> validity_mask)
    : payload_(std::move(payload)),
      kind_(infer_kind(payload_)),
      size_(infer_size(payload_)),
      dtype_name_(infer_dtype_name(payload_)),
      validity_mask_(std::move(validity_mask)) {
  validate_validity_mask(validity_mask_, size_);
}

namespace {

pluss::msharp::intrusive_ptr<detail::ThreadImpl> build_impl(
    Payload payload,
    std::optional<pluss::Series> validity_mask) {
  return pluss::msharp::make_intrusive<detail::ThreadImpl>(std::move(payload), std::move(validity_mask));
}

pluss::msharp::intrusive_ptr<detail::ThreadImpl> clone_impl(const detail::ThreadImpl& src) {
  return build_impl(src.payload_, src.validity_mask_);
}

pluss::Series scalar_like_column(const pluss::Series& reference, double value) {
  return pluss::Series::full({reference.numel()}, value, reference.scalar_type());
}

pluss::Series apply_math_series_series(
    const pluss::Series& lhs,
    const pluss::Series& rhs,
    ThreadBinaryMathOp op) {
  auto floor_divide = [](const pluss::Series& a, const pluss::Series& b) {
    auto af = a.to(pluss::codem::ScalarType::Float64);
    auto bf = b.to(pluss::codem::ScalarType::Float64);
    return af.true_divide(bf).floor();
  };
  auto safe_pow = [](const pluss::Series& a, const pluss::Series& b) {
    auto af = a.to(pluss::codem::ScalarType::Float64);
    auto bf = b.to(pluss::codem::ScalarType::Float64);
    PLUSS_CHECK(af.numel() == bf.numel(), "Thread::pow: operand length mismatch");
    const auto av = af.to_vec<double>();
    const auto bv = bf.to_vec<double>();
    auto out = pluss::Series::empty({af.numel()}, pluss::codem::ScalarType::Float64);
    auto* p = reinterpret_cast<double*>(out.raw_data());
    for (std::int64_t i = 0; i < af.numel(); ++i) {
      p[i] = std::pow(av[static_cast<std::size_t>(i)], bv[static_cast<std::size_t>(i)]);
    }
    return out;
  };

  switch (op) {
    case ThreadBinaryMathOp::Add:
      return lhs.add(rhs);
    case ThreadBinaryMathOp::Sub:
      return lhs.sub(rhs);
    case ThreadBinaryMathOp::Mul:
      return lhs.mul(rhs);
    case ThreadBinaryMathOp::Div:
      return lhs.div(rhs);
    case ThreadBinaryMathOp::TrueDiv:
      return lhs.true_divide(rhs);
    case ThreadBinaryMathOp::FloorDiv:
      return floor_divide(lhs, rhs);
    case ThreadBinaryMathOp::Mod:
      return lhs.remainder(rhs);
    case ThreadBinaryMathOp::Pow:
      return safe_pow(lhs, rhs);
    default:
      PLUSS_FAIL("Thread: unsupported numeric binary op");
  }
}

pluss::Series apply_math_series_scalar(
    const pluss::Series& lhs,
    double rhs,
    ThreadBinaryMathOp op,
    bool reverse) {
  auto floor_divide_scalar = [&](double scalar, bool rev) {
    const auto s = scalar_like_column(lhs, scalar);
    return rev ? apply_math_series_series(s, lhs, ThreadBinaryMathOp::FloorDiv)
               : apply_math_series_series(lhs, s, ThreadBinaryMathOp::FloorDiv);
  };

  if (!reverse) {
    switch (op) {
      case ThreadBinaryMathOp::Add:
        return lhs.add(rhs);
      case ThreadBinaryMathOp::Sub:
        return lhs.sub(rhs);
      case ThreadBinaryMathOp::Mul:
        return lhs.mul(rhs);
      case ThreadBinaryMathOp::Div:
        return lhs.div(rhs);
      case ThreadBinaryMathOp::TrueDiv:
        return lhs.true_divide(rhs);
      case ThreadBinaryMathOp::FloorDiv:
        return floor_divide_scalar(rhs, false);
      case ThreadBinaryMathOp::Mod:
        return lhs.remainder(rhs);
      case ThreadBinaryMathOp::Pow:
        return apply_math_series_series(lhs, scalar_like_column(lhs, rhs), ThreadBinaryMathOp::Pow);
      default:
        PLUSS_FAIL("Thread: unsupported numeric binary op");
    }
  }

  const auto scalar = scalar_like_column(lhs, rhs);
  if (op == ThreadBinaryMathOp::FloorDiv) {
    return floor_divide_scalar(rhs, true);
  }
  return apply_math_series_series(scalar, lhs, op);
}

pluss::Series apply_compare_series_series(
    const pluss::Series& lhs,
    const pluss::Series& rhs,
    ThreadBinaryCompareOp op) {
  switch (op) {
    case ThreadBinaryCompareOp::Eq:
      return lhs.eq(rhs);
    case ThreadBinaryCompareOp::Ne:
      return lhs.ne(rhs);
    case ThreadBinaryCompareOp::Lt:
      return lhs.lt(rhs);
    case ThreadBinaryCompareOp::Le:
      return lhs.le(rhs);
    case ThreadBinaryCompareOp::Gt:
      return lhs.gt(rhs);
    case ThreadBinaryCompareOp::Ge:
      return lhs.ge(rhs);
    default:
      PLUSS_FAIL("Thread: unsupported compare op");
  }
}

pluss::Series apply_compare_series_scalar(
    const pluss::Series& lhs,
    double rhs,
    ThreadBinaryCompareOp op) {
  switch (op) {
    case ThreadBinaryCompareOp::Eq:
      return lhs.eq(rhs);
    case ThreadBinaryCompareOp::Ne:
      return lhs.ne(rhs);
    case ThreadBinaryCompareOp::Lt:
      return lhs.lt(rhs);
    case ThreadBinaryCompareOp::Le:
      return lhs.le(rhs);
    case ThreadBinaryCompareOp::Gt:
      return lhs.gt(rhs);
    case ThreadBinaryCompareOp::Ge:
      return lhs.ge(rhs);
    default:
      PLUSS_FAIL("Thread: unsupported compare op");
  }
}

std::optional<pluss::Series> combine_validity_masks(const Thread& lhs, const Thread& rhs) {
  if (lhs.has_validity_mask() && rhs.has_validity_mask()) {
    return lhs.validity_mask()->logical_and(*rhs.validity_mask());
  }
  if (lhs.has_validity_mask()) {
    return lhs.validity_mask();
  }
  if (rhs.has_validity_mask()) {
    return rhs.validity_mask();
  }
  return std::nullopt;
}

bool parse_double_exact(std::string_view text, double& out) {
  std::string owned(text);
  char* end = nullptr;
  out = std::strtod(owned.c_str(), &end);
  return end != nullptr && *end == '\0' && std::isfinite(out);
}

bool parse_bool_like(std::string_view text, bool& out) {
  std::string low(text);
  std::transform(low.begin(), low.end(), low.begin(), [](char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  });
  if (low == "true" || low == "1" || low == "yes") {
    out = true;
    return true;
  }
  if (low == "false" || low == "0" || low == "no") {
    out = false;
    return true;
  }
  return false;
}

bool extract_numeric_value(const pluss::Series& s, std::int64_t idx, double& out) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      out = reinterpret_cast<const bool*>(base)[idx] ? 1.0 : 0.0;
      return true;
    case pluss::codem::ScalarType::Int8:
      out = static_cast<double>(reinterpret_cast<const std::int8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int16:
      out = static_cast<double>(reinterpret_cast<const std::int16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int32:
      out = static_cast<double>(reinterpret_cast<const std::int32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int64:
      out = static_cast<double>(reinterpret_cast<const std::int64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt8:
      out = static_cast<double>(reinterpret_cast<const std::uint8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt16:
      out = static_cast<double>(reinterpret_cast<const std::uint16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt32:
      out = static_cast<double>(reinterpret_cast<const std::uint32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt64:
      out = static_cast<double>(reinterpret_cast<const std::uint64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float16:
      out = pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::BFloat16:
      out = pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float32:
      out = static_cast<double>(reinterpret_cast<const float*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float64:
      out = reinterpret_cast<const double*>(base)[idx];
      return true;
    default:
      return false;
  }
}

std::string thread_scalar_string_at(const Thread& t, std::int64_t row) {
  switch (t.kind()) {
    case ThreadKind::Series: {
      if (is_numeric_series_type(t.as_series())) {
        double value = 0.0;
        if (extract_numeric_value(t.as_series(), row, value)) {
          return std::to_string(value);
        }
      }
      if (t.as_series().scalar_type() == pluss::codem::ScalarType::Bool) {
        return t.as_series().to_vec<bool>(row) ? "true" : "false";
      }
      return "";
    }
    case ThreadKind::Note:
      return std::string(t.as_note().at(row));
    case ThreadKind::Timeline:
      return std::to_string(t.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    case ThreadKind::TimeIndex:
      return std::to_string(t.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    default:
      return "";
  }
}

bool compare_numbers(double lhs, double rhs, ThreadBinaryCompareOp op) {
  switch (op) {
    case ThreadBinaryCompareOp::Eq:
      return lhs == rhs;
    case ThreadBinaryCompareOp::Ne:
      return lhs != rhs;
    case ThreadBinaryCompareOp::Lt:
      return lhs < rhs;
    case ThreadBinaryCompareOp::Le:
      return lhs <= rhs;
    case ThreadBinaryCompareOp::Gt:
      return lhs > rhs;
    case ThreadBinaryCompareOp::Ge:
      return lhs >= rhs;
    default:
      return false;
  }
}

pluss::Series bool_series_from_bools(const std::vector<bool>& values) {
  auto out = pluss::Series::zeros({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    p[i] = values[i];
  }
  return out;
}

bool all_true(const std::vector<bool>& values) {
  return std::all_of(values.begin(), values.end(), [](bool v) { return v; });
}

void copy_series_value(
    const pluss::Series& src,
    std::int64_t src_idx,
    pluss::Series& dst,
    std::int64_t dst_idx) {
  PLUSS_CHECK(src.scalar_type() == dst.scalar_type(), "Thread: dtype mismatch in copy_series_value");
  switch (src.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      reinterpret_cast<bool*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const bool*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Int8:
      reinterpret_cast<std::int8_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::int8_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Int16:
      reinterpret_cast<std::int16_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::int16_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Int32:
      reinterpret_cast<std::int32_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::int32_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Int64:
      reinterpret_cast<std::int64_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::int64_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::UInt8:
      reinterpret_cast<std::uint8_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::uint8_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::UInt16:
      reinterpret_cast<std::uint16_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::uint16_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::UInt32:
      reinterpret_cast<std::uint32_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::uint32_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::UInt64:
      reinterpret_cast<std::uint64_t*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const std::uint64_t*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Float16:
      reinterpret_cast<pluss::codem::Float16*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const pluss::codem::Float16*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::BFloat16:
      reinterpret_cast<pluss::codem::BFloat16*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const pluss::codem::BFloat16*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Float32:
      reinterpret_cast<float*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const float*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Float64:
      reinterpret_cast<double*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const double*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Complex64:
      reinterpret_cast<pluss::codem::Complex64*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const pluss::codem::Complex64*>(src.raw_data())[src_idx];
      return;
    case pluss::codem::ScalarType::Complex128:
      reinterpret_cast<pluss::codem::Complex128*>(dst.raw_data())[dst_idx] =
          reinterpret_cast<const pluss::codem::Complex128*>(src.raw_data())[src_idx];
      return;
    default:
      PLUSS_FAIL("Thread: unsupported Series dtype in copy_series_value");
  }
}

pluss::Note rebuild_note_like(const pluss::Note& reference, const std::vector<std::string>& values) {
  std::vector<std::string_view> views;
  views.reserve(values.size());
  for (const auto& v : values) {
    views.emplace_back(v);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  if (static_cast<const pluss::coden::NoteBase&>(reference).is_bytes()) {
    return pluss::coden::ops_internal::note_from_bytes(
        std::span<const std::string_view>(views.data(), views.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));
  }
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
}

pluss::Timeline rebuild_timeline_like(const pluss::Timeline& reference, const std::vector<std::int64_t>& ticks) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(ticks.size())};
  auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(ticks.data(), ticks.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  out.unsafe_tz_name() = std::string(reference.tz());
  return out;
}

pluss::Note make_utf8_note(const std::vector<std::string>& values) {
  std::vector<std::string_view> views;
  views.reserve(values.size());
  for (const auto& v : values) {
    views.emplace_back(v);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
}

std::optional<pluss::Series> optional_mask_from_valid(const std::vector<bool>& valid) {
  if (all_true(valid)) {
    return std::nullopt;
  }
  return bool_series_from_bools(valid);
}

std::optional<pluss::Series> merge_masks(
    const std::optional<pluss::Series>& lhs,
    const std::optional<pluss::Series>& rhs) {
  if (lhs.has_value() && rhs.has_value()) {
    return lhs->logical_and(*rhs);
  }
  if (lhs.has_value()) {
    return lhs;
  }
  if (rhs.has_value()) {
    return rhs;
  }
  return std::nullopt;
}

Thread forward_fill_thread(const Thread& t) {
  if (!t.has_validity_mask()) {
    return t;
  }
  auto valid = t.validity_mask()->to_vec<bool>();
  if (valid.empty()) {
    return t;
  }

  switch (t.kind()) {
    case ThreadKind::Series: {
      auto out = t.as_series().clone();
      for (std::int64_t i = 1; i < t.size(); ++i) {
        if (!valid[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i - 1)]) {
          copy_series_value(out, i - 1, out, i);
          valid[static_cast<std::size_t>(i)] = true;
        }
      }
      if (all_true(valid)) {
        return Thread(out);
      }
      return Thread(out, bool_series_from_bools(valid));
    }
    case ThreadKind::Note: {
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(t.size()));
      for (std::int64_t i = 0; i < t.size(); ++i) {
        values.emplace_back(t.as_note().at(i));
      }
      for (std::int64_t i = 1; i < t.size(); ++i) {
        if (!valid[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i - 1)]) {
          values[static_cast<std::size_t>(i)] = values[static_cast<std::size_t>(i - 1)];
          valid[static_cast<std::size_t>(i)] = true;
        }
      }
      auto out = rebuild_note_like(t.as_note(), values);
      if (all_true(valid)) {
        return Thread(out);
      }
      return Thread(out, bool_series_from_bools(valid));
    }
    case ThreadKind::Timeline: {
      auto ticks = t.as_timeline().to_epoch_nanos_vector();
      for (std::int64_t i = 1; i < t.size(); ++i) {
        if (!valid[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i - 1)]) {
          ticks[static_cast<std::size_t>(i)] = ticks[static_cast<std::size_t>(i - 1)];
          valid[static_cast<std::size_t>(i)] = true;
        }
      }
      auto out = rebuild_timeline_like(t.as_timeline(), ticks);
      if (all_true(valid)) {
        return Thread(out);
      }
      return Thread(out, bool_series_from_bools(valid));
    }
    case ThreadKind::TimeIndex: {
      auto base = Thread(t.as_timeindex().timeline(), t.validity_mask());
      auto filled = forward_fill_thread(base);
      auto out = filled.as_timeline().to_timeindex(false);
      if (!filled.has_validity_mask()) {
        return Thread(out);
      }
      return Thread(out, *filled.validity_mask());
    }
    default:
      return t;
  }
}

Thread backward_fill_thread(const Thread& t) {
  if (!t.has_validity_mask()) {
    return t;
  }
  auto valid = t.validity_mask()->to_vec<bool>();
  if (valid.empty()) {
    return t;
  }

  switch (t.kind()) {
    case ThreadKind::Series: {
      auto out = t.as_series().clone();
      for (std::int64_t i = t.size() - 2; i >= 0; --i) {
        if (!valid[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i + 1)]) {
          copy_series_value(out, i + 1, out, i);
          valid[static_cast<std::size_t>(i)] = true;
        }
      }
      if (all_true(valid)) {
        return Thread(out);
      }
      return Thread(out, bool_series_from_bools(valid));
    }
    case ThreadKind::Note: {
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(t.size()));
      for (std::int64_t i = 0; i < t.size(); ++i) {
        values.emplace_back(t.as_note().at(i));
      }
      for (std::int64_t i = t.size() - 2; i >= 0; --i) {
        if (!valid[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i + 1)]) {
          values[static_cast<std::size_t>(i)] = values[static_cast<std::size_t>(i + 1)];
          valid[static_cast<std::size_t>(i)] = true;
        }
      }
      auto out = rebuild_note_like(t.as_note(), values);
      if (all_true(valid)) {
        return Thread(out);
      }
      return Thread(out, bool_series_from_bools(valid));
    }
    case ThreadKind::Timeline: {
      auto ticks = t.as_timeline().to_epoch_nanos_vector();
      for (std::int64_t i = t.size() - 2; i >= 0; --i) {
        if (!valid[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i + 1)]) {
          ticks[static_cast<std::size_t>(i)] = ticks[static_cast<std::size_t>(i + 1)];
          valid[static_cast<std::size_t>(i)] = true;
        }
      }
      auto out = rebuild_timeline_like(t.as_timeline(), ticks);
      if (all_true(valid)) {
        return Thread(out);
      }
      return Thread(out, bool_series_from_bools(valid));
    }
    case ThreadKind::TimeIndex: {
      auto base = Thread(t.as_timeindex().timeline(), t.validity_mask());
      auto filled = backward_fill_thread(base);
      auto out = filled.as_timeline().to_timeindex(false);
      if (!filled.has_validity_mask()) {
        return Thread(out);
      }
      return Thread(out, *filled.validity_mask());
    }
    default:
      return t;
  }
}

} // namespace

Thread::Thread(pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl) : impl_(std::move(impl)) {}

Thread::Thread() = default;

Thread::Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask)
    : impl_(build_impl(Payload{data}, std::move(validity_mask))) {}

Thread::Thread(const Thread& other) = default;
Thread::Thread(Thread&& other) noexcept = default;
Thread& Thread::operator=(const Thread& other) = default;
Thread& Thread::operator=(Thread&& other) noexcept = default;
Thread::~Thread() = default;

bool Thread::defined() const noexcept { return static_cast<bool>(impl_); }
bool Thread::unique() const noexcept { return impl_.unique(); }

const detail::ThreadImpl& Thread::impl_ref_() const {
  PLUSS_CHECK(defined(), "Thread: undefined handle");
  return *impl_;
}

detail::ThreadImpl& Thread::mut_impl_() {
  PLUSS_CHECK(defined(), "Thread: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

std::int64_t Thread::size() const { return impl_ref_().size_; }
ThreadKind Thread::kind() const { return impl_ref_().kind_; }
std::string Thread::dtype_name() const { return impl_ref_().dtype_name_; }
ThreadValue Thread::value() const { return ThreadValue(*this); }
const Thread& Thread::values() const noexcept { return *this; }
bool Thread::is_valid(std::int64_t row) const {
  const auto idx = normalize_index(row, size());
  if (!has_validity_mask()) {
    return true;
  }
  const auto valid = validity_mask()->to_vec<bool>();
  return valid[static_cast<std::size_t>(idx)];
}

bool Thread::has_validity_mask() const { return impl_ref_().validity_mask_.has_value(); }

const std::optional<pluss::Series>& Thread::validity_mask() const {
  return impl_ref_().validity_mask_;
}

Thread Thread::with_validity_mask(const pluss::Series& validity_mask) const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, validity_mask));
}

Thread& Thread::with_validity_mask_(const pluss::Series& validity_mask) {
  auto& w = mut_impl_();
  validate_validity_mask(validity_mask, w.size_);
  w.validity_mask_ = validity_mask;
  return *this;
}

Thread Thread::clear_validity_mask() const {
  const auto& src = impl_ref_();
  return Thread(build_impl(src.payload_, std::nullopt));
}

Thread& Thread::clear_validity_mask_() {
  auto& w = mut_impl_();
  w.validity_mask_.reset();
  return *this;
}

pluss::Series Thread::isnull() const {
  const auto& src = impl_ref_();
  if (!src.validity_mask_.has_value()) {
    return bool_series_zeros(src.size_);
  }
  return src.validity_mask_->logical_not();
}

pluss::Series Thread::notnull() const {
  if (!has_validity_mask()) {
    auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(out.raw_data());
    for (std::int64_t i = 0; i < size(); ++i) {
      p[i] = true;
    }
    return out;
  }
  return *validity_mask();
}

pluss::Series Thread::isnan() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isnan is only valid for Series payload");
  return as_series().isnan();
}

pluss::Series Thread::isfinite() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::isfinite is only valid for Series payload");
  auto finite = as_series().isfinite();
  if (has_validity_mask()) {
    finite = finite.logical_and(*validity_mask());
  }
  return finite;
}

Thread Thread::fillna(double value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::fillna(double) is only valid for Series payload");
  if (!has_validity_mask()) {
    return *this;
  }
  auto out = as_series().where(*validity_mask(), value);
  return Thread(out, std::nullopt);
}

Thread Thread::abs() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::abs is only valid for Series payload");
  return Thread(as_series().abs(), validity_mask());
}

Thread Thread::clip(double min_value, double max_value) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::clip is only valid for Series payload");
  return Thread(as_series().clamp(min_value, max_value), validity_mask());
}

Thread Thread::log() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::log is only valid for Series payload");
  auto s = as_series();
  if (s.scalar_type() != pluss::codem::ScalarType::Float16
      && s.scalar_type() != pluss::codem::ScalarType::BFloat16
      && s.scalar_type() != pluss::codem::ScalarType::Float32
      && s.scalar_type() != pluss::codem::ScalarType::Float64) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  return Thread(s.log(), validity_mask());
}

Thread Thread::exp() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::exp is only valid for Series payload");
  auto s = as_series();
  if (s.scalar_type() != pluss::codem::ScalarType::Float16
      && s.scalar_type() != pluss::codem::ScalarType::BFloat16
      && s.scalar_type() != pluss::codem::ScalarType::Float32
      && s.scalar_type() != pluss::codem::ScalarType::Float64) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  return Thread(s.exp(), validity_mask());
}

Thread& Thread::fillna_(double value) {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::fillna_(double) is only valid for Series payload");
  if (!has_validity_mask()) {
    return *this;
  }
  auto& w = mut_impl_();
  auto* s = std::get_if<pluss::Series>(&w.payload_);
  PLUSS_CHECK(s != nullptr, "Thread::fillna_: internal payload mismatch");
  *s = s->where(*w.validity_mask_, value);
  w.validity_mask_.reset();
  return *this;
}

Thread Thread::head(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(0, clipped);
}

Thread Thread::tail(std::int64_t n) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(size() - clipped, size());
}

Thread Thread::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, size());
  std::vector<std::int64_t> indices;
  indices.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    indices.push_back(i);
  }
  return take(indices);
}

Thread Thread::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  Payload out_payload = std::visit([&](const auto& payload) -> Payload {
    using T = std::decay_t<decltype(payload)>;
    if constexpr (std::is_same_v<T, pluss::Series>) {
      std::vector<std::int64_t> idx_norm;
      idx_norm.reserve(indices.size());
      for (const auto raw_idx : indices) {
        idx_norm.push_back(normalize_index(raw_idx, src.size_));
      }
      auto idx_series = int64_index_series(idx_norm);
      return payload.take(idx_series);
    } else if constexpr (std::is_same_v<T, pluss::Note>) {
      const auto values = note_take_strings(payload, indices, src.size_);
      if (static_cast<const pluss::coden::NoteBase&>(payload).is_bytes()) {
        std::vector<std::string_view> views;
        views.reserve(values.size());
        for (const auto& v : values) views.emplace_back(v);
        const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
        return pluss::coden::ops_internal::note_from_bytes(
            std::span<const std::string_view>(views.data(), views.size()),
            std::span<const std::int64_t>(shape.data(), shape.size()));
      }
      std::vector<std::string_view> views;
      views.reserve(values.size());
      for (const auto& v : values) views.emplace_back(v);
      const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
      return pluss::coden::ops_internal::note_from_utf8(
          std::span<const std::string_view>(views.data(), views.size()),
          std::span<const std::int64_t>(shape.data(), shape.size()));
    } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
      return timeline_take(payload, indices, src.size_);
    } else {
      auto t = timeline_take(payload.timeline(), indices, src.size_);
      return t.to_timeindex(false);
    }
  }, src.payload_);

  std::optional<pluss::Series> out_mask = std::nullopt;
  if (src.validity_mask_.has_value()) {
    std::vector<std::int64_t> idx_norm;
    idx_norm.reserve(indices.size());
    for (const auto raw_idx : indices) {
      idx_norm.push_back(normalize_index(raw_idx, src.size_));
    }
    auto idx_series = int64_index_series(idx_norm);
    out_mask = src.validity_mask_->take(idx_series);
  }

  return Thread(build_impl(std::move(out_payload), std::move(out_mask)));
}

Thread Thread::filter(const pluss::Series& bool_mask) const {
  const auto indices = true_indices_from_mask(bool_mask, size());
  return take(indices);
}

Thread Thread::dropna() const {
  if (!has_validity_mask()) {
    return *this;
  }
  const auto valid = validity_mask()->to_vec<bool>();
  std::vector<std::int64_t> indices;
  indices.reserve(valid.size());
  for (std::size_t i = 0; i < valid.size(); ++i) {
    if (valid[i]) {
      indices.push_back(static_cast<std::int64_t>(i));
    }
  }
  auto out = take(indices);
  if (out.has_validity_mask()) {
    return out.clear_validity_mask();
  }
  return out;
}

Thread Thread::ffill() const {
  return forward_fill_thread(*this);
}

Thread Thread::bfill() const {
  return backward_fill_thread(*this);
}

pluss::Series Thread::sum() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::sum is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::sum on an empty (all-null) selection");
  return s.sum();
}

pluss::Series Thread::mean() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::mean is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::mean on an empty (all-null) selection");
  return s.mean();
}

pluss::Series Thread::min() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::min is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::min on an empty (all-null) selection");
  return s.min();
}

pluss::Series Thread::max() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::max is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::max on an empty (all-null) selection");
  return s.max();
}

pluss::Series Thread::var(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::var is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::var on an empty (all-null) selection");
  return s.var(unbiased);
}

pluss::Series Thread::std(bool unbiased) const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::std is only valid for Series payload");
  auto s = as_series();
  if (has_validity_mask()) {
    s = s.masked_select(*validity_mask());
  }
  if (is_numeric_series_type(s)) {
    s = s.to(pluss::codem::ScalarType::Float64);
  }
  PLUSS_CHECK(s.numel() > 0, "Thread::std on an empty (all-null) selection");
  return s.std(unbiased);
}

Thread Thread::lower() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::lower is only valid for Note payload");
  return Thread(as_note().lower(), validity_mask());
}

pluss::Series Thread::contains(std::string_view needle) const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::contains is only valid for Note payload");
  return bool_series_from_u8(as_note().contains(needle));
}

Thread Thread::numeric_binary(double other, ThreadBinaryMathOp op, bool reverse) const {
  if (kind() != ThreadKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  return Thread(apply_math_series_scalar(as_series(), other, op, reverse), validity_mask());
}

Thread Thread::numeric_binary(const Thread& other, ThreadBinaryMathOp op) const {
  if (kind() != ThreadKind::Series || !is_numeric_series_type(as_series())) {
    return *this;
  }
  PLUSS_CHECK(other.kind() == ThreadKind::Series && is_numeric_series_type(other.as_series()),
              "Thread::numeric_binary: rhs must be numeric Series");
  return Thread(
      apply_math_series_series(as_series(), other.as_series(), op),
      combine_validity_masks(*this, other));
}

Thread Thread::compare(double other, ThreadBinaryCompareOp op) const {
  if (kind() == ThreadKind::Series) {
    return Thread(apply_compare_series_scalar(as_series(), other, op), validity_mask());
  }

  auto out = pluss::Series::zeros({size()}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());

  std::optional<std::vector<bool>> valid;
  if (has_validity_mask()) {
    valid = validity_mask()->to_vec<bool>();
  }

  for (std::int64_t i = 0; i < size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      p[i] = false;
      continue;
    }
    const auto text = thread_scalar_string_at(*this, i);
    double num = 0.0;
    bool bv = false;
    if (parse_double_exact(text, num)) {
      p[i] = compare_numbers(num, other, op);
    } else if (parse_bool_like(text, bv)) {
      p[i] = compare_numbers(bv ? 1.0 : 0.0, other, op);
    } else {
      p[i] = false;
    }
  }
  return Thread(out, validity_mask());
}

Thread Thread::compare(const Thread& other, ThreadBinaryCompareOp op) const {
  PLUSS_CHECK(size() == other.size(), "Thread::compare: size mismatch");

  if (kind() == ThreadKind::Series && other.kind() == ThreadKind::Series) {
    return Thread(
        apply_compare_series_series(as_series(), other.as_series(), op),
        combine_validity_masks(*this, other));
  }

  if (kind() == ThreadKind::Timeline && other.kind() == ThreadKind::Timeline) {
    const auto& lhs = as_timeline();
    const auto& rhs = other.as_timeline();
    pluss::Series out = lhs.eq(rhs);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        out = lhs.eq(rhs);
        break;
      case ThreadBinaryCompareOp::Ne:
        out = lhs.ne(rhs);
        break;
      case ThreadBinaryCompareOp::Lt:
        out = lhs.lt(rhs);
        break;
      case ThreadBinaryCompareOp::Le:
        out = lhs.le(rhs);
        break;
      case ThreadBinaryCompareOp::Gt:
        out = lhs.gt(rhs);
        break;
      case ThreadBinaryCompareOp::Ge:
        out = lhs.ge(rhs);
        break;
    }
    return Thread(out, combine_validity_masks(*this, other));
  }

  if (kind() == ThreadKind::TimeIndex && other.kind() == ThreadKind::TimeIndex) {
    const auto& lhs = as_timeindex().timeline();
    const auto& rhs = other.as_timeindex().timeline();
    pluss::Series out = lhs.eq(rhs);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        out = lhs.eq(rhs);
        break;
      case ThreadBinaryCompareOp::Ne:
        out = lhs.ne(rhs);
        break;
      case ThreadBinaryCompareOp::Lt:
        out = lhs.lt(rhs);
        break;
      case ThreadBinaryCompareOp::Le:
        out = lhs.le(rhs);
        break;
      case ThreadBinaryCompareOp::Gt:
        out = lhs.gt(rhs);
        break;
      case ThreadBinaryCompareOp::Ge:
        out = lhs.ge(rhs);
        break;
    }
    return Thread(out, combine_validity_masks(*this, other));
  }

  if (kind() == ThreadKind::Note && other.kind() == ThreadKind::Note) {
    const auto& lhs = as_note();
    const auto& rhs = other.as_note();
    std::vector<std::uint8_t> raw;
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        raw = (lhs == rhs);
        break;
      case ThreadBinaryCompareOp::Ne:
        raw = (lhs != rhs);
        break;
      case ThreadBinaryCompareOp::Lt:
        raw = (lhs < rhs);
        break;
      case ThreadBinaryCompareOp::Le:
        raw = (lhs <= rhs);
        break;
      case ThreadBinaryCompareOp::Gt:
        raw = (lhs > rhs);
        break;
      case ThreadBinaryCompareOp::Ge:
        raw = (lhs >= rhs);
        break;
    }
    return Thread(bool_series_from_u8(raw), combine_validity_masks(*this, other));
  }

  if ((kind() == ThreadKind::Timeline && other.kind() == ThreadKind::TimeIndex)
      || (kind() == ThreadKind::TimeIndex && other.kind() == ThreadKind::Timeline)) {
    const auto lhs_t = (kind() == ThreadKind::Timeline) ? as_timeline() : as_timeindex().timeline();
    const auto rhs_t = (other.kind() == ThreadKind::Timeline) ? other.as_timeline() : other.as_timeindex().timeline();
    pluss::Series out = lhs_t.eq(rhs_t);
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        out = lhs_t.eq(rhs_t);
        break;
      case ThreadBinaryCompareOp::Ne:
        out = lhs_t.ne(rhs_t);
        break;
      case ThreadBinaryCompareOp::Lt:
        out = lhs_t.lt(rhs_t);
        break;
      case ThreadBinaryCompareOp::Le:
        out = lhs_t.le(rhs_t);
        break;
      case ThreadBinaryCompareOp::Gt:
        out = lhs_t.gt(rhs_t);
        break;
      case ThreadBinaryCompareOp::Ge:
        out = lhs_t.ge(rhs_t);
        break;
    }
    return Thread(out, combine_validity_masks(*this, other));
  }

  PLUSS_FAIL("Thread::compare: unsupported payload combination");
}

int Thread::compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last) const {
  const auto l = normalize_index(lhs_row, size());
  const auto r = normalize_index(rhs_row, size());

  const bool lv = is_valid(l);
  const bool rv = is_valid(r);
  if (!lv || !rv) {
    if (!lv && !rv) {
      return 0;
    }
    if (nulls_last) {
      return lv ? -1 : 1;
    }
    return lv ? 1 : -1;
  }

  auto cmp_numeric = [](double a, double b) -> int {
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
    return 0;
  };
  auto cmp_string = [](const std::string& a, const std::string& b) -> int {
    if (a < b) {
      return -1;
    }
    if (a > b) {
      return 1;
    }
    return 0;
  };

  switch (kind()) {
    case ThreadKind::Series: {
      const auto& s = as_series();
      if (is_numeric_series_type(s)) {
        double lv_num = 0.0;
        double rv_num = 0.0;
        const bool ok_l = extract_numeric_value(s, l, lv_num);
        const bool ok_r = extract_numeric_value(s, r, rv_num);
        if (ok_l && ok_r) {
          return cmp_numeric(lv_num, rv_num);
        }
      }
      return cmp_string(thread_scalar_string_at(*this, l), thread_scalar_string_at(*this, r));
    }
    case ThreadKind::Note:
      return cmp_string(std::string(as_note().at(l)), std::string(as_note().at(r)));
    case ThreadKind::Timeline: {
      const auto ticks = as_timeline().to_epoch_nanos_vector();
      const auto lv_tick = ticks[static_cast<std::size_t>(l)];
      const auto rv_tick = ticks[static_cast<std::size_t>(r)];
      if (lv_tick < rv_tick) {
        return -1;
      }
      if (lv_tick > rv_tick) {
        return 1;
      }
      return 0;
    }
    case ThreadKind::TimeIndex: {
      const auto ticks = as_timeindex().timeline().to_epoch_nanos_vector();
      const auto lv_tick = ticks[static_cast<std::size_t>(l)];
      const auto rv_tick = ticks[static_cast<std::size_t>(r)];
      if (lv_tick < rv_tick) {
        return -1;
      }
      if (lv_tick > rv_tick) {
        return 1;
      }
      return 0;
    }
    default:
      return 0;
  }
}

Thread Thread::window(std::int64_t window_size, ThreadWindowOp op, std::int64_t min_periods) const {
  PLUSS_CHECK(window_size > 0, "Thread::window: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "Thread::window: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window_size, "Thread::window: min_periods cannot exceed window");
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::window: only Series payload supports rolling ops");
  PLUSS_CHECK(is_numeric_series_type(as_series()), "Thread::window: Series payload must be numeric");

  const auto n = size();
  auto out = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  auto* out_data = reinterpret_cast<double*>(out.raw_data());
  std::vector<bool> out_valid(static_cast<std::size_t>(n), false);

  std::vector<bool> in_valid(static_cast<std::size_t>(n), true);
  if (has_validity_mask()) {
    in_valid = validity_mask()->to_vec<bool>();
  }

  std::vector<double> values(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    if (!in_valid[static_cast<std::size_t>(i)]) {
      continue;
    }
    double v = 0.0;
    const bool ok = extract_numeric_value(as_series(), i, v);
    PLUSS_CHECK(ok, "Thread::window: failed numeric extraction");
    values[static_cast<std::size_t>(i)] = v;
  }

  std::vector<double> prefix_sum(static_cast<std::size_t>(n + 1), 0.0);
  std::vector<std::int64_t> prefix_count(static_cast<std::size_t>(n + 1), 0);
  for (std::int64_t i = 0; i < n; ++i) {
    prefix_sum[static_cast<std::size_t>(i + 1)] =
        prefix_sum[static_cast<std::size_t>(i)] + (in_valid[static_cast<std::size_t>(i)] ? values[static_cast<std::size_t>(i)] : 0.0);
    prefix_count[static_cast<std::size_t>(i + 1)] =
        prefix_count[static_cast<std::size_t>(i)] + (in_valid[static_cast<std::size_t>(i)] ? 1 : 0);
  }

  auto valid_count_in_window = [&](std::int64_t start, std::int64_t stop_exclusive) {
    return prefix_count[static_cast<std::size_t>(stop_exclusive)] - prefix_count[static_cast<std::size_t>(start)];
  };

  if (op == ThreadWindowOp::RollingSum || op == ThreadWindowOp::RollingMean) {
    for (std::int64_t i = 0; i < n; ++i) {
      const auto start = std::max<std::int64_t>(0, i - window_size + 1);
      const auto stop = i + 1;
      const auto cnt = valid_count_in_window(start, stop);
      if (cnt < min_periods) {
        out_data[i] = 0.0;
        continue;
      }
      const auto sum =
          prefix_sum[static_cast<std::size_t>(stop)] - prefix_sum[static_cast<std::size_t>(start)];
      out_data[i] = (op == ThreadWindowOp::RollingMean)
                        ? (sum / static_cast<double>(cnt))
                        : sum;
      out_valid[static_cast<std::size_t>(i)] = true;
    }
    return Thread(out, optional_mask_from_valid(out_valid));
  }

  const bool is_min = (op == ThreadWindowOp::RollingMin);
  std::deque<std::int64_t> mono;
  for (std::int64_t i = 0; i < n; ++i) {
    if (in_valid[static_cast<std::size_t>(i)]) {
      while (!mono.empty()) {
        const auto back_idx = mono.back();
        const auto lhs = values[static_cast<std::size_t>(back_idx)];
        const auto rhs = values[static_cast<std::size_t>(i)];
        const bool pop = is_min ? (lhs >= rhs) : (lhs <= rhs);
        if (!pop) {
          break;
        }
        mono.pop_back();
      }
      mono.push_back(i);
    }

    const auto start = std::max<std::int64_t>(0, i - window_size + 1);
    while (!mono.empty() && mono.front() < start) {
      mono.pop_front();
    }

    const auto cnt = valid_count_in_window(start, i + 1);
    if (cnt < min_periods || mono.empty()) {
      out_data[i] = 0.0;
      continue;
    }
    out_data[i] = values[static_cast<std::size_t>(mono.front())];
    out_valid[static_cast<std::size_t>(i)] = true;
  }

  return Thread(out, optional_mask_from_valid(out_valid));
}

Thread Thread::cast(
    ThreadCastTarget target,
    std::optional<pluss::codem::ScalarType> series_dtype) const {
  switch (target) {
    case ThreadCastTarget::Series: {
      if (kind() == ThreadKind::Series) {
        if (!series_dtype.has_value() || as_series().scalar_type() == *series_dtype) {
          return *this;
        }
        return Thread(as_series().to(*series_dtype), validity_mask());
      }

      if (kind() == ThreadKind::Timeline || kind() == ThreadKind::TimeIndex) {
        const auto ticks = (kind() == ThreadKind::Timeline)
                               ? as_timeline().to_epoch_nanos_vector()
                               : as_timeindex().timeline().to_epoch_nanos_vector();
        auto out = pluss::Series::empty({static_cast<std::int64_t>(ticks.size())}, pluss::codem::ScalarType::Int64);
        if (!ticks.empty()) {
          std::memcpy(out.raw_data(), ticks.data(), ticks.size() * sizeof(std::int64_t));
        }
        if (series_dtype.has_value() && *series_dtype != pluss::codem::ScalarType::Int64) {
          out = out.to(*series_dtype);
        }
        return Thread(out, validity_mask());
      }

      PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::cast(Series): unsupported source type");
      const auto n = size();
      const auto dt = series_dtype.value_or(pluss::codem::ScalarType::Float64);

      if (dt == pluss::codem::ScalarType::Bool) {
        auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
        auto* p = reinterpret_cast<bool*>(out.raw_data());
        std::vector<bool> parsed_valid(static_cast<std::size_t>(n), true);
        for (std::int64_t i = 0; i < n; ++i) {
          if (!is_valid(i)) {
            parsed_valid[static_cast<std::size_t>(i)] = false;
            p[i] = false;
            continue;
          }
          bool value = false;
          if (!parse_bool_like(as_note().at(i), value)) {
            parsed_valid[static_cast<std::size_t>(i)] = false;
            p[i] = false;
            continue;
          }
          p[i] = value;
        }
        return Thread(out, optional_mask_from_valid(parsed_valid));
      }

      auto out = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
      auto* p = reinterpret_cast<double*>(out.raw_data());
      std::vector<bool> parsed_valid(static_cast<std::size_t>(n), true);
      for (std::int64_t i = 0; i < n; ++i) {
        if (!is_valid(i)) {
          parsed_valid[static_cast<std::size_t>(i)] = false;
          p[i] = 0.0;
          continue;
        }
        double value = 0.0;
        if (!parse_double_exact(as_note().at(i), value)) {
          parsed_valid[static_cast<std::size_t>(i)] = false;
          p[i] = 0.0;
          continue;
        }
        p[i] = value;
      }
      if (dt != pluss::codem::ScalarType::Float64) {
        out = out.to(dt);
      }
      return Thread(out, optional_mask_from_valid(parsed_valid));
    }
    case ThreadCastTarget::Note: {
      if (kind() == ThreadKind::Note) {
        return *this;
      }
      if (kind() == ThreadKind::Timeline) {
        return Thread(as_timeline().to_note_iso(), validity_mask());
      }
      if (kind() == ThreadKind::TimeIndex) {
        return Thread(as_timeindex().timeline().to_note_iso(), validity_mask());
      }

      PLUSS_CHECK(kind() == ThreadKind::Series, "Thread::cast(Note): unsupported source type");
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(size()));
      for (std::int64_t i = 0; i < size(); ++i) {
        values.emplace_back(thread_scalar_string_at(*this, i));
      }
      return Thread(make_utf8_note(values), validity_mask());
    }
    case ThreadCastTarget::Timeline: {
      if (kind() == ThreadKind::Timeline) {
        return *this;
      }
      if (kind() == ThreadKind::TimeIndex) {
        return Thread(as_timeindex().timeline(), validity_mask());
      }
      if (kind() == ThreadKind::Series) {
        PLUSS_CHECK(is_numeric_series_type(as_series()),
                    "Thread::cast(Timeline): Series source must be numeric epoch-nanos");
        std::vector<std::int64_t> ticks(static_cast<std::size_t>(size()), 0);
        for (std::int64_t i = 0; i < size(); ++i) {
          double v = 0.0;
          const bool ok = extract_numeric_value(as_series(), i, v);
          PLUSS_CHECK(ok, "Thread::cast(Timeline): failed numeric extraction from Series");
          ticks[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(v);
        }
        const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(ticks.size())};
        auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
            std::span<const std::int64_t>(ticks.data(), ticks.size()),
            std::span<const std::int64_t>(shape.data(), shape.size()));
        return Thread(out, validity_mask());
      }

      PLUSS_CHECK(kind() == ThreadKind::Note, "Thread::cast(Timeline): unsupported source type");
      std::vector<std::string> vals;
      vals.reserve(static_cast<std::size_t>(size()));
      for (std::int64_t i = 0; i < size(); ++i) {
        vals.emplace_back(as_note().at(i));
      }
      std::vector<std::string_view> views;
      views.reserve(vals.size());
      for (const auto& v : vals) {
        views.emplace_back(v);
      }
      const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
      auto out = pluss::codeh::ops_internal::timeline_from_utf8(
          std::span<const std::string_view>(views.data(), views.size()),
          std::span<const std::int64_t>(shape.data(), shape.size()));
      return Thread(out, validity_mask());
    }
    case ThreadCastTarget::TimeIndex: {
      if (kind() == ThreadKind::TimeIndex) {
        return *this;
      }
      auto t = cast(ThreadCastTarget::Timeline).as_timeline();
      return Thread(t.to_timeindex(false), validity_mask());
    }
    default:
      return *this;
  }
}

Thread Thread::astype(pluss::codem::ScalarType dtype) const {
  return cast(ThreadCastTarget::Series, dtype);
}

namespace {
const char* thread_kind_name(ThreadKind k) {
  switch (k) {
    case ThreadKind::Series:
      return "Series";
    case ThreadKind::Note:
      return "Note";
    case ThreadKind::Timeline:
      return "Timeline";
    case ThreadKind::TimeIndex:
      return "TimeIndex";
    default:
      return "Unknown";
  }
}

void ensure_numeric_thread_for_math(const Thread& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Thread is undefined");
  PLUSS_CHECK(t.kind() == ThreadKind::Series,
              std::string(where) + ": operation requires a Series-backed Thread");
  PLUSS_CHECK(is_numeric_series_type(t.as_series()),
              std::string(where) + ": operation requires a numeric Series payload");
}

void ensure_defined_thread_for_compare(const Thread& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Thread is undefined");
}

std::optional<std::int64_t> nonnegative_integral_scalar(double v) {
  if (!std::isfinite(v) || v < 0.0) {
    return std::nullopt;
  }
  const auto rounded = std::round(v);
  if (std::fabs(v - rounded) > 1e-12) {
    return std::nullopt;
  }
  if (rounded > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return static_cast<std::int64_t>(rounded);
}

const char* math_op_name(ThreadBinaryMathOp op) {
  switch (op) {
    case ThreadBinaryMathOp::Add:
      return "+";
    case ThreadBinaryMathOp::Sub:
      return "-";
    case ThreadBinaryMathOp::Mul:
      return "*";
    case ThreadBinaryMathOp::Div:
      return "/";
    case ThreadBinaryMathOp::TrueDiv:
      return "/";
    case ThreadBinaryMathOp::FloorDiv:
      return "//";
    case ThreadBinaryMathOp::Mod:
      return "%";
    case ThreadBinaryMathOp::Pow:
      return "**";
    default:
      return "?";
  }
}

const char* cmp_op_name(ThreadBinaryCompareOp op) {
  switch (op) {
    case ThreadBinaryCompareOp::Eq:
      return "==";
    case ThreadBinaryCompareOp::Ne:
      return "!=";
    case ThreadBinaryCompareOp::Lt:
      return "<";
    case ThreadBinaryCompareOp::Le:
      return "<=";
    case ThreadBinaryCompareOp::Gt:
      return ">";
    case ThreadBinaryCompareOp::Ge:
      return ">=";
    default:
      return "?";
  }
}

template <class Fn>
Thread invoke_underlying(Fn&& fn, std::string_view where, std::string_view what) {
  try {
    return fn();
  } catch (const std::exception& e) {
    PLUSS_FAIL(std::string(where) + ": underlying " + std::string(what) + " operation failed: " + e.what());
  } catch (...) {
    PLUSS_FAIL(std::string(where) + ": underlying " + std::string(what) + " operation failed");
  }
}

Thread binary_math_scalar(const Thread& lhs, double rhs, ThreadBinaryMathOp op, bool reverse, std::string_view where) {
  PLUSS_CHECK(lhs.defined(), std::string(where) + ": Thread is undefined");
  if (lhs.kind() == ThreadKind::Series) {
    ensure_numeric_thread_for_math(lhs, where);
    return invoke_underlying([&] { return lhs.numeric_binary(rhs, op, reverse); }, where, "Series math");
  }

  if (lhs.kind() == ThreadKind::Note && op == ThreadBinaryMathOp::Mul) {
    const auto times = nonnegative_integral_scalar(rhs);
    PLUSS_CHECK(times.has_value(),
                std::string(where) + ": Note repetition requires a non-negative integral scalar");
    return invoke_underlying(
        [&] { return Thread(lhs.as_note() * *times, lhs.validity_mask()); },
        where,
        "Note repetition");
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload for operator '" + math_op_name(op)
             + "' (" + thread_kind_name(lhs.kind()) + " with scalar)");
}

Thread binary_math_pair(const Thread& lhs, const Thread& rhs, ThreadBinaryMathOp op, std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  PLUSS_CHECK(lhs.size() == rhs.size(), std::string(where) + ": Thread size mismatch");

  if (lhs.kind() == ThreadKind::Series && rhs.kind() == ThreadKind::Series) {
    ensure_numeric_thread_for_math(lhs, where);
    ensure_numeric_thread_for_math(rhs, where);
    return invoke_underlying([&] { return lhs.numeric_binary(rhs, op); }, where, "Series math");
  }

  if (lhs.kind() == ThreadKind::Note && rhs.kind() == ThreadKind::Note && op == ThreadBinaryMathOp::Add) {
    return invoke_underlying(
        [&] { return Thread(lhs.as_note() + rhs.as_note(), combine_validity_masks(lhs, rhs)); },
        where,
        "Note concatenation");
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload combination for operator '" + math_op_name(op)
             + "' (" + thread_kind_name(lhs.kind()) + " vs " + thread_kind_name(rhs.kind()) + ")");
}

Thread compare_scalar(const Thread& lhs, double rhs, ThreadBinaryCompareOp op, std::string_view where) {
  ensure_numeric_thread_for_math(lhs, where);
  return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "Series compare");
}

Thread compare_pair(const Thread& lhs, const Thread& rhs, ThreadBinaryCompareOp op, std::string_view where) {
  ensure_defined_thread_for_compare(lhs, where);
  ensure_defined_thread_for_compare(rhs, where);
  PLUSS_CHECK(lhs.size() == rhs.size(), std::string(where) + ": Thread size mismatch");

  const auto lk = lhs.kind();
  const auto rk = rhs.kind();
  const bool temporal_compare =
      (lk == ThreadKind::Timeline && rk == ThreadKind::Timeline) ||
      (lk == ThreadKind::TimeIndex && rk == ThreadKind::TimeIndex) ||
      (lk == ThreadKind::Timeline && rk == ThreadKind::TimeIndex) ||
      (lk == ThreadKind::TimeIndex && rk == ThreadKind::Timeline);

  if (lk == ThreadKind::Series && rk == ThreadKind::Series) {
    return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "Series compare");
  }

  if (lk == ThreadKind::Note && rk == ThreadKind::Note) {
    return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "Note compare");
  }

  if (temporal_compare) {
    return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "temporal compare");
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload combination for operator '" + cmp_op_name(op)
             + "' (" + thread_kind_name(lk) + " vs " + thread_kind_name(rk) + ")");
}
} // namespace

Thread operator+(const Thread& lhs, double rhs) {
  return binary_math_scalar(lhs, rhs, ThreadBinaryMathOp::Add, false, "Thread::operator+(Thread,double)");
}

Thread operator+(double lhs, const Thread& rhs) {
  return binary_math_scalar(rhs, lhs, ThreadBinaryMathOp::Add, true, "Thread::operator+(double,Thread)");
}

Thread operator+(const Thread& lhs, const Thread& rhs) {
  return binary_math_pair(lhs, rhs, ThreadBinaryMathOp::Add, "Thread::operator+(Thread,Thread)");
}

Thread operator-(const Thread& lhs, double rhs) {
  return binary_math_scalar(lhs, rhs, ThreadBinaryMathOp::Sub, false, "Thread::operator-(Thread,double)");
}

Thread operator-(double lhs, const Thread& rhs) {
  return binary_math_scalar(rhs, lhs, ThreadBinaryMathOp::Sub, true, "Thread::operator-(double,Thread)");
}

Thread operator-(const Thread& lhs, const Thread& rhs) {
  return binary_math_pair(lhs, rhs, ThreadBinaryMathOp::Sub, "Thread::operator-(Thread,Thread)");
}

Thread operator*(const Thread& lhs, double rhs) {
  return binary_math_scalar(lhs, rhs, ThreadBinaryMathOp::Mul, false, "Thread::operator*(Thread,double)");
}

Thread operator*(double lhs, const Thread& rhs) {
  return binary_math_scalar(rhs, lhs, ThreadBinaryMathOp::Mul, true, "Thread::operator*(double,Thread)");
}

Thread operator*(const Thread& lhs, const Thread& rhs) {
  return binary_math_pair(lhs, rhs, ThreadBinaryMathOp::Mul, "Thread::operator*(Thread,Thread)");
}

Thread operator/(const Thread& lhs, double rhs) {
  return binary_math_scalar(lhs, rhs, ThreadBinaryMathOp::TrueDiv, false, "Thread::operator/(Thread,double)");
}

Thread operator/(double lhs, const Thread& rhs) {
  return binary_math_scalar(rhs, lhs, ThreadBinaryMathOp::TrueDiv, true, "Thread::operator/(double,Thread)");
}

Thread operator/(const Thread& lhs, const Thread& rhs) {
  return binary_math_pair(lhs, rhs, ThreadBinaryMathOp::TrueDiv, "Thread::operator/(Thread,Thread)");
}

Thread operator%(const Thread& lhs, double rhs) {
  return binary_math_scalar(lhs, rhs, ThreadBinaryMathOp::Mod, false, "Thread::operator%(Thread,double)");
}

Thread operator%(double lhs, const Thread& rhs) {
  return binary_math_scalar(rhs, lhs, ThreadBinaryMathOp::Mod, true, "Thread::operator%(double,Thread)");
}

Thread operator%(const Thread& lhs, const Thread& rhs) {
  return binary_math_pair(lhs, rhs, ThreadBinaryMathOp::Mod, "Thread::operator%(Thread,Thread)");
}

Thread operator==(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Eq, "Thread::operator==(Thread,double)");
}

Thread operator==(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Eq, "Thread::operator==(double,Thread)");
}

Thread operator==(const Thread& lhs, const Thread& rhs) {
  return compare_pair(lhs, rhs, ThreadBinaryCompareOp::Eq, "Thread::operator==(Thread,Thread)");
}

Thread operator!=(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Ne, "Thread::operator!=(Thread,double)");
}

Thread operator!=(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Ne, "Thread::operator!=(double,Thread)");
}

Thread operator!=(const Thread& lhs, const Thread& rhs) {
  return compare_pair(lhs, rhs, ThreadBinaryCompareOp::Ne, "Thread::operator!=(Thread,Thread)");
}

Thread operator<(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Lt, "Thread::operator<(Thread,double)");
}

Thread operator<(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Gt, "Thread::operator<(double,Thread)");
}

Thread operator<(const Thread& lhs, const Thread& rhs) {
  return compare_pair(lhs, rhs, ThreadBinaryCompareOp::Lt, "Thread::operator<(Thread,Thread)");
}

Thread operator<=(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Le, "Thread::operator<=(Thread,double)");
}

Thread operator<=(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Ge, "Thread::operator<=(double,Thread)");
}

Thread operator<=(const Thread& lhs, const Thread& rhs) {
  return compare_pair(lhs, rhs, ThreadBinaryCompareOp::Le, "Thread::operator<=(Thread,Thread)");
}

Thread operator>(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Gt, "Thread::operator>(Thread,double)");
}

Thread operator>(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Lt, "Thread::operator>(double,Thread)");
}

Thread operator>(const Thread& lhs, const Thread& rhs) {
  return compare_pair(lhs, rhs, ThreadBinaryCompareOp::Gt, "Thread::operator>(Thread,Thread)");
}

Thread operator>=(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Ge, "Thread::operator>=(Thread,double)");
}

Thread operator>=(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Le, "Thread::operator>=(double,Thread)");
}

Thread operator>=(const Thread& lhs, const Thread& rhs) {
  return compare_pair(lhs, rhs, ThreadBinaryCompareOp::Ge, "Thread::operator>=(Thread,Thread)");
}

const pluss::Series& Thread::as_series() const {
  PLUSS_CHECK(kind() == ThreadKind::Series, "Thread: payload is not Series");
  return std::get<pluss::Series>(impl_ref_().payload_);
}

const pluss::Note& Thread::as_note() const {
  PLUSS_CHECK(kind() == ThreadKind::Note, "Thread: payload is not Note");
  return std::get<pluss::Note>(impl_ref_().payload_);
}

const pluss::Timeline& Thread::as_timeline() const {
  PLUSS_CHECK(kind() == ThreadKind::Timeline, "Thread: payload is not Timeline");
  return std::get<pluss::Timeline>(impl_ref_().payload_);
}

const pluss::TimeIndex& Thread::as_timeindex() const {
  PLUSS_CHECK(kind() == ThreadKind::TimeIndex, "Thread: payload is not TimeIndex");
  return std::get<pluss::TimeIndex>(impl_ref_().payload_);
}

std::string Thread::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

void Thread::show(const display::DisplayOptions& options) const {
  display::print(std::cout, *this, options);
}

void Thread::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

std::ostream& operator<<(std::ostream& os, const Thread& thread) {
  thread.show(os);
  return os;
}

ThreadValue::ThreadValue(Thread thread) : thread_(std::move(thread)) {}

bool ThreadValue::defined() const noexcept { return thread_.defined(); }
ThreadKind ThreadValue::kind() const { return thread_.kind(); }
const Thread& ThreadValue::thread() const noexcept { return thread_; }

pluss::Series ThreadValue::as_series() const { return thread_.as_series(); }
pluss::Note ThreadValue::as_note() const { return thread_.as_note(); }
pluss::Timeline ThreadValue::as_timeline() const { return thread_.as_timeline(); }
pluss::TimeIndex ThreadValue::as_timeindex() const { return thread_.as_timeindex(); }

ThreadValue::operator pluss::Series() const { return as_series(); }
ThreadValue::operator pluss::Note() const { return as_note(); }
ThreadValue::operator pluss::Timeline() const { return as_timeline(); }
ThreadValue::operator pluss::TimeIndex() const { return as_timeindex(); }

} // namespace pluss::table
