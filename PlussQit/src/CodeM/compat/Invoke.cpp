#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Scalar.h"
#include <type_traits>

#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/Errors.h"
#include "CodeM/native/cpu/IndexingExtras.h"
#include "CodeM/native/cpu/SortingKernels.h"

#include <sstream>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

namespace pluss::codem::native::cpu {
Series matmul(const Series& a, const Series& b);
Series index_select(const Series& a, std::int64_t dim, const Series& index);
}

namespace pluss::codem::_compat {

using pluss::codem::warn;

constexpr double kPi = 3.14159265358979323846;


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
  return Series::undefined();
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

static std::string_view as_string(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<std::string>(a.v), "Expected string");
  return std::get<std::string>(a.v);
}

static bool is_numeric_scalar_arg(const Arg& a) {
  return std::holds_alternative<double>(a.v) ||
         std::holds_alternative<std::int64_t>(a.v) ||
         std::holds_alternative<Scalar>(a.v);
}

static Series scalar_like(const Series& self, double value) {
  const std::array<std::int64_t, 1> one{{1}};
  return Series::full(std::span<const std::int64_t>(one.data(), one.size()), value, self.scalar_type());
}

static Series add_scalar(const Series& a, double s) { return ops_internal::add(a, scalar_like(a, s)); }
static Series sub_scalar(const Series& a, double s) { return ops_internal::sub(a, scalar_like(a, s)); }
static Series mul_scalar(const Series& a, double s) { return ops_internal::mul(a, scalar_like(a, s)); }
static Series div_scalar(const Series& a, double s) { return ops_internal::div(a, scalar_like(a, s)); }
static Series pow_scalar(const Series& a, double s) { return ops_internal::pow(a, scalar_like(a, s)); }

static Series bool_scalar(bool value) {
  Series out = Series::zeros({}, ScalarType::Bool);
  *reinterpret_cast<std::uint8_t*>(out.raw_data()) = value ? 1 : 0;
  return out;
}

static Series i64_scalar(std::int64_t value) {
  Series out = Series::zeros({}, ScalarType::Int64);
  *reinterpret_cast<std::int64_t*>(out.raw_data()) = value;
  return out;
}

static Series i64_vector(std::span<const std::int64_t> values) {
  const std::array<std::int64_t, 1> shape{{static_cast<std::int64_t>(values.size())}};
  Series out = Series::empty(std::span<const std::int64_t>(shape.data(), shape.size()), ScalarType::Int64);
  auto* p = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(values.size()); ++i) p[i] = values[(std::size_t)i];
  return out;
}

static bool is_floating_dtype(ScalarType st) {
  return st == ScalarType::Float16 || st == ScalarType::BFloat16 ||
         st == ScalarType::Float32 || st == ScalarType::Float64;
}

static bool is_complex_dtype(ScalarType st) {
  return st == ScalarType::Complex64 || st == ScalarType::Complex128;
}

static bool is_signed_dtype(ScalarType st) {
  return st == ScalarType::Int8 || st == ScalarType::Int16 ||
         st == ScalarType::Int32 || st == ScalarType::Int64 ||
         is_floating_dtype(st) || is_complex_dtype(st);
}

static Series combine_beta_alpha(const Series& base, const Series& update, double beta, double alpha) {
  if (beta == 1.0 && alpha == 1.0) return ops_internal::add(base, update);
  return ops_internal::add(mul_scalar(base, beta), mul_scalar(update, alpha));
}

static std::int64_t wrap_dim(std::int64_t dim, std::int64_t nd, const char* op_name) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, std::string(op_name) + ": dim out of range");
  return dim;
}

static Series select_range(const Series& self, std::int64_t dim, std::int64_t start, std::int64_t len) {
  const auto nd = self.dim();
  dim = wrap_dim(dim, nd, "split/chunk/tensor_split");
  const auto dsz = self.sizes()[(std::size_t)dim];
  PLUSS_CHECK(start >= 0 && start <= dsz, "split/chunk/tensor_split: start out of range");
  PLUSS_CHECK(len >= 0, "split/chunk/tensor_split: len must be >= 0");
  if (len == 0) {
    auto out_sizes = std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end());
    out_sizes[(std::size_t)dim] = 0;
    return Series::empty(out_sizes, self.scalar_type());
  }
  std::vector<std::int64_t> idx((std::size_t)len);
  for (std::int64_t i = 0; i < len; ++i) idx[(std::size_t)i] = start + i;
  Series index = i64_vector(std::span<const std::int64_t>(idx.data(), idx.size()));
  return native::cpu::index_select(self, dim, index);
}

static Series split_first_by_size(const Series& self, std::int64_t split_size, std::int64_t dim) {
  PLUSS_CHECK(split_size > 0, "split: split_size must be > 0");
  dim = wrap_dim(dim, self.dim(), "split");
  const auto dsz = self.sizes()[(std::size_t)dim];
  return select_range(self, dim, 0, std::min<std::int64_t>(split_size, dsz));
}

static Series split_first_by_boundaries(const Series& self, std::span<const std::int64_t> boundaries, std::int64_t dim, const char* op_name) {
  dim = wrap_dim(dim, self.dim(), op_name);
  const auto dsz = self.sizes()[(std::size_t)dim];
  if (boundaries.empty()) return self.clone();
  std::int64_t first = boundaries[0];
  if (first < 0) first += dsz;
  first = std::clamp<std::int64_t>(first, 0, dsz);
  return select_range(self, dim, 0, first);
}

static Series chunk_first(const Series& self, std::int64_t chunks, std::int64_t dim) {
  PLUSS_CHECK(chunks > 0, "chunk: chunks must be > 0");
  dim = wrap_dim(dim, self.dim(), "chunk");
  const auto dsz = self.sizes()[(std::size_t)dim];
  const auto split_size = (dsz + chunks - 1) / chunks;
  return select_range(self, dim, 0, std::min<std::int64_t>(split_size, dsz));
}

static Series tensor_split_first(const Series& self, const Arg& sections_or_indices, std::int64_t dim) {
  if (is_numeric_scalar_arg(sections_or_indices)) {
    const auto sections = as_i64(sections_or_indices);
    PLUSS_CHECK(sections > 0, "tensor_split: sections must be > 0");
    return chunk_first(self, sections, dim);
  }
  if (std::holds_alternative<Arg::IntList>(sections_or_indices.v)) {
    return split_first_by_boundaries(self, as_intlist(sections_or_indices), dim, "tensor_split");
  }
  PLUSS_CHECK(false, "tensor_split: expected sections(int) or indices(int[])");
  return Series::undefined();
}

static Series sort_values_only(const Series& self, std::int64_t dim, bool descending, bool stable) {
  auto pair = native::cpu::sort(self, dim, descending, stable);
  return pair.first;
}

static Series topk_values_only(const Series& self, std::int64_t k, std::int64_t dim, bool largest, bool sorted) {
  auto pair = native::cpu::topk(self, k, dim, largest, sorted);
  return pair.first;
}

