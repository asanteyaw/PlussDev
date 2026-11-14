// Elementwise arithmetic / comparison between streams of different lengths,
// decided by a RaggedPolicy (OpClass::Elementwise defaults: longest domain, head
// aligned, an absent operand makes the result null there). Nothing is dropped or
// padded unless the policy says so; equal lengths take the plain operator path.

namespace {

enum class BinaryOp { Add, Sub, Mul, Div, Mod, Eq, Ne, Lt, Le, Gt, Ge };

BinaryOp parse_binary_op(std::string_view op) {
  if (op == "+" || op == "add") return BinaryOp::Add;
  if (op == "-" || op == "sub") return BinaryOp::Sub;
  if (op == "*" || op == "mul") return BinaryOp::Mul;
  if (op == "/" || op == "div") return BinaryOp::Div;
  if (op == "%" || op == "mod") return BinaryOp::Mod;
  if (op == "==" || op == "eq") return BinaryOp::Eq;
  if (op == "!=" || op == "ne") return BinaryOp::Ne;
  if (op == "<" || op == "lt") return BinaryOp::Lt;
  if (op == "<=" || op == "le") return BinaryOp::Le;
  if (op == ">" || op == "gt") return BinaryOp::Gt;
  if (op == ">=" || op == "ge") return BinaryOp::Ge;
  PLUSS_FAIL("ragged_binary: unknown operator '" + std::string(op) + "' (use + - * / % == != < <= > >=)");
}

Stream apply_binary_op(BinaryOp op, const Stream& a, const Stream& b) {
  switch (op) {
    case BinaryOp::Add: return a + b;
    case BinaryOp::Sub: return a - b;
    case BinaryOp::Mul: return a * b;
    case BinaryOp::Div: return a / b;
    case BinaryOp::Mod: return a % b;
    case BinaryOp::Eq: return a == b;
    case BinaryOp::Ne: return a != b;
    case BinaryOp::Lt: return a < b;
    case BinaryOp::Le: return a <= b;
    case BinaryOp::Gt: return a > b;
    case BinaryOp::Ge: return a >= b;
  }
  PLUSS_FAIL("ragged_binary: unsupported operator");
}

// `s` laid onto indices [0, width) with its first record at `offset`.
Stream lay_operand(const Stream& s, std::int64_t width, std::int64_t offset, const RaggedPolicy& p) {
  const std::int64_t n = s.size();
  if (offset == 0 && n == width) return s;
  if (p.absent == RaggedAbsent::Fill) {
    PLUSS_CHECK(s.kind() == StreamKind::Series,
                "ragged_binary: absent=Fill needs numeric streams; use absent=Null for other kinds");
    std::vector<double> x = s.as_series().to(pluss::codem::ScalarType::Float64).host<double>();
    std::vector<bool> ok(x.size(), true);
    if (detail::has_validity_mask(s)) ok = detail::validity_mask(s)->host<bool>();
    std::vector<double> vals(static_cast<std::size_t>(width), *p.fill);
    std::vector<double> valid(static_cast<std::size_t>(width), 1.0);
    bool any_null = false;
    for (std::int64_t j = 0; j < width; ++j) {
      const std::int64_t i = j - offset;
      if (i >= 0 && i < n) {
        vals[static_cast<std::size_t>(j)] = x[static_cast<std::size_t>(i)];
        const bool v = ok[static_cast<std::size_t>(i)];
        valid[static_cast<std::size_t>(j)] = v ? 1.0 : 0.0;
        any_null = any_null || !v;
      }
    }
    Stream out(pluss::make_series(vals, {width}));
    if (any_null) {
      out = detail::with_validity_mask(out, pluss::make_series(valid, {width}).to(pluss::codem::ScalarType::Bool));
    }
    return out;
  }
  std::vector<std::int64_t> src(static_cast<std::size_t>(width));
  for (std::int64_t j = 0; j < width; ++j) {
    const std::int64_t i = j - offset;
    if (i >= 0 && i < n) {
      src[static_cast<std::size_t>(j)] = i;
    } else if (p.absent == RaggedAbsent::ForwardFill) {
      src[static_cast<std::size_t>(j)] = i >= n ? n - 1 : -1;  // before the first record: unknown
    } else if (p.absent == RaggedAbsent::BackwardFill) {
      src[static_cast<std::size_t>(j)] = i < 0 ? 0 : -1;       // after the last record: unknown
    } else {
      src[static_cast<std::size_t>(j)] = -1;                   // absent: null
    }
  }
  return s.gather(src, 0, /*trim_trailing=*/false);
}

}  // namespace

Stream ragged_binary(const Stream& lhs, const Stream& rhs, std::string_view op, const RaggedPolicy& policy) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), "ragged_binary: undefined stream");
  policy.validate("ragged_binary");
  PLUSS_CHECK(policy.alignment != RaggedAlignment::Label,
              "ragged_binary: label alignment is not supported here; align the streams by label first (Stream::align)");
  PLUSS_CHECK(policy.domain != RaggedDomain::Anchor, "ragged_binary: there is no named stream to anchor on");
  PLUSS_CHECK(policy.absent != RaggedAbsent::Skip,
              "ragged_binary: an absent operand cannot be skipped; use absent=Null, Fill or a fill mode, or domain=Shortest");
  const auto kind = parse_binary_op(op);
  const std::int64_t n = lhs.size(), m = rhs.size();
  const std::int64_t longest = std::max(n, m), shortest = std::min(n, m);

  if (n != m) {
    PLUSS_CHECK(policy.domain != RaggedDomain::Exact,
                pluss::tils::OutputEngine::format("ragged_binary: streams have different lengths ({} vs {}) and the policy requires equal lengths", n, m));
    PLUSS_CHECK(policy.absent != RaggedAbsent::Error,
                pluss::tils::OutputEngine::format("ragged_binary: streams have different lengths ({} vs {}) and absence is an error", n, m));
    PLUSS_CHECK(!(policy.domain == RaggedDomain::Shortest && policy.loss == RaggedLoss::Error),
                pluss::tils::OutputEngine::format("ragged_binary: domain=Shortest would drop {} record(s) and loss=Error refuses that",
                            longest - shortest));
  }

  const std::int64_t width = policy.domain == RaggedDomain::Shortest ? shortest : longest;
  // Position of each stream's first record in the shared index space.
  const bool tail = policy.alignment == RaggedAlignment::Tail;
  std::int64_t off_l = tail ? longest - n : 0, off_r = tail ? longest - m : 0;
  std::int64_t begin = 0;
  if (policy.domain == RaggedDomain::Shortest) begin = tail ? longest - shortest : 0;
  // Shift so the kept window starts at index 0.
  off_l -= begin;
  off_r -= begin;
  return apply_binary_op(kind, lay_operand(lhs, width, off_l, policy), lay_operand(rhs, width, off_r, policy));
}
