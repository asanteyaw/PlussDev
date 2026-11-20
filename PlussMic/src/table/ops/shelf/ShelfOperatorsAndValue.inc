std::string_view shelf_kind_name(ShelfKind k) noexcept {
  switch (k) {
    case ShelfKind::Series:
      return "Series";
    case ShelfKind::Note:
      return "Note";
    case ShelfKind::Timeline:
      return "Timeline";
    case ShelfKind::Duration:
      return "Duration";
    default:
      return "Unknown";
  }
}

namespace {
void ensure_numeric_shelf_for_math(const Shelf& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Shelf is undefined");
  PLUSS_CHECK(t.kind() == ShelfKind::Series,
              std::string(where) + ": operation requires a Series-backed Shelf");
  PLUSS_CHECK(is_numeric_series_type(t.as_series()),
              std::string(where) + ": operation requires a numeric Series payload");
}

void ensure_defined_shelf_for_compare(const Shelf& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Shelf is undefined");
}

void ensure_bool_series_shelf_for_logic(const Shelf& t, std::string_view where) {
  PLUSS_CHECK(t.defined(), std::string(where) + ": Shelf is undefined");
  PLUSS_CHECK(t.kind() == ShelfKind::Series,
              std::string(where) + ": operation requires a Series-backed Shelf");
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

const char* math_op_name(ShelfBinaryMathOp op) {
  switch (op) {
    case ShelfBinaryMathOp::Add:
      return "+";
    case ShelfBinaryMathOp::Sub:
      return "-";
    case ShelfBinaryMathOp::Mul:
      return "*";
    case ShelfBinaryMathOp::Div:
      return "/";
    case ShelfBinaryMathOp::TrueDiv:
      return "/";
    case ShelfBinaryMathOp::FloorDiv:
      return "//";
    case ShelfBinaryMathOp::Mod:
      return "%";
    case ShelfBinaryMathOp::Pow:
      return "**";
    default:
      return "?";
  }
}

const char* cmp_op_name(ShelfBinaryCompareOp op) {
  switch (op) {
    case ShelfBinaryCompareOp::Eq:
      return "==";
    case ShelfBinaryCompareOp::Ne:
      return "!=";
    case ShelfBinaryCompareOp::Lt:
      return "<";
    case ShelfBinaryCompareOp::Le:
      return "<=";
    case ShelfBinaryCompareOp::Gt:
      return ">";
    case ShelfBinaryCompareOp::Ge:
      return ">=";
    default:
      return "?";
  }
}

ShelfBinaryCompareOp reverse_cmp_op(ShelfBinaryCompareOp op) {
  switch (op) {
    case ShelfBinaryCompareOp::Eq:
      return ShelfBinaryCompareOp::Eq;
    case ShelfBinaryCompareOp::Ne:
      return ShelfBinaryCompareOp::Ne;
    case ShelfBinaryCompareOp::Lt:
      return ShelfBinaryCompareOp::Gt;
    case ShelfBinaryCompareOp::Le:
      return ShelfBinaryCompareOp::Ge;
    case ShelfBinaryCompareOp::Gt:
      return ShelfBinaryCompareOp::Lt;
    case ShelfBinaryCompareOp::Ge:
      return ShelfBinaryCompareOp::Le;
    default:
      return op;
  }
}

template <class Fn>
Shelf invoke_underlying(Fn&& fn, std::string_view where, std::string_view what) {
  try {
    return fn();
  } catch (const std::exception& e) {
    PLUSS_FAIL(std::string(where) + ": underlying " + std::string(what) + " operation failed: " + e.what());
  } catch (...) {
    PLUSS_FAIL(std::string(where) + ": underlying " + std::string(what) + " operation failed");
  }
}

Shelf carry_case_labels_(const Shelf& value, const Shelf& reference) {
  return detail::has_display_labels(reference)
             ? detail::with_display_labels(value, detail::display_labels(reference))
             : value;
}

Shelf binary_math_scalar(const Shelf& lhs, double rhs, ShelfBinaryMathOp op, bool reverse, std::string_view where) {
  PLUSS_CHECK(lhs.defined(), std::string(where) + ": Shelf is undefined");
  if (lhs.kind() == ShelfKind::Series) {
    ensure_numeric_shelf_for_math(lhs, where);
    return invoke_underlying([&] { return detail::math(lhs, rhs, op, reverse); }, where, "Series math");
  }

  if (lhs.kind() == ShelfKind::Note && op == ShelfBinaryMathOp::Mul) {
    const auto times = nonnegative_integral_scalar(rhs);
    PLUSS_CHECK(times.has_value(),
                std::string(where) + ": Note repetition requires a non-negative integral scalar");
    return invoke_underlying(
        [&] { return Shelf(lhs.as_note() * *times, detail::validity_mask(lhs)); },
        where,
        "Note repetition");
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload for operator '" + math_op_name(op)
             + "' (" + std::string(shelf_kind_name(lhs.kind())) + " with scalar)");
}

Shelf binary_math_pair(const Shelf& lhs, const Shelf& rhs, ShelfBinaryMathOp op, std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional operation requires identical lengths");
  }
  const auto& left = lhs;
  const auto& right = rhs;
  const auto lk = left.kind();
  const auto rk = right.kind();
  const bool temporal_pair = lk == ShelfKind::Timeline && rk == ShelfKind::Timeline;

  if (left.kind() == ShelfKind::Series && right.kind() == ShelfKind::Series) {
    ensure_numeric_shelf_for_math(left, where);
    ensure_numeric_shelf_for_math(right, where);
    return carry_case_labels_(
        invoke_underlying([&] { return detail::math(left, right, op); }, where, "Series math"),
        left);
  }

  if (left.kind() == ShelfKind::Note && right.kind() == ShelfKind::Note && op == ShelfBinaryMathOp::Add) {
    return carry_case_labels_(
        invoke_underlying(
            [&] { return Shelf(left.as_note() + right.as_note(), combine_validity_masks(left, right)); },
            where,
            "Note concatenation"),
        left);
  }

  if (temporal_pair && op == ShelfBinaryMathOp::Sub) {
    return carry_case_labels_(
        invoke_underlying(
            [&] {
              const auto left_tl = left.as_timeline();
              const auto right_tl = right.as_timeline();
              TimelineSpanOptions options;
              options.unit = TimeUnit::Nanos;
              options.signed_result = true;
              return Shelf(right_tl.span(left_tl, to_codeh_span_options_(options, where)),
                            combine_validity_masks(left, right));
            },
            where,
            "temporal diff"),
        left);
  }

  PLUSS_FAIL(std::string(where)
             + ": unsupported payload combination for operator '" + math_op_name(op)
             + "' (" + std::string(shelf_kind_name(left.kind())) + " vs " +
             std::string(shelf_kind_name(right.kind())) + ")");
}

Shelf compare_scalar(const Shelf& lhs, double rhs, ShelfBinaryCompareOp op, std::string_view where) {
  ensure_numeric_shelf_for_math(lhs, where);
  return invoke_underlying([&] { return lhs.compare(rhs, op); }, where, "Series compare");
}

Shelf compare_pair(const Shelf& lhs, const Shelf& rhs, ShelfBinaryCompareOp op, std::string_view where) {
  ensure_defined_shelf_for_compare(lhs, where);
  ensure_defined_shelf_for_compare(rhs, where);
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional comparison requires identical lengths");
  }
  const auto& left = lhs;
  const auto& right = rhs;

