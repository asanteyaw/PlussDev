std::string_view slate_kind_name(SlateKind k) noexcept {
  switch (k) {
    case SlateKind::Series:
      return "Series";
    case SlateKind::Note:
      return "Note";
    case SlateKind::Timeline:
      return "Timeline";
    case SlateKind::Duration:
      return "Duration";
    default:
      return "Unknown";
  }
}

namespace {
void ensure_numeric_slate_for_math(const Slate& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Slate is undefined");
  PLUSS_CHECK(t.kind() == SlateKind::Series,
              std::string(where) + ": operation requires a Series-backed Slate");
  PLUSS_CHECK(is_numeric_series_type(t.as_series()),
              std::string(where) + ": operation requires a numeric Series payload");
}

void ensure_defined_slate_for_compare(const Slate& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Slate is undefined");
}

void ensure_bool_series_slate_for_logic(const Slate& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Slate is undefined");
  PLUSS_CHECK(t.kind() == SlateKind::Series,
              std::string(where) + ": operation requires a Series-backed Slate");
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

const char* math_op_name(SlateBinaryMathOp op) {
  switch (op) {
    case SlateBinaryMathOp::Add:
      return "+";
    case SlateBinaryMathOp::Sub:
      return "-";
    case SlateBinaryMathOp::Mul:
      return "*";
    case SlateBinaryMathOp::Div:
      return "/";
    case SlateBinaryMathOp::TrueDiv:
      return "/";
    case SlateBinaryMathOp::FloorDiv:
      return "//";
    case SlateBinaryMathOp::Mod:
      return "%";
    case SlateBinaryMathOp::Pow:
      return "**";
    default:
      return "?";
  }
}

const char* cmp_op_name(SlateBinaryCompareOp op) {
  switch (op) {
    case SlateBinaryCompareOp::Eq:
      return "==";
    case SlateBinaryCompareOp::Ne:
      return "!=";
    case SlateBinaryCompareOp::Lt:
      return "<";
    case SlateBinaryCompareOp::Le:
      return "<=";
    case SlateBinaryCompareOp::Gt:
      return ">";
    case SlateBinaryCompareOp::Ge:
      return ">=";
    default:
      return "?";
  }
}

SlateBinaryCompareOp reverse_cmp_op(SlateBinaryCompareOp op) {
  switch (op) {
    case SlateBinaryCompareOp::Eq:
      return SlateBinaryCompareOp::Eq;
    case SlateBinaryCompareOp::Ne:
      return SlateBinaryCompareOp::Ne;
    case SlateBinaryCompareOp::Lt:
      return SlateBinaryCompareOp::Gt;
    case SlateBinaryCompareOp::Le:
      return SlateBinaryCompareOp::Ge;
    case SlateBinaryCompareOp::Gt:
      return SlateBinaryCompareOp::Lt;
    case SlateBinaryCompareOp::Ge:
      return SlateBinaryCompareOp::Le;
    default:
      return op;
  }
}

template <class Fn>
Slate invoke_underlying(Fn&& fn, std::string_view where, std::string_view what) {
  try {
    return fn();
  } catch (const std::exception& e) {
    PLUSS_FAIL(std::string(where) + ": underlying " + std::string(what) + " operation failed: " + e.what());
  } catch (...) {
    PLUSS_FAIL(std::string(where) + ": underlying " + std::string(what) + " operation failed");
  }
}

Slate carry_case_labels_(const Slate& value, const Slate& reference) {
  return detail::has_display_labels(reference)
             ? detail::with_display_labels(value, detail::display_labels(reference))
             : value;
}

Slate binary_math_scalar(const Slate& lhs, double rhs, SlateBinaryMathOp op, bool reverse, std::string_view where) {
  PLUSS_CHECK(lhs.defined(), std::string(where) + ": Slate is undefined");
  if (lhs.kind() == SlateKind::Series) {
    ensure_numeric_slate_for_math(lhs, where);
    return invoke_underlying([&] { return detail::math(lhs, rhs, op, reverse); }, where, "Series math");
  }

  if (lhs.kind() == SlateKind::Note && op == SlateBinaryMathOp::Mul) {
    const auto times = nonnegative_integral_scalar(rhs);
    PLUSS_CHECK(times.has_value(),
                std::string(where) + ": Note repetition requires a non-negative integral scalar");
    return invoke_underlying(
        [&] { return Slate(lhs.as_note() * *times, detail::validity_mask(lhs)); },
        where,
        "Note repetition");
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload for operator '" + math_op_name(op)
             + "' (" + std::string(slate_kind_name(lhs.kind())) + " with scalar)");
}

Slate binary_math_pair(const Slate& lhs, const Slate& rhs, SlateBinaryMathOp op, std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional operation requires identical lengths");
  }
  const auto& left = lhs;
  const auto& right = rhs;
  const auto lk = left.kind();
  const auto rk = right.kind();
  const bool temporal_pair = lk == SlateKind::Timeline && rk == SlateKind::Timeline;

  if (left.kind() == SlateKind::Series && right.kind() == SlateKind::Series) {
    ensure_numeric_slate_for_math(left, where);
    ensure_numeric_slate_for_math(right, where);
    return carry_case_labels_(
        invoke_underlying([&] { return detail::math(left, right, op); }, where, "Series math"),
        left);
  }

  if (left.kind() == SlateKind::Note && right.kind() == SlateKind::Note && op == SlateBinaryMathOp::Add) {
    return carry_case_labels_(
        invoke_underlying(
            [&] { return Slate(left.as_note() + right.as_note(), combine_validity_masks(left, right)); },
            where,
            "Note concatenation"),
        left);
  }

  if (temporal_pair && op == SlateBinaryMathOp::Sub) {
    return carry_case_labels_(
        invoke_underlying(
            [&] {
              const auto left_tl = left.as_timeline();
              const auto right_tl = right.as_timeline();
              TimelineSpanOptions options;
              options.unit = TimeUnit::Nanos;
              options.signed_result = true;
              return Slate(right_tl.span(left_tl, to_codeh_span_options_(options, where)),
                            combine_validity_masks(left, right));
            },
            where,
            "temporal diff"),
        left);
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload combination for operator '" + math_op_name(op)
             + "' (" + std::string(slate_kind_name(left.kind())) + " vs " +
             std::string(slate_kind_name(right.kind())) + ")");
}

Slate compare_scalar(const Slate& lhs, double rhs, SlateBinaryCompareOp op, std::string_view where) {
  ensure_numeric_slate_for_math(lhs, where);
  return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "Series compare");
}

