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

Thread operator&(const Thread& lhs, const Thread& rhs) {
  const std::string_view where = "Thread::operator&(Thread,Thread)";
  ensure_bool_series_thread_for_logic(lhs, where);
  ensure_bool_series_thread_for_logic(rhs, where);
  PLUSS_CHECK(lhs.size() == rhs.size(), std::string(where) + ": Thread size mismatch");
  return invoke_underlying(
      [&] { return Thread(lhs.as_series().logical_and(rhs.as_series()), combine_validity_masks(lhs, rhs)); },
      where,
      "Series logical_and");
}

Thread operator|(const Thread& lhs, const Thread& rhs) {
  const std::string_view where = "Thread::operator|(Thread,Thread)";
  ensure_bool_series_thread_for_logic(lhs, where);
  ensure_bool_series_thread_for_logic(rhs, where);
  PLUSS_CHECK(lhs.size() == rhs.size(), std::string(where) + ": Thread size mismatch");
  return invoke_underlying(
      [&] { return Thread(lhs.as_series().logical_or(rhs.as_series()), combine_validity_masks(lhs, rhs)); },
      where,
      "Series logical_or");
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

ThreadValue::operator pluss::Series() const { return as_series(); }
ThreadValue::operator pluss::Note() const { return as_note(); }
ThreadValue::operator pluss::Timeline() const { return as_timeline(); }
ThreadValue::operator pluss::TimeIndex() const { return as_timeindex(); }

std::ostream& operator<<(std::ostream& os, const ThreadValue& value) {
  output_render(os, value);
  return os;
}

void output_render(std::ostream& os, const ThreadValue& value) {
  std::visit([&](const auto& v) { os << v; }, value.payload());
}