  const auto lk = left.kind();
  const auto rk = right.kind();
  const bool temporal_compare = lk == ShelfKind::Timeline && rk == ShelfKind::Timeline;

  if (lk == ShelfKind::Series && rk == ShelfKind::Series) {
    return carry_case_labels_(
        invoke_underlying([&] { return left.compare(right, op); }, where, "Series compare"),
        left);
  }

  if (lk == ShelfKind::Note && rk == ShelfKind::Note) {
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
             + "' (" + std::string(shelf_kind_name(lk)) + " vs " +
             std::string(shelf_kind_name(rk)) + ")");
}

Shelf logical_pair(const Shelf& lhs,
                    const Shelf& rhs,
                    std::string_view where,
                    std::string_view op_name) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), std::string(where) + ": operand is undefined");
  if (lhs.size() != rhs.size()) {
    throw ::pluss::Error(std::string(where) + ": positional logical operation requires identical lengths");
  }
  ensure_bool_series_shelf_for_logic(lhs, where);
  ensure_bool_series_shelf_for_logic(rhs, where);
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
            Shelf(values, combine_validity_masks(lhs, rhs)),
            lhs);
      },
      where,
      std::string("Series logical_") + std::string(op_name));
}
} // namespace

Shelf Shelf::add(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Add, false, "Shelf::add(double)"); }
Shelf Shelf::add(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::Add, "Shelf::add(Shelf)"); }
Shelf Shelf::radd(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Add, true, "Shelf::radd(double)"); }
Shelf Shelf::radd(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::Add, "Shelf::radd(Shelf)"); }