static Series kthvalue_values_only(const Series& self, std::int64_t k, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(k >= 1, "kthvalue: k must be >= 1");
  dim = wrap_dim(dim, self.dim(), "kthvalue");
  PLUSS_CHECK(k <= self.sizes()[(std::size_t)dim], "kthvalue: k must be <= size(dim)");
  auto pair = native::cpu::topk(self, k, dim, /*largest=*/false, /*sorted=*/true);
  Series value = pair.first.select(dim, k - 1);
  if (keepdim) value = value.unsqueeze(dim);
  return value;
}

static Series tril_triu_impl(const Series& self, std::int64_t diagonal, bool upper) {
  PLUSS_CHECK(self.dim() >= 2, "tril/triu: expected tensor with at least 2 dimensions");
  auto x = self.contiguous();
  auto out = Series::zeros(x.sizes(), x.scalar_type());

  const auto nd = x.dim();
  const auto rows = x.sizes()[(std::size_t)(nd - 2)];
  const auto cols = x.sizes()[(std::size_t)(nd - 1)];
  const auto item = x.dtype().itemsize;
  const auto batches = (rows == 0 || cols == 0) ? 0 : (x.numel() / (rows * cols));

  const auto* src = x.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  for (std::int64_t b = 0; b < batches; ++b) {
    for (std::int64_t r = 0; r < rows; ++r) {
      std::int64_t lo = 0;
      std::int64_t hi = 0;
      if (!upper) {
        lo = 0;
        hi = std::clamp<std::int64_t>(r + diagonal + 1, 0, cols);
      } else {
        lo = std::clamp<std::int64_t>(r + diagonal, 0, cols);
        hi = cols;
      }
      if (hi <= lo) continue;
      const auto row_base = (b * rows + r) * cols;
      const auto byte_off = (row_base + lo) * item;
      const auto nbytes = static_cast<std::size_t>((hi - lo) * item);
      std::memcpy(dst + byte_off, src + byte_off, nbytes);
    }
  }
  return out;
}

static Series diag_extract_2d(const Series& self, std::int64_t diagonal) {
  PLUSS_CHECK(self.dim() == 2, "diag/diagonal: extraction expects a 2D input");
  auto x = self.contiguous();
  const auto rows = x.sizes()[0];
  const auto cols = x.sizes()[1];
  const auto row0 = diagonal < 0 ? -diagonal : 0;
  const auto col0 = diagonal > 0 ? diagonal : 0;
  const auto len = std::max<std::int64_t>(0, std::min(rows - row0, cols - col0));
  auto out = Series::empty({len}, x.scalar_type());
  const auto item = x.dtype().itemsize;
  const auto* src = x.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  for (std::int64_t i = 0; i < len; ++i) {
    const auto src_off = ((row0 + i) * cols + (col0 + i)) * item;
    const auto dst_off = i * item;
    std::memcpy(dst + dst_off, src + src_off, static_cast<std::size_t>(item));
  }
  return out;
}

static Series diag_embed_from_1d(const Series& self, std::int64_t diagonal) {
  auto v = self.flatten(0, -1).contiguous();
  PLUSS_CHECK(v.dim() == 1, "diag_embed/diagflat: expected 1D after flatten");
  const auto n = v.numel();
  const auto side = n + std::llabs(diagonal);
  auto out = Series::zeros({side, side}, v.scalar_type());
  const auto row0 = diagonal < 0 ? -diagonal : 0;
  const auto col0 = diagonal > 0 ? diagonal : 0;
  const auto item = v.dtype().itemsize;
  const auto* src = v.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    const auto src_off = i * item;
    const auto dst_off = ((row0 + i) * side + (col0 + i)) * item;
    std::memcpy(dst + dst_off, src + src_off, static_cast<std::size_t>(item));
  }
  return out;
}

static Series isclose_impl(const Series& a, const Series& b, double rtol, double atol, bool equal_nan) {
  Series diff = ops_internal::abs(ops_internal::sub(a, b));
  Series tol = add_scalar(mul_scalar(ops_internal::abs(b), rtol), atol);
  Series close = ops_internal::le(diff, tol);
  if (equal_nan) {
    Series both_nan = ops_internal::logical_and(ops_internal::isnan(a), ops_internal::isnan(b));
    close = ops_internal::logical_or(close, both_nan);
  }
  return close;
}

static std::string_view canonical_op(std::string_view op) {
  if (op == "absolute") return "abs";
  if (op == "arccos") return "acos";
  if (op == "arccosh") return "acosh";
  if (op == "arcsin") return "asin";
  if (op == "arcsinh") return "asinh";
  if (op == "arctan") return "atan";
  if (op == "arctan2") return "atan2";
  if (op == "arctanh") return "atanh";
  if (op == "divide") return "div";
  if (op == "true_divide") return "div";
  if (op == "multiply") return "mul";
  if (op == "subtract") return "sub";
  if (op == "greater") return "gt";
  if (op == "greater_equal") return "ge";
  if (op == "less") return "lt";
  if (op == "less_equal") return "le";
  if (op == "not_equal") return "ne";
  if (op == "negative") return "neg";
  if (op == "argwhere") return "nonzero";
  if (op == "nonzero_numpy") return "nonzero";
  if (op == "nonzero_static") return "nonzero";
  if (op == "conj_physical") return "conj";
  if (op == "_conj") return "conj";
  if (op == "_conj_physical") return "conj";
  if (op == "_to_dense") return "to_dense";
  return op;
}

static std::string_view canonical_inplace_op(std::string_view op) {
  if (op == "absolute_") return "abs_";
  if (op == "arccos_") return "acos_";
  if (op == "arccosh_") return "acosh_";
  if (op == "arcsin_") return "asin_";
  if (op == "arcsinh_") return "asinh_";
  if (op == "arctan_") return "atan_";
  if (op == "arctan2_") return "atan2_";
  if (op == "arctanh_") return "atanh_";
  if (op == "divide_") return "div_";
  if (op == "true_divide_") return "div_";
  if (op == "multiply_") return "mul_";
  if (op == "subtract_") return "sub_";
  if (op == "negative_") return "neg_";
  if (op == "greater_") return "gt_";
  if (op == "greater_equal_") return "ge_";
  if (op == "less_") return "lt_";
  if (op == "less_equal_") return "le_";
  if (op == "not_equal_") return "ne_";
  return op;
}