Slate compare_pair(const Slate& lhs, const Slate& rhs, SlateBinaryCompareOp op, std::string_view where) {
  ensure_defined_slate_for_compare(lhs, where);
  ensure_defined_slate_for_compare(rhs, where);
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional comparison requires identical lengths");
  }
  const auto& left = lhs;
  const auto& right = rhs;

  const auto lk = left.kind();
  const auto rk = right.kind();
  const bool temporal_compare = lk == SlateKind::Timeline && rk == SlateKind::Timeline;

  if (lk == SlateKind::Series && rk == SlateKind::Series) {
    return carry_case_labels_(
        invoke_underlying([&] { return left.compare(right, op); }, where, "Series compare"),
        left);
  }

  if (lk == SlateKind::Note && rk == SlateKind::Note) {
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
             + "' (" + std::string(slate_kind_name(lk)) + " vs " +
             std::string(slate_kind_name(rk)) + ")");
}

Slate logical_pair(const Slate& lhs,
                    const Slate& rhs,
                    std::string_view where,
                    std::string_view op_name) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional logical operation requires identical lengths");
  }
  ensure_bool_series_slate_for_logic(lhs, where);
  ensure_bool_series_slate_for_logic(rhs, where);
  return invoke_underlying(
      [&] {
        pluss::Series values = pluss::series::undefined();
        if (op_name == "and") {
          values = lhs.as_series().logical_and(rhs.as_series());
        } else if (op_name == "or") {
          values = lhs.as_series().logical_or(rhs.as_series());
        } else {
          values = lhs.as_series().logical_xor(rhs.as_series());
        }
        return carry_case_labels_(
            Slate(values, combine_validity_masks(lhs, rhs)),
            lhs);
      },
      where,
      std::string("Series logical_") + std::string(op_name));
}
} // namespace