Shelf Shelf::sub(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Sub, false, "Shelf::sub(double)"); }
Shelf Shelf::sub(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::Sub, "Shelf::sub(Shelf)"); }
Shelf Shelf::rsub(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Sub, true, "Shelf::rsub(double)"); }
Shelf Shelf::rsub(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::Sub, "Shelf::rsub(Shelf)"); }

Shelf Shelf::mul(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Mul, false, "Shelf::mul(double)"); }
Shelf Shelf::mul(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::Mul, "Shelf::mul(Shelf)"); }
Shelf Shelf::rmul(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Mul, true, "Shelf::rmul(double)"); }
Shelf Shelf::rmul(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::Mul, "Shelf::rmul(Shelf)"); }

Shelf Shelf::div(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Div, false, "Shelf::div(double)"); }
Shelf Shelf::div(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::Div, "Shelf::div(Shelf)"); }
Shelf Shelf::rdiv(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Div, true, "Shelf::rdiv(double)"); }
Shelf Shelf::rdiv(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::Div, "Shelf::rdiv(Shelf)"); }

Shelf Shelf::truediv(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::TrueDiv, false, "Shelf::truediv(double)"); }
Shelf Shelf::truediv(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::TrueDiv, "Shelf::truediv(Shelf)"); }
Shelf Shelf::rtruediv(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::TrueDiv, true, "Shelf::rtruediv(double)"); }
Shelf Shelf::rtruediv(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::TrueDiv, "Shelf::rtruediv(Shelf)"); }

Shelf Shelf::floordiv(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::FloorDiv, false, "Shelf::floordiv(double)"); }
Shelf Shelf::floordiv(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::FloorDiv, "Shelf::floordiv(Shelf)"); }
Shelf Shelf::rfloordiv(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::FloorDiv, true, "Shelf::rfloordiv(double)"); }
Shelf Shelf::rfloordiv(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::FloorDiv, "Shelf::rfloordiv(Shelf)"); }

Shelf Shelf::mod(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Mod, false, "Shelf::mod(double)"); }
Shelf Shelf::mod(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::Mod, "Shelf::mod(Shelf)"); }
Shelf Shelf::rmod(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Mod, true, "Shelf::rmod(double)"); }
Shelf Shelf::rmod(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::Mod, "Shelf::rmod(Shelf)"); }

Shelf Shelf::pow(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Pow, false, "Shelf::pow(double)"); }
Shelf Shelf::pow(const Shelf& other) const { return binary_math_pair(*this, other, ShelfBinaryMathOp::Pow, "Shelf::pow(Shelf)"); }
Shelf Shelf::rpow(double other) const { return binary_math_scalar(*this, other, ShelfBinaryMathOp::Pow, true, "Shelf::rpow(double)"); }
Shelf Shelf::rpow(const Shelf& other) const { return binary_math_pair(other, *this, ShelfBinaryMathOp::Pow, "Shelf::rpow(Shelf)"); }

