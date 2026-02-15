#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Scalar.h"
#include <type_traits>

#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/Series.h"
#include "CodeM/core/Errors.h"

#include <sstream>

namespace pluss::codem::_compat {

using pluss::codem::warn;


static std::string sig(std::string_view op, const Args& args) {
  std::ostringstream os;
  os << op << "(";
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i) os << ", ";
    const auto& v = args[i].v;
    if (std::holds_alternative<const Series*>(v)) os << "Series";
    else if (std::holds_alternative<Scalar>(v)) os << "Scalar";
    else if (std::holds_alternative<double>(v)) os << "double";
    else if (std::holds_alternative<std::int64_t>(v)) os << "int64";
    else if (std::holds_alternative<bool>(v)) os << "bool";
    else if (std::holds_alternative<std::string>(v)) os << "string";
    else if (std::holds_alternative<Arg::IntList>(v)) os << "int64[]";
    else if (std::holds_alternative<std::monostate>(v)) os << "None";
    else os << "?";
  }
  os << ")";
  return os.str();
}

static Series not_impl(std::string_view op, const Args& args, const Series&) {
  // Fail fast: silent identity fallbacks hide bugs and break correctness.
  PLUSS_CHECK(false, std::string("Unrouted op: ") + sig(op, args));
  return Series{};
}

static Series& not_impl_inplace(std::string_view op, const Args& args, Series&) {
  // Fail fast: silent no-op fallbacks hide bugs and break correctness.
  PLUSS_CHECK(false, std::string("Unrouted in-place op: ") + sig(op, args));
  // Unreachable; keep return for compiler.
  static Series* dummy = nullptr;
  return *dummy;
}

static const Series& as_series(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<const Series*>(a.v), "Expected Series");
  return *std::get<const Series*>(a.v);
}


static double scalar_to_double(const Scalar& s) {
  return std::visit(
      [](const auto& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) return v;
        else if constexpr (std::is_same_v<T, bool>) return v ? 1.0 : 0.0;
        else if constexpr (std::is_same_v<T, std::int64_t>) return static_cast<double>(v);
        else if constexpr (std::is_same_v<T, std::uint64_t>) return static_cast<double>(v);
        else return 0.0;
      },
      s.storage());
}

static std::int64_t scalar_to_i64(const Scalar& s) {
  return std::visit(
      [](const auto& v) -> std::int64_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t>) return v;
        else if constexpr (std::is_same_v<T, std::uint64_t>) return static_cast<std::int64_t>(v);
        else if constexpr (std::is_same_v<T, bool>) return v ? 1 : 0;
        else if constexpr (std::is_same_v<T, double>) return static_cast<std::int64_t>(v);
        else return 0;
      },
      s.storage());
}

static double as_double(const Arg& a) {
  if (std::holds_alternative<double>(a.v)) return std::get<double>(a.v);
  if (std::holds_alternative<std::int64_t>(a.v)) return static_cast<double>(std::get<std::int64_t>(a.v));
  if (std::holds_alternative<Scalar>(a.v)) return scalar_to_double(std::get<Scalar>(a.v));
  PLUSS_CHECK(false, "Expected numeric scalar");
  return 0.0;
}

static std::int64_t as_i64(const Arg& a) {
  if (std::holds_alternative<std::int64_t>(a.v)) return std::get<std::int64_t>(a.v);
  if (std::holds_alternative<double>(a.v)) return static_cast<std::int64_t>(std::get<double>(a.v));
  if (std::holds_alternative<Scalar>(a.v)) return scalar_to_i64(std::get<Scalar>(a.v));
  PLUSS_CHECK(false, "Expected int64 scalar");
  return 0;
}

static bool as_bool(const Arg& a) {
  if (std::holds_alternative<bool>(a.v)) return std::get<bool>(a.v);
  if (std::holds_alternative<std::int64_t>(a.v)) return std::get<std::int64_t>(a.v) != 0;
  if (std::holds_alternative<double>(a.v)) return std::get<double>(a.v) != 0.0;
  if (std::holds_alternative<Scalar>(a.v)) return scalar_to_double(std::get<Scalar>(a.v)) != 0.0;
  PLUSS_CHECK(false, "Expected bool");
  return false;
}


static std::span<const std::int64_t> as_intlist(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<Arg::IntList>(a.v), "Expected int64 list");
  return std::get<Arg::IntList>(a.v);
}