Slate Slate::add(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Add, false, "Slate::add(double)"); }
Slate Slate::add(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::Add, "Slate::add(Slate)"); }
Slate Slate::radd(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Add, true, "Slate::radd(double)"); }
Slate Slate::radd(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::Add, "Slate::radd(Slate)"); }

Slate Slate::sub(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Sub, false, "Slate::sub(double)"); }
Slate Slate::sub(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::Sub, "Slate::sub(Slate)"); }
Slate Slate::rsub(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Sub, true, "Slate::rsub(double)"); }
Slate Slate::rsub(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::Sub, "Slate::rsub(Slate)"); }

Slate Slate::mul(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Mul, false, "Slate::mul(double)"); }
Slate Slate::mul(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::Mul, "Slate::mul(Slate)"); }
Slate Slate::rmul(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Mul, true, "Slate::rmul(double)"); }
Slate Slate::rmul(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::Mul, "Slate::rmul(Slate)"); }

Slate Slate::div(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Div, false, "Slate::div(double)"); }
Slate Slate::div(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::Div, "Slate::div(Slate)"); }
Slate Slate::rdiv(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Div, true, "Slate::rdiv(double)"); }
Slate Slate::rdiv(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::Div, "Slate::rdiv(Slate)"); }

Slate Slate::truediv(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::TrueDiv, false, "Slate::truediv(double)"); }
Slate Slate::truediv(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::TrueDiv, "Slate::truediv(Slate)"); }
Slate Slate::rtruediv(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::TrueDiv, true, "Slate::rtruediv(double)"); }
Slate Slate::rtruediv(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::TrueDiv, "Slate::rtruediv(Slate)"); }

Slate Slate::floordiv(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::FloorDiv, false, "Slate::floordiv(double)"); }
Slate Slate::floordiv(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::FloorDiv, "Slate::floordiv(Slate)"); }
Slate Slate::rfloordiv(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::FloorDiv, true, "Slate::rfloordiv(double)"); }
Slate Slate::rfloordiv(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::FloorDiv, "Slate::rfloordiv(Slate)"); }

Slate Slate::mod(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Mod, false, "Slate::mod(double)"); }
Slate Slate::mod(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::Mod, "Slate::mod(Slate)"); }
Slate Slate::rmod(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Mod, true, "Slate::rmod(double)"); }
Slate Slate::rmod(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::Mod, "Slate::rmod(Slate)"); }

Slate Slate::pow(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Pow, false, "Slate::pow(double)"); }
Slate Slate::pow(const Slate& other) const { return binary_math_pair(*this, other, SlateBinaryMathOp::Pow, "Slate::pow(Slate)"); }
Slate Slate::rpow(double other) const { return binary_math_scalar(*this, other, SlateBinaryMathOp::Pow, true, "Slate::rpow(double)"); }
Slate Slate::rpow(const Slate& other) const { return binary_math_pair(other, *this, SlateBinaryMathOp::Pow, "Slate::rpow(Slate)"); }