Shelf Shelf::eq(double other) const { return compare_scalar(*this, other, ShelfBinaryCompareOp::Eq, "Shelf::eq(double)"); }
Shelf Shelf::eq(std::string_view other) const { return compare(other, ShelfBinaryCompareOp::Eq); }
Shelf Shelf::eq(const Shelf& other) const { return compare_pair(*this, other, ShelfBinaryCompareOp::Eq, "Shelf::eq(Shelf)"); }
Shelf Shelf::ne(double other) const { return compare_scalar(*this, other, ShelfBinaryCompareOp::Ne, "Shelf::ne(double)"); }
Shelf Shelf::ne(std::string_view other) const { return compare(other, ShelfBinaryCompareOp::Ne); }
Shelf Shelf::ne(const Shelf& other) const { return compare_pair(*this, other, ShelfBinaryCompareOp::Ne, "Shelf::ne(Shelf)"); }
Shelf Shelf::lt(double other) const { return compare_scalar(*this, other, ShelfBinaryCompareOp::Lt, "Shelf::lt(double)"); }
Shelf Shelf::lt(std::string_view other) const { return compare(other, ShelfBinaryCompareOp::Lt); }
Shelf Shelf::lt(const Shelf& other) const { return compare_pair(*this, other, ShelfBinaryCompareOp::Lt, "Shelf::lt(Shelf)"); }
Shelf Shelf::le(double other) const { return compare_scalar(*this, other, ShelfBinaryCompareOp::Le, "Shelf::le(double)"); }
Shelf Shelf::le(std::string_view other) const { return compare(other, ShelfBinaryCompareOp::Le); }
Shelf Shelf::le(const Shelf& other) const { return compare_pair(*this, other, ShelfBinaryCompareOp::Le, "Shelf::le(Shelf)"); }
Shelf Shelf::gt(double other) const { return compare_scalar(*this, other, ShelfBinaryCompareOp::Gt, "Shelf::gt(double)"); }
Shelf Shelf::gt(std::string_view other) const { return compare(other, ShelfBinaryCompareOp::Gt); }
Shelf Shelf::gt(const Shelf& other) const { return compare_pair(*this, other, ShelfBinaryCompareOp::Gt, "Shelf::gt(Shelf)"); }
Shelf Shelf::ge(double other) const { return compare_scalar(*this, other, ShelfBinaryCompareOp::Ge, "Shelf::ge(double)"); }
Shelf Shelf::ge(std::string_view other) const { return compare(other, ShelfBinaryCompareOp::Ge); }
Shelf Shelf::ge(const Shelf& other) const { return compare_pair(*this, other, ShelfBinaryCompareOp::Ge, "Shelf::ge(Shelf)"); }

Shelf Shelf::land(const Shelf& other) const { return logical_pair(*this, other, "Shelf::land(Shelf)", "and"); }
Shelf Shelf::rand(const Shelf& other) const { return logical_pair(other, *this, "Shelf::rand(Shelf)", "and"); }
Shelf Shelf::lor(const Shelf& other) const { return logical_pair(*this, other, "Shelf::lor(Shelf)", "or"); }
Shelf Shelf::ror(const Shelf& other) const { return logical_pair(other, *this, "Shelf::ror(Shelf)", "or"); }
Shelf Shelf::lxor(const Shelf& other) const { return logical_pair(*this, other, "Shelf::lxor(Shelf)", "xor"); }
Shelf Shelf::rxor(const Shelf& other) const { return logical_pair(other, *this, "Shelf::rxor(Shelf)", "xor"); }

Shelf operator+(const Shelf& lhs, double rhs) {
  return lhs.add(rhs);
}

Shelf operator+(double lhs, const Shelf& rhs) {
  return rhs.radd(lhs);
}

Shelf operator+(const Shelf& lhs, const Shelf& rhs) {
  return lhs.add(rhs);
}

Shelf operator-(const Shelf& lhs, double rhs) {
  return lhs.sub(rhs);
}

Shelf operator-(double lhs, const Shelf& rhs) {
  return rhs.rsub(lhs);
}

Shelf operator-(const Shelf& lhs, const Shelf& rhs) {
  return lhs.sub(rhs);
}

Shelf operator*(const Shelf& lhs, double rhs) {
  return lhs.mul(rhs);
}

Shelf operator*(double lhs, const Shelf& rhs) {
  return rhs.rmul(lhs);
}

Shelf operator*(const Shelf& lhs, const Shelf& rhs) {
  return lhs.mul(rhs);
}

Shelf operator/(const Shelf& lhs, double rhs) {
  return lhs.truediv(rhs);
}

Shelf operator/(double lhs, const Shelf& rhs) {
  return rhs.rtruediv(lhs);
}

Shelf operator/(const Shelf& lhs, const Shelf& rhs) {
  return lhs.truediv(rhs);
}