Series invoke_any(std::string_view op, const Series& self, const Args& args) {
  // 0-arg common (unary)
  if (args.empty()) {
    // elementwise math
    if (op == "relu") return ops_internal::relu(self);
    if (op == "sigmoid") return ops_internal::sigmoid(self);
    if (op == "softplus") return ops_internal::softplus(self, /*beta=*/1.0, /*threshold=*/20.0);
    if (op == "silu") return ops_internal::silu(self);
    if (op == "gelu") return ops_internal::gelu(self);

    if (op == "exp") return ops_internal::exp(self);
    if (op == "log") return ops_internal::log(self);
    if (op == "sqrt") return ops_internal::sqrt(self);
    if (op == "rsqrt") return ops_internal::rsqrt(self);

    if (op == "sin") return ops_internal::sin(self);
    if (op == "cos") return ops_internal::cos(self);
    if (op == "tan") return ops_internal::tan(self);
    if (op == "asin") return ops_internal::asin(self);
    if (op == "acos") return ops_internal::acos(self);
    if (op == "atan") return ops_internal::atan(self);
    if (op == "sinh") return ops_internal::sinh(self);
    if (op == "cosh") return ops_internal::cosh(self);
    if (op == "tanh") return ops_internal::tanh(self);
    if (op == "asinh") return ops_internal::asinh(self);
    if (op == "acosh") return ops_internal::acosh(self);
    if (op == "atanh") return ops_internal::atanh(self);

    if (op == "erf") return ops_internal::erf(self);
    if (op == "erfc") return ops_internal::erfc(self);
    if (op == "lgamma") return ops_internal::lgamma(self);

    if (op == "log2") return ops_internal::log2(self);
    if (op == "log10") return ops_internal::log10(self);
    if (op == "exp2") return ops_internal::exp2(self);
    if (op == "cbrt") return ops_internal::cbrt(self);
    if (op == "square") return ops_internal::square(self);
    if (op == "reciprocal") return ops_internal::reciprocal(self);

    if (op == "trunc") return ops_internal::trunc(self);
    if (op == "floor") return ops_internal::floor(self);
    if (op == "ceil") return ops_internal::ceil(self);
    if (op == "round") return ops_internal::round(self);

    // predicates
    if (op == "isnan") return ops_internal::isnan(self);
    if (op == "isinf") return ops_internal::isinf(self);
    if (op == "isfinite") return ops_internal::isfinite(self);
    if (op == "logical_not") return ops_internal::logical_not(self);

    if (op == "abs") return ops_internal::abs(self);

    if (op == "contiguous") return self.contiguous();
    if (op == "clone") return self.clone();
    if (op == "squeeze") {
      if (args.empty()) return ops_internal::squeeze(self);
      if (args.size() == 1) return ops_internal::squeeze(self, as_i64(args[0]));
      PLUSS_CHECK(false, "squeeze: expected 0 or 1 args");
    }

    if (op == "unique") {
      // Torch has more variants; we support unique(sorted=true/false)
      bool sorted = true;
      if (!args.empty()) sorted = as_bool(args[0]);
      return ops_internal::unique(self, sorted);
    }

    // bitwise unary
    if (op == "bitwise_not") {
      PLUSS_CHECK(args.empty(), "bitwise_not: expected 0 args");
      return ops_internal::bitwise_not(self);
    }
    if (op == "nonzero") return ops_internal::nonzero(self);

    if (op == "bitwise_and") {
      PLUSS_CHECK(args.size() == 1, "bitwise_and: expected 1 arg");
      return ops_internal::bitwise_and(self, as_series(args[0]));
    }

    if (op == "bitwise_or") {
      PLUSS_CHECK(args.size() == 1, "bitwise_or: expected 1 arg");
      return ops_internal::bitwise_or(self, as_series(args[0]));
    }

    if (op == "bitwise_xor") {
      PLUSS_CHECK(args.size() == 1, "bitwise_xor: expected 1 arg");
      return ops_internal::bitwise_xor(self, as_series(args[0]));
    }

    return not_impl(op, args, self);
  }

  // 1-arg
  if (args.size() == 1) {
    const auto& a0 = args[0].v;

    // scalar
    if (std::holds_alternative<double>(a0) || std::holds_alternative<std::int64_t>(a0)) {
      const double s = as_double(args[0]);
      if (op == "mul") return ops_internal::mul(self, s);
      if (op == "add") return ops_internal::add(self, s);
      if (op == "sub") return ops_internal::sub(self, s);
      if (op == "div") return ops_internal::div(self, s);
      if (op == "pow") return ops_internal::pow(self, s);
      if (op == "clamp_min") return ops_internal::clamp_min(self, s);
      if (op == "clamp_max") return ops_internal::clamp_max(self, s);
      if (op == "bincount") return ops_internal::bincount(self, (std::int64_t)s);
      if (op == "argsort") return ops_internal::argsort(self, (std::int64_t)s, /*descending=*/false, /*stable=*/false);
      if (op == "softmax") return ops_internal::softmax(self, (std::int64_t)s);
      if (op == "log_softmax") return ops_internal::log_softmax(self, (std::int64_t)s);
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amax") return ops_internal::amax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amin") return ops_internal::amin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "all") return ops_internal::all(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "any") return ops_internal::any(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)s, /*keepdim=*/false);
      return not_impl(op, args, self);
    }

    // bool
    if (std::holds_alternative<bool>(a0)) {
      const bool b = as_bool(args[0]);
      if (op == "unique") return self.unique(b);
      return not_impl(op, args, self);
    }

    // Series
    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "add") return ops_internal::add(self, other);
      if (op == "sub") return ops_internal::sub(self, other);
      if (op == "mul") return ops_internal::mul(self, other);
      if (op == "div") return ops_internal::div(self, other);

      if (op == "eq") return ops_internal::eq(self, other);
      if (op == "ne") return ops_internal::ne(self, other);
      if (op == "lt") return ops_internal::lt(self, other);
      if (op == "le") return ops_internal::le(self, other);
      if (op == "gt") return ops_internal::gt(self, other);
      if (op == "ge") return ops_internal::ge(self, other);

      if (op == "logical_and") return ops_internal::logical_and(self, other);
      if (op == "logical_or") return ops_internal::logical_or(self, other);
      if (op == "logical_xor") return ops_internal::logical_xor(self, other);

      if (op == "maximum") return ops_internal::maximum(self, other);
      if (op == "minimum") return ops_internal::minimum(self, other);
      if (op == "fmod") return ops_internal::fmod(self, other);
      if (op == "remainder") return ops_internal::remainder(self, other);
      if (op == "atan2") return ops_internal::atan2(self, other);

      if (op == "matmul") return ops_internal::matmul(self, other);

      if (op == "masked_select") return ops_internal::masked_select(self, other);
      if (op == "gather") return not_impl(op, args, self); // gather requires (dim, index)
      // note: masked_scatter needs 2 args (mask, src)
      return not_impl(op, args, self);
    }

    // intlist
    if (std::holds_alternative<Arg::IntList>(a0)) {
      auto sizes = as_intlist(args[0]);
      if (op == "reshape") return ops_internal::reshape(self, sizes);
      if (op == "view") return ops_internal::view(self, sizes);
      if (op == "expand") return ops_internal::expand(self, sizes);
      if (op == "permute") return ops_internal::permute(self, sizes);
      return not_impl(op, args, self);
    }

    return not_impl(op, args, self);
  }

  // 2-arg
  if (args.size() == 2) {
    const auto& a0 = args[0].v;
    const auto& a1 = args[1].v;

    // scalar + scalar
    if ((std::holds_alternative<double>(a0) || std::holds_alternative<std::int64_t>(a0)) &&
        (std::holds_alternative<double>(a1) || std::holds_alternative<std::int64_t>(a1))) {
      const double x = as_double(args[0]);
      const double y = as_double(args[1]);
      if (op == "clamp") return self.clamp(x, y);
      if (op == "softplus") return ops_internal::softplus(self, x, y);
      if (op == "leaky_relu") return not_impl(op, args, self); // leaky_relu takes 1 scalar (negative_slope) in this dispatcher
      if (op == "squeeze") return ops_internal::squeeze(self, (std::int64_t)x);
      if (op == "unsqueeze") return ops_internal::unsqueeze(self, (std::int64_t)x);
      if (op == "transpose") return ops_internal::transpose(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "amax") return ops_internal::amax(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "amin") return ops_internal::amin(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "all") return ops_internal::all(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "any") return ops_internal::any(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argsort") return ops_internal::argsort(self, (std::int64_t)x, as_bool(args[1]), /*stable=*/false);
      return not_impl(op, args, self);
    }


// scalar + Series (dim, index/src)
if ((std::holds_alternative<double>(a0) || std::holds_alternative<std::int64_t>(a0)) &&
    std::holds_alternative<const Series*>(a1)) {
  const auto dim = as_i64(args[0]);
  const Series& other = as_series(args[1]);
  if (op == "gather") return self.gather(dim, other);
  if (op == "scatter") return not_impl(op, args, self); // scatter requires (dim, index, src)
  if (op == "scatter_add") return not_impl(op, args, self); // scatter_add requires (dim, index, src)
  return not_impl(op, args, self);
}
// Series + Series
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& m = as_series(args[0]);
      const Series& s = as_series(args[1]);
      if (op == "masked_scatter") return self.masked_scatter(m, s);
      if (op == "where") return ops_internal::where(self, m, s);
      return not_impl(op, args, self);
    }

    return not_impl(op, args, self);
  }

  // 3-arg
  if (args.size() == 3) {
    // histc(bins, min, max)
    if ((std::holds_alternative<std::int64_t>(args[0].v) || std::holds_alternative<double>(args[0].v)) &&
        (std::holds_alternative<std::int64_t>(args[1].v) || std::holds_alternative<double>(args[1].v)) &&
        (std::holds_alternative<std::int64_t>(args[2].v) || std::holds_alternative<double>(args[2].v))) {
      const auto bins = (std::int64_t)as_i64(args[0]);
      const double mn = as_double(args[1]);
      const double mx = as_double(args[2]);
      if (op == "histc") return self.histc(bins, mn, mx);
    }

    // int,int,int for narrow/slice/select
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<std::int64_t>(args[1].v) &&
        std::holds_alternative<std::int64_t>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const auto start = as_i64(args[1]);
      const auto len = as_i64(args[2]);
      if (op == "narrow") return ops_internal::narrow(self, dim, start, len);
      if (op == "slice") return ops_internal::slice(self, dim, start, start + len, /*step=*/1);
      if (op == "select") return ops_internal::select(self, dim, start);
    }

    return not_impl(op, args, self);
  }

  return not_impl(op, args, self);
}