Series invoke_any(std::string_view op, const Series& self, const Args& args) {
  op = canonical_op(op);

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
    if (op == "log1p") return ops_internal::log1p(self);
    if (op == "expm1") return ops_internal::expm1(self);
    if (op == "neg") return ops_internal::neg(self);
    if (op == "conj") return ops_internal::conj(self);
    if (op == "real") return ops_internal::real(self);
    if (op == "imag") return ops_internal::imag(self);
    if (op == "angle") return ops_internal::angle(self);

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
    if (op == "isposinf") return ops_internal::logical_and(ops_internal::isinf(self), ops_internal::gt(self, scalar_like(self, 0.0)));
    if (op == "isneginf") return ops_internal::logical_and(ops_internal::isinf(self), ops_internal::lt(self, scalar_like(self, 0.0)));
    if (op == "isreal") {
      if (!is_complex_dtype(self.scalar_type())) return Series::ones(self.sizes(), ScalarType::Bool);
      auto im = ops_internal::imag(self);
      return ops_internal::eq(im, scalar_like(im, 0.0));
    }
    if (op == "logical_not") return ops_internal::logical_not(self);
    if (op == "signbit") return ops_internal::lt(self, scalar_like(self, 0.0));

    if (op == "abs") return ops_internal::abs(self);
    if (op == "sign" || op == "sgn") {
      Series pos = ops_internal::gt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      Series neg = ops_internal::lt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      return ops_internal::sub(pos, neg).to(self.scalar_type());
    }

    if (op == "sum") return ops_internal::sum(self);
    if (op == "mean") return ops_internal::mean(self);
    if (op == "prod") return ops_internal::prod(self);
    if (op == "amax") return ops_internal::amax(self);
    if (op == "amin") return ops_internal::amin(self);
    if (op == "all") return ops_internal::all(self);
    if (op == "any") return ops_internal::any(self);
    if (op == "argmax") return ops_internal::argmax(self, -1, false);
    if (op == "argmin") return ops_internal::argmin(self, -1, false);
    if (op == "count_nonzero") {
      auto nz = ops_internal::nonzero(self);
      Series out = Series::zeros({}, ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = (nz.dim() > 0) ? nz.sizes()[0] : 0;
      return out;
    }
    if (op == "_nnz") {
      auto nz = ops_internal::nonzero(self);
      return i64_scalar((nz.dim() > 0) ? nz.sizes()[0] : 0);
    }
    if (op == "_is_all_true") return ops_internal::all(self);
    if (op == "_is_any_true") return ops_internal::any(self);
    if (op == "_is_zerotensor") return ops_internal::all(ops_internal::eq(self, scalar_like(self, 0.0)));
    if (op == "is_complex") return bool_scalar(is_complex_dtype(self.scalar_type()));
    if (op == "is_floating_point") return bool_scalar(is_floating_dtype(self.scalar_type()));
    if (op == "is_signed") return bool_scalar(is_signed_dtype(self.scalar_type()));
    if (op == "is_conj" || op == "is_neg" || op == "is_leaf" || op == "is_inference" ||
        op == "is_distributed" || op == "is_pinned" || op == "is_coalesced" || op == "is_set_to") {
      return bool_scalar(false);
    }
    if (op == "output_nr" || op == "_version") return i64_scalar(0);
    if (op == "size") return i64_vector(self.sizes());
    if (op == "stride") return i64_vector(self.strides());
    if (op == "item") {
      PLUSS_CHECK(self.numel() == 1, "item: expected tensor with exactly one element");
      return self.flatten(0, -1).select(0, 0);
    }
    if (op == "is_nonzero") {
      PLUSS_CHECK(self.numel() == 1, "is_nonzero: expected tensor with exactly one element");
      Series s = self.flatten(0, -1).to(ScalarType::Float64);
      const double v = *reinterpret_cast<const double*>(s.raw_data());
      return bool_scalar(v != 0.0);
    }
    if (op == "alias") return self;
    if (op == "data") return self;
    if (op == "detach") return self;
    if (op == "_lazy_clone") return self.clone();
    if (op == "_neg_view") return ops_internal::neg(self);
    if (op == "to_dense") return self.clone();
    if (op == "fix") return ops_internal::trunc(self);
    if (op == "frac") return ops_internal::sub(self, ops_internal::floor(self));
    if (op == "deg2rad") return mul_scalar(self, kPi / 180.0);
    if (op == "rad2deg") return mul_scalar(self, 180.0 / kPi);
    if (op == "positive") return self;
    if (op == "ravel") return self.flatten(0, -1);
    if (op == "mT") return self.transpose(-2, -1);
    if (op == "mH" || op == "matrix_H") return ops_internal::conj(self.transpose(-2, -1));
    if (op == "adjoint") return ops_internal::conj(self.transpose(-2, -1));
    if (op == "numpy_T") return self.transpose(-2, -1);
    if (op == "sort") return sort_values_only(self, /*dim=*/-1, /*descending=*/false, /*stable=*/false);
    if (op == "tril") return tril_triu_impl(self, /*diagonal=*/0, /*upper=*/false);
    if (op == "triu") return tril_triu_impl(self, /*diagonal=*/0, /*upper=*/true);
    if (op == "trace") return ops_internal::sum(diag_extract_2d(self, /*diagonal=*/0));
    if (op == "diag") {
      if (self.dim() == 1) return diag_embed_from_1d(self, /*diagonal=*/0);
      return diag_extract_2d(self, /*diagonal=*/0);
    }
    if (op == "diagonal") return diag_extract_2d(self, /*diagonal=*/0);
    if (op == "diagflat") return diag_embed_from_1d(self, /*diagonal=*/0);
    if (op == "diag_embed") return diag_embed_from_1d(self, /*diagonal=*/0);
    if (op == "t") {
      if (self.dim() != 2) return self;
      return self.transpose(0, 1);
    }

    if (op == "contiguous") return self.contiguous();
    if (op == "clone") return self.clone();
    if (op == "dequantize" || op == "int_repr" || op == "coalesce" || op == "pin_memory" ||
        op == "record_stream" || op == "resolve_conj" || op == "resolve_neg") return self;
    if (op == "_indices" || op == "_values" || op == "indices" || op == "values" ||
        op == "crow_indices" || op == "ccol_indices" || op == "col_indices" || op == "row_indices") return self.nonzero();
    if (op == "dense_dim" || op == "sparse_dim") return i64_scalar(0);
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
    if (is_numeric_scalar_arg(args[0])) {
      const double s = as_double(args[0]);
      if (op == "mul") return mul_scalar(self, s);
      if (op == "add") return add_scalar(self, s);
      if (op == "sub") return sub_scalar(self, s);
      if (op == "div") return div_scalar(self, s);
      if (op == "float_power") return pow_scalar(self, s);
      if (op == "floor_divide") return ops_internal::floor(div_scalar(self, s));
      if (op == "pow") return pow_scalar(self, s);
      if (op == "copysign") {
        const double sign = std::signbit(s) ? -1.0 : 1.0;
        return mul_scalar(ops_internal::abs(self), sign);
      }
      if (op == "logaddexp") return ops_internal::log(ops_internal::add(ops_internal::exp(self), ops_internal::exp(scalar_like(self, s))));
      if (op == "logaddexp2") return ops_internal::log2(ops_internal::add(ops_internal::exp2(self), ops_internal::exp2(scalar_like(self, s))));
      if (op == "hypot") return ops_internal::sqrt(ops_internal::add(ops_internal::square(self), ops_internal::square(scalar_like(self, s))));
      if (op == "xlogy") return ops_internal::mul(self, ops_internal::log(scalar_like(self, s)));
      if (op == "clamp_min") return ops_internal::clamp_min(self, s);
      if (op == "clamp_max") return ops_internal::clamp_max(self, s);
      if (op == "maximum") return ops_internal::maximum(self, scalar_like(self, s));
      if (op == "minimum") return ops_internal::minimum(self, scalar_like(self, s));
      if (op == "fmod") return ops_internal::fmod(self, scalar_like(self, s));
      if (op == "remainder") return ops_internal::remainder(self, scalar_like(self, s));
      if (op == "atan2") return ops_internal::atan2(self, scalar_like(self, s));
      if (op == "eq") return ops_internal::eq(self, scalar_like(self, s));
      if (op == "ne") return ops_internal::ne(self, scalar_like(self, s));
      if (op == "lt") return ops_internal::lt(self, scalar_like(self, s));
      if (op == "le") return ops_internal::le(self, scalar_like(self, s));
      if (op == "gt") return ops_internal::gt(self, scalar_like(self, s));
      if (op == "ge") return ops_internal::ge(self, scalar_like(self, s));
      if (op == "bincount") return ops_internal::bincount(self, (std::int64_t)s);
      if (op == "argsort") return native::cpu::argsort(self, (std::int64_t)s, /*descending=*/false, /*stable=*/false);
      if (op == "sort") return sort_values_only(self, (std::int64_t)s, /*descending=*/false, /*stable=*/false);
      if (op == "topk") return topk_values_only(self, (std::int64_t)s, /*dim=*/-1, /*largest=*/true, /*sorted=*/true);
      if (op == "kthvalue") return kthvalue_values_only(self, (std::int64_t)s, /*dim=*/-1, /*keepdim=*/false);
      if (op == "softmax") return ops_internal::softmax(self, (std::int64_t)s);
      if (op == "log_softmax") return ops_internal::log_softmax(self, (std::int64_t)s);
      if (op == "factorial") return self.factorial((std::int64_t)s);
      if (op == "unsqueeze") return self.unsqueeze((std::int64_t)s);
      if (op == "squeeze") return ops_internal::squeeze(self, (std::int64_t)s);
      if (op == "split" || op == "unsafe_split") return split_first_by_size(self, (std::int64_t)s, /*dim=*/0);
      if (op == "chunk" || op == "unsafe_chunk") return chunk_first(self, (std::int64_t)s, /*dim=*/0);
      if (op == "tensor_split") return chunk_first(self, (std::int64_t)s, /*dim=*/0);
      if (op == "tril") return tril_triu_impl(self, (std::int64_t)s, /*upper=*/false);
      if (op == "triu") return tril_triu_impl(self, (std::int64_t)s, /*upper=*/true);
      if (op == "diag") {
        if (self.dim() == 1) return diag_embed_from_1d(self, (std::int64_t)s);
        return diag_extract_2d(self, (std::int64_t)s);
      }
      if (op == "diagonal") return diag_extract_2d(self, (std::int64_t)s);
      if (op == "diagflat") return diag_embed_from_1d(self, (std::int64_t)s);
      if (op == "diag_embed") return diag_embed_from_1d(self, (std::int64_t)s);
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amax") return ops_internal::amax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amin") return ops_internal::amin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "all") return ops_internal::all(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "any") return ops_internal::any(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "cumsum") return self.cumsum((std::int64_t)s, /*keepdim=*/false);
      if (op == "cumprod") return self.cumprod((std::int64_t)s, /*keepdim=*/false);
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
      if (op == "pow") return ops_internal::pow(self, other);
      if (op == "floor_divide") return ops_internal::floor(ops_internal::div(self, other));

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
      if (op == "float_power") return ops_internal::pow(self, other);
      if (op == "copysign") {
        Series mag = ops_internal::abs(self);
        Series pos = ops_internal::ge(other, scalar_like(other, 0.0));
        return ops_internal::where(pos, mag, ops_internal::neg(mag));
      }
      if (op == "logaddexp") return ops_internal::log(ops_internal::add(ops_internal::exp(self), ops_internal::exp(other)));
      if (op == "logaddexp2") return ops_internal::log2(ops_internal::add(ops_internal::exp2(self), ops_internal::exp2(other)));
      if (op == "hypot") return ops_internal::sqrt(ops_internal::add(ops_internal::square(self), ops_internal::square(other)));
      if (op == "xlogy") return ops_internal::mul(self, ops_internal::log(other));
      if (op == "fmax") return ops_internal::maximum(self, other);
      if (op == "fmin") return ops_internal::minimum(self, other);
      if (op == "equal") return ops_internal::all(ops_internal::eq(self, other));
      if (op == "allclose") return ops_internal::all(isclose_impl(self, other, 1e-5, 1e-8, false));
      if (op == "isclose") return isclose_impl(self, other, 1e-5, 1e-8, false);
      if (op == "is_same_size") return bool_scalar(self.sizes().size() == other.sizes().size() &&
                                                   std::equal(self.sizes().begin(), self.sizes().end(), other.sizes().begin()));
      if (op == "reshape_as" || op == "view_as") return self.reshape(other.sizes());
      if (op == "type_as") return self.to(other.scalar_type());
      if (op == "ger") return native::cpu::matmul(self.unsqueeze(1), other.unsqueeze(0));

      if (op == "matmul") return native::cpu::matmul(self, other);
      if (op == "mm") return native::cpu::matmul(self, other);
      if (op == "mv") return native::cpu::matmul(self, other);
      if (op == "bmm") return native::cpu::matmul(self, other);
      if (op == "index") return self.take(other);
      if (op == "take") return self.take(other);
      if (op == "take_along_dim") {
        auto flat_self = self.flatten(0, -1);
        auto flat_idx = other.flatten(0, -1).to(ScalarType::Int64);
        return native::cpu::take(flat_self, flat_idx);
      }
      if (op == "dot") {
        auto a1 = self.flatten(0, -1);
        auto b1 = other.flatten(0, -1);
        PLUSS_CHECK(a1.numel() == b1.numel(), "dot/inner: flattened inputs must have same numel");
        return ops_internal::sum(ops_internal::mul(a1, b1));
      }
      if (op == "inner") {
        auto a1 = self.flatten(0, -1);
        auto b1 = other.flatten(0, -1);
        PLUSS_CHECK(a1.numel() == b1.numel(), "inner: flattened inputs must have same numel");
        return ops_internal::sum(ops_internal::mul(a1, b1));
      }
      if (op == "outer") return native::cpu::matmul(self.unsqueeze(1), other.unsqueeze(0));
      if (op == "dist") return ops_internal::sub(self, other).norm(2.0);
      if (op == "expand_as") return self.expand(other.sizes());

      if (op == "masked_select") return ops_internal::masked_select(self, other);
      if (op == "gather") return not_impl(op, args, self); // gather requires (dim, index)
      // note: masked_scatter needs 2 args (mask, src)
      return not_impl(op, args, self);
    }

    // intlist
    if (std::holds_alternative<Arg::IntList>(a0)) {
      auto sizes = as_intlist(args[0]);
      if (op == "reshape") return self.reshape(sizes);
      if (op == "view") return self.view(sizes);
      if (op == "expand") return self.expand(sizes);
      if (op == "broadcast_to") return self.expand(sizes);
      if (op == "permute") return self.permute(sizes);
      if (op == "split" || op == "unsafe_split") {
        if (sizes.empty()) return self.clone();
        PLUSS_CHECK(sizes[0] >= 0, "split: split_sizes entries must be >= 0");
        const auto dsz = self.sizes().empty() ? 0 : self.sizes()[0];
        return select_range(self, /*dim=*/0, /*start=*/0, std::min<std::int64_t>(sizes[0], dsz));
      }
      if (op == "tensor_split") return split_first_by_boundaries(self, sizes, /*dim=*/0, "tensor_split");
      if (op == "new_empty") return Series::empty(sizes, self.scalar_type());
      if (op == "new_zeros") return Series::zeros(sizes, self.scalar_type());
      if (op == "new_ones") return Series::ones(sizes, self.scalar_type());
      return not_impl(op, args, self);
    }

    return not_impl(op, args, self);
  }

  // 2-arg
  if (args.size() == 2) {
    const auto& a0 = args[0].v;
    const auto& a1 = args[1].v;

    // scalar + scalar
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1])) {
      const double x = as_double(args[0]);
      const double y = as_double(args[1]);
      if (op == "clamp") return ops_internal::clamp(self, x, y);
      if (op == "softplus") return ops_internal::softplus(self, x, y);
      if (op == "nan_to_num") return ops_internal::nan_to_num(self, x, y, y);
      if (op == "lerp") return ops_internal::add(self, mul_scalar(ops_internal::sub(scalar_like(self, x), self), y));
      if (op == "leaky_relu") return not_impl(op, args, self); // leaky_relu takes 1 scalar (negative_slope) in this dispatcher
      if (op == "squeeze") return ops_internal::squeeze(self, (std::int64_t)x);
      if (op == "unsqueeze") return self.unsqueeze((std::int64_t)x);
      if (op == "transpose") return self.transpose((std::int64_t)x, (std::int64_t)y);
      if (op == "movedim" || op == "moveaxis" || op == "swapdims" || op == "swapaxes")
        return self.transpose((std::int64_t)x, (std::int64_t)y);
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "amax") return ops_internal::amax(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "amin") return ops_internal::amin(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "all") return ops_internal::all(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "any") return ops_internal::any(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argsort") return native::cpu::argsort(self, (std::int64_t)x, as_bool(args[1]), /*stable=*/false);
      if (op == "sort") return sort_values_only(self, (std::int64_t)x, /*descending=*/(y != 0.0), /*stable=*/false);
      if (op == "topk") return topk_values_only(self, (std::int64_t)x, (std::int64_t)y, /*largest=*/true, /*sorted=*/true);
      if (op == "kthvalue") return kthvalue_values_only(self, (std::int64_t)x, (std::int64_t)y, /*keepdim=*/false);
      if (op == "split" || op == "unsafe_split") return split_first_by_size(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "chunk" || op == "unsafe_chunk") return chunk_first(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "tensor_split") return tensor_split_first(self, args[0], (std::int64_t)y);
      return not_impl(op, args, self);
    }

    // dim + keepdim(bool)
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<bool>(a1)) {
      const auto dim = as_i64(args[0]);
      const bool keepdim = as_bool(args[1]);
      if (op == "sum") return ops_internal::sum(self, dim, keepdim);
      if (op == "mean") return ops_internal::mean(self, dim, keepdim);
      if (op == "prod") return ops_internal::prod(self, dim, keepdim);
      if (op == "amax") return ops_internal::amax(self, dim, keepdim);
      if (op == "amin") return ops_internal::amin(self, dim, keepdim);
      if (op == "all") return ops_internal::all(self, dim, keepdim);
      if (op == "any") return ops_internal::any(self, dim, keepdim);
      if (op == "argmax") return ops_internal::argmax(self, dim, keepdim);
      if (op == "argmin") return ops_internal::argmin(self, dim, keepdim);
      if (op == "argsort") return native::cpu::argsort(self, dim, keepdim, /*stable=*/false);
      if (op == "sort") return sort_values_only(self, dim, keepdim, /*stable=*/false);
      if (op == "cumsum") return self.cumsum(dim, keepdim);
      if (op == "cumprod") return self.cumprod(dim, keepdim);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::IntList>(a0) && is_numeric_scalar_arg(args[1])) {
      auto sizes = as_intlist(args[0]);
      if (op == "split" || op == "unsafe_split") {
        if (sizes.empty()) return self.clone();
        PLUSS_CHECK(sizes[0] >= 0, "split: split_sizes entries must be >= 0");
        const auto dim = as_i64(args[1]);
        return split_first_by_size(self, sizes[0], dim);
      }
      if (op == "tensor_split") return split_first_by_boundaries(self, sizes, as_i64(args[1]), "tensor_split");
      if (op == "new_full") return Series::full(sizes, as_double(args[1]), self.scalar_type());
      return not_impl(op, args, self);
    }

    // Series + scalar
    if (std::holds_alternative<const Series*>(a0) && is_numeric_scalar_arg(args[1])) {
      const Series& other = as_series(args[0]);
      const double s = as_double(args[1]);
      if (op == "lerp") return ops_internal::add(self, mul_scalar(ops_internal::sub(other, self), s));
      if (op == "take_along_dim") return self.gather((std::int64_t)s, other);
      return not_impl(op, args, self);
    }


