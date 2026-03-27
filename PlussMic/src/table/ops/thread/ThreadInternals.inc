namespace detail {

using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::TimeIndex>;

class ThreadImpl final : public pluss::msharp::RefCounted {
public:
  ThreadImpl(Payload payload,
             std::optional<pluss::Series> validity_mask,
             std::optional<pluss::Note> display_labels,
             std::optional<std::string> display_name);

  Payload payload_;
  ThreadKind kind_{ThreadKind::Series};
  std::int64_t size_{0};
  std::string dtype_name_;
  std::optional<pluss::Series> validity_mask_;
  std::optional<pluss::Note> display_labels_;
  std::optional<std::string> display_name_;
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

void validate_display_labels(const std::optional<pluss::Note>& labels, std::int64_t size) {
  if (!labels.has_value()) {
    return;
  }
  const auto& note = *labels;
  PLUSS_CHECK(note.defined(), "Thread: display labels are undefined");
  PLUSS_CHECK(note.dim() == 1, "Thread: display labels must be 1D");
  PLUSS_CHECK(note.numel() == size, "Thread: display labels length mismatch");
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

double pearson_corr_from_vectors_(const std::vector<double>& x, const std::vector<double>& y) {
  const auto n = x.size();
  if (n < 2 || y.size() != n) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  long double sum_x = 0.0L;
  long double sum_y = 0.0L;
  for (std::size_t i = 0; i < n; ++i) {
    sum_x += static_cast<long double>(x[i]);
    sum_y += static_cast<long double>(y[i]);
  }

  const long double n_ld = static_cast<long double>(n);
  const long double mean_x = sum_x / n_ld;
  const long double mean_y = sum_y / n_ld;

  long double centered_xy = 0.0L;
  long double centered_xx = 0.0L;
  long double centered_yy = 0.0L;
  for (std::size_t i = 0; i < n; ++i) {
    const long double dx = static_cast<long double>(x[i]) - mean_x;
    const long double dy = static_cast<long double>(y[i]) - mean_y;
    centered_xy += dx * dy;
    centered_xx += dx * dx;
    centered_yy += dy * dy;
  }

  if (centered_xx <= 0.0L || centered_yy <= 0.0L) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const long double denom = std::sqrt(centered_xx * centered_yy);
  if (!(denom > 0.0L)) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const double corr = static_cast<double>(centered_xy / denom);
  if (!std::isfinite(corr)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::clamp(corr, -1.0, 1.0);
}

std::vector<double> rank_average_ties_(const std::vector<double>& input, double eps = 1e-12) {
  const auto n = input.size();
  std::vector<double> out(n, 0.0);
  std::vector<std::size_t> order(n, 0);
  std::iota(order.begin(), order.end(), static_cast<std::size_t>(0));
  std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return input[a] < input[b];
  });

  std::size_t start = 0;
  while (start < n) {
    std::size_t stop = start + 1;
    while (stop < n && std::abs(input[order[stop]] - input[order[start]]) <= eps) {
      ++stop;
    }
    const double rank = 0.5 * (static_cast<double>(start + 1) + static_cast<double>(stop));
    for (std::size_t k = start; k < stop; ++k) {
      out[order[k]] = rank;
    }
    start = stop;
  }
  return out;
}

double kendall_tau_b_from_vectors_(const std::vector<double>& x, const std::vector<double>& y, double eps = 1e-12) {
  const auto n = x.size();
  if (n < 2 || y.size() != n) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  std::int64_t concordant = 0;
  std::int64_t discordant = 0;
  std::int64_t tie_x_only = 0;
  std::int64_t tie_y_only = 0;
  std::int64_t tie_xy = 0;

  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i + 1; j < n; ++j) {
      const double dx = x[i] - x[j];
      const double dy = y[i] - y[j];
      const bool tx = std::abs(dx) <= eps;
      const bool ty = std::abs(dy) <= eps;
      if (tx && ty) {
        ++tie_xy;
        continue;
      }
      if (tx) {
        ++tie_x_only;
        continue;
      }
      if (ty) {
        ++tie_y_only;
        continue;
      }
      const double prod = dx * dy;
      if (prod > 0.0) {
        ++concordant;
      } else if (prod < 0.0) {
        ++discordant;
      }
    }
  }

  const double n0 = static_cast<double>(concordant + discordant + tie_x_only + tie_y_only + tie_xy);
  const double n1 = static_cast<double>(tie_x_only + tie_xy);
  const double n2 = static_cast<double>(tie_y_only + tie_xy);
  const double den_x = n0 - n1;
  const double den_y = n0 - n2;
  if (den_x <= 0.0 || den_y <= 0.0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double tau = static_cast<double>(concordant - discordant) / std::sqrt(den_x * den_y);
  if (!std::isfinite(tau)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::clamp(tau, -1.0, 1.0);
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
    std::optional<pluss::Series> validity_mask,
    std::optional<pluss::Note> display_labels = std::nullopt,
    std::optional<std::string> display_name = std::nullopt);

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

detail::ThreadImpl::ThreadImpl(Payload payload,
                               std::optional<pluss::Series> validity_mask,
                               std::optional<pluss::Note> display_labels,
                               std::optional<std::string> display_name)
    : payload_(std::move(payload)),
      kind_(infer_kind(payload_)),
      size_(infer_size(payload_)),
      dtype_name_(infer_dtype_name(payload_)),
      validity_mask_(std::move(validity_mask)),
      display_labels_(std::move(display_labels)),
      display_name_(std::move(display_name)) {
  validate_validity_mask(validity_mask_, size_);
  validate_display_labels(display_labels_, size_);
}

namespace {

pluss::msharp::intrusive_ptr<detail::ThreadImpl> build_impl(
    Payload payload,
    std::optional<pluss::Series> validity_mask,
    std::optional<pluss::Note> display_labels,
    std::optional<std::string> display_name) {
  return pluss::msharp::make_intrusive<detail::ThreadImpl>(
      std::move(payload), std::move(validity_mask), std::move(display_labels), std::move(display_name));
}

pluss::msharp::intrusive_ptr<detail::ThreadImpl> clone_impl(const detail::ThreadImpl& src) {
  return build_impl(src.payload_, src.validity_mask_, src.display_labels_, src.display_name_);
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
  auto format_float = [](double value, int precision) {
    std::ostringstream oss;
    oss << std::setprecision(precision) << std::defaultfloat << value;
    return oss.str();
  };
  auto series_scalar_string_at = [&](const pluss::Series& s, std::int64_t idx) -> std::string {
    const auto* base = s.raw_data();
    switch (s.scalar_type()) {
      case pluss::codem::ScalarType::Bool:
        return reinterpret_cast<const bool*>(base)[idx] ? "true" : "false";
      case pluss::codem::ScalarType::Int8:
        return std::to_string(static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[idx]));
      case pluss::codem::ScalarType::Int16:
        return std::to_string(reinterpret_cast<const std::int16_t*>(base)[idx]);
      case pluss::codem::ScalarType::Int32:
        return std::to_string(reinterpret_cast<const std::int32_t*>(base)[idx]);
      case pluss::codem::ScalarType::Int64:
        return std::to_string(reinterpret_cast<const std::int64_t*>(base)[idx]);
      case pluss::codem::ScalarType::UInt8:
        return std::to_string(static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[idx]));
      case pluss::codem::ScalarType::UInt16:
        return std::to_string(reinterpret_cast<const std::uint16_t*>(base)[idx]);
      case pluss::codem::ScalarType::UInt32:
        return std::to_string(reinterpret_cast<const std::uint32_t*>(base)[idx]);
      case pluss::codem::ScalarType::UInt64:
        return std::to_string(reinterpret_cast<const std::uint64_t*>(base)[idx]);
      case pluss::codem::ScalarType::Float16:
        return format_float(pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[idx]), 6);
      case pluss::codem::ScalarType::BFloat16:
        return format_float(pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[idx]), 6);
      case pluss::codem::ScalarType::Float32:
        return format_float(static_cast<double>(reinterpret_cast<const float*>(base)[idx]), 8);
      case pluss::codem::ScalarType::Float64:
        return format_float(reinterpret_cast<const double*>(base)[idx], 12);
      default:
        return "";
    }
  };

  switch (t.kind()) {
    case ThreadKind::Series:
      return series_scalar_string_at(t.as_series(), row);
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