Slate Slate::eq(double other) const { return compare_scalar(*this, other, SlateBinaryCompareOp::Eq, "Slate::eq(double)"); }
Slate Slate::eq(std::string_view other) const { return compare(other, SlateBinaryCompareOp::Eq); }
Slate Slate::eq(const Slate& other) const { return compare_pair(*this, other, SlateBinaryCompareOp::Eq, "Slate::eq(Slate)"); }
Slate Slate::ne(double other) const { return compare_scalar(*this, other, SlateBinaryCompareOp::Ne, "Slate::ne(double)"); }
Slate Slate::ne(std::string_view other) const { return compare(other, SlateBinaryCompareOp::Ne); }
Slate Slate::ne(const Slate& other) const { return compare_pair(*this, other, SlateBinaryCompareOp::Ne, "Slate::ne(Slate)"); }
Slate Slate::lt(double other) const { return compare_scalar(*this, other, SlateBinaryCompareOp::Lt, "Slate::lt(double)"); }
Slate Slate::lt(std::string_view other) const { return compare(other, SlateBinaryCompareOp::Lt); }
Slate Slate::lt(const Slate& other) const { return compare_pair(*this, other, SlateBinaryCompareOp::Lt, "Slate::lt(Slate)"); }
Slate Slate::le(double other) const { return compare_scalar(*this, other, SlateBinaryCompareOp::Le, "Slate::le(double)"); }
Slate Slate::le(std::string_view other) const { return compare(other, SlateBinaryCompareOp::Le); }
Slate Slate::le(const Slate& other) const { return compare_pair(*this, other, SlateBinaryCompareOp::Le, "Slate::le(Slate)"); }
Slate Slate::gt(double other) const { return compare_scalar(*this, other, SlateBinaryCompareOp::Gt, "Slate::gt(double)"); }
Slate Slate::gt(std::string_view other) const { return compare(other, SlateBinaryCompareOp::Gt); }
Slate Slate::gt(const Slate& other) const { return compare_pair(*this, other, SlateBinaryCompareOp::Gt, "Slate::gt(Slate)"); }
Slate Slate::ge(double other) const { return compare_scalar(*this, other, SlateBinaryCompareOp::Ge, "Slate::ge(double)"); }
Slate Slate::ge(std::string_view other) const { return compare(other, SlateBinaryCompareOp::Ge); }
Slate Slate::ge(const Slate& other) const { return compare_pair(*this, other, SlateBinaryCompareOp::Ge, "Slate::ge(Slate)"); }

Slate Slate::land(const Slate& other) const { return logical_pair(*this, other, "Slate::land(Slate)", "and"); }
Slate Slate::rand(const Slate& other) const { return logical_pair(other, *this, "Slate::rand(Slate)", "and"); }
Slate Slate::lor(const Slate& other) const { return logical_pair(*this, other, "Slate::lor(Slate)", "or"); }
Slate Slate::ror(const Slate& other) const { return logical_pair(other, *this, "Slate::ror(Slate)", "or"); }
Slate Slate::lxor(const Slate& other) const { return logical_pair(*this, other, "Slate::lxor(Slate)", "xor"); }
Slate Slate::rxor(const Slate& other) const { return logical_pair(other, *this, "Slate::rxor(Slate)", "xor"); }

Slate operator+(const Slate& lhs, double rhs) {
  return lhs.add(rhs);
}

Slate operator+(double lhs, const Slate& rhs) {
  return rhs.radd(lhs);
}

Slate operator+(const Slate& lhs, const Slate& rhs) {
  return lhs.add(rhs);
}

Slate operator-(const Slate& lhs, double rhs) {
  return lhs.sub(rhs);
}

Slate operator-(double lhs, const Slate& rhs) {
  return rhs.rsub(lhs);
}

Slate operator-(const Slate& lhs, const Slate& rhs) {
  return lhs.sub(rhs);
}

Slate operator*(const Slate& lhs, double rhs) {
  return lhs.mul(rhs);
}

Slate operator*(double lhs, const Slate& rhs) {
  return rhs.rmul(lhs);
}

Slate operator*(const Slate& lhs, const Slate& rhs) {
  return lhs.mul(rhs);
}

Slate operator/(const Slate& lhs, double rhs) {
  return lhs.truediv(rhs);
}

Slate operator/(double lhs, const Slate& rhs) {
  return rhs.rtruediv(lhs);
}

Slate operator/(const Slate& lhs, const Slate& rhs) {
  return lhs.truediv(rhs);
}

Slate operator%(const Slate& lhs, double rhs) {
  return lhs.mod(rhs);
}

Slate operator%(double lhs, const Slate& rhs) {
  return rhs.rmod(lhs);
}

Slate operator%(const Slate& lhs, const Slate& rhs) {
  return lhs.mod(rhs);
}

Slate operator&(const Slate& lhs, const Slate& rhs) {
  return lhs.land(rhs);
}