Shelf operator%(const Shelf& lhs, double rhs) {
  return lhs.mod(rhs);
}

Shelf operator%(double lhs, const Shelf& rhs) {
  return rhs.rmod(lhs);
}

Shelf operator%(const Shelf& lhs, const Shelf& rhs) {
  return lhs.mod(rhs);
}

Shelf operator&(const Shelf& lhs, const Shelf& rhs) {
  return lhs.land(rhs);
}

Shelf operator|(const Shelf& lhs, const Shelf& rhs) {
  return lhs.lor(rhs);
}

Shelf operator^(const Shelf& lhs, const Shelf& rhs) {
  return lhs.lxor(rhs);
}

Shelf operator==(const Shelf& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ShelfBinaryCompareOp::Eq, "Shelf::operator==(Shelf,double)");
}

Shelf operator==(double lhs, const Shelf& rhs) {
  return compare_scalar(rhs, lhs, ShelfBinaryCompareOp::Eq, "Shelf::operator==(double,Shelf)");
}

Shelf operator==(const Shelf& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Eq);
}

Shelf operator==(std::string_view lhs, const Shelf& rhs) {
  return rhs.compare(lhs, ShelfBinaryCompareOp::Eq);
}

Shelf operator==(const Shelf& lhs, const char* rhs) {
  return lhs == std::string_view(rhs != nullptr ? rhs : "");
}

Shelf operator==(const char* lhs, const Shelf& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") == rhs;
}

Shelf operator==(const Shelf& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Eq);
}

Shelf operator==(const Shelf& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Eq);
}

Shelf operator==(const Shelf& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Eq);
}


Shelf operator==(const Shelf& lhs, const Shelf& rhs) {
  return compare_pair(lhs, rhs, ShelfBinaryCompareOp::Eq, "Shelf::operator==(Shelf,Shelf)");
}

Shelf operator!=(const Shelf& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ShelfBinaryCompareOp::Ne, "Shelf::operator!=(Shelf,double)");
}

Shelf operator!=(double lhs, const Shelf& rhs) {
  return compare_scalar(rhs, lhs, ShelfBinaryCompareOp::Ne, "Shelf::operator!=(double,Shelf)");
}

Shelf operator!=(const Shelf& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ne);
}

Shelf operator!=(std::string_view lhs, const Shelf& rhs) {
  return rhs.compare(lhs, ShelfBinaryCompareOp::Ne);
}

Shelf operator!=(const Shelf& lhs, const char* rhs) {
  return lhs != std::string_view(rhs != nullptr ? rhs : "");
}

Shelf operator!=(const char* lhs, const Shelf& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") != rhs;
}

Shelf operator!=(const Shelf& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ne);
}

Shelf operator!=(const Shelf& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ne);
}

Shelf operator!=(const Shelf& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ne);
}


Shelf operator!=(const Shelf& lhs, const Shelf& rhs) {
  return compare_pair(lhs, rhs, ShelfBinaryCompareOp::Ne, "Shelf::operator!=(Shelf,Shelf)");
}

Shelf operator<(const Shelf& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ShelfBinaryCompareOp::Lt, "Shelf::operator<(Shelf,double)");
}

Shelf operator<(double lhs, const Shelf& rhs) {
  return compare_scalar(rhs, lhs, ShelfBinaryCompareOp::Gt, "Shelf::operator<(double,Shelf)");
}

Shelf operator<(const Shelf& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Lt);
}

Shelf operator<(std::string_view lhs, const Shelf& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ShelfBinaryCompareOp::Lt));
}

Shelf operator<(const Shelf& lhs, const char* rhs) {
  return lhs < std::string_view(rhs != nullptr ? rhs : "");
}

Shelf operator<(const char* lhs, const Shelf& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") < rhs;
}

Shelf operator<(const Shelf& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Lt);
}

Shelf operator<(const Shelf& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Lt);
}

Shelf operator<(const Shelf& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Lt);
}


Shelf operator<(const Shelf& lhs, const Shelf& rhs) {
  return compare_pair(lhs, rhs, ShelfBinaryCompareOp::Lt, "Shelf::operator<(Shelf,Shelf)");
}