// scalar + Series (dim, index/src)
if (is_numeric_scalar_arg(args[0]) &&
    std::holds_alternative<const Series*>(a1)) {
  const auto dim = as_i64(args[0]);
  const Series& other = as_series(args[1]);
  if (op == "gather") return self.gather(dim, other);
  if (op == "take_along_dim") return self.gather(dim, other);
  if (op == "index_select") return native::cpu::index_select(self, dim, other);
  if (op == "scatter") return not_impl(op, args, self); // scatter requires (dim, index, src)
  if (op == "scatter_add") return not_impl(op, args, self); // scatter_add requires (dim, index, src)
  return not_impl(op, args, self);
}
// Series + Series
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& m = as_series(args[0]);
      const Series& s = as_series(args[1]);
      if (op == "addmm") return combine_beta_alpha(self, native::cpu::matmul(m, s), /*beta=*/1.0, /*alpha=*/1.0);
      if (op == "addmv") return combine_beta_alpha(self, native::cpu::matmul(m, s), /*beta=*/1.0, /*alpha=*/1.0);
      if (op == "addr") return combine_beta_alpha(self, native::cpu::matmul(m.unsqueeze(1), s.unsqueeze(0)), /*beta=*/1.0, /*alpha=*/1.0);
      if (op == "addbmm") {
        Series upd = native::cpu::matmul(m, s).sum(/*dim=*/0, /*keepdim=*/false);
        return combine_beta_alpha(self, upd, /*beta=*/1.0, /*alpha=*/1.0);
      }
      if (op == "baddbmm") return combine_beta_alpha(self, native::cpu::matmul(m, s), /*beta=*/1.0, /*alpha=*/1.0);
      if (op == "lerp") return ops_internal::add(self, ops_internal::mul(ops_internal::sub(m, self), s));
      if (op == "masked_scatter") return self.masked_scatter(m, s);
      if (op == "where") return ops_internal::where(self, m, s);
      if (op == "index_put") return native::cpu::put(self, m, s, /*accumulate=*/false);
      if (op == "put") {
        return native::cpu::put(self, m, s, /*accumulate=*/false);
      }
      return not_impl(op, args, self);
    }

    return not_impl(op, args, self);
  }

  // 3-arg
  if (args.size() == 3) {
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<bool>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const bool descending = as_bool(args[1]);
      const bool stable = as_bool(args[2]);
      if (op == "sort") return sort_values_only(self, dim, descending, stable);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        std::holds_alternative<bool>(args[2].v)) {
      const auto x = as_i64(args[0]);
      const auto y = as_i64(args[1]);
      const bool b = as_bool(args[2]);
      if (op == "topk") return topk_values_only(self, x, y, b, /*sorted=*/true);
      if (op == "kthvalue") return kthvalue_values_only(self, x, y, b);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      if (op == "index_copy") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_copy: currently implemented for 1D dim=0");
        return native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/false);
      }
      if (op == "index_add") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_add: currently implemented for 1D dim=0");
        return native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/true);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      const double fill_value = as_double(args[2]);
      if (op == "index_fill") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_fill: currently implemented for 1D dim=0");
        const auto n = index.numel();
        auto src = Series::full({n}, fill_value, self.scalar_type());
        return native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/false);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      const auto diagonal = as_i64(args[0]);
      const auto dim1 = as_i64(args[1]);
      const auto dim2 = as_i64(args[2]);
      if (op == "diagonal") {
        PLUSS_CHECK(self.dim() == 2, "diagonal: currently implemented for 2D");
        auto d1 = wrap_dim(dim1, self.dim(), "diagonal");
        auto d2 = wrap_dim(dim2, self.dim(), "diagonal");
        PLUSS_CHECK(d1 != d2, "diagonal: dim1 and dim2 must differ");
        if (d1 == 0 && d2 == 1) return diag_extract_2d(self, diagonal);
        if (d1 == 1 && d2 == 0) return diag_extract_2d(self.transpose(0, 1), diagonal);
        return not_impl(op, args, self);
      }
    }

    // histc(bins, min, max)
    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      const auto bins = (std::int64_t)as_i64(args[0]);
      const double mn = as_double(args[1]);
      const double mx = as_double(args[2]);
      if (op == "histc") return self.histc(bins, mn, mx);
      if (op == "nan_to_num") return ops_internal::nan_to_num(self, as_double(args[0]), mn, mx);
    }

    // int,int,int for narrow/slice/select
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<std::int64_t>(args[1].v) &&
        std::holds_alternative<std::int64_t>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const auto start = as_i64(args[1]);
      const auto len = as_i64(args[2]);
      const auto wd = wrap_dim(dim, self.dim(), "narrow/slice/select");
      const auto dsz = self.sizes()[(std::size_t)wd];
      auto st = start;
      if (st < 0) st += dsz;
      st = std::clamp<std::int64_t>(st, 0, dsz);
      if (op == "narrow") return select_range(self, wd, st, std::min<std::int64_t>(len, dsz - st));
      if (op == "narrow_copy") return select_range(self, wd, st, std::min<std::int64_t>(len, dsz - st)).clone();
      if (op == "slice") return select_range(self, wd, st, std::min<std::int64_t>(len, dsz - st));
      if (op == "select") return self.select(wd, st);
    }

    // addcmul(self, tensor1, tensor2, value)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      const Series& other = as_series(args[0]);
      const double rtol = as_double(args[1]);
      const double atol = as_double(args[2]);
      if (op == "allclose") return ops_internal::all(isclose_impl(self, other, rtol, atol, false));
      if (op == "isclose") return isclose_impl(self, other, rtol, atol, false);
    }

    // addcmul(self, tensor1, tensor2, value)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        (std::holds_alternative<double>(args[2].v) || std::holds_alternative<std::int64_t>(args[2].v) || std::holds_alternative<Scalar>(args[2].v))) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double v = as_double(args[2]);
      if (op == "addcmul") return ops_internal::add(self, mul_scalar(ops_internal::mul(b, c), v));
      if (op == "addcdiv") return ops_internal::add(self, mul_scalar(ops_internal::div(b, c), v));
    }
    // put(index, src, accumulate)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const Series& index = as_series(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put") return native::cpu::put(self, index, src, accumulate);
      if (op == "put") {
        return native::cpu::put(self, index, src, accumulate);
      }
    }

    return not_impl(op, args, self);
  }

  // 4-arg
  if (args.size() == 4) {
    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        std::holds_alternative<bool>(args[2].v) &&
        std::holds_alternative<bool>(args[3].v)) {
      const auto k = as_i64(args[0]);
      const auto dim = as_i64(args[1]);
      const bool largest = as_bool(args[2]);
      const bool sorted = as_bool(args[3]);
      if (op == "topk") return topk_values_only(self, k, dim, largest, sorted);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        is_numeric_scalar_arg(args[3])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const double alpha = as_double(args[3]);
      if (op == "index_add") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_add: currently implemented for 1D dim=0");
        if (alpha != 1.0) src = mul_scalar(src, alpha).to(self.scalar_type());
        return native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/true);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      if (op == "index_reduce") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_reduce: currently implemented for 1D dim=0");
        PLUSS_CHECK(reduce == "sum", "index_reduce: only 'sum' is implemented");
        return native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/true);
      }
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        std::holds_alternative<bool>(args[3].v)) {
      const Series& other = as_series(args[0]);
      const double rtol = as_double(args[1]);
      const double atol = as_double(args[2]);
      const bool equal_nan = as_bool(args[3]);
      if (op == "allclose") return ops_internal::all(isclose_impl(self, other, rtol, atol, equal_nan));
      if (op == "isclose") return isclose_impl(self, other, rtol, atol, equal_nan);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double beta = as_double(args[2]);
      const double alpha = as_double(args[3]);
      if (op == "addmm") return combine_beta_alpha(self, native::cpu::matmul(b, c), beta, alpha);
      if (op == "addmv") return combine_beta_alpha(self, native::cpu::matmul(b, c), beta, alpha);
      if (op == "addr") return combine_beta_alpha(self, native::cpu::matmul(b.unsqueeze(1), c.unsqueeze(0)), beta, alpha);
      if (op == "addbmm") {
        Series upd = native::cpu::matmul(b, c).sum(/*dim=*/0, /*keepdim=*/false);
        return combine_beta_alpha(self, upd, beta, alpha);
      }
      if (op == "baddbmm") return combine_beta_alpha(self, native::cpu::matmul(b, c), beta, alpha);
    }
    return not_impl(op, args, self);
  }

  // 5-arg
  if (args.size() == 5) {
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v) &&
        std::holds_alternative<bool>(args[4].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      const bool include_self = as_bool(args[4]);
      if (op == "index_reduce") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_reduce: currently implemented for 1D dim=0");
        PLUSS_CHECK(reduce == "sum", "index_reduce: only 'sum' is implemented");
        auto out = include_self ? self.clone() : self.clone();
        if (!include_self) {
          auto zeros = Series::zeros({index.numel()}, out.scalar_type());
          out = native::cpu::put(out, index.to(ScalarType::Int64), zeros, /*accumulate=*/false);
        }
        return native::cpu::put(out, index.to(ScalarType::Int64), src, /*accumulate=*/true);
      }
    }
    return not_impl(op, args, self);
  }

  return not_impl(op, args, self);
}