Slate operator|(const Slate& lhs, const Slate& rhs) {
  return lhs.lor(rhs);
}

Slate operator^(const Slate& lhs, const Slate& rhs) {
  return lhs.lxor(rhs);
}

Slate operator==(const Slate& lhs, double rhs) {
  return compare_scalar(lhs, rhs, SlateBinaryCompareOp::Eq, "Slate::operator==(Slate,double)");
}

Slate operator==(double lhs, const Slate& rhs) {
  return compare_scalar(rhs, lhs, SlateBinaryCompareOp::Eq, "Slate::operator==(double,Slate)");
}

Slate operator==(const Slate& lhs, std::string_view rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Eq);
}

Slate operator==(std::string_view lhs, const Slate& rhs) {
  return rhs.compare(lhs, SlateBinaryCompareOp::Eq);
}

Slate operator==(const Slate& lhs, const char* rhs) {
  return lhs == std::string_view(rhs != nullptr ? rhs : "");
}

Slate operator==(const char* lhs, const Slate& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") == rhs;
}

Slate operator==(const Slate& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Eq);
}

Slate operator==(const Slate& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Eq);
}

Slate operator==(const Slate& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Eq);
}


Slate operator==(const Slate& lhs, const Slate& rhs) {
  return compare_pair(lhs, rhs, SlateBinaryCompareOp::Eq, "Slate::operator==(Slate,Slate)");
}

Slate operator!=(const Slate& lhs, double rhs) {
  return compare_scalar(lhs, rhs, SlateBinaryCompareOp::Ne, "Slate::operator!=(Slate,double)");
}

Slate operator!=(double lhs, const Slate& rhs) {
  return compare_scalar(rhs, lhs, SlateBinaryCompareOp::Ne, "Slate::operator!=(double,Slate)");
}

Slate operator!=(const Slate& lhs, std::string_view rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ne);
}

Slate operator!=(std::string_view lhs, const Slate& rhs) {
  return rhs.compare(lhs, SlateBinaryCompareOp::Ne);
}

Slate operator!=(const Slate& lhs, const char* rhs) {
  return lhs != std::string_view(rhs != nullptr ? rhs : "");
}

Slate operator!=(const char* lhs, const Slate& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") != rhs;
}

Slate operator!=(const Slate& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ne);
}

Slate operator!=(const Slate& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ne);
}

Slate operator!=(const Slate& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ne);
}


Slate operator!=(const Slate& lhs, const Slate& rhs) {
  return compare_pair(lhs, rhs, SlateBinaryCompareOp::Ne, "Slate::operator!=(Slate,Slate)");
}

Slate operator<(const Slate& lhs, double rhs) {
  return compare_scalar(lhs, rhs, SlateBinaryCompareOp::Lt, "Slate::operator<(Slate,double)");
}

Slate operator<(double lhs, const Slate& rhs) {
  return compare_scalar(rhs, lhs, SlateBinaryCompareOp::Gt, "Slate::operator<(double,Slate)");
}

Slate operator<(const Slate& lhs, std::string_view rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Lt);
}

Slate operator<(std::string_view lhs, const Slate& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(SlateBinaryCompareOp::Lt));
}

Slate operator<(const Slate& lhs, const char* rhs) {
  return lhs < std::string_view(rhs != nullptr ? rhs : "");
}

Slate operator<(const char* lhs, const Slate& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") < rhs;
}

Slate operator<(const Slate& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Lt);
}

Slate operator<(const Slate& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Lt);
}

Slate operator<(const Slate& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Lt);
}


Slate operator<(const Slate& lhs, const Slate& rhs) {
  return compare_pair(lhs, rhs, SlateBinaryCompareOp::Lt, "Slate::operator<(Slate,Slate)");
}

Slate operator<=(const Slate& lhs, double rhs) {
  return compare_scalar(lhs, rhs, SlateBinaryCompareOp::Le, "Slate::operator<=(Slate,double)");
}

