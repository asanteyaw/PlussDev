std::string_view thread_kind_name(ThreadKind k) noexcept {
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

namespace {
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

void ensure_bool_series_thread_for_logic(const Thread& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Thread is undefined");
  PLUSS_CHECK(t.kind() == ThreadKind::Series,
              std::string(where) + ": operation requires a Series-backed Thread");
  PLUSS_CHECK(t.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
              std::string(where) + ": operation requires a Bool Series payload");
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

ThreadBinaryCompareOp reverse_cmp_op(ThreadBinaryCompareOp op) {
  switch (op) {
    case ThreadBinaryCompareOp::Eq:
      return ThreadBinaryCompareOp::Eq;
    case ThreadBinaryCompareOp::Ne:
      return ThreadBinaryCompareOp::Ne;
    case ThreadBinaryCompareOp::Lt:
      return ThreadBinaryCompareOp::Gt;
    case ThreadBinaryCompareOp::Le:
      return ThreadBinaryCompareOp::Ge;
    case ThreadBinaryCompareOp::Gt:
      return ThreadBinaryCompareOp::Lt;
    case ThreadBinaryCompareOp::Ge:
      return ThreadBinaryCompareOp::Le;
    default:
      return op;
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

Thread carry_case_labels_(const Thread& value, const Thread& reference) {
  return detail::has_display_labels(reference)
             ? detail::with_display_labels(value, detail::display_labels(reference))
             : value;
}

Thread binary_math_scalar(const Thread& lhs, double rhs, ThreadBinaryMathOp op, bool reverse, std::string_view where) {
  PLUSS_CHECK(lhs.defined(), std::string(where) + ": Thread is undefined");
  if (lhs.kind() == ThreadKind::Series) {
    ensure_numeric_thread_for_math(lhs, where);
    return invoke_underlying([&] { return detail::math(lhs, rhs, op, reverse); }, where, "Series math");
  }

  if (lhs.kind() == ThreadKind::Note && op == ThreadBinaryMathOp::Mul) {
    const auto times = nonnegative_integral_scalar(rhs);
    PLUSS_CHECK(times.has_value(),
                std::string(where) + ": Note repetition requires a non-negative integral scalar");
    return invoke_underlying(
        [&] { return Thread(lhs.as_note() * *times, detail::validity_mask(lhs)); },
        where,
        "Note repetition");
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload for operator '" + math_op_name(op)
             + "' (" + std::string(thread_kind_name(lhs.kind())) + " with scalar)");
}

Thread binary_math_pair(const Thread& lhs, const Thread& rhs, ThreadBinaryMathOp op, std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional operation requires identical lengths");
  }
  const auto& left = lhs;
  const auto& right = rhs;
  const auto lk = left.kind();
  const auto rk = right.kind();
  const bool temporal_pair =
      (lk == ThreadKind::Timeline && rk == ThreadKind::Timeline) ||
      (lk == ThreadKind::TimeIndex && rk == ThreadKind::TimeIndex) ||
      (lk == ThreadKind::Timeline && rk == ThreadKind::TimeIndex) ||
      (lk == ThreadKind::TimeIndex && rk == ThreadKind::Timeline);

  if (left.kind() == ThreadKind::Series && right.kind() == ThreadKind::Series) {
    ensure_numeric_thread_for_math(left, where);
    ensure_numeric_thread_for_math(right, where);
    return carry_case_labels_(
        invoke_underlying([&] { return detail::math(left, right, op); }, where, "Series math"),
        left);
  }

  if (left.kind() == ThreadKind::Note && right.kind() == ThreadKind::Note && op == ThreadBinaryMathOp::Add) {
    return carry_case_labels_(
        invoke_underlying(
            [&] { return Thread(left.as_note() + right.as_note(), combine_validity_masks(left, right)); },
            where,
            "Note concatenation"),
        left);
  }

  if (temporal_pair && op == ThreadBinaryMathOp::Sub) {
    return carry_case_labels_(
        invoke_underlying(
            [&] {
              const auto left_tl = (lk == ThreadKind::Timeline) ? left.as_timeline()
                                                                : left.as_timeindex().timeline();
              const auto right_tl = (rk == ThreadKind::Timeline) ? right.as_timeline()
                                                                 : right.as_timeindex().timeline();
              TimelineSpanOptions options;
              options.unit = TimeUnit::Nanos;
              options.signed_result = true;
              return Thread(right_tl.span(left_tl, to_codeh_span_options_(options, where)),
                            combine_validity_masks(left, right));
            },
            where,
            "temporal diff"),
        left);
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload combination for operator '" + math_op_name(op)
             + "' (" + std::string(thread_kind_name(left.kind())) + " vs " +
             std::string(thread_kind_name(right.kind())) + ")");
}

Thread compare_scalar(const Thread& lhs, double rhs, ThreadBinaryCompareOp op, std::string_view where) {
  ensure_numeric_thread_for_math(lhs, where);
  return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "Series compare");
}

Thread compare_pair(const Thread& lhs, const Thread& rhs, ThreadBinaryCompareOp op, std::string_view where) {
  ensure_defined_thread_for_compare(lhs, where);
  ensure_defined_thread_for_compare(rhs, where);
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional comparison requires identical lengths");
  }
  const auto& left = lhs;
  const auto& right = rhs;

  const auto lk = left.kind();
  const auto rk = right.kind();
  const bool temporal_compare =
      (lk == ThreadKind::Timeline && rk == ThreadKind::Timeline) ||
      (lk == ThreadKind::TimeIndex && rk == ThreadKind::TimeIndex) ||
      (lk == ThreadKind::Timeline && rk == ThreadKind::TimeIndex) ||
      (lk == ThreadKind::TimeIndex && rk == ThreadKind::Timeline);

  if (lk == ThreadKind::Series && rk == ThreadKind::Series) {
    return carry_case_labels_(
        invoke_underlying([&] { return left.compare(right, op); }, where, "Series compare"),
        left);
  }

  if (lk == ThreadKind::Note && rk == ThreadKind::Note) {
    return carry_case_labels_(
        invoke_underlying([&] { return left.compare(right, op); }, where, "Note compare"),
        left);
  }

  if (temporal_compare) {
    return carry_case_labels_(
        invoke_underlying([&] { return left.compare(right, op); }, where, "temporal compare"),
        left);
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload combination for operator '" + cmp_op_name(op)
             + "' (" + std::string(thread_kind_name(lk)) + " vs " +
             std::string(thread_kind_name(rk)) + ")");
}

Thread logical_pair(const Thread& lhs,
                    const Thread& rhs,
                    std::string_view where,
                    std::string_view op_name) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional logical operation requires identical lengths");
  }
  ensure_bool_series_thread_for_logic(lhs, where);
  ensure_bool_series_thread_for_logic(rhs, where);
  return invoke_underlying(
      [&] {
        pluss::Series values;
        if (op_name == "and") {
          values = lhs.as_series().logical_and(rhs.as_series());
        } else if (op_name == "or") {
          values = lhs.as_series().logical_or(rhs.as_series());
        } else {
          values = lhs.as_series().logical_xor(rhs.as_series());
        }
        return carry_case_labels_(
            Thread(values, combine_validity_masks(lhs, rhs)),
            lhs);
      },
      where,
      std::string("Series logical_") + std::string(op_name));
}
} // namespace