Shelf operator<=(const Shelf& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ShelfBinaryCompareOp::Le, "Shelf::operator<=(Shelf,double)");
}

Shelf operator<=(double lhs, const Shelf& rhs) {
  return compare_scalar(rhs, lhs, ShelfBinaryCompareOp::Ge, "Shelf::operator<=(double,Shelf)");
}

Shelf operator<=(const Shelf& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Le);
}

Shelf operator<=(std::string_view lhs, const Shelf& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ShelfBinaryCompareOp::Le));
}

Shelf operator<=(const Shelf& lhs, const char* rhs) {
  return lhs <= std::string_view(rhs != nullptr ? rhs : "");
}

Shelf operator<=(const char* lhs, const Shelf& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") <= rhs;
}

Shelf operator<=(const Shelf& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Le);
}

Shelf operator<=(const Shelf& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Le);
}

Shelf operator<=(const Shelf& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Le);
}


Shelf operator<=(const Shelf& lhs, const Shelf& rhs) {
  return compare_pair(lhs, rhs, ShelfBinaryCompareOp::Le, "Shelf::operator<=(Shelf,Shelf)");
}

Shelf operator>(const Shelf& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ShelfBinaryCompareOp::Gt, "Shelf::operator>(Shelf,double)");
}

Shelf operator>(double lhs, const Shelf& rhs) {
  return compare_scalar(rhs, lhs, ShelfBinaryCompareOp::Lt, "Shelf::operator>(double,Shelf)");
}

Shelf operator>(const Shelf& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Gt);
}

Shelf operator>(std::string_view lhs, const Shelf& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ShelfBinaryCompareOp::Gt));
}

Shelf operator>(const Shelf& lhs, const char* rhs) {
  return lhs > std::string_view(rhs != nullptr ? rhs : "");
}

Shelf operator>(const char* lhs, const Shelf& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") > rhs;
}

Shelf operator>(const Shelf& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Gt);
}

Shelf operator>(const Shelf& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Gt);
}

Shelf operator>(const Shelf& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Gt);
}


Shelf operator>(const Shelf& lhs, const Shelf& rhs) {
  return compare_pair(lhs, rhs, ShelfBinaryCompareOp::Gt, "Shelf::operator>(Shelf,Shelf)");
}

Shelf operator>=(const Shelf& lhs, double rhs) {
  return compare_scalar(lhs, rhs, ShelfBinaryCompareOp::Ge, "Shelf::operator>=(Shelf,double)");
}

Shelf operator>=(double lhs, const Shelf& rhs) {
  return compare_scalar(rhs, lhs, ShelfBinaryCompareOp::Le, "Shelf::operator>=(double,Shelf)");
}

Shelf operator>=(const Shelf& lhs, std::string_view rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ge);
}

Shelf operator>=(std::string_view lhs, const Shelf& rhs) {
  return rhs.compare(lhs, reverse_cmp_op(ShelfBinaryCompareOp::Ge));
}

Shelf operator>=(const Shelf& lhs, const char* rhs) {
  return lhs >= std::string_view(rhs != nullptr ? rhs : "");
}

Shelf operator>=(const char* lhs, const Shelf& rhs) {
  return std::string_view(lhs != nullptr ? lhs : "") >= rhs;
}

Shelf operator>=(const Shelf& lhs, const pluss::Series& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ge);
}

Shelf operator>=(const Shelf& lhs, const pluss::Note& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ge);
}

Shelf operator>=(const Shelf& lhs, const pluss::Timeline& rhs) {
  return lhs.compare(rhs, ShelfBinaryCompareOp::Ge);
}


Shelf operator>=(const Shelf& lhs, const Shelf& rhs) {
  return compare_pair(lhs, rhs, ShelfBinaryCompareOp::Ge, "Shelf::operator>=(Shelf,Shelf)");
}

const pluss::Series& Shelf::as_series() const {
  PLUSS_CHECK(kind() == ShelfKind::Series, "Shelf: payload is not Series");
  return std::get<pluss::Series>(impl_ref_().payload_);
}

const pluss::Note& Shelf::as_note() const {
  PLUSS_CHECK(kind() == ShelfKind::Note, "Shelf: payload is not Note");
  return std::get<pluss::Note>(impl_ref_().payload_);
}