Slate operator<=(double lhs, const Slate& rhs) {
  return compare_scalar(rhs, lhs, SlateBinaryCompareOp::Ge, "Slate::operator<=(double,Slate)");
}

Slate operator<=(const Slate& lhs, std::string_view rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Le);
}

Slate operator<=(std::string_view lhs, const Slate& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(SlateBinaryCompareOp::Le));
}

Slate operator<=(const Slate& lhs, const char* rhs) {
  return lhs <= std::string_view(rhs != nullptr ? rhs : "");
}

Slate operator<=(const char* lhs, const Slate& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") <= rhs;
}

Slate operator<=(const Slate& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Le);
}

Slate operator<=(const Slate& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Le);
}

Slate operator<=(const Slate& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Le);
}


Slate operator<=(const Slate& lhs, const Slate& rhs) {
  return compare_pair(lhs, rhs, SlateBinaryCompareOp::Le, "Slate::operator<=(Slate,Slate)");
}

Slate operator>(const Slate& lhs, double rhs) {
  return compare_scalar(lhs, rhs, SlateBinaryCompareOp::Gt, "Slate::operator>(Slate,double)");
}

Slate operator>(double lhs, const Slate& rhs) {
  return compare_scalar(rhs, lhs, SlateBinaryCompareOp::Lt, "Slate::operator>(double,Slate)");
}

Slate operator>(const Slate& lhs, std::string_view rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Gt);
}

Slate operator>(std::string_view lhs, const Slate& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(SlateBinaryCompareOp::Gt));
}

Slate operator>(const Slate& lhs, const char* rhs) {
  return lhs > std::string_view(rhs != nullptr ? rhs : "");
}

Slate operator>(const char* lhs, const Slate& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") > rhs;
}

Slate operator>(const Slate& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Gt);
}

Slate operator>(const Slate& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Gt);
}

Slate operator>(const Slate& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Gt);
}


Slate operator>(const Slate& lhs, const Slate& rhs) {
  return compare_pair(lhs, rhs, SlateBinaryCompareOp::Gt, "Slate::operator>(Slate,Slate)");
}

Slate operator>=(const Slate& lhs, double rhs) {
  return compare_scalar(lhs, rhs, SlateBinaryCompareOp::Ge, "Slate::operator>=(Slate,double)");
}

Slate operator>=(double lhs, const Slate& rhs) {
  return compare_scalar(rhs, lhs, SlateBinaryCompareOp::Le, "Slate::operator>=(double,Slate)");
}

Slate operator>=(const Slate& lhs, std::string_view rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ge);
}

Slate operator>=(std::string_view lhs, const Slate& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(SlateBinaryCompareOp::Ge));
}

Slate operator>=(const Slate& lhs, const char* rhs) {
  return lhs >= std::string_view(rhs != nullptr ? rhs : "");
}

Slate operator>=(const char* lhs, const Slate& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") >= rhs;
}

Slate operator>=(const Slate& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ge);
}

Slate operator>=(const Slate& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ge);
}

Slate operator>=(const Slate& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, SlateBinaryCompareOp::Ge);
}


Slate operator>=(const Slate& lhs, const Slate& rhs) {
  return compare_pair(lhs, rhs, SlateBinaryCompareOp::Ge, "Slate::operator>=(Slate,Slate)");
}

const pluss::Series& Slate::as_series() const {
  PLUSS_CHECK(kind() == SlateKind::Series, "Slate: payload is not Series");
  return std::get<pluss::Series>(impl_ref_().payload_);
}

const pluss::Note& Slate::as_note() const {
  PLUSS_CHECK(kind() == SlateKind::Note, "Slate: payload is not Note");
  return std::get<pluss::Note>(impl_ref_().payload_);
}

const pluss::Timeline& Slate::as_timeline() const {
  PLUSS_CHECK(kind() == SlateKind::Timeline, "Slate: payload is not Timeline");
  return std::get<pluss::Timeline>(impl_ref_().payload_);
}

const pluss::Duration& Slate::as_duration() const {
  PLUSS_CHECK(kind() == SlateKind::Duration, "Slate: payload is not Duration");
  return std::get<pluss::Duration>(impl_ref_().payload_);
}