Thread Thread::add(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Add, false, "Thread::add(double)"); }
Thread Thread::add(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::Add, "Thread::add(Thread)"); }
Thread Thread::radd(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Add, true, "Thread::radd(double)"); }
Thread Thread::radd(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::Add, "Thread::radd(Thread)"); }

Thread Thread::sub(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Sub, false, "Thread::sub(double)"); }
Thread Thread::sub(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::Sub, "Thread::sub(Thread)"); }
Thread Thread::rsub(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Sub, true, "Thread::rsub(double)"); }
Thread Thread::rsub(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::Sub, "Thread::rsub(Thread)"); }

Thread Thread::mul(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Mul, false, "Thread::mul(double)"); }
Thread Thread::mul(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::Mul, "Thread::mul(Thread)"); }
Thread Thread::rmul(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Mul, true, "Thread::rmul(double)"); }
Thread Thread::rmul(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::Mul, "Thread::rmul(Thread)"); }

Thread Thread::div(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Div, false, "Thread::div(double)"); }
Thread Thread::div(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::Div, "Thread::div(Thread)"); }
Thread Thread::rdiv(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Div, true, "Thread::rdiv(double)"); }
Thread Thread::rdiv(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::Div, "Thread::rdiv(Thread)"); }

Thread Thread::truediv(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::TrueDiv, false, "Thread::truediv(double)"); }
Thread Thread::truediv(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::TrueDiv, "Thread::truediv(Thread)"); }
Thread Thread::rtruediv(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::TrueDiv, true, "Thread::rtruediv(double)"); }
Thread Thread::rtruediv(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::TrueDiv, "Thread::rtruediv(Thread)"); }