const pluss::Timeline& Shelf::as_timeline() const {
  PLUSS_CHECK(kind() == ShelfKind::Timeline, "Shelf: payload is not Timeline");
  return std::get<pluss::Timeline>(impl_ref_().payload_);
}

const pluss::Duration& Shelf::as_duration() const {
  PLUSS_CHECK(kind() == ShelfKind::Duration, "Shelf: payload is not Duration");
  return std::get<pluss::Duration>(impl_ref_().payload_);
}

namespace {

std::string scalar_time_iso(std::int64_t epoch_nanos) {
  return pluss::timeline::from_epoch(epoch_nanos).isoformat();
}

ShelfValue::HostAny scalar_series_host(const pluss::Series& series) {
  PLUSS_CHECK(series.numel() == 1,
              "ShelfValue::host: Series payload must contain exactly one value");
  const auto scalar = series.reshape(std::vector<std::int64_t>{}).item();
  return std::visit(
      [](const auto& value) -> ShelfValue::HostAny {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;
        if constexpr (std::is_same_v<T, bool>) {
          return ShelfValue::HostAny::boolean(value);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
          return ShelfValue::HostAny::integer(value);
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
          return ShelfValue::HostAny::unsigned_integer(value);
        } else if constexpr (std::is_same_v<T, double>) {
          return ShelfValue::HostAny::floating(value);
        } else {
          PLUSS_FAIL("ShelfValue::host: unsupported Series scalar storage");
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

ShelfValue::HostAny ShelfValue::HostAny::boolean(bool value) {
  HostAny out;
  out.kind_ = Kind::Bool;
  out.storage_ = value;
  return out;
}

ShelfValue::HostAny ShelfValue::HostAny::integer(std::int64_t value) {
  HostAny out;
  out.kind_ = Kind::Int;
  out.storage_ = value;
  return out;
}

ShelfValue::HostAny ShelfValue::HostAny::unsigned_integer(std::uint64_t value) {
  HostAny out;
  out.kind_ = Kind::UInt;
  out.storage_ = value;
  return out;
}

ShelfValue::HostAny ShelfValue::HostAny::floating(double value) {
  HostAny out;
  out.kind_ = Kind::Double;
  out.storage_ = value;
  return out;
}

ShelfValue::HostAny ShelfValue::HostAny::string(std::string value) {
  HostAny out;
  out.kind_ = Kind::String;
  out.storage_ = std::move(value);
  return out;
}

ShelfValue::HostAny ShelfValue::HostAny::time(std::int64_t epoch_nanos,
                                                std::string iso) {
  HostAny out;
  out.kind_ = Kind::Time;
  out.storage_ = epoch_nanos;
  out.display_ = std::move(iso);
  return out;
}

ShelfValue::HostAny ShelfValue::HostAny::duration(std::int64_t nanos,
                                                    std::string text) {
  HostAny out;
  out.kind_ = Kind::Duration;
  out.storage_ = nanos;
  out.display_ = std::move(text);
  return out;
}

ShelfValue::HostAny::Kind ShelfValue::HostAny::kind() const noexcept {
  return kind_;
}

const ShelfValue::HostAny::Storage& ShelfValue::HostAny::storage() const noexcept {
  return storage_;
}

double ShelfValue::HostAny::to_dbl() const {
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
      PLUSS_FAIL("ShelfValue::host().to_dbl: Note values are not numeric");
    case Kind::Time:
      PLUSS_FAIL("ShelfValue::host().to_dbl: time values do not convert to double; use to_int() for epoch nanoseconds");
    case Kind::Duration:
      PLUSS_FAIL("ShelfValue::host().to_dbl: duration values do not convert to double; use to_int() for nanoseconds");
    default:
      PLUSS_FAIL("ShelfValue::host().to_dbl: unsupported scalar kind");
  }
}

std::int64_t ShelfValue::HostAny::to_int() const {
  switch (kind_) {
    case Kind::Bool:
      return std::get<bool>(storage_) ? 1 : 0;
    case Kind::Int:
      return std::get<std::int64_t>(storage_);
    case Kind::UInt: {
      const auto value = std::get<std::uint64_t>(storage_);
      PLUSS_CHECK(value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
                  "ShelfValue::host().to_int: unsigned value exceeds int64 range");
      return static_cast<std::int64_t>(value);
    }
    case Kind::Double: {
      const auto value = std::get<double>(storage_);
      PLUSS_CHECK(std::isfinite(value), "ShelfValue::host().to_int: floating value is not finite");
      PLUSS_CHECK(value >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
                      value <= static_cast<double>(std::numeric_limits<std::int64_t>::max()),
                  "ShelfValue::host().to_int: floating value exceeds int64 range");
      return static_cast<std::int64_t>(value);
    }
    case Kind::Time:
      return std::get<std::int64_t>(storage_);
    case Kind::Duration:
      return std::get<std::int64_t>(storage_);
    case Kind::String:
      PLUSS_FAIL("ShelfValue::host().to_int: Note values are not integral");
    default:
      PLUSS_FAIL("ShelfValue::host().to_int: unsupported scalar kind");
  }
}

std::string ShelfValue::HostAny::to_str() const {
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
      PLUSS_FAIL("ShelfValue::host().to_str: unsupported scalar kind");
  }
}

ShelfValue::ShelfValue(Shelf shelf) : shelf_(std::move(shelf)) {}

bool ShelfValue::defined() const noexcept { return shelf_.defined(); }
ShelfKind ShelfValue::kind() const { return shelf_.kind(); }
const Shelf& ShelfValue::shelf() const noexcept { return shelf_; }
ShelfValue::Payload ShelfValue::payload() const {
  switch (kind()) {
    case ShelfKind::Series:
      return as_series();
    case ShelfKind::Note:
      return as_note();
    case ShelfKind::Timeline:
      return as_timeline();
    case ShelfKind::Duration:
      return as_duration();
    default:
      PLUSS_FAIL("ShelfValue::payload: unsupported ShelfKind");
  }
}

pluss::Series ShelfValue::as_series() const { return shelf_.as_series(); }
pluss::Note ShelfValue::as_note() const { return shelf_.as_note(); }
pluss::Timeline ShelfValue::as_timeline() const { return shelf_.as_timeline(); }
pluss::Duration ShelfValue::as_duration() const { return shelf_.as_duration(); }

ShelfValue::HostAny ShelfValue::host() const {
  PLUSS_CHECK(defined(), "ShelfValue::host: ShelfValue is undefined");
  PLUSS_CHECK(shelf_.size() == 1,
              "ShelfValue::host: Shelf payload must contain exactly one value");
  switch (kind()) {
    case ShelfKind::Series:
      return scalar_series_host(as_series());
    case ShelfKind::Note: {
      const auto& note = as_note();
      PLUSS_CHECK(note.numel() == 1,
                  "ShelfValue::host: Note payload must contain exactly one value");
      return HostAny::string(std::string(note.scalar()));
    }
    case ShelfKind::Timeline: {
      const auto nanos = scalar_timeline_nanos(as_timeline(), "ShelfValue::host");
      return HostAny::time(nanos, scalar_time_iso(nanos));
    }
    case ShelfKind::Duration: {
      const auto duration = as_duration();
      const auto nanos = scalar_duration_nanos(duration, "ShelfValue::host");
      return HostAny::duration(nanos, std::string(duration.to_note_iso().scalar()));
    }
    default:
      PLUSS_FAIL("ShelfValue::host: unsupported ShelfKind");
  }
}

double ShelfValue::to_dbl() const { return host().to_dbl(); }
std::int64_t ShelfValue::to_int() const { return host().to_int(); }
std::string ShelfValue::to_str() const { return host().to_str(); }

ShelfValue::operator pluss::Series() const { return as_series(); }
ShelfValue::operator pluss::Note() const { return as_note(); }
ShelfValue::operator pluss::Timeline() const { return as_timeline(); }
ShelfValue::operator pluss::Duration() const { return as_duration(); }

std::ostream& operator<<(std::ostream& os, const ShelfValue::HostAny& value) {
  os << value.to_str();
  return os;
}

std::ostream& operator<<(std::ostream& os, const ShelfValue& value) {
  output_render(os, value);
  return os;
}

void output_render(std::ostream& os, const ShelfValue& value) {
  std::visit([&](const auto& v) { os << v; }, value.payload());
}