namespace {

std::string scalar_time_iso(std::int64_t epoch_nanos) {
  return pluss::timeline::from_epoch(epoch_nanos).isoformat();
}

SlateValue::HostAny scalar_series_host(const pluss::Series& series) {
  PLUSS_CHECK(series.numel() == 1,
              "SlateValue::host: Series payload must contain exactly one value");
  const auto scalar = series.item();
  return std::visit(
      [](const auto& value) -> SlateValue::HostAny {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;
        if constexpr (std::is_same_v<T, bool>) {
          return SlateValue::HostAny::boolean(value);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
          return SlateValue::HostAny::integer(value);
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
          return SlateValue::HostAny::unsigned_integer(value);
        } else if constexpr (std::is_same_v<T, double>) {
          return SlateValue::HostAny::floating(value);
        } else {
          PLUSS_FAIL("SlateValue::host: unsupported Series scalar storage");
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

std::int64_t scalar_duration_nanos(const pluss::Duration& duration,
                                   std::string_view where) {
  const auto values = duration.host();
  PLUSS_CHECK(values.size() == 1,
              std::string(where) + ": Duration payload must contain exactly one value");
  return values.front();
}

} // namespace

SlateValue::HostAny SlateValue::HostAny::boolean(bool value) {
  HostAny out;
  out.kind_ = Kind::Bool;
  out.storage_ = value;
  return out;
}

SlateValue::HostAny SlateValue::HostAny::integer(std::int64_t value) {
  HostAny out;
  out.kind_ = Kind::Int;
  out.storage_ = value;
  return out;
}

SlateValue::HostAny SlateValue::HostAny::unsigned_integer(std::uint64_t value) {
  HostAny out;
  out.kind_ = Kind::UInt;
  out.storage_ = value;
  return out;
}

SlateValue::HostAny SlateValue::HostAny::floating(double value) {
  HostAny out;
  out.kind_ = Kind::Double;
  out.storage_ = value;
  return out;
}

SlateValue::HostAny SlateValue::HostAny::string(std::string value) {
  HostAny out;
  out.kind_ = Kind::String;
  out.storage_ = std::move(value);
  return out;
}

SlateValue::HostAny SlateValue::HostAny::time(std::int64_t epoch_nanos,
                                                std::string iso) {
  HostAny out;
  out.kind_ = Kind::Time;
  out.storage_ = epoch_nanos;
  out.display_ = std::move(iso);
  return out;
}

SlateValue::HostAny SlateValue::HostAny::duration(std::int64_t nanos,
                                                    std::string text) {
  HostAny out;
  out.kind_ = Kind::Duration;
  out.storage_ = nanos;
  out.display_ = std::move(text);
  return out;
}

SlateValue::HostAny::Kind SlateValue::HostAny::kind() const noexcept {
  return kind_;
}

const SlateValue::HostAny::Storage& SlateValue::HostAny::storage() const noexcept {
  return storage_;
}

double SlateValue::HostAny::to_dbl() const {
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
      PLUSS_FAIL("SlateValue::host().to_dbl: Note values are not numeric");
    case Kind::Time:
      PLUSS_FAIL("SlateValue::host().to_dbl: time values do not convert to double; use to_int() for epoch nanoseconds");
    case Kind::Duration:
      PLUSS_FAIL("SlateValue::host().to_dbl: duration values do not convert to double; use to_int() for nanoseconds");
    default:
      PLUSS_FAIL("SlateValue::host().to_dbl: unsupported scalar kind");
  }
}

std::int64_t SlateValue::HostAny::to_int() const {
  switch (kind_) {
    case Kind::Bool:
      return std::get<bool>(storage_) ? 1 : 0;
    case Kind::Int:
      return std::get<std::int64_t>(storage_);
    case Kind::UInt: {
      const auto value = std::get<std::uint64_t>(storage_);
      PLUSS_CHECK(value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
                  "SlateValue::host().to_int: unsigned value exceeds int64 range");
      return static_cast<std::int64_t>(value);
    }
    case Kind::Double: {
      const auto value = std::get<double>(storage_);
      PLUSS_CHECK(std::isfinite(value), "SlateValue::host().to_int: floating value is not finite");
      PLUSS_CHECK(value >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
                      value <= static_cast<double>(std::numeric_limits<std::int64_t>::max()),
                  "SlateValue::host().to_int: floating value exceeds int64 range");
      return static_cast<std::int64_t>(value);
    }
    case Kind::Time:
      return std::get<std::int64_t>(storage_);
    case Kind::Duration:
      return std::get<std::int64_t>(storage_);
    case Kind::String:
      PLUSS_FAIL("SlateValue::host().to_int: Note values are not integral");
    default:
      PLUSS_FAIL("SlateValue::host().to_int: unsupported scalar kind");
  }
}

std::string SlateValue::HostAny::to_str() const {
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
    case Kind::Duration:
      return display_.empty() ? std::to_string(std::get<std::int64_t>(storage_)) : display_;
    default:
      PLUSS_FAIL("SlateValue::host().to_str: unsupported scalar kind");
  }
}

SlateValue::SlateValue(Slate slate) : slate_(std::move(slate)) {}

bool SlateValue::defined() const noexcept { return slate_.defined(); }
SlateKind SlateValue::kind() const { return slate_.kind(); }
const Slate& SlateValue::slate() const noexcept { return slate_; }
SlateValue::Payload SlateValue::payload() const {
  switch (kind()) {
    case SlateKind::Series:
      return as_series();
    case SlateKind::Note:
      return as_note();
    case SlateKind::Timeline:
      return as_timeline();
    case SlateKind::Duration:
      return as_duration();
    default:
      PLUSS_FAIL("SlateValue::payload: unsupported SlateKind");
  }
}

pluss::Series SlateValue::as_series() const { return slate_.as_series(); }
pluss::Note SlateValue::as_note() const { return slate_.as_note(); }
pluss::Timeline SlateValue::as_timeline() const { return slate_.as_timeline(); }
pluss::Duration SlateValue::as_duration() const { return slate_.as_duration(); }

SlateValue::HostAny SlateValue::host() const {
  PLUSS_CHECK(defined(), "SlateValue::host: SlateValue is undefined");
  PLUSS_CHECK(slate_.size() == 1,
              "SlateValue::host: Slate payload must contain exactly one value");
  switch (kind()) {
    case SlateKind::Series:
      return scalar_series_host(as_series());
    case SlateKind::Note: {
      const auto& note = as_note();
      PLUSS_CHECK(note.numel() == 1,
                  "SlateValue::host: Note payload must contain exactly one value");
      return HostAny::string(std::string(note.scalar()));
    }
    case SlateKind::Timeline: {
      const auto nanos = scalar_timeline_nanos(as_timeline(), "SlateValue::host");
      return HostAny::time(nanos, scalar_time_iso(nanos));
    }
    case SlateKind::Duration: {
      const auto duration = as_duration();
      const auto nanos = scalar_duration_nanos(duration, "SlateValue::host");
      return HostAny::duration(nanos, std::string(duration.to_note_iso().scalar()));
    }
    default:
      PLUSS_FAIL("SlateValue::host: unsupported SlateKind");
  }
}

double SlateValue::to_dbl() const { return host().to_dbl(); }
std::int64_t SlateValue::to_int() const { return host().to_int(); }
std::string SlateValue::to_str() const { return host().to_str(); }

SlateValue::operator pluss::Series() const { return as_series(); }
SlateValue::operator pluss::Note() const { return as_note(); }
SlateValue::operator pluss::Timeline() const { return as_timeline(); }
SlateValue::operator pluss::Duration() const { return as_duration(); }

std::ostream& operator<<(std::ostream& os, const SlateValue::HostAny& value) {
  os << value.to_str();
  return os;
}

std::ostream& operator<<(std::ostream& os, const SlateValue& value) {
  output_render(os, value);
  return os;
}

void output_render(std::ostream& os, const SlateValue& value) {
  std::visit([&](const auto& v) { os << v; }, value.payload());
}