Thread Thread::floordiv(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::FloorDiv, false, "Thread::floordiv(double)"); }
Thread Thread::floordiv(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::FloorDiv, "Thread::floordiv(Thread)"); }
Thread Thread::rfloordiv(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::FloorDiv, true, "Thread::rfloordiv(double)"); }
Thread Thread::rfloordiv(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::FloorDiv, "Thread::rfloordiv(Thread)"); }

Thread Thread::mod(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Mod, false, "Thread::mod(double)"); }
Thread Thread::mod(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::Mod, "Thread::mod(Thread)"); }
Thread Thread::rmod(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Mod, true, "Thread::rmod(double)"); }
Thread Thread::rmod(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::Mod, "Thread::rmod(Thread)"); }

Thread Thread::pow(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Pow, false, "Thread::pow(double)"); }
Thread Thread::pow(const Thread& other) const { return binary_math_pair(*this, other, ThreadBinaryMathOp::Pow, "Thread::pow(Thread)"); }
Thread Thread::rpow(double other) const { return binary_math_scalar(*this, other, ThreadBinaryMathOp::Pow, true, "Thread::rpow(double)"); }
Thread Thread::rpow(const Thread& other) const { return binary_math_pair(other, *this, ThreadBinaryMathOp::Pow, "Thread::rpow(Thread)"); }

Thread Thread::eq(double other) const { return compare_scalar(*this, other, ThreadBinaryCompareOp::Eq, "Thread::eq(double)"); }
Thread Thread::eq(std::string_view other) const { return compare(other, ThreadBinaryCompareOp::Eq); }
Thread Thread::eq(const Thread& other) const { return compare_pair(*this, other, ThreadBinaryCompareOp::Eq, "Thread::eq(Thread)"); }
Thread Thread::ne(double other) const { return compare_scalar(*this, other, ThreadBinaryCompareOp::Ne, "Thread::ne(double)"); }
Thread Thread::ne(std::string_view other) const { return compare(other, ThreadBinaryCompareOp::Ne); }
Thread Thread::ne(const Thread& other) const { return compare_pair(*this, other, ThreadBinaryCompareOp::Ne, "Thread::ne(Thread)"); }
Thread Thread::lt(double other) const { return compare_scalar(*this, other, ThreadBinaryCompareOp::Lt, "Thread::lt(double)"); }
Thread Thread::lt(std::string_view other) const { return compare(other, ThreadBinaryCompareOp::Lt); }
Thread Thread::lt(const Thread& other) const { return compare_pair(*this, other, ThreadBinaryCompareOp::Lt, "Thread::lt(Thread)"); }
Thread Thread::le(double other) const { return compare_scalar(*this, other, ThreadBinaryCompareOp::Le, "Thread::le(double)"); }
Thread Thread::le(std::string_view other) const { return compare(other, ThreadBinaryCompareOp::Le); }
Thread Thread::le(const Thread& other) const { return compare_pair(*this, other, ThreadBinaryCompareOp::Le, "Thread::le(Thread)"); }
Thread Thread::gt(double other) const { return compare_scalar(*this, other, ThreadBinaryCompareOp::Gt, "Thread::gt(double)"); }
Thread Thread::gt(std::string_view other) const { return compare(other, ThreadBinaryCompareOp::Gt); }
Thread Thread::gt(const Thread& other) const { return compare_pair(*this, other, ThreadBinaryCompareOp::Gt, "Thread::gt(Thread)"); }
Thread Thread::ge(double other) const { return compare_scalar(*this, other, ThreadBinaryCompareOp::Ge, "Thread::ge(double)"); }
Thread Thread::ge(std::string_view other) const { return compare(other, ThreadBinaryCompareOp::Ge); }
Thread Thread::ge(const Thread& other) const { return compare_pair(*this, other, ThreadBinaryCompareOp::Ge, "Thread::ge(Thread)"); }