Series& invoke_any_(std::string_view op, Series& self, const Args& args) {
  // 0-arg inplace
  if (args.empty()) {
    if (op == "relu_") return ops_internal::relu_(self);
    if (op == "sigmoid_") return ops_internal::sigmoid_(self);
    if (op == "softplus_") return ops_internal::softplus_(self, 1.0, 20.0);
    if (op == "silu_") return ops_internal::silu_(self);
    if (op == "gelu_") return ops_internal::gelu_(self);

    if (op == "exp_") return ops_internal::exp_(self);
    if (op == "log_") return ops_internal::log_(self);
    if (op == "sqrt_") return ops_internal::sqrt_(self);

    return not_impl_inplace(op, args, self);
  }

  // 1-arg inplace (scalar)
  if (args.size() == 1) {
    const auto& a0 = args[0].v;
    if (std::holds_alternative<double>(a0) || std::holds_alternative<std::int64_t>(a0)) {
      const double s = as_double(args[0]);
      if (op == "add_") return ops_internal::add_(self, s);
      if (op == "sub_") return ops_internal::sub_(self, s);
      if (op == "mul_") return ops_internal::mul_(self, s);
      if (op == "div_") return ops_internal::div_(self, s);
      if (op == "leaky_relu_") return ops_internal::leaky_relu_(self, s);
      if (op == "clamp_min_") return ops_internal::clamp_min_(self, s);
      if (op == "clamp_max_") return ops_internal::clamp_max_(self, s);
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "add_") return ops_internal::add_(self, other);
      if (op == "sub_") return ops_internal::sub_(self, other);
      if (op == "mul_") return ops_internal::mul_(self, other);
      if (op == "div_") return ops_internal::div_(self, other);
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  // 2-arg inplace (scalar, scalar)
  if (args.size() == 2) {
    const auto& a0 = args[0].v;
    const auto& a1 = args[1].v;
    if ((std::holds_alternative<double>(a0) || std::holds_alternative<std::int64_t>(a0)) &&
        (std::holds_alternative<double>(a1) || std::holds_alternative<std::int64_t>(a1))) {
      const double x = as_double(args[0]);
      const double y = as_double(args[1]);
      if (op == "clamp_") return ops_internal::clamp_(self, x, y);
      if (op == "softplus_") return ops_internal::softplus_(self, x, y);
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& mask = as_series(args[0]);
      const Series& src = as_series(args[1]);
      if (op == "masked_scatter_") return ops_internal::masked_scatter_(self, mask, src);
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }


// 3-arg inplace: scatter_(dim, index, src), scatter_add_(dim, index, src)
if (args.size() == 3) {
  if (std::holds_alternative<std::int64_t>(args[0].v) &&
      std::holds_alternative<const Series*>(args[1].v) &&
      std::holds_alternative<const Series*>(args[2].v)) {
    const auto dim = as_i64(args[0]);
    const Series& index = as_series(args[1]);
    const Series& src = as_series(args[2]);
    if (op == "scatter_") return ops_internal::scatter_(self, dim, index, src);
    if (op == "scatter_add_") return ops_internal::scatter_add_(self, dim, index, src);
    return not_impl_inplace(op, args, self);
  }
  return not_impl_inplace(op, args, self);
}
return not_impl_inplace(op, args, self);
}

} // namespace pluss::codem::_compat