Series& invoke_any_(std::string_view op, Series& self, const Args& args) {
  op = canonical_inplace_op(op);

  if (args.empty()) {
    if (op == "relu_") return ops_internal::relu_(self);
    if (op == "sigmoid_") return ops_internal::sigmoid_(self);
    if (op == "softplus_") return ops_internal::softplus_(self, 1.0, 20.0);
    if (op == "silu_") return ops_internal::silu_(self);
    if (op == "gelu_") return ops_internal::gelu_(self);

    if (op == "exp_") return ops_internal::exp_(self);
    if (op == "log_") return ops_internal::log_(self);
    if (op == "sqrt_") return ops_internal::sqrt_(self);
    if (op == "rsqrt_") { self = ops_internal::rsqrt(self); return self; }
    if (op == "log2_") { self = ops_internal::log2(self); return self; }
    if (op == "log10_") { self = ops_internal::log10(self); return self; }
    if (op == "exp2_") { self = ops_internal::exp2(self); return self; }
    if (op == "square_") { self = ops_internal::square(self); return self; }

    if (op == "sin_") return self.sin_();
    if (op == "cos_") return self.cos_();
    if (op == "tanh_") return self.tanh_();
    if (op == "tan_") return self.tan_();
    if (op == "asin_") return self.asin_();
    if (op == "acos_") return self.acos_();
    if (op == "atan_") return self.atan_();
    if (op == "sinh_") return self.sinh_();
    if (op == "cosh_") return self.cosh_();

    if (op == "floor_") return self.floor_();
    if (op == "ceil_") return self.ceil_();
    if (op == "round_") return self.round_();
    if (op == "trunc_") return self.trunc_();
    if (op == "reciprocal_") return self.reciprocal_();
    if (op == "logical_not_") return self.logical_not_();
    if (op == "log1p_") return self.log1p_();
    if (op == "expm1_") return self.expm1_();
    if (op == "erf_") return self.erf_();
    if (op == "erfc_") return self.erfc_();
    if (op == "lgamma_") return self.lgamma_();
    if (op == "asinh_") return self.asinh_();
    if (op == "acosh_") return self.acosh_();
    if (op == "atanh_") return self.atanh_();

    if (op == "conj_physical_") { self = ops_internal::conj(self); return self; }
    if (op == "neg_") { self = ops_internal::neg(self); return self; }
    if (op == "sign_" || op == "sgn_") {
      Series pos = ops_internal::gt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      Series neg = ops_internal::lt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      self = ops_internal::sub(pos, neg).to(self.scalar_type());
      return self;
    }
    if (op == "fix_") { self = ops_internal::trunc(self); return self; }
    if (op == "frac_") { self = ops_internal::sub(self, ops_internal::floor(self)); return self; }
    if (op == "deg2rad_") { self = mul_scalar(self, kPi / 180.0); return self; }
    if (op == "rad2deg_") { self = mul_scalar(self, 180.0 / kPi); return self; }
    if (op == "positive_") return self;
    if (op == "isnan_") return self.isnan_();
    if (op == "isinf_") return self.isinf_();
    if (op == "isfinite_") return self.isfinite_();
    if (op == "detach_") return self;
    if (op == "abs_") { self = ops_internal::abs(self); return self; }
    if (op == "bitwise_not_") return ops_internal::bitwise_not_(self);
    if (op == "squeeze_") { self = ops_internal::squeeze(self); return self; }
    if (op == "t_") {
      if (self.dim() == 2) self = self.transpose(0, 1);
      return self;
    }
    if (op == "zero_") { self = Series::zeros(self.sizes(), self.scalar_type()); return self; }

    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 1) {
    const auto& a0 = args[0].v;
    if (is_numeric_scalar_arg(args[0])) {
      const double s = as_double(args[0]);
      if (op == "add_") { self = add_scalar(self, s); return self; }
      if (op == "sub_") { self = sub_scalar(self, s); return self; }
      if (op == "mul_") { self = mul_scalar(self, s); return self; }
      if (op == "div_") { self = div_scalar(self, s); return self; }
      if (op == "fill_") { self = Series::full(self.sizes(), s, self.scalar_type()); return self; }
      if (op == "pow_" || op == "float_power_") { self = pow_scalar(self, s); return self; }
      if (op == "floor_divide_") { self = ops_internal::floor(div_scalar(self, s)); return self; }
      if (op == "fmod_") { self = ops_internal::fmod(self, scalar_like(self, s)); return self; }
      if (op == "remainder_") { self = ops_internal::remainder(self, scalar_like(self, s)); return self; }
      if (op == "atan2_") { self = ops_internal::atan2(self, scalar_like(self, s)); return self; }
      if (op == "minimum_") { self = ops_internal::minimum(self, scalar_like(self, s)); return self; }
      if (op == "maximum_") { self = ops_internal::maximum(self, scalar_like(self, s)); return self; }
      if (op == "eq_") { self = ops_internal::eq(self, scalar_like(self, s)); return self; }
      if (op == "ne_") { self = ops_internal::ne(self, scalar_like(self, s)); return self; }
      if (op == "lt_") { self = ops_internal::lt(self, scalar_like(self, s)); return self; }
      if (op == "le_") { self = ops_internal::le(self, scalar_like(self, s)); return self; }
      if (op == "gt_") { self = ops_internal::gt(self, scalar_like(self, s)); return self; }
      if (op == "ge_") { self = ops_internal::ge(self, scalar_like(self, s)); return self; }
      if (op == "copysign_") {
        const double sign = std::signbit(s) ? -1.0 : 1.0;
        self = mul_scalar(ops_internal::abs(self), sign);
        return self;
      }
      if (op == "logaddexp_") {
        self = ops_internal::log(ops_internal::add(ops_internal::exp(self), ops_internal::exp(scalar_like(self, s))));
        return self;
      }
      if (op == "logaddexp2_") {
        self = ops_internal::log2(ops_internal::add(ops_internal::exp2(self), ops_internal::exp2(scalar_like(self, s))));
        return self;
      }
      if (op == "hypot_") {
        self = ops_internal::sqrt(ops_internal::add(ops_internal::square(self), ops_internal::square(scalar_like(self, s))));
        return self;
      }
      if (op == "xlogy_") { self = ops_internal::mul(self, ops_internal::log(scalar_like(self, s))); return self; }
      if (op == "leaky_relu_") return ops_internal::leaky_relu_(self, s);
      if (op == "clamp_min_") return ops_internal::clamp_min_(self, s);
      if (op == "clamp_max_") return ops_internal::clamp_max_(self, s);
      if (op == "squeeze_") { self = ops_internal::squeeze(self, (std::int64_t)s); return self; }
      if (op == "unsqueeze_") { self = self.unsqueeze((std::int64_t)s); return self; }
      if (op == "cumsum_") return self.cumsum_((std::int64_t)s, /*keepdim=*/false);
      if (op == "cumprod_") return self.cumprod_((std::int64_t)s, /*keepdim=*/false);
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, s, 0.0, 0.0);
      return not_impl_inplace(op, args, self);
    }

    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "add_") { self = ops_internal::add(self, other); return self; }
      if (op == "sub_") { self = ops_internal::sub(self, other); return self; }
      if (op == "mul_") { self = ops_internal::mul(self, other); return self; }
      if (op == "div_") { self = ops_internal::div(self, other); return self; }
      if (op == "pow_" || op == "float_power_") { self = ops_internal::pow(self, other); return self; }
      if (op == "floor_divide_") { self = ops_internal::floor(ops_internal::div(self, other)); return self; }
      if (op == "fmod_") { self = ops_internal::fmod(self, other); return self; }
      if (op == "remainder_") { self = ops_internal::remainder(self, other); return self; }
      if (op == "atan2_") { self = ops_internal::atan2(self, other); return self; }
      if (op == "minimum_") { self = ops_internal::minimum(self, other); return self; }
      if (op == "maximum_") { self = ops_internal::maximum(self, other); return self; }
      if (op == "eq_") { self = ops_internal::eq(self, other); return self; }
      if (op == "ne_") { self = ops_internal::ne(self, other); return self; }
      if (op == "lt_") { self = ops_internal::lt(self, other); return self; }
      if (op == "le_") { self = ops_internal::le(self, other); return self; }
      if (op == "gt_") { self = ops_internal::gt(self, other); return self; }
      if (op == "ge_") { self = ops_internal::ge(self, other); return self; }
      if (op == "logical_and_") { self = ops_internal::logical_and(self, other); return self; }
      if (op == "logical_or_") { self = ops_internal::logical_or(self, other); return self; }
      if (op == "logical_xor_") { self = ops_internal::logical_xor(self, other); return self; }
      if (op == "bitwise_and_") return ops_internal::bitwise_and_(self, other);
      if (op == "bitwise_or_") return ops_internal::bitwise_or_(self, other);
      if (op == "bitwise_xor_") return ops_internal::bitwise_xor_(self, other);
      if (op == "fmax_") { self = ops_internal::maximum(self, other); return self; }
      if (op == "fmin_") { self = ops_internal::minimum(self, other); return self; }
      if (op == "copysign_") {
        Series mag = ops_internal::abs(self);
        Series pos = ops_internal::ge(other, scalar_like(other, 0.0));
        self = ops_internal::where(pos, mag, ops_internal::neg(mag));
        return self;
      }
      if (op == "logaddexp_") { self = ops_internal::log(ops_internal::add(ops_internal::exp(self), ops_internal::exp(other))); return self; }
      if (op == "logaddexp2_") { self = ops_internal::log2(ops_internal::add(ops_internal::exp2(self), ops_internal::exp2(other))); return self; }
      if (op == "hypot_") { self = ops_internal::sqrt(ops_internal::add(ops_internal::square(self), ops_internal::square(other))); return self; }
      if (op == "xlogy_") { self = ops_internal::mul(self, ops_internal::log(other)); return self; }
      if (op == "type_as_") { self = self.to(other.scalar_type()); return self; }
      if (op == "copy_") {
        self = (other.scalar_type() == self.scalar_type()) ? other : other.to(self.scalar_type());
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 2) {
    const auto& a0 = args[0].v;
    const auto& a1 = args[1].v;
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1])) {
      const double x = as_double(args[0]);
      const double y = as_double(args[1]);
      if (op == "clamp_") return ops_internal::clamp_(self, x, y);
      if (op == "softplus_") return ops_internal::softplus_(self, x, y);
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, x, y, y);
      if (op == "transpose_" || op == "movedim_" || op == "moveaxis_" || op == "swapdims_" || op == "swapaxes_") {
        self = self.transpose((std::int64_t)x, (std::int64_t)y);
        return self;
      }
      if (op == "lerp_") {
        self = ops_internal::add(self, mul_scalar(ops_internal::sub(scalar_like(self, x), self), y));
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<bool>(a1)) {
      const auto dim = as_i64(args[0]);
      const bool keepdim = as_bool(args[1]);
      if (op == "cumsum_") return self.cumsum_(dim, keepdim);
      if (op == "cumprod_") return self.cumprod_(dim, keepdim);
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && is_numeric_scalar_arg(args[1])) {
      const Series& other = as_series(args[0]);
      const double s = as_double(args[1]);
      if (op == "lerp_") {
        self = ops_internal::add(self, mul_scalar(ops_internal::sub(other, self), s));
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<bool>(a1)) {
      const Series& other = as_series(args[0]);
      if (op == "copy_") {
        // CPU path ignores non_blocking flag; keep dtype behavior aligned with copy_(Tensor).
        self = (other.scalar_type() == self.scalar_type()) ? other : other.to(self.scalar_type());
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      if (op == "addmm_") { self = combine_beta_alpha(self, native::cpu::matmul(b, c), 1.0, 1.0); return self; }
      if (op == "addmv_") { self = combine_beta_alpha(self, native::cpu::matmul(b, c), 1.0, 1.0); return self; }
      if (op == "addr_") { self = combine_beta_alpha(self, native::cpu::matmul(b.unsqueeze(1), c.unsqueeze(0)), 1.0, 1.0); return self; }
      if (op == "addbmm_") {
        self = combine_beta_alpha(self, native::cpu::matmul(b, c).sum(/*dim=*/0, /*keepdim=*/false), 1.0, 1.0);
        return self;
      }
      if (op == "baddbmm_") { self = combine_beta_alpha(self, native::cpu::matmul(b, c), 1.0, 1.0); return self; }
      if (op == "masked_scatter_") return ops_internal::masked_scatter_(self, b, c);
      if (op == "index_put_") { self = native::cpu::put(self, b, c, /*accumulate=*/false); return self; }
      if (op == "put_") { self = native::cpu::put(self, b, c, /*accumulate=*/false); return self; }
      if (op == "lerp_") { self = ops_internal::add(self, ops_internal::mul(ops_internal::sub(b, self), c)); return self; }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 3) {
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1]) && is_numeric_scalar_arg(args[2])) {
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, as_double(args[0]), as_double(args[1]), as_double(args[2]));
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const Series& index = as_series(args[1]);
      const Series& src = as_series(args[2]);
      if (op == "scatter_") { self = self.scatter(dim, index, src); return self; }
      if (op == "scatter_add_") return ops_internal::scatter_add_(self, dim, index, src);
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const Series& index = as_series(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put_") { self = native::cpu::put(self, index, src, accumulate); return self; }
      if (op == "put_") { self = native::cpu::put(self, index, src, accumulate); return self; }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double v = as_double(args[2]);
      if (op == "addcmul_") { self = ops_internal::add(self, mul_scalar(ops_internal::mul(b, c), v)); return self; }
      if (op == "addcdiv_") { self = ops_internal::add(self, mul_scalar(ops_internal::div(b, c), v)); return self; }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 4) {
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double beta = as_double(args[2]);
      const double alpha = as_double(args[3]);
      if (op == "addmm_") { self = combine_beta_alpha(self, native::cpu::matmul(b, c), beta, alpha); return self; }
      if (op == "addmv_") { self = combine_beta_alpha(self, native::cpu::matmul(b, c), beta, alpha); return self; }
      if (op == "addr_") { self = combine_beta_alpha(self, native::cpu::matmul(b.unsqueeze(1), c.unsqueeze(0)), beta, alpha); return self; }
      if (op == "addbmm_") {
        self = combine_beta_alpha(self, native::cpu::matmul(b, c).sum(/*dim=*/0, /*keepdim=*/false), beta, alpha);
        return self;
      }
      if (op == "baddbmm_") { self = combine_beta_alpha(self, native::cpu::matmul(b, c), beta, alpha); return self; }
    }
    return not_impl_inplace(op, args, self);
  }

  return not_impl_inplace(op, args, self);
}

} // namespace pluss::codem::_compat