Thread Thread::land(const Thread& other) const { return logical_pair(*this, other, "Thread::land(Thread)", "and"); }
Thread Thread::rand(const Thread& other) const { return logical_pair(other, *this, "Thread::rand(Thread)", "and"); }
Thread Thread::lor(const Thread& other) const { return logical_pair(*this, other, "Thread::lor(Thread)", "or"); }
Thread Thread::ror(const Thread& other) const { return logical_pair(other, *this, "Thread::ror(Thread)", "or"); }
Thread Thread::lxor(const Thread& other) const { return logical_pair(*this, other, "Thread::lxor(Thread)", "xor"); }
Thread Thread::rxor(const Thread& other) const { return logical_pair(other, *this, "Thread::rxor(Thread)", "xor"); }

Thread operator+(const Thread& lhs, double rhs) {
  return lhs.add(rhs);
}

Thread operator+(double lhs, const Thread& rhs) {
  return rhs.radd(lhs);
}

Thread operator+(const Thread& lhs, const Thread& rhs) {
  return lhs.add(rhs);
}

Thread operator-(const Thread& lhs, double rhs) {
  return lhs.sub(rhs);
}

Thread operator-(double lhs, const Thread& rhs) {
  return rhs.rsub(lhs);
}

Thread operator-(const Thread& lhs, const Thread& rhs) {
  return lhs.sub(rhs);
}

Thread operator*(const Thread& lhs, double rhs) {
  return lhs.mul(rhs);
}

Thread operator*(double lhs, const Thread& rhs) {
  return rhs.rmul(lhs);
}

Thread operator*(const Thread& lhs, const Thread& rhs) {
  return lhs.mul(rhs);
}

Thread operator/(const Thread& lhs, double rhs) {
  return lhs.truediv(rhs);
}

Thread operator/(double lhs, const Thread& rhs) {
  return rhs.rtruediv(lhs);
}

Thread operator/(const Thread& lhs, const Thread& rhs) {
  return lhs.truediv(rhs);
}

Thread operator%(const Thread& lhs, double rhs) {
  return lhs.mod(rhs);
}

Thread operator%(double lhs, const Thread& rhs) {
  return rhs.rmod(lhs);
}

Thread operator%(const Thread& lhs, const Thread& rhs) {
  return lhs.mod(rhs);
}

Thread operator&(const Thread& lhs, const Thread& rhs) {
  return lhs.land(rhs);
}

Thread operator|(const Thread& lhs, const Thread& rhs) {
  return lhs.lor(rhs);
}

Thread operator^(const Thread& lhs, const Thread& rhs) {
  return lhs.lxor(rhs);
}

Thread operator==(const Thread& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ThreadBinaryCompareOp::Eq, "Thread::operator==(Thread,double)");
}

Thread operator==(double lhs, const Thread& rhs) {
  return compare_scalar(rhs, lhs, ThreadBinaryCompareOp::Eq, "Thread::operator==(double,Thread)");
}

Thread operator==(const Thread& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Eq);
}

Thread operator==(std::string_view lhs, const Thread& rhs) {
  return rhs.compare(lhs, ThreadBinaryCompareOp::Eq);
}

Thread operator==(const Thread& lhs, const char* rhs) {
  return lhs == std::string_view(rhs != nullptr ? rhs : "");
}

Thread operator==(const char* lhs, const Thread& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") == rhs;
}

Thread operator==(const Thread& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Eq);
}

Thread operator==(const Thread& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Eq);
}

Thread operator==(const Thread& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Eq);
}

Thread operator==(const Thread& lhs, const pluss::TimeIndex& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Eq);
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

Thread operator!=(const Thread& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ne);
}

Thread operator!=(std::string_view lhs, const Thread& rhs) {
  return rhs.compare(lhs, ThreadBinaryCompareOp::Ne);
}

Thread operator!=(const Thread& lhs, const char* rhs) {
  return lhs != std::string_view(rhs != nullptr ? rhs : "");
}

Thread operator!=(const char* lhs, const Thread& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") != rhs;
}

Thread operator!=(const Thread& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ne);
}

Thread operator!=(const Thread& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ne);
}

Thread operator!=(const Thread& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ne);
}

Thread operator!=(const Thread& lhs, const pluss::TimeIndex& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ne);
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

Thread operator<(const Thread& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Lt);
}

Thread operator<(std::string_view lhs, const Thread& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ThreadBinaryCompareOp::Lt));
}

Thread operator<(const Thread& lhs, const char* rhs) {
  return lhs < std::string_view(rhs != nullptr ? rhs : "");
}

Thread operator<(const char* lhs, const Thread& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") < rhs;
}

Thread operator<(const Thread& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Lt);
}

Thread operator<(const Thread& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Lt);
}

Thread operator<(const Thread& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Lt);
}

Thread operator<(const Thread& lhs, const pluss::TimeIndex& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Lt);
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

Thread operator<=(const Thread& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Le);
}

Thread operator<=(std::string_view lhs, const Thread& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ThreadBinaryCompareOp::Le));
}

Thread operator<=(const Thread& lhs, const char* rhs) {
  return lhs <= std::string_view(rhs != nullptr ? rhs : "");
}

Thread operator<=(const char* lhs, const Thread& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") <= rhs;
}

Thread operator<=(const Thread& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Le);
}

Thread operator<=(const Thread& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Le);
}

Thread operator<=(const Thread& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Le);
}

Thread operator<=(const Thread& lhs, const pluss::TimeIndex& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Le);
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

Thread operator>(const Thread& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Gt);
}

Thread operator>(std::string_view lhs, const Thread& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ThreadBinaryCompareOp::Gt));
}

Thread operator>(const Thread& lhs, const char* rhs) {
  return lhs > std::string_view(rhs != nullptr ? rhs : "");
}

Thread operator>(const char* lhs, const Thread& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") > rhs;
}

Thread operator>(const Thread& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Gt);
}

Thread operator>(const Thread& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Gt);
}

Thread operator>(const Thread& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Gt);
}

Thread operator>(const Thread& lhs, const pluss::TimeIndex& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Gt);
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

Thread operator>=(const Thread& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ge);
}

Thread operator>=(std::string_view lhs, const Thread& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ThreadBinaryCompareOp::Ge));
}

Thread operator>=(const Thread& lhs, const char* rhs) {
  return lhs >= std::string_view(rhs != nullptr ? rhs : "");
}

Thread operator>=(const char* lhs, const Thread& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") >= rhs;
}

Thread operator>=(const Thread& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ge);
}

Thread operator>=(const Thread& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ge);
}

Thread operator>=(const Thread& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ge);
}

Thread operator>=(const Thread& lhs, const pluss::TimeIndex& rhs) {
  return lhs.compare(rhs, ThreadBinaryCompareOp::Ge);
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

namespace {

std::string scalar_time_iso(std::int64_t epoch_nanos) {
  return pluss::timeline::from_epoch(epoch_nanos).isoformat();
}

ThreadValue::HostAny scalar_series_host(const pluss::Series& series) {
  PLUSS_CHECK(series.numel() == 1,
              "ThreadValue::host: Series payload must contain exactly one value");
  const auto scalar = series.item();
  return std::visit(
      [](const auto& value) -> ThreadValue::HostAny {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;
        if constexpr (std::is_same_v<T, bool>) {
          return ThreadValue::HostAny::boolean(value);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
          return ThreadValue::HostAny::integer(value);
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
          return ThreadValue::HostAny::unsigned_integer(value);
        } else if constexpr (std::is_same_v<T, double>) {
          return ThreadValue::HostAny::floating(value);
        } else {
          PLUSS_FAIL("ThreadValue::host: unsupported Series scalar storage");
        }
      },
      scalar.storage());
}

std::int64_t scalar_timeline_nanos(const pluss::Timeline& timeline,
                                   std::string_view where) {
  PLUSS_CHECK(timeline.numel() == 1,
              std::string(where) + ": Timeline payload must contain exactly one value");
  const auto values = timeline.ticks();
  PLUSS_CHECK(values.size() == 1,
              std::string(where) + ": Timeline payload host export returned unexpected size");
  return values.front();
}

std::int64_t scalar_timeindex_nanos(const pluss::TimeIndex& timeindex,
                                    std::string_view where) {
  const auto values = timeindex.host();
  PLUSS_CHECK(values.size() == 1,
              std::string(where) + ": TimeIndex payload must contain exactly one value");
  return values.front();
}

} // namespace

ThreadValue::HostAny ThreadValue::HostAny::boolean(bool value) {
  HostAny out;
  out.kind_ = Kind::Bool;
  out.storage_ = value;
  return out;
}

ThreadValue::HostAny ThreadValue::HostAny::integer(std::int64_t value) {
  HostAny out;
  out.kind_ = Kind::Int;
  out.storage_ = value;
  return out;
}

ThreadValue::HostAny ThreadValue::HostAny::unsigned_integer(std::uint64_t value) {
  HostAny out;
  out.kind_ = Kind::UInt;
  out.storage_ = value;
  return out;
}

ThreadValue::HostAny ThreadValue::HostAny::floating(double value) {
  HostAny out;
  out.kind_ = Kind::Double;
  out.storage_ = value;
  return out;
}

ThreadValue::HostAny ThreadValue::HostAny::string(std::string value) {
  HostAny out;
  out.kind_ = Kind::String;
  out.storage_ = std::move(value);
  return out;
}

ThreadValue::HostAny ThreadValue::HostAny::time(std::int64_t epoch_nanos,
                                                std::string iso) {
  HostAny out;
  out.kind_ = Kind::Time;
  out.storage_ = epoch_nanos;
  out.display_ = std::move(iso);
  return out;
}

ThreadValue::HostAny::Kind ThreadValue::HostAny::kind() const noexcept {
  return kind_;
}

const ThreadValue::HostAny::Storage& ThreadValue::HostAny::storage() const noexcept {
  return storage_;
}

double ThreadValue::HostAny::to_dbl() const {
  switch (kind_) {
    case Kind::Bool:
      return std::get<bool>(storage_) ? 1.0 : 0.0;
    case Kind::Int:
      return static_cast<double>(std::get<std::int64_t>(storage_));
    case Kind::UInt:
      return static_cast<double>(std::get<std::uint64_t>(storage_));
    case Kind::Double:
      return std::get<double>(storage_);
    case Kind::String:
      PLUSS_FAIL("ThreadValue::host().to_dbl: Note values are not numeric");
    case Kind::Time:
      PLUSS_FAIL("ThreadValue::host().to_dbl: time values do not convert to double; use to_int() for epoch nanoseconds");
    default:
      PLUSS_FAIL("ThreadValue::host().to_dbl: unsupported scalar kind");
  }
}

std::int64_t ThreadValue::HostAny::to_int() const {
  switch (kind_) {
    case Kind::Bool:
      return std::get<bool>(storage_) ? 1 : 0;
    case Kind::Int:
      return std::get<std::int64_t>(storage_);
    case Kind::UInt: {
      const auto value = std::get<std::uint64_t>(storage_);
      PLUSS_CHECK(value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
                  "ThreadValue::host().to_int: unsigned value exceeds int64 range");
      return static_cast<std::int64_t>(value);
    }
    case Kind::Double: {
      const auto value = std::get<double>(storage_);
      PLUSS_CHECK(std::isfinite(value), "ThreadValue::host().to_int: floating value is not finite");
      PLUSS_CHECK(value >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
                      value <= static_cast<double>(std::numeric_limits<std::int64_t>::max()),
                  "ThreadValue::host().to_int: floating value exceeds int64 range");
      return static_cast<std::int64_t>(value);
    }
    case Kind::Time:
      return std::get<std::int64_t>(storage_);
    case Kind::String:
      PLUSS_FAIL("ThreadValue::host().to_int: Note values are not integral");
    default:
      PLUSS_FAIL("ThreadValue::host().to_int: unsupported scalar kind");
  }
}

std::string ThreadValue::HostAny::to_str() const {
  switch (kind_) {
    case Kind::Bool:
      return std::get<bool>(storage_) ? "true" : "false";
    case Kind::Int:
      return std::to_string(std::get<std::int64_t>(storage_));
    case Kind::UInt:
      return std::to_string(std::get<std::uint64_t>(storage_));
    case Kind::Double: {
      std::ostringstream os;
      os << std::get<double>(storage_);
      return os.str();
    }
    case Kind::String:
      return std::get<std::string>(storage_);
    case Kind::Time:
      return display_.empty() ? std::to_string(std::get<std::int64_t>(storage_)) : display_;
    default:
      PLUSS_FAIL("ThreadValue::host().to_str: unsupported scalar kind");
  }
}

ThreadValue::ThreadValue(Thread thread) : thread_(std::move(thread)) {}

bool ThreadValue::defined() const noexcept { return thread_.defined(); }
ThreadKind ThreadValue::kind() const { return thread_.kind(); }
const Thread& ThreadValue::thread() const noexcept { return thread_; }
ThreadValue::Payload ThreadValue::payload() const {
  switch (kind()) {
    case ThreadKind::Series:
      return as_series();
    case ThreadKind::Note:
      return as_note();
    case ThreadKind::Timeline:
      return as_timeline();
    case ThreadKind::TimeIndex:
      return as_timeindex();
    default:
      PLUSS_FAIL("ThreadValue::payload: unsupported ThreadKind");
  }
}

pluss::Series ThreadValue::as_series() const { return thread_.as_series(); }
pluss::Note ThreadValue::as_note() const { return thread_.as_note(); }
pluss::Timeline ThreadValue::as_timeline() const { return thread_.as_timeline(); }
pluss::TimeIndex ThreadValue::as_timeindex() const { return thread_.as_timeindex(); }

ThreadValue::HostAny ThreadValue::host() const {
  PLUSS_CHECK(defined(), "ThreadValue::host: ThreadValue is undefined");
  PLUSS_CHECK(thread_.size() == 1,
              "ThreadValue::host: Thread payload must contain exactly one value");
  switch (kind()) {
    case ThreadKind::Series:
      return scalar_series_host(as_series());
    case ThreadKind::Note: {
      const auto& note = as_note();
      PLUSS_CHECK(note.numel() == 1,
                  "ThreadValue::host: Note payload must contain exactly one value");
      return HostAny::string(std::string(note.flat_at(0)));
    }
    case ThreadKind::Timeline: {
      const auto nanos = scalar_timeline_nanos(as_timeline(), "ThreadValue::host");
      return HostAny::time(nanos, scalar_time_iso(nanos));
    }
    case ThreadKind::TimeIndex: {
      const auto nanos = scalar_timeindex_nanos(as_timeindex(), "ThreadValue::host");
      return HostAny::time(nanos, scalar_time_iso(nanos));
    }
    default:
      PLUSS_FAIL("ThreadValue::host: unsupported ThreadKind");
  }
}

double ThreadValue::to_dbl() const { return host().to_dbl(); }
std::int64_t ThreadValue::to_int() const { return host().to_int(); }
std::string ThreadValue::to_str() const { return host().to_str(); }

ThreadValue::operator pluss::Series() const { return as_series(); }
ThreadValue::operator pluss::Note() const { return as_note(); }
ThreadValue::operator pluss::Timeline() const { return as_timeline(); }
ThreadValue::operator pluss::TimeIndex() const { return as_timeindex(); }

std::ostream& operator<<(std::ostream& os, const ThreadValue::HostAny& value) {
  os << value.to_str();
  return os;
}

std::ostream& operator<<(std::ostream& os, const ThreadValue& value) {
  output_render(os, value);
  return os;
}

void output_render(std::ostream& os, const ThreadValue& value) {
  std::visit([&](const auto& v) { os << v; }, value.payload());
}
