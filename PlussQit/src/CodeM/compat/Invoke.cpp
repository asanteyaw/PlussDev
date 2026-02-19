#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Scalar.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <type_traits>

#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Errors.h"
#include "CodeM/native/cpu/ConcatStackKernels.h"
#include "CodeM/native/cpu/GatherScatterKernels.h"
#include "CodeM/native/cpu/IndexingExtras.h"
#include "CodeM/native/cpu/SignalDistanceKernels.h"
#include "CodeM/native/cpu/SortingKernels.h"

#include <sstream>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <numeric>
#include <limits>
#include <optional>
#include <complex>
#include <random>

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

static Series not_impl(std::string_view op, const Args& args, const Series& self) {
  warn(std::string("Unrouted op fallback: ") + sig(op, args));
  return self;
}

static Series& not_impl_inplace(std::string_view op, const Args& args, Series& self) {
  warn(std::string("Unrouted in-place op fallback: ") + sig(op, args));
  return self;
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

static std::int64_t split_family_dim(const Series& self, std::string_view op_name) {
  if (op_name == "vsplit") {
    PLUSS_CHECK(self.dim() >= 2, "vsplit: expected input with rank >= 2");
    return 0;
  }
  if (op_name == "hsplit") {
    PLUSS_CHECK(self.dim() >= 1, "hsplit: expected input with rank >= 1");
    return self.dim() == 1 ? 0 : 1;
  }
  if (op_name == "dsplit") {
    PLUSS_CHECK(self.dim() >= 3, "dsplit: expected input with rank >= 3");
    return 2;
  }
  PLUSS_CHECK(false, "internal error: unsupported split family op");
  return 0;
}

static Series split_family_first(const Series& self, std::string_view op_name, const Arg& sections_or_indices) {
  const auto dim = split_family_dim(self, op_name);
  return tensor_split_first(self, sections_or_indices, dim);
}

static Series split_first_with_sizes(const Series& self, std::span<const std::int64_t> split_sizes, std::int64_t dim, const char* op_name) {
  PLUSS_CHECK(!split_sizes.empty(), std::string(op_name) + ": expected non-empty split sizes");
  for (const auto v : split_sizes) {
    PLUSS_CHECK(v >= 0, std::string(op_name) + ": split sizes must be >= 0");
  }
  dim = wrap_dim(dim, self.dim(), op_name);
  const auto dsz = self.sizes()[(std::size_t)dim];
  return select_range(self, dim, 0, std::min<std::int64_t>(split_sizes[0], dsz));
}

static Series flip_impl(const Series& self, std::span<const std::int64_t> dims, const char* op_name) {
  PLUSS_CHECK(!dims.empty(), std::string(op_name) + ": expected at least one dimension");
  Series out = self;
  std::vector<std::int64_t> normalized;
  normalized.reserve(dims.size());
  for (const auto d : dims) {
    const auto wd = wrap_dim(d, out.dim(), op_name);
    if (std::find(normalized.begin(), normalized.end(), wd) == normalized.end()) {
      normalized.push_back(wd);
    }
  }
  for (const auto dim : normalized) {
    const auto n = out.sizes()[(std::size_t)dim];
    std::vector<std::int64_t> idx((std::size_t)n);
    for (std::int64_t i = 0; i < n; ++i) {
      idx[(std::size_t)i] = n - 1 - i;
    }
    out = native::cpu::index_select(out, dim, i64_vector(std::span<const std::int64_t>(idx.data(), idx.size())));
  }
  return out;
}

static Series diff_impl(const Series& self, std::int64_t n, std::int64_t dim) {
  PLUSS_CHECK(n >= 0, "diff: expected n >= 0");
  if (n == 0) return self.clone();
  dim = wrap_dim(dim, self.dim(), "diff");
  Series out = self;
  for (std::int64_t i = 0; i < n; ++i) {
    const auto dsz = out.sizes()[(std::size_t)dim];
    if (dsz <= 1) {
      auto out_sizes = std::vector<std::int64_t>(out.sizes().begin(), out.sizes().end());
      out_sizes[(std::size_t)dim] = 0;
      return Series::empty(out_sizes, out.scalar_type());
    }
    const Series hi = select_range(out, dim, 1, dsz - 1);
    const Series lo = select_range(out, dim, 0, dsz - 1);
    out = ops_internal::sub(hi, lo);
  }
  return out;
}

static Series repeat_impl(const Series& self, std::span<const std::int64_t> repeats) {
  PLUSS_CHECK(!repeats.empty(), "repeat: expected non-empty repeats");
  for (const auto r : repeats) PLUSS_CHECK(r >= 0, "repeat: repeats must be >= 0");

  std::vector<std::int64_t> base_sizes(self.sizes().begin(), self.sizes().end());
  std::vector<std::int64_t> reps(repeats.begin(), repeats.end());
  Series out = self;

  if (reps.size() < base_sizes.size()) {
    reps.insert(reps.begin(), base_sizes.size() - reps.size(), 1);
  } else if (reps.size() > base_sizes.size()) {
    std::vector<std::int64_t> new_sizes(reps.size() - base_sizes.size(), 1);
    new_sizes.insert(new_sizes.end(), base_sizes.begin(), base_sizes.end());
    out = self.reshape(std::span<const std::int64_t>(new_sizes.data(), new_sizes.size()));
    base_sizes = std::move(new_sizes);
  }

  for (std::size_t d = 0; d < reps.size(); ++d) {
    const auto rep = reps[d];
    const auto sz = out.sizes()[d];
    if (rep == 0 || sz == 0) {
      std::vector<std::int64_t> out_sizes(out.sizes().begin(), out.sizes().end());
      out_sizes[d] = 0;
      return Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), out.scalar_type());
    }
    if (rep == 1) continue;
    std::vector<std::int64_t> idx((std::size_t)(sz * rep));
    for (std::int64_t i = 0; i < sz * rep; ++i) idx[(std::size_t)i] = i % sz;
    out = native::cpu::index_select(out, static_cast<std::int64_t>(d), i64_vector(std::span<const std::int64_t>(idx.data(), idx.size())));
  }
  return out;
}

static Series expand_impl(const Series& self, std::span<const std::int64_t> target_sizes) {
  PLUSS_CHECK(!target_sizes.empty() || self.dim() == 0, "expand: invalid target shape");

  std::vector<std::int64_t> target(target_sizes.begin(), target_sizes.end());
  std::vector<std::int64_t> base_sizes(self.sizes().begin(), self.sizes().end());
  Series out = self;

  if (base_sizes.size() > target.size()) {
    PLUSS_CHECK(false, "expand: cannot expand to fewer dimensions");
  }
  if (base_sizes.size() < target.size()) {
    std::vector<std::int64_t> padded(target.size() - base_sizes.size(), 1);
    padded.insert(padded.end(), base_sizes.begin(), base_sizes.end());
    out = self.reshape(std::span<const std::int64_t>(padded.data(), padded.size()));
    base_sizes = std::move(padded);
  }

  for (std::size_t d = 0; d < target.size(); ++d) {
    const auto src = out.sizes()[d];
    auto dst = target[d];
    if (dst == -1) dst = src;
    PLUSS_CHECK(dst >= 0, "expand: target size must be >= 0 or -1");
    if (src == dst) continue;
    PLUSS_CHECK(src == 1, "expand: non-singleton dimension mismatch");
    std::vector<std::int64_t> idx((std::size_t)dst, 0);
    out = native::cpu::index_select(out, static_cast<std::int64_t>(d),
                                    i64_vector(std::span<const std::int64_t>(idx.data(), idx.size())));
  }
  return out;
}

static Series repeat_interleave_impl(const Series& self, std::int64_t repeats, std::int64_t dim) {
  PLUSS_CHECK(repeats >= 0, "repeat_interleave: repeats must be >= 0");
  dim = wrap_dim(dim, self.dim(), "repeat_interleave");
  const auto sz = self.sizes()[(std::size_t)dim];
  if (repeats == 0 || sz == 0) {
    std::vector<std::int64_t> out_sizes(self.sizes().begin(), self.sizes().end());
    out_sizes[(std::size_t)dim] = 0;
    return Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), self.scalar_type());
  }
  std::vector<std::int64_t> idx((std::size_t)(sz * repeats));
  for (std::int64_t i = 0; i < sz; ++i) {
    for (std::int64_t r = 0; r < repeats; ++r) idx[(std::size_t)(i * repeats + r)] = i;
  }
  return native::cpu::index_select(self, dim, i64_vector(std::span<const std::int64_t>(idx.data(), idx.size())));
}

static Series roll_impl(const Series& self, std::span<const std::int64_t> shifts, std::span<const std::int64_t> dims) {
  PLUSS_CHECK(!shifts.empty(), "roll: shifts must be non-empty");
  PLUSS_CHECK(!dims.empty(), "roll: dims must be non-empty");
  PLUSS_CHECK(shifts.size() == dims.size() || shifts.size() == 1,
              "roll: shifts and dims must have same length, or shifts length 1");
  Series out = self;
  for (std::size_t i = 0; i < dims.size(); ++i) {
    const auto dim = wrap_dim(dims[i], out.dim(), "roll");
    const auto n = out.sizes()[(std::size_t)dim];
    if (n <= 1) continue;
    std::int64_t shift = shifts[shifts.size() == 1 ? 0 : i] % n;
    if (shift < 0) shift += n;
    if (shift == 0) continue;
    std::vector<std::int64_t> idx((std::size_t)n);
    for (std::int64_t j = 0; j < n; ++j) idx[(std::size_t)j] = (j - shift + n) % n;
    out = native::cpu::index_select(out, dim, i64_vector(std::span<const std::int64_t>(idx.data(), idx.size())));
  }
  return out;
}

static Series roll_flat_impl(const Series& self, std::int64_t shift) {
  auto flat = self.flatten(0, -1);
  const std::array<std::int64_t, 1> shifts{{shift}};
  const std::array<std::int64_t, 1> dims{{0}};
  return roll_impl(flat,
                   std::span<const std::int64_t>(shifts.data(), shifts.size()),
                   std::span<const std::int64_t>(dims.data(), dims.size()))
      .reshape(self.sizes());
}

static void write_scalar_into_ptr(std::byte* dst, ScalarType st, double value) {
  switch (st) {
    case ScalarType::Bool: *reinterpret_cast<std::uint8_t*>(dst) = value != 0.0 ? 1 : 0; return;
    case ScalarType::UInt8: *reinterpret_cast<std::uint8_t*>(dst) = static_cast<std::uint8_t>(value); return;
    case ScalarType::UInt16: *reinterpret_cast<std::uint16_t*>(dst) = static_cast<std::uint16_t>(value); return;
    case ScalarType::UInt32: *reinterpret_cast<std::uint32_t*>(dst) = static_cast<std::uint32_t>(value); return;
    case ScalarType::UInt64: *reinterpret_cast<std::uint64_t*>(dst) = static_cast<std::uint64_t>(value); return;
    case ScalarType::Int8: *reinterpret_cast<std::int8_t*>(dst) = static_cast<std::int8_t>(value); return;
    case ScalarType::Int16: *reinterpret_cast<std::int16_t*>(dst) = static_cast<std::int16_t>(value); return;
    case ScalarType::Int32: *reinterpret_cast<std::int32_t*>(dst) = static_cast<std::int32_t>(value); return;
    case ScalarType::Int64: *reinterpret_cast<std::int64_t*>(dst) = static_cast<std::int64_t>(value); return;
    case ScalarType::Float16: *reinterpret_cast<Float16*>(dst) = float16_from_float(static_cast<float>(value)); return;
    case ScalarType::BFloat16: *reinterpret_cast<BFloat16*>(dst) = bfloat16_from_float(static_cast<float>(value)); return;
    case ScalarType::Float32: *reinterpret_cast<float*>(dst) = static_cast<float>(value); return;
    case ScalarType::Float64: *reinterpret_cast<double*>(dst) = value; return;
    case ScalarType::Complex64: *reinterpret_cast<Complex64*>(dst) = Complex64(static_cast<float>(value), 0.0f); return;
    case ScalarType::Complex128: *reinterpret_cast<Complex128*>(dst) = Complex128(value, 0.0); return;
    default: break;
  }
  PLUSS_CHECK(false, "fill_diagonal_: dtype not implemented");
}

static Series& fill_diagonal_impl(Series& self, double value, bool /*wrap*/) {
  PLUSS_CHECK(self.dim() == 2, "fill_diagonal_: expected a 2D tensor");
  auto x = self.contiguous();
  const auto rows = x.sizes()[0];
  const auto cols = x.sizes()[1];
  const auto n = std::min(rows, cols);
  auto* base = reinterpret_cast<std::byte*>(x.raw_mutable_data());
  const auto item = x.dtype().itemsize;
  for (std::int64_t i = 0; i < n; ++i) {
    const auto off = (i * cols + i) * item;
    write_scalar_into_ptr(base + off, x.scalar_type(), value);
  }
  self = x;
  return self;
}

static Series bernoulli_sample_from_probs(const Series& probs) {
  auto p = ops_internal::clamp(probs.to(ScalarType::Float64), 0.0, 1.0);
  auto u = Series::rand(p.sizes(), ScalarType::Float64, /*seed=*/0);
  return ops_internal::lt(u, p);
}

static Series uniform_like(const Series& self, double from, double to) {
  PLUSS_CHECK(to > from, "uniform_: expected to > from");
  Series x = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  x = add_scalar(mul_scalar(x, to - from), from);
  return x.to(self.scalar_type());
}

static Series normal_like(const Series& self, double mean, double std) {
  PLUSS_CHECK(std >= 0.0, "normal_: expected std >= 0");
  Series x = Series::randn(self.sizes(), ScalarType::Float64, /*seed=*/0);
  x = add_scalar(mul_scalar(x, std), mean);
  return x.to(self.scalar_type());
}

static Series exponential_like(const Series& self, double lambda) {
  PLUSS_CHECK(lambda > 0.0, "exponential_: expected lambda > 0");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  u = ops_internal::clamp_min(u, 1e-12);
  Series x = mul_scalar(ops_internal::log(u), -1.0 / lambda);
  return x.to(self.scalar_type());
}

static Series geometric_like(const Series& self, double p) {
  PLUSS_CHECK(p > 0.0 && p <= 1.0, "geometric_: expected 0 < p <= 1");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  Series one_minus_u = ops_internal::sub(scalar_like(u, 1.0), u);
  const double denom = std::log(1.0 - p);
  Series x = ops_internal::floor(div_scalar(ops_internal::log(one_minus_u), denom));
  x = add_scalar(x, 1.0);
  return x.to(self.scalar_type());
}

static Series cauchy_like(const Series& self, double median, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "cauchy_: expected sigma > 0");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  Series centered = sub_scalar(u, 0.5);
  Series t = ops_internal::tan(mul_scalar(centered, kPi));
  Series x = add_scalar(mul_scalar(t, sigma), median);
  return x.to(self.scalar_type());
}

static Series bernoulli_like(const Series& self, double p) {
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "bernoulli_: expected 0 <= p <= 1");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  return ops_internal::lt(u, scalar_like(u, p)).to(self.scalar_type());
}

static Series sort_values_only(const Series& self, std::int64_t dim, bool descending, bool stable);

static Series nan_sum_impl(const Series& self) {
  return ops_internal::sum(ops_internal::nan_to_num(self, 0.0, 0.0, 0.0));
}

static Series nan_sum_impl(const Series& self, std::int64_t dim, bool keepdim) {
  return ops_internal::sum(ops_internal::nan_to_num(self, 0.0, 0.0, 0.0), dim, keepdim);
}

static Series nan_mean_impl(const Series& self) {
  Series s = ops_internal::sum(ops_internal::nan_to_num(self, 0.0, 0.0, 0.0).to(ScalarType::Float64));
  Series c = ops_internal::sum(ops_internal::logical_not(ops_internal::isnan(self)).to(ScalarType::Float64));
  return ops_internal::div(s, c);
}

static Series nan_mean_impl(const Series& self, std::int64_t dim, bool keepdim) {
  Series s = ops_internal::sum(ops_internal::nan_to_num(self, 0.0, 0.0, 0.0).to(ScalarType::Float64), dim, keepdim);
  Series c = ops_internal::sum(ops_internal::logical_not(ops_internal::isnan(self)).to(ScalarType::Float64), dim, keepdim);
  return ops_internal::div(s, c);
}

static Series nan_var_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt, bool keepdim = false) {
  auto x = self.to(ScalarType::Float64).contiguous();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  if (!dim.has_value()) {
    double sum = 0.0;
    std::int64_t cnt = 0;
    for (std::int64_t i = 0; i < x.numel(); ++i) {
      const double v = ip[i];
      if (std::isnan(v)) continue;
      sum += v;
      ++cnt;
    }
    auto out = Series::empty({}, ScalarType::Float64);
    auto* op = reinterpret_cast<double*>(out.raw_data());
    if (cnt == 0) { op[0] = std::numeric_limits<double>::quiet_NaN(); return out; }
    const double mean = sum / static_cast<double>(cnt);
    double var = 0.0;
    for (std::int64_t i = 0; i < x.numel(); ++i) {
      const double v = ip[i];
      if (std::isnan(v)) continue;
      const double dv = v - mean;
      var += dv * dv;
    }
    op[0] = var / static_cast<double>(cnt);
    return out;
  }

  const auto d = wrap_dim(*dim, x.dim(), "nanvar");
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)d];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t k = 0; k < d; ++k) outer *= x.sizes()[(std::size_t)k];
  for (std::int64_t k = d + 1; k < nd; ++k) inner *= x.sizes()[(std::size_t)k];
  std::vector<std::int64_t> out_sizes(x.sizes().begin(), x.sizes().end());
  if (keepdim) out_sizes[(std::size_t)d] = 1;
  else out_sizes.erase(out_sizes.begin() + d);
  auto out = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), ScalarType::Float64);
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      double sum = 0.0;
      std::int64_t cnt = 0;
      for (std::int64_t t = 0; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        const double v = ip[off];
        if (std::isnan(v)) continue;
        sum += v;
        ++cnt;
      }
      const auto dst = o * inner + i;
      if (cnt == 0) { op[dst] = std::numeric_limits<double>::quiet_NaN(); continue; }
      const double mean = sum / static_cast<double>(cnt);
      double var = 0.0;
      for (std::int64_t t = 0; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        const double v = ip[off];
        if (std::isnan(v)) continue;
        const double dv = v - mean;
        var += dv * dv;
      }
      op[dst] = var / static_cast<double>(cnt);
    }
  }
  return out;
}

static Series nan_std_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt, bool keepdim = false) {
  return ops_internal::sqrt(nan_var_impl(self, dim, keepdim));
}

template <class Fn>
static Series unary_f64_impl(const Series& self, Fn&& fn) {
  auto x = self.to(ScalarType::Float64).contiguous();
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = fn(ip[i]);
  return out;
}

static double erfinv_approx(double x) {
  constexpr double a = 0.147;
  const double xc = std::clamp(x, -0.999999999, 0.999999999);
  const double ln = std::log(1.0 - xc * xc);
  const double tt = 2.0 / (kPi * a) + ln / 2.0;
  const double inside = tt * tt - ln / a;
  return std::copysign(std::sqrt(std::sqrt(std::max(inside, 0.0)) - tt), xc);
}

static double erfcinv_approx(double y) {
  if (y <= 0.0) return std::numeric_limits<double>::infinity();
  if (y >= 2.0) return -std::numeric_limits<double>::infinity();
  return erfinv_approx(1.0 - y);
}

static Series erfcinv_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) { return erfcinv_approx(x); });
}

static Series betaln_impl(const Series& a, const Series& b) {
  auto aa = a.to(ScalarType::Float64);
  auto bb = b.to(ScalarType::Float64);
  return ops_internal::sub(
      ops_internal::add(ops_internal::lgamma(aa), ops_internal::lgamma(bb)),
      ops_internal::lgamma(ops_internal::add(aa, bb)));
}

static Series beta_impl(const Series& a, const Series& b) {
  return ops_internal::exp(betaln_impl(a, b));
}

static Series zeta_impl(const Series& s, const Series& q) {
  auto ss = s.to(ScalarType::Float64);
  auto qq = q.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(ss.sizes(), qq.sizes());
  auto sse = expand_impl(ss, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto qqe = expand_impl(qq, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* sp = reinterpret_cast<const double*>(sse.raw_data());
  const auto* qp = reinterpret_cast<const double*>(qqe.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  constexpr std::int64_t kTerms = 4096;
  for (std::int64_t i = 0; i < n; ++i) {
    const double sv = sp[i];
    const double qv = qp[i];
    if (!(sv > 1.0) || !(qv > 0.0)) {
      op[i] = std::numeric_limits<double>::quiet_NaN();
      continue;
    }
    double acc = 0.0;
    for (std::int64_t k = 0; k < kTerms; ++k) acc += std::pow(qv + static_cast<double>(k), -sv);
    op[i] = acc;
  }
  return out;
}

static Series airy_impl(const Series& self) {
  constexpr double ai0 = 0.35502805388781723926;
  return unary_f64_impl(self, [](double x) {
    if (x == 0.0) return ai0;
    if (x > 0.0) {
      const double t = 2.0 * std::pow(x, 1.5) / 3.0;
      return 0.5 * std::exp(-t) / std::sqrt(kPi) / std::pow(x, 0.25);
    }
    const double ax = -x;
    const double t = 2.0 * std::pow(ax, 1.5) / 3.0;
    return std::sin(t + kPi / 4.0) / std::sqrt(kPi) / std::pow(ax, 0.25);
  });
}

static double bessel_j_series(double nu, double x) {
  double acc = 0.0;
  double half = x * 0.5;
  for (int m = 0; m < 24; ++m) {
    const double sign = (m % 2 == 0) ? 1.0 : -1.0;
    const double den = std::tgamma(m + 1.0) * std::tgamma(m + nu + 1.0);
    const double term = sign * std::pow(half, 2.0 * m + nu) / den;
    acc += term;
    if (std::abs(term) < 1e-16) break;
  }
  return acc;
}

static double bessel_i_series(double nu, double x) {
  double acc = 0.0;
  double half = x * 0.5;
  for (int m = 0; m < 24; ++m) {
    const double den = std::tgamma(m + 1.0) * std::tgamma(m + nu + 1.0);
    const double term = std::pow(half, 2.0 * m + nu) / den;
    acc += term;
    if (std::abs(term) < 1e-16) break;
  }
  return acc;
}

static Series bessel_impl(const Series& self, double order, std::string_view kind) {
  const double nu = (std::abs(std::sin(kPi * order)) < 1e-8) ? (order + 1e-6) : order;
  if (kind == "j") {
    return unary_f64_impl(self, [nu](double x) { return bessel_j_series(nu, x); });
  }
  if (kind == "y") {
    return unary_f64_impl(self, [nu](double x) {
      const double s = std::sin(kPi * nu);
      return (bessel_j_series(nu, x) * std::cos(kPi * nu) - bessel_j_series(-nu, x)) / s;
    });
  }
  if (kind == "i") {
    return unary_f64_impl(self, [nu](double x) { return bessel_i_series(nu, x); });
  }
  if (kind == "k") {
    return unary_f64_impl(self, [nu](double x) {
      const double s = std::sin(kPi * nu);
      return (kPi * 0.5) * (bessel_i_series(-nu, x) - bessel_i_series(nu, x)) / s;
    });
  }
  PLUSS_CHECK(false, "bessel: unsupported kind");
  return Series::undefined();
}

static Series pdf_impl(const Series& self, double mean, double std) {
  PLUSS_CHECK(std > 0.0, "pdf: std must be > 0");
  const double inv = 1.0 / (std * std::sqrt(2.0 * kPi));
  return unary_f64_impl(self, [mean, std, inv](double x) {
    const double z = (x - mean) / std;
    return inv * std::exp(-0.5 * z * z);
  });
}

static Series cdf_impl(const Series& self, double mean, double std) {
  PLUSS_CHECK(std > 0.0, "cdf: std must be > 0");
  const double den = std * std::sqrt(2.0);
  return unary_f64_impl(self, [mean, den](double x) {
    return 0.5 * (1.0 + std::erf((x - mean) / den));
  });
}

enum class MovKind { Sum, Mean, Max, Min, Var, Std };

static Series mov_impl(const Series& self, std::int64_t window, std::int64_t dim, MovKind kind) {
  PLUSS_CHECK(window > 0, "mov*: window must be > 0");
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), "mov*");
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      for (std::int64_t t = 0; t < n; ++t) {
        const auto start = std::max<std::int64_t>(0, t - window + 1);
        const auto len = t - start + 1;
        double acc = 0.0;
        double mn = std::numeric_limits<double>::infinity();
        double mx = -std::numeric_limits<double>::infinity();
        for (std::int64_t k = start; k <= t; ++k) {
          const auto off = (o * n + k) * inner + i;
          const double v = ip[off];
          acc += v;
          mn = std::min(mn, v);
          mx = std::max(mx, v);
        }
        const auto dst = (o * n + t) * inner + i;
        switch (kind) {
          case MovKind::Sum: op[dst] = acc; break;
          case MovKind::Mean: op[dst] = acc / static_cast<double>(len); break;
          case MovKind::Max: op[dst] = mx; break;
          case MovKind::Min: op[dst] = mn; break;
          case MovKind::Var: {
            const double mean = acc / static_cast<double>(len);
            double var = 0.0;
            for (std::int64_t k = start; k <= t; ++k) {
              const auto off = (o * n + k) * inner + i;
              const double dv = ip[off] - mean;
              var += dv * dv;
            }
            op[dst] = var / static_cast<double>(len);
            break;
          }
          case MovKind::Std: {
            const double mean = acc / static_cast<double>(len);
            double var = 0.0;
            for (std::int64_t k = start; k <= t; ++k) {
              const auto off = (o * n + k) * inner + i;
              const double dv = ip[off] - mean;
              var += dv * dv;
            }
            op[dst] = std::sqrt(var / static_cast<double>(len));
            break;
          }
        }
      }
    }
  }
  return out;
}

static Series ediff1d_impl(const Series& self) {
  auto x = self.flatten(0, -1).to(ScalarType::Float64).contiguous();
  const auto n = x.numel();
  if (n <= 1) return Series::empty({0}, ScalarType::Float64);
  auto out = Series::empty({n - 1}, ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n - 1; ++i) op[i] = ip[i + 1] - ip[i];
  return out;
}

static Series centered_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt) {
  auto x = self.to(ScalarType::Float64);
  if (!dim.has_value()) return ops_internal::sub(x, ops_internal::mean(x));
  const auto d = wrap_dim(*dim, x.dim(), "center");
  return ops_internal::sub(x, ops_internal::mean(x, d, /*keepdim=*/true));
}

static Series standardize_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt, double eps = 1e-12) {
  auto centered = centered_impl(self, dim);
  if (!dim.has_value()) {
    auto denom = ops_internal::sqrt(ops_internal::mean(ops_internal::square(centered)));
    return ops_internal::div(centered, add_scalar(denom, eps));
  }
  const auto d = wrap_dim(*dim, centered.dim(), "standardize");
  auto denom = ops_internal::sqrt(ops_internal::mean(ops_internal::square(centered), d, /*keepdim=*/true));
  return ops_internal::div(centered, add_scalar(denom, eps));
}

static Series skewness_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt) {
  auto c = centered_impl(self, dim);
  if (!dim.has_value()) {
    auto m2 = ops_internal::mean(ops_internal::square(c));
    auto m3 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 3.0)));
    return ops_internal::div(m3, ops_internal::pow(m2, scalar_like(m2, 1.5)));
  } else {
    const auto d = wrap_dim(*dim, c.dim(), "skewness");
    auto m2 = ops_internal::mean(ops_internal::square(c), d, false);
    auto m3 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 3.0)), d, false);
    return ops_internal::div(m3, ops_internal::pow(m2, scalar_like(m2, 1.5)));
  }
}

static Series kurtosis_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt) {
  auto c = centered_impl(self, dim);
  if (!dim.has_value()) {
    auto m2 = ops_internal::mean(ops_internal::square(c));
    auto m4 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 4.0)));
    return sub_scalar(ops_internal::div(m4, ops_internal::pow(m2, scalar_like(m2, 2.0))), 3.0);
  } else {
    const auto d = wrap_dim(*dim, c.dim(), "kurtosis");
    auto m2 = ops_internal::mean(ops_internal::square(c), d, false);
    auto m4 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 4.0)), d, false);
    return sub_scalar(ops_internal::div(m4, ops_internal::pow(m2, scalar_like(m2, 2.0))), 3.0);
  }
}

static Series norm_along_impl(const Series& self, std::int64_t dim, double eps = 1e-12) {
  auto x = self.to(ScalarType::Float64);
  const auto d = wrap_dim(dim, x.dim(), "normr/normc");
  auto n = ops_internal::sqrt(ops_internal::sum(ops_internal::square(x), d, /*keepdim=*/true));
  return ops_internal::div(x, add_scalar(n, eps));
}

static Series cdist_impl(const Series& a, const Series& b, double p = 2.0) {
  return native::cpu::cdist_kernel(a, b, p);
}

enum class TrigXform { Dct, Idct, Dst, Idst };

static Series trig_xform_impl(const Series& self, std::int64_t dim, TrigXform kind) {
  native::cpu::TrigTransformKind mapped = native::cpu::TrigTransformKind::Dct;
  switch (kind) {
    case TrigXform::Dct: mapped = native::cpu::TrigTransformKind::Dct; break;
    case TrigXform::Idct: mapped = native::cpu::TrigTransformKind::Idct; break;
    case TrigXform::Dst: mapped = native::cpu::TrigTransformKind::Dst; break;
    case TrigXform::Idst: mapped = native::cpu::TrigTransformKind::Idst; break;
  }
  return native::cpu::trig_transform(self, dim, mapped);
}

static Series hilbert_impl(const Series& self, std::int64_t dim = -1) {
  return native::cpu::hilbert_transform(self, dim);
}

static Series shift_impl(const Series& self, std::span<const std::int64_t> dims, bool inverse) {
  Series out = self;
  std::vector<std::int64_t> use_dims;
  if (dims.empty()) {
    use_dims.resize((std::size_t)self.dim());
    for (std::int64_t d = 0; d < self.dim(); ++d) use_dims[(std::size_t)d] = d;
  } else {
    use_dims.assign(dims.begin(), dims.end());
  }
  for (auto& d : use_dims) d = wrap_dim(d, out.dim(), "fftshift/ifftshift");
  for (const auto d : use_dims) {
    const auto n = out.sizes()[(std::size_t)d];
    const auto s = inverse ? ((n + 1) / 2) : (n / 2);
    const std::array<std::int64_t, 1> shifts{{s}};
    const std::array<std::int64_t, 1> dd{{d}};
    out = roll_impl(out,
                    std::span<const std::int64_t>(shifts.data(), shifts.size()),
                    std::span<const std::int64_t>(dd.data(), dd.size()));
  }
  return out;
}

static Series unwrap_impl(const Series& self, double discont = kPi, std::int64_t dim = -1) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), "unwrap");
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      double offset = 0.0;
      const auto off0 = (o * n + 0) * inner + i;
      op[off0] = ip[off0];
      for (std::int64_t t = 1; t < n; ++t) {
        const auto prev = (o * n + (t - 1)) * inner + i;
        const auto cur = (o * n + t) * inner + i;
        const double delta = ip[cur] - ip[prev];
        if (delta > discont) offset -= 2.0 * kPi;
        else if (delta < -discont) offset += 2.0 * kPi;
        op[cur] = ip[cur] + offset;
      }
    }
  }
  return out;
}

static Series cplxpair_impl(const Series& self) {
  if (!is_complex_dtype(self.scalar_type())) return sort_values_only(self, -1, false, false);
  auto x = self.to(ScalarType::Complex128).contiguous();
  auto flat = x.flatten(0, -1).contiguous();
  const auto n = flat.numel();
  std::vector<Complex128> vals((std::size_t)n);
  const auto* ip = reinterpret_cast<const Complex128*>(flat.raw_data());
  for (std::int64_t i = 0; i < n; ++i) vals[(std::size_t)i] = ip[i];
  std::sort(vals.begin(), vals.end(), [](const Complex128& a, const Complex128& b) {
    if (std::real(a) == std::real(b)) return std::imag(a) < std::imag(b);
    return std::real(a) < std::real(b);
  });
  auto out_flat = Series::empty({n}, ScalarType::Complex128);
  auto* op = reinterpret_cast<Complex128*>(out_flat.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = vals[(std::size_t)i];
  return out_flat.reshape(self.sizes());
}

static Series abs2_impl(const Series& self) {
  return ops_internal::square(ops_internal::abs(self));
}

static Series vdot_impl(const Series& self, const Series& other) {
  auto a1 = self.flatten(0, -1);
  auto b1 = other.flatten(0, -1);
  PLUSS_CHECK(a1.numel() == b1.numel(), "vdot: flattened inputs must have same numel");
  return ops_internal::sum(ops_internal::mul(ops_internal::conj(a1), b1));
}

static Series logit_impl(const Series& self, std::optional<double> eps = std::nullopt) {
  auto x = self.to(ScalarType::Float64);
  if (eps.has_value()) {
    const double e = std::clamp(*eps, 0.0, 0.5);
    x = ops_internal::clamp(x, e, 1.0 - e);
  }
  return ops_internal::log(ops_internal::div(x, ops_internal::sub(scalar_like(x, 1.0), x)));
}

static Series nextafter_impl(const Series& self, const Series& other) {
  auto a = self.to(ScalarType::Float64);
  auto b = other.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const double*>(bb.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::nextafter(ap[i], bp[i]);
  return out;
}

static Series nextafter_impl(const Series& self, double toward) {
  return nextafter_impl(self, scalar_like(self, toward));
}

static Series random_like(const Series& self, double lo, double hi) {
  PLUSS_CHECK(hi > lo, "random_: expected hi > lo");
  if (is_floating_dtype(self.scalar_type()) || is_complex_dtype(self.scalar_type())) {
    return uniform_like(self, lo, hi);
  }
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  Series x = ops_internal::floor(add_scalar(mul_scalar(u, hi - lo), lo));
  return x.to(self.scalar_type());
}

static Series bitshift_scalar_impl(const Series& self, std::int64_t shift, bool left) {
  auto x = self.to(ScalarType::Int64).contiguous();
  auto out = Series::empty(x.sizes(), ScalarType::Int64);
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const std::int64_t*>(x.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  if (left) {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ip[i] << shift;
  } else {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ip[i] >> shift;
  }
  return out.to(self.scalar_type());
}

static Series bitshift_series_impl(const Series& self, const Series& other, bool left) {
  auto a = self.to(ScalarType::Int64);
  auto b = other.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Int64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  if (left) {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ap[i] << bp[i];
  } else {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ap[i] >> bp[i];
  }
  return out.to(self.scalar_type());
}

static Series gcd_impl(const Series& self, const Series& other) {
  auto a = self.to(ScalarType::Int64);
  auto b = other.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Int64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::gcd(ap[i], bp[i]);
  return out.to(self.scalar_type());
}

static Series lcm_impl(const Series& self, const Series& other) {
  auto a = self.to(ScalarType::Int64);
  auto b = other.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Int64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::lcm(ap[i], bp[i]);
  return out.to(self.scalar_type());
}

static Series sinc_impl(const Series& self) {
  auto x = self.to(ScalarType::Float64).contiguous();
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    const double t = ip[i];
    if (t == 0.0) op[i] = 1.0;
    else op[i] = std::sin(kPi * t) / (kPi * t);
  }
  return out.to(self.scalar_type());
}

static Series hardshrink_impl(const Series& self, double lambda) {
  const double l = std::abs(lambda);
  auto x = self.to(ScalarType::Float64).contiguous();
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    const double v = ip[i];
    op[i] = (std::abs(v) > l) ? v : 0.0;
  }
  return out.to(self.scalar_type());
}

static Series hardshrink_backward_impl(const Series& grad_out, const Series& input, double lambda) {
  const double l = std::abs(lambda);
  auto g = grad_out.to(ScalarType::Float64);
  auto x = input.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(g.sizes(), x.sizes());
  auto gg = expand_impl(g, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto xx = expand_impl(x, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* gp = reinterpret_cast<const double*>(gg.raw_data());
  const auto* xp = reinterpret_cast<const double*>(xx.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    op[i] = (std::abs(xp[i]) > l) ? gp[i] : 0.0;
  }
  return out.to(grad_out.scalar_type());
}

static Series heaviside_impl(const Series& self, const Series& values) {
  auto x = self.to(ScalarType::Float64);
  auto v = values.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(x.sizes(), v.sizes());
  auto xx = expand_impl(x, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto vv = expand_impl(v, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* xp = reinterpret_cast<const double*>(xx.raw_data());
  const auto* vp = reinterpret_cast<const double*>(vv.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    if (xp[i] < 0.0) op[i] = 0.0;
    else if (xp[i] > 0.0) op[i] = 1.0;
    else op[i] = vp[i];
  }
  return out.to(self.scalar_type());
}

static Series aminmax_impl(const Series& self) {
  std::vector<Series> parts;
  parts.reserve(2);
  parts.push_back(ops_internal::amin(self));
  parts.push_back(ops_internal::amax(self));
  return native::cpu::stack(parts, 0);
}

static Series logcumsumexp_impl(const Series& self, std::int64_t dim) {
  auto x = self.to(ScalarType::Float64);
  auto c = ops_internal::exp(x).cumsum(dim, /*keepdim=*/false);
  return ops_internal::log(c).to(self.scalar_type());
}

static Series cross_impl(const Series& a, const Series& b, std::int64_t dim) {
  PLUSS_CHECK(a.dim() == b.dim(), "cross: expected same rank");
  PLUSS_CHECK(std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin()), "cross: expected same shape");
  dim = wrap_dim(dim, a.dim(), "cross");
  PLUSS_CHECK(a.sizes()[(std::size_t)dim] == 3, "cross: expected dim extent == 3");

  auto a0 = a.select(dim, 0);
  auto a1 = a.select(dim, 1);
  auto a2 = a.select(dim, 2);
  auto b0 = b.select(dim, 0);
  auto b1 = b.select(dim, 1);
  auto b2 = b.select(dim, 2);

  auto c0 = ops_internal::sub(ops_internal::mul(a1, b2), ops_internal::mul(a2, b1));
  auto c1 = ops_internal::sub(ops_internal::mul(a2, b0), ops_internal::mul(a0, b2));
  auto c2 = ops_internal::sub(ops_internal::mul(a0, b1), ops_internal::mul(a1, b0));

  std::vector<Series> parts;
  parts.reserve(3);
  parts.push_back(c0);
  parts.push_back(c1);
  parts.push_back(c2);
  return native::cpu::stack(parts, dim);
}

static double quantile_from_sorted(std::span<const double> vals, double q) {
  if (vals.empty()) return std::numeric_limits<double>::quiet_NaN();
  const double qq = std::clamp(q, 0.0, 1.0);
  const double pos = qq * static_cast<double>(vals.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(pos));
  const auto hi = static_cast<std::size_t>(std::ceil(pos));
  const double t = pos - static_cast<double>(lo);
  return vals[lo] * (1.0 - t) + vals[hi] * t;
}

static Series quantile_flat_impl(const Series& self, double q, bool ignore_nan) {
  auto x = self.to(ScalarType::Float64).flatten(0, -1).contiguous();
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  std::vector<double> vals;
  vals.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const double v = ip[i];
    if (std::isnan(v)) {
      if (ignore_nan) continue;
      auto out = Series::empty({}, ScalarType::Float64);
      *reinterpret_cast<double*>(out.raw_data()) = std::numeric_limits<double>::quiet_NaN();
      return out;
    }
    vals.push_back(v);
  }
  std::sort(vals.begin(), vals.end());
  auto out = Series::empty({}, ScalarType::Float64);
  *reinterpret_cast<double*>(out.raw_data()) = quantile_from_sorted(vals, q);
  return out;
}

static Series quantile_dim_impl(const Series& self, double q, std::int64_t dim, bool keepdim, bool ignore_nan) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), "quantile");
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  std::vector<std::int64_t> out_sizes(x.sizes().begin(), x.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  auto out = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  std::vector<double> vals;
  vals.reserve(static_cast<std::size_t>(n));
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      vals.clear();
      bool saw_nan = false;
      for (std::int64_t t = 0; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        const double v = ip[off];
        if (std::isnan(v)) {
          saw_nan = true;
          if (ignore_nan) continue;
        } else {
          vals.push_back(v);
        }
      }
      const auto dst = o * inner + i;
      if (!ignore_nan && saw_nan) {
        op[dst] = std::numeric_limits<double>::quiet_NaN();
      } else {
        std::sort(vals.begin(), vals.end());
        op[dst] = quantile_from_sorted(vals, q);
      }
    }
  }
  return out;
}

static Series mode_flat_impl(const Series& self) {
  auto x = self.to(ScalarType::Float64).flatten(0, -1).contiguous();
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  if (n == 0) return Series::empty({}, ScalarType::Float64);
  std::vector<double> vals((std::size_t)n);
  for (std::int64_t i = 0; i < n; ++i) vals[(std::size_t)i] = ip[i];
  std::sort(vals.begin(), vals.end());
  double best = vals[0];
  std::int64_t bestc = 1;
  std::int64_t curc = 1;
  for (std::size_t i = 1; i < vals.size(); ++i) {
    if (vals[i] == vals[i - 1]) ++curc;
    else {
      if (curc > bestc) { bestc = curc; best = vals[i - 1]; }
      curc = 1;
    }
  }
  if (curc > bestc) best = vals.back();
  auto out = Series::empty({}, ScalarType::Float64);
  *reinterpret_cast<double*>(out.raw_data()) = best;
  return out;
}

static Series mode_dim_impl(const Series& self, std::int64_t dim, bool keepdim) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), "mode");
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  std::vector<std::int64_t> out_sizes(x.sizes().begin(), x.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  auto out = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  std::vector<double> vals;
  vals.reserve(static_cast<std::size_t>(n));
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      vals.clear();
      for (std::int64_t t = 0; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        vals.push_back(ip[off]);
      }
      std::sort(vals.begin(), vals.end());
      double best = vals[0];
      std::int64_t bestc = 1;
      std::int64_t curc = 1;
      for (std::size_t j = 1; j < vals.size(); ++j) {
        if (vals[j] == vals[j - 1]) ++curc;
        else {
          if (curc > bestc) { bestc = curc; best = vals[j - 1]; }
          curc = 1;
        }
      }
      if (curc > bestc) best = vals.back();
      op[o * inner + i] = best;
    }
  }
  return out;
}

static Series rot90_impl(const Series& self, std::int64_t k, std::int64_t d0, std::int64_t d1) {
  auto out = self;
  const auto nd = self.dim();
  d0 = wrap_dim(d0, nd, "rot90");
  d1 = wrap_dim(d1, nd, "rot90");
  PLUSS_CHECK(d0 != d1, "rot90: dims must be different");
  k %= 4;
  if (k < 0) k += 4;
  if (k == 0) return out;
  if (k == 1) {
    out = out.transpose(d0, d1);
    const std::array<std::int64_t, 1> dims{{d0}};
    return flip_impl(out, std::span<const std::int64_t>(dims.data(), dims.size()), "rot90");
  }
  if (k == 2) {
    const std::array<std::int64_t, 2> dims{{d0, d1}};
    return flip_impl(out, std::span<const std::int64_t>(dims.data(), dims.size()), "rot90");
  }
  out = out.transpose(d0, d1);
  const std::array<std::int64_t, 1> dims{{d1}};
  return flip_impl(out, std::span<const std::int64_t>(dims.data(), dims.size()), "rot90");
}

static Series unflatten_impl(const Series& self, std::int64_t dim, std::span<const std::int64_t> new_sizes) {
  PLUSS_CHECK(!new_sizes.empty(), "unflatten: new_sizes must be non-empty");
  dim = wrap_dim(dim, self.dim(), "unflatten");
  std::int64_t prod = 1;
  for (const auto s : new_sizes) {
    PLUSS_CHECK(s >= 0, "unflatten: sizes must be >= 0");
    prod *= s;
  }
  PLUSS_CHECK(prod == self.sizes()[(std::size_t)dim], "unflatten: product mismatch");
  std::vector<std::int64_t> out_sizes;
  out_sizes.reserve((std::size_t)(self.dim() - 1 + static_cast<std::int64_t>(new_sizes.size())));
  for (std::int64_t d = 0; d < dim; ++d) out_sizes.push_back(self.sizes()[(std::size_t)d]);
  out_sizes.insert(out_sizes.end(), new_sizes.begin(), new_sizes.end());
  for (std::int64_t d = dim + 1; d < self.dim(); ++d) out_sizes.push_back(self.sizes()[(std::size_t)d]);
  return self.reshape(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()));
}

static Series unfold_impl(const Series& self, std::int64_t dim, std::int64_t size, std::int64_t step) {
  PLUSS_CHECK(size > 0 && step > 0, "unfold: size and step must be > 0");
  auto x = self.contiguous();
  dim = wrap_dim(dim, x.dim(), "unfold");
  const auto n = x.sizes()[(std::size_t)dim];
  const auto m = (n < size) ? 0 : (1 + (n - size) / step);
  std::vector<std::int64_t> out_sizes;
  out_sizes.reserve((std::size_t)(x.dim() + 1));
  for (std::int64_t d = 0; d < dim; ++d) out_sizes.push_back(x.sizes()[(std::size_t)d]);
  out_sizes.push_back(m);
  out_sizes.push_back(size);
  for (std::int64_t d = dim + 1; d < x.dim(); ++d) out_sizes.push_back(x.sizes()[(std::size_t)d]);
  auto out = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), x.scalar_type());
  const auto item = x.dtype().itemsize;
  const auto* src = x.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < x.dim(); ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t w = 0; w < m; ++w) {
      const auto start = w * step;
      for (std::int64_t k = 0; k < size; ++k) {
        const auto src_base = ((o * n + (start + k)) * inner);
        const auto dst_base = (((o * m + w) * size + k) * inner);
        std::memcpy(dst + dst_base * item, src + src_base * item, static_cast<std::size_t>(inner * item));
      }
    }
  }
  return out;
}

static Series sum_to_size_impl(const Series& self, std::span<const std::int64_t> target) {
  auto out = self;
  PLUSS_CHECK(target.size() <= static_cast<std::size_t>(out.dim()), "sum_to_size: target rank must be <= input rank");
  while (out.dim() > static_cast<std::int64_t>(target.size())) {
    out = ops_internal::sum(out, 0, false);
  }
  for (std::int64_t d = 0; d < out.dim(); ++d) {
    const auto t = target[(std::size_t)d];
    const auto s = out.sizes()[(std::size_t)d];
    if (t == s) continue;
    PLUSS_CHECK(t == 1, "sum_to_size: incompatible target dimension");
    out = ops_internal::sum(out, d, true);
  }
  return out.reshape(target);
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

static Series identity_f64(std::int64_t n) {
  auto out = Series::zeros({n, n}, ScalarType::Float64);
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i * n + i] = 1.0;
  return out;
}

static Series cum_extrema_impl(const Series& self, std::int64_t dim, bool is_max) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), is_max ? "cummax" : "cummin");
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      double cur = ip[(o * n + 0) * inner + i];
      op[(o * n + 0) * inner + i] = cur;
      for (std::int64_t t = 1; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        if (is_max) cur = std::max(cur, ip[off]);
        else cur = std::min(cur, ip[off]);
        op[off] = cur;
      }
    }
  }
  return out.to(self.scalar_type());
}

static Series det_impl(const Series& self) {
  auto a = self.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(a.dim() == 2 && a.sizes()[0] == a.sizes()[1], "det: expected square 2D tensor");
  const auto n = a.sizes()[0];
  std::vector<double> m((std::size_t)(n * n));
  const auto* ap = reinterpret_cast<const double*>(a.raw_data());
  for (std::int64_t i = 0; i < n * n; ++i) m[(std::size_t)i] = ap[i];
  double det = 1.0;
  int sign = 1;
  for (std::int64_t k = 0; k < n; ++k) {
    std::int64_t piv = k;
    double maxv = std::abs(m[(std::size_t)(k * n + k)]);
    for (std::int64_t r = k + 1; r < n; ++r) {
      const double v = std::abs(m[(std::size_t)(r * n + k)]);
      if (v > maxv) {
        maxv = v;
        piv = r;
      }
    }
    if (maxv < 1e-18) {
      det = 0.0;
      break;
    }
    if (piv != k) {
      for (std::int64_t c = k; c < n; ++c) std::swap(m[(std::size_t)(k * n + c)], m[(std::size_t)(piv * n + c)]);
      sign = -sign;
    }
    const double akk = m[(std::size_t)(k * n + k)];
    det *= akk;
    for (std::int64_t r = k + 1; r < n; ++r) {
      const double f = m[(std::size_t)(r * n + k)] / akk;
      m[(std::size_t)(r * n + k)] = 0.0;
      for (std::int64_t c = k + 1; c < n; ++c) m[(std::size_t)(r * n + c)] -= f * m[(std::size_t)(k * n + c)];
    }
  }
  auto out = Series::empty({}, ScalarType::Float64);
  *reinterpret_cast<double*>(out.raw_data()) = det * static_cast<double>(sign);
  return out;
}

static Series inverse_impl(const Series& self) {
  auto a = self.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(a.dim() == 2 && a.sizes()[0] == a.sizes()[1], "inverse: expected square 2D tensor");
  const auto n = a.sizes()[0];
  std::vector<double> aug((std::size_t)(n * 2 * n), 0.0);
  const auto* ap = reinterpret_cast<const double*>(a.raw_data());
  for (std::int64_t r = 0; r < n; ++r) {
    for (std::int64_t c = 0; c < n; ++c) aug[(std::size_t)(r * (2 * n) + c)] = ap[r * n + c];
    aug[(std::size_t)(r * (2 * n) + (n + r))] = 1.0;
  }
  for (std::int64_t k = 0; k < n; ++k) {
    std::int64_t piv = k;
    double maxv = std::abs(aug[(std::size_t)(k * (2 * n) + k)]);
    for (std::int64_t r = k + 1; r < n; ++r) {
      const double v = std::abs(aug[(std::size_t)(r * (2 * n) + k)]);
      if (v > maxv) {
        maxv = v;
        piv = r;
      }
    }
    PLUSS_CHECK(maxv > 1e-18, "inverse: singular matrix");
    if (piv != k) {
      for (std::int64_t c = 0; c < 2 * n; ++c) std::swap(aug[(std::size_t)(k * (2 * n) + c)], aug[(std::size_t)(piv * (2 * n) + c)]);
    }
    const double diag = aug[(std::size_t)(k * (2 * n) + k)];
    for (std::int64_t c = 0; c < 2 * n; ++c) aug[(std::size_t)(k * (2 * n) + c)] /= diag;
    for (std::int64_t r = 0; r < n; ++r) {
      if (r == k) continue;
      const double f = aug[(std::size_t)(r * (2 * n) + k)];
      if (f == 0.0) continue;
      for (std::int64_t c = 0; c < 2 * n; ++c) aug[(std::size_t)(r * (2 * n) + c)] -= f * aug[(std::size_t)(k * (2 * n) + c)];
    }
  }
  auto out = Series::empty({n, n}, ScalarType::Float64);
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t r = 0; r < n; ++r) {
    for (std::int64_t c = 0; c < n; ++c) op[r * n + c] = aug[(std::size_t)(r * (2 * n) + (n + c))];
  }
  return out;
}

static Series logdet_impl(const Series& self) {
  auto d = det_impl(self);
  const double v = *reinterpret_cast<const double*>(d.raw_data());
  auto out = Series::empty({}, ScalarType::Float64);
  *reinterpret_cast<double*>(out.raw_data()) = std::log(std::abs(v));
  return out;
}

static Series slogdet_impl(const Series& self) {
  auto d = det_impl(self);
  const double v = *reinterpret_cast<const double*>(d.raw_data());
  auto out = Series::empty({2}, ScalarType::Float64);
  auto* op = reinterpret_cast<double*>(out.raw_data());
  op[0] = (v > 0.0) ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
  op[1] = std::log(std::abs(v));
  return out;
}

static Series matrix_power_impl(const Series& self, std::int64_t n) {
  PLUSS_CHECK(self.dim() == 2 && self.sizes()[0] == self.sizes()[1], "matrix_power: expected square 2D tensor");
  if (n == 0) return identity_f64(self.sizes()[0]);
  Series base = self.to(ScalarType::Float64);
  std::uint64_t e = static_cast<std::uint64_t>(n < 0 ? -n : n);
  if (n < 0) base = inverse_impl(base);
  Series res = identity_f64(base.sizes()[0]);
  while (e > 0) {
    if (e & 1ULL) res = native::cpu::matmul(res, base);
    e >>= 1ULL;
    if (e) base = native::cpu::matmul(base, base);
  }
  return res;
}

static Series matrix_exp_impl(const Series& self) {
  PLUSS_CHECK(self.dim() == 2 && self.sizes()[0] == self.sizes()[1], "matrix_exp: expected square 2D tensor");
  auto a = self.to(ScalarType::Float64);
  const auto n = a.sizes()[0];
  Series res = identity_f64(n);
  Series term = identity_f64(n);
  for (std::int64_t k = 1; k <= 18; ++k) {
    term = native::cpu::matmul(term, a);
    term = div_scalar(term, static_cast<double>(k));
    res = ops_internal::add(res, term);
  }
  return res;
}

static Series cholesky_impl(const Series& self, bool upper) {
  auto a = self.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(a.dim() == 2 && a.sizes()[0] == a.sizes()[1], "cholesky: expected square 2D tensor");
  const auto n = a.sizes()[0];
  auto out = Series::zeros({n, n}, ScalarType::Float64);
  const auto* ap = reinterpret_cast<const double*>(a.raw_data());
  auto* lp = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    for (std::int64_t j = 0; j <= i; ++j) {
      double s = ap[i * n + j];
      for (std::int64_t k = 0; k < j; ++k) s -= lp[i * n + k] * lp[j * n + k];
      if (i == j) {
        PLUSS_CHECK(s > 0.0, "cholesky: matrix is not positive definite");
        lp[i * n + j] = std::sqrt(s);
      } else {
        lp[i * n + j] = s / lp[j * n + j];
      }
    }
  }
  if (upper) return out.transpose(0, 1);
  return out;
}

static Series cholesky_inverse_impl(const Series& factor, bool upper) {
  Series l = factor.to(ScalarType::Float64);
  if (upper) l = l.transpose(0, 1);
  auto a = native::cpu::matmul(l, l.transpose(0, 1));
  return inverse_impl(a);
}

static Series cholesky_solve_impl(const Series& b, const Series& factor, bool upper) {
  auto rhs = b.to(ScalarType::Float64).contiguous();
  Series l = factor.to(ScalarType::Float64).contiguous();
  if (upper) l = l.transpose(0, 1).contiguous();
  PLUSS_CHECK(l.dim() == 2 && l.sizes()[0] == l.sizes()[1], "cholesky_solve: factor must be square 2D");
  PLUSS_CHECK(rhs.dim() == 2 && rhs.sizes()[0] == l.sizes()[0], "cholesky_solve: rhs shape mismatch");
  const auto n = l.sizes()[0];
  const auto nrhs = rhs.sizes()[1];
  const auto* lp = reinterpret_cast<const double*>(l.raw_data());
  const auto* bp = reinterpret_cast<const double*>(rhs.raw_data());
  auto y = Series::empty(rhs.sizes(), ScalarType::Float64);
  auto x = Series::empty(rhs.sizes(), ScalarType::Float64);
  auto* yp = reinterpret_cast<double*>(y.raw_data());
  auto* xp = reinterpret_cast<double*>(x.raw_data());
  for (std::int64_t c = 0; c < nrhs; ++c) {
    for (std::int64_t i = 0; i < n; ++i) {
      double s = bp[i * nrhs + c];
      for (std::int64_t k = 0; k < i; ++k) s -= lp[i * n + k] * yp[k * nrhs + c];
      yp[i * nrhs + c] = s / lp[i * n + i];
    }
    for (std::int64_t i = n - 1; i >= 0; --i) {
      double s = yp[i * nrhs + c];
      for (std::int64_t k = i + 1; k < n; ++k) s -= lp[k * n + i] * xp[k * nrhs + c];
      xp[i * nrhs + c] = s / lp[i * n + i];
    }
  }
  return x;
}

static Series kron_impl(const Series& a, const Series& b) {
  if (a.dim() == 2 && b.dim() == 2) {
    auto aa = a.to(ScalarType::Float64).contiguous();
    auto bb = b.to(ScalarType::Float64).contiguous();
    const auto m = aa.sizes()[0], n = aa.sizes()[1];
    const auto p = bb.sizes()[0], q = bb.sizes()[1];
    auto out = Series::empty({m * p, n * q}, ScalarType::Float64);
    const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
    const auto* bp = reinterpret_cast<const double*>(bb.raw_data());
    auto* op = reinterpret_cast<double*>(out.raw_data());
    for (std::int64_t i = 0; i < m; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        const double av = ap[i * n + j];
        for (std::int64_t r = 0; r < p; ++r) {
          for (std::int64_t c = 0; c < q; ++c) {
            op[(i * p + r) * (n * q) + (j * q + c)] = av * bp[r * q + c];
          }
        }
      }
    }
    return out;
  }
  auto af = a.to(ScalarType::Float64).flatten(0, -1).contiguous();
  auto bf = b.to(ScalarType::Float64).flatten(0, -1).contiguous();
  const auto an = af.numel();
  const auto bn = bf.numel();
  auto out = Series::empty({an * bn}, ScalarType::Float64);
  const auto* ap = reinterpret_cast<const double*>(af.raw_data());
  const auto* bp = reinterpret_cast<const double*>(bf.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < an; ++i) {
    for (std::int64_t j = 0; j < bn; ++j) op[i * bn + j] = ap[i] * bp[j];
  }
  std::vector<std::int64_t> shape;
  shape.reserve((std::size_t)(a.dim() + b.dim()));
  shape.insert(shape.end(), a.sizes().begin(), a.sizes().end());
  shape.insert(shape.end(), b.sizes().begin(), b.sizes().end());
  return out.reshape(std::span<const std::int64_t>(shape.data(), shape.size()));
}

static Series covariance_impl(const Series& self) {
  auto x = self.to(ScalarType::Float64).contiguous();
  if (x.dim() == 1) {
    auto xc = centered_impl(x);
    const double n = static_cast<double>(x.numel());
    return div_scalar(ops_internal::sum(ops_internal::square(xc)), std::max(1.0, n - 1.0));
  }
  PLUSS_CHECK(x.dim() == 2, "cov: expected 1D or 2D");
  const auto obs = x.sizes()[1];
  PLUSS_CHECK(obs >= 1, "cov: expected at least one observation");
  auto xc = centered_impl(x, 1);
  auto cov = native::cpu::matmul(xc, xc.transpose(0, 1));
  return div_scalar(cov, std::max<std::int64_t>(1, obs - 1));
}

static Series corrcoef_impl(const Series& self) {
  auto cov = covariance_impl(self).to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(cov.dim() == 2 && cov.sizes()[0] == cov.sizes()[1], "corrcoef: expected square covariance");
  const auto n = cov.sizes()[0];
  auto out = Series::empty(cov.sizes(), ScalarType::Float64);
  const auto* cp = reinterpret_cast<const double*>(cov.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  std::vector<double> s((std::size_t)n, 0.0);
  for (std::int64_t i = 0; i < n; ++i) s[(std::size_t)i] = std::sqrt(std::max(0.0, cp[i * n + i]));
  for (std::int64_t i = 0; i < n; ++i) {
    for (std::int64_t j = 0; j < n; ++j) {
      const double den = s[(std::size_t)i] * s[(std::size_t)j];
      op[i * n + j] = (den > 0.0) ? (cp[i * n + j] / den) : 0.0;
    }
  }
  return out;
}

static double digamma_scalar(double x) {
  if (std::isnan(x)) return x;
  if (x <= 0.0) {
    const double r = x - std::floor(x);
    if (std::abs(r) < 1e-15) return std::numeric_limits<double>::quiet_NaN();
    return digamma_scalar(1.0 - x) - kPi / std::tan(kPi * x);
  }
  double y = 0.0;
  while (x < 6.0) {
    y -= 1.0 / x;
    x += 1.0;
  }
  const double inv = 1.0 / x;
  const double inv2 = inv * inv;
  y += std::log(x) - 0.5 * inv - inv2 * (1.0 / 12.0 - inv2 * (1.0 / 120.0 - inv2 * (1.0 / 252.0)));
  return y;
}

static Series digamma_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) { return digamma_scalar(x); });
}

static Series erfinv_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) { return erfinv_approx(x); });
}

static double i0_scalar(double x) {
  const double ax = std::abs(x);
  if (ax < 3.75) {
    const double t = x / 3.75;
    const double t2 = t * t;
    return 1.0 + t2 * (3.5156229 + t2 * (3.0899424 + t2 * (1.2067492 +
           t2 * (0.2659732 + t2 * (0.0360768 + t2 * 0.0045813)))));
  }
  const double t = 3.75 / ax;
  return (std::exp(ax) / std::sqrt(ax)) *
         (0.39894228 + t * (0.01328592 + t * (0.00225319 + t * (-0.00157565 +
         t * (0.00916281 + t * (-0.02057706 + t * (0.02635537 +
         t * (-0.01647633 + t * 0.00392377))))))));
}

static Series i0_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) { return i0_scalar(x); });
}

static Series mvlgamma_impl(const Series& self, std::int64_t p) {
  PLUSS_CHECK(p >= 1, "mvlgamma: p must be >= 1");
  const double c = 0.25 * static_cast<double>(p) * static_cast<double>(p - 1) * std::log(kPi);
  return unary_f64_impl(self, [p, c](double x) {
    double acc = c;
    for (std::int64_t j = 1; j <= p; ++j) acc += std::lgamma(x + 0.5 * (1.0 - static_cast<double>(j)));
    return acc;
  });
}

static Series polygamma_impl(const Series& self, std::int64_t n) {
  PLUSS_CHECK(n >= 0, "polygamma: n must be >= 0");
  if (n == 0) return digamma_impl(self);
  double fact = 1.0;
  for (std::int64_t i = 2; i <= n; ++i) fact *= static_cast<double>(i);
  const double sign = ((n + 1) % 2 == 0) ? 1.0 : -1.0;
  return unary_f64_impl(self, [n, fact, sign](double x) {
    if (x <= 0.0) return std::numeric_limits<double>::quiet_NaN();
    double acc = 0.0;
    constexpr std::int64_t terms = 2048;
    for (std::int64_t k = 0; k < terms; ++k) acc += std::pow(x + static_cast<double>(k), -static_cast<double>(n + 1));
    return sign * fact * acc;
  });
}

static Series frexp_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) {
    int e = 0;
    return std::frexp(x, &e);
  });
}

static Series ldexp_impl(const Series& self, double expv) {
  const int ie = static_cast<int>(expv);
  return unary_f64_impl(self, [ie](double x) { return std::ldexp(x, ie); });
}

static Series ldexp_impl(const Series& self, const Series& exponents) {
  auto x = self.to(ScalarType::Float64);
  auto e = exponents.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(x.sizes(), e.sizes());
  auto xx = expand_impl(x, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto ee = expand_impl(e, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* xp = reinterpret_cast<const double*>(xx.raw_data());
  const auto* ep = reinterpret_cast<const std::int64_t*>(ee.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::ldexp(xp[i], static_cast<int>(ep[i]));
  return out;
}

static Series prelu_impl(const Series& self, const Series& weight) {
  auto x = self.to(ScalarType::Float64);
  auto w = weight.to(ScalarType::Float64);
  auto zero = scalar_like(x, 0.0);
  auto pos = ops_internal::gt(x, zero);
  auto neg = ops_internal::mul(w, x);
  return ops_internal::where(pos, x, neg).to(self.scalar_type());
}

static Series prelu_impl(const Series& self, double weight) {
  return prelu_impl(self, scalar_like(self, weight));
}

static Series renorm_impl(const Series& self, double p, std::int64_t dim, double maxnorm) {
  PLUSS_CHECK(p > 0.0, "renorm: p must be > 0");
  auto x = self.to(ScalarType::Float64);
  dim = wrap_dim(dim, x.dim(), "renorm");
  auto norm = ops_internal::pow(ops_internal::sum(ops_internal::pow(ops_internal::abs(x), scalar_like(x, p)), dim, true),
                                scalar_like(x, 1.0 / p));
  auto denom = ops_internal::maximum(norm, scalar_like(norm, 1e-12));
  auto scale = ops_internal::minimum(scalar_like(norm, 1.0), ops_internal::div(scalar_like(norm, maxnorm), denom));
  return ops_internal::mul(x, scale).to(self.scalar_type());
}

static Series multinomial_impl(const Series& self, std::int64_t num_samples, bool replacement) {
  PLUSS_CHECK(num_samples >= 0, "multinomial: num_samples must be >= 0");
  auto p = self.to(ScalarType::Float64).flatten(0, -1).contiguous();
  const auto n = p.numel();
  const auto* pp = reinterpret_cast<const double*>(p.raw_data());
  std::vector<double> probs((std::size_t)n, 0.0);
  double z = 0.0;
  for (std::int64_t i = 0; i < n; ++i) {
    probs[(std::size_t)i] = std::max(0.0, pp[i]);
    z += probs[(std::size_t)i];
  }
  PLUSS_CHECK(z > 0.0, "multinomial: probabilities must sum to > 0");
  for (auto& v : probs) v /= z;
  std::mt19937_64 gen(0);
  auto out = Series::empty({num_samples}, ScalarType::Int64);
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t s = 0; s < num_samples; ++s) {
    std::discrete_distribution<std::int64_t> dist(probs.begin(), probs.end());
    const auto idx = dist(gen);
    op[s] = idx;
    if (!replacement) probs[(std::size_t)idx] = 0.0;
  }
  return out;
}

static double igamma_p_scalar(double a, double x) {
  if (!(a > 0.0) || x < 0.0 || std::isnan(a) || std::isnan(x)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (x == 0.0) return 0.0;
  constexpr int itmax = 512;
  constexpr double eps = 1e-12;
  constexpr double fpmin = 1e-300;

  const double gln = std::lgamma(a);
  if (x < a + 1.0) {
    double ap = a;
    double del = 1.0 / a;
    double sum = del;
    for (int n = 1; n <= itmax; ++n) {
      ap += 1.0;
      del *= x / ap;
      sum += del;
      if (std::abs(del) < std::abs(sum) * eps) {
        return sum * std::exp(-x + a * std::log(x) - gln);
      }
    }
    return sum * std::exp(-x + a * std::log(x) - gln);
  }

  double b = x + 1.0 - a;
  double c = 1.0 / fpmin;
  double d = 1.0 / b;
  double h = d;
  for (int i = 1; i <= itmax; ++i) {
    const double an = -static_cast<double>(i) * (static_cast<double>(i) - a);
    b += 2.0;
    d = an * d + b;
    if (std::abs(d) < fpmin) d = fpmin;
    c = b + an / c;
    if (std::abs(c) < fpmin) c = fpmin;
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < eps) break;
  }
  return 1.0 - std::exp(-x + a * std::log(x) - gln) * h;
}

static Series igamma_impl(const Series& a, const Series& x, bool complement) {
  auto aa = a.to(ScalarType::Float64);
  auto xx = x.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), xx.sizes());
  auto abb = expand_impl(aa, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto xbb = expand_impl(xx, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const double*>(abb.raw_data());
  const auto* xp = reinterpret_cast<const double*>(xbb.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    const double p = igamma_p_scalar(ap[i], xp[i]);
    op[i] = complement ? (1.0 - p) : p;
  }
  return out;
}

static Series as_strided_impl(
    const Series& self,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    std::int64_t storage_offset_elems) {
  PLUSS_CHECK(sizes.size() == strides.size(), "as_strided: sizes/strides rank mismatch");
  PLUSS_CHECK(storage_offset_elems >= 0, "as_strided: storage_offset must be >= 0");
  return Series::from_storage(
      self.storage(),
      storage_offset_elems,
      std::vector<std::int64_t>(sizes.begin(), sizes.end()),
      std::vector<std::int64_t>(strides.begin(), strides.end()),
      self.scalar_type());
}

static Series as_strided_impl(
    const Series& self,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides) {
  const auto base_off = self.storage_offset() / std::max<std::int64_t>(1, self.itemsize());
  return as_strided_impl(self, sizes, strides, base_off);
}

static Series as_strided_scatter_impl(
    const Series& self,
    const Series& src,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    std::int64_t storage_offset_elems) {
  PLUSS_CHECK(sizes.size() == strides.size(), "as_strided_scatter: sizes/strides rank mismatch");
  auto out = self.clone().contiguous();
  auto in = src.to(self.scalar_type()).contiguous();
  std::int64_t n = 1;
  for (auto s : sizes) {
    PLUSS_CHECK(s >= 0, "as_strided_scatter: invalid size");
    n *= s;
  }
  PLUSS_CHECK(in.numel() == n, "as_strided_scatter: src.numel must match view numel");
  const auto item = out.itemsize();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_mutable_data());
  const auto* sp = in.raw_data();
  std::vector<std::int64_t> idx(sizes.size(), 0);
  for (std::int64_t lin = 0; lin < n; ++lin) {
    std::int64_t rem = lin;
    for (std::int64_t d = static_cast<std::int64_t>(sizes.size()) - 1; d >= 0; --d) {
      const auto sd = sizes[(std::size_t)d];
      idx[(std::size_t)d] = rem % sd;
      rem /= sd;
    }
    std::int64_t off = storage_offset_elems;
    for (std::size_t d = 0; d < idx.size(); ++d) off += idx[d] * strides[d];
    std::memcpy(dst + off * item, sp + lin * item, static_cast<std::size_t>(item));
  }
  return out;
}

static Series diagonal_scatter_impl(const Series& self, const Series& src, std::int64_t diagonal) {
  PLUSS_CHECK(self.dim() == 2, "diagonal_scatter: expected 2D input");
  auto out = self.clone().contiguous();
  auto in = src.to(self.scalar_type()).flatten(0, -1).contiguous();
  const auto rows = out.sizes()[0];
  const auto cols = out.sizes()[1];
  const auto row0 = diagonal < 0 ? -diagonal : 0;
  const auto col0 = diagonal > 0 ? diagonal : 0;
  const auto maxlen = std::max<std::int64_t>(0, std::min(rows - row0, cols - col0));
  const auto len = std::min(maxlen, in.numel());
  const auto item = out.itemsize();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_mutable_data());
  const auto* sp = in.raw_data();
  for (std::int64_t i = 0; i < len; ++i) {
    const auto doff = ((row0 + i) * cols + (col0 + i)) * item;
    std::memcpy(dst + doff, sp + i * item, static_cast<std::size_t>(item));
  }
  return out;
}

static Series select_scatter_impl(const Series& self, const Series& src, std::int64_t dim, std::int64_t index) {
  dim = wrap_dim(dim, self.dim(), "select_scatter");
  const auto dsz = self.sizes()[(std::size_t)dim];
  if (index < 0) index += dsz;
  PLUSS_CHECK(index >= 0 && index < dsz, "select_scatter: index out of range");
  auto x = src.to(self.scalar_type());
  if (x.dim() == self.dim() - 1) x = x.unsqueeze(dim);
  PLUSS_CHECK(x.dim() == self.dim(), "select_scatter: src rank mismatch");
  PLUSS_CHECK(x.sizes()[(std::size_t)dim] == 1, "select_scatter: src size at dim must be 1");
  auto idx = Series::full(x.sizes(), static_cast<double>(index), ScalarType::Int64);
  return native::cpu::scatter(self, dim, idx, x);
}

static Series slice_scatter_impl(
    const Series& self,
    const Series& src,
    std::int64_t dim,
    std::int64_t start,
    std::int64_t end,
    std::int64_t step) {
  PLUSS_CHECK(step > 0, "slice_scatter: step must be > 0");
  dim = wrap_dim(dim, self.dim(), "slice_scatter");
  const auto dsz = self.sizes()[(std::size_t)dim];
  if (start < 0) start += dsz;
  if (end < 0) end += dsz;
  start = std::clamp<std::int64_t>(start, 0, dsz);
  end = std::clamp<std::int64_t>(end, 0, dsz);
  PLUSS_CHECK(end >= start, "slice_scatter: end must be >= start");
  std::vector<std::int64_t> ids;
  for (std::int64_t i = start; i < end; i += step) ids.push_back(i);

  auto x = src.to(self.scalar_type());
  if (x.dim() == self.dim() - 1 && ids.size() == 1) return select_scatter_impl(self, x, dim, ids[0]);
  if (x.dim() == self.dim() - 1) x = x.unsqueeze(dim);
  PLUSS_CHECK(x.dim() == self.dim(), "slice_scatter: src rank mismatch");
  PLUSS_CHECK(x.sizes()[(std::size_t)dim] == static_cast<std::int64_t>(ids.size()),
              "slice_scatter: src slice length mismatch");

  auto idx = Series::empty(x.sizes(), ScalarType::Int64).contiguous();
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_mutable_data());
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < x.dim(); ++d) inner *= x.sizes()[(std::size_t)d];
  const auto n = x.sizes()[(std::size_t)dim];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t k = 0; k < n; ++k) {
      for (std::int64_t i = 0; i < inner; ++i) ip[(o * n + k) * inner + i] = ids[(std::size_t)k];
    }
  }
  return native::cpu::scatter(self, dim, idx, x);
}

static Series triangular_solve_impl(const Series& b, const Series& a, bool upper, bool unitriangular) {
  auto aa = a.to(ScalarType::Float64).contiguous();
  auto bb = b.to(ScalarType::Float64).contiguous();
  PLUSS_CHECK(aa.dim() == 2 && aa.sizes()[0] == aa.sizes()[1], "triangular_solve: A must be square 2D");
  const auto n = aa.sizes()[0];
  bool squeezed = false;
  if (bb.dim() == 1) {
    PLUSS_CHECK(bb.sizes()[0] == n, "triangular_solve: rhs size mismatch");
    bb = bb.unsqueeze(1).contiguous();
    squeezed = true;
  }
  PLUSS_CHECK(bb.dim() == 2 && bb.sizes()[0] == n, "triangular_solve: rhs shape mismatch");
  const auto nrhs = bb.sizes()[1];
  auto out = Series::empty(bb.sizes(), ScalarType::Float64);
  const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const double*>(bb.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_mutable_data());

  if (upper) {
    for (std::int64_t c = 0; c < nrhs; ++c) {
      for (std::int64_t i = n - 1; i >= 0; --i) {
        double s = bp[i * nrhs + c];
        for (std::int64_t k = i + 1; k < n; ++k) s -= ap[i * n + k] * op[k * nrhs + c];
        const double den = unitriangular ? 1.0 : ap[i * n + i];
        op[i * nrhs + c] = s / den;
      }
    }
  } else {
    for (std::int64_t c = 0; c < nrhs; ++c) {
      for (std::int64_t i = 0; i < n; ++i) {
        double s = bp[i * nrhs + c];
        for (std::int64_t k = 0; k < i; ++k) s -= ap[i * n + k] * op[k * nrhs + c];
        const double den = unitriangular ? 1.0 : ap[i * n + i];
        op[i * nrhs + c] = s / den;
      }
    }
  }
  if (squeezed) return out.select(1, 0);
  return out;
}

static Series pinverse_impl(const Series& self, double rcond) {
  auto a = self.to(ScalarType::Float64);
  PLUSS_CHECK(a.dim() == 2, "pinverse: expected a 2D input");
  const auto m = a.sizes()[0];
  const auto n = a.sizes()[1];
  if (m == n) return inverse_impl(a);
  const auto at = a.transpose(0, 1);
  if (m >= n) {
    auto gram = native::cpu::matmul(at, a);
    auto reg = ops_internal::add(gram, mul_scalar(identity_f64(n), rcond));
    return native::cpu::matmul(inverse_impl(reg), at);
  }
  auto gram = native::cpu::matmul(a, at);
  auto reg = ops_internal::add(gram, mul_scalar(identity_f64(m), rcond));
  return native::cpu::matmul(at, inverse_impl(reg));
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
  if (op == "split_with_sizes") return "split";
  if (op == "unsafe_split_with_sizes") return "split";
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
    if (op == "_addmm_activation" || op == "_backward" || op == "_fw_primal") return self;
    if (op == "_autocast_to_full_precision") {
      if (self.scalar_type() == ScalarType::Float16 || self.scalar_type() == ScalarType::BFloat16) {
        return self.to(ScalarType::Float32);
      }
      return self;
    }
    if (op == "_autocast_to_reduced_precision") {
      if (self.scalar_type() == ScalarType::Float64 || self.scalar_type() == ScalarType::Float32) {
        return self.to(ScalarType::Float16);
      }
      return self;
    }
    if (op == "_dimI" || op == "_dimV") return i64_scalar(0);
    if (op == "_nested_tensor_size") return i64_vector(self.sizes());
    if (op == "_nested_tensor_storage_offsets") {
      const std::array<std::int64_t, 1> off{{self.storage_offset() / std::max<std::int64_t>(1, self.itemsize())}};
      return i64_vector(std::span<const std::int64_t>(off.data(), off.size()));
    }
    if (op == "_nested_tensor_strides") return i64_vector(self.strides());
    if (op == "_sparse_mask_projection") return self;
    if (op == "chalf") return self.to(ScalarType::Complex64);

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
    if (op == "abs2") return abs2_impl(self);
    if (op == "sign" || op == "sgn") {
      Series pos = ops_internal::gt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      Series neg = ops_internal::lt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      return ops_internal::sub(pos, neg).to(self.scalar_type());
    }

    if (op == "sum") return ops_internal::sum(self);
    if (op == "mean") return ops_internal::mean(self);
    if (op == "median") return quantile_flat_impl(self, 0.5, /*ignore_nan=*/false);
    if (op == "nanmedian") return quantile_flat_impl(self, 0.5, /*ignore_nan=*/true);
    if (op == "mode") return mode_flat_impl(self);
    if (op == "quantile") return quantile_flat_impl(self, 0.5, /*ignore_nan=*/false);
    if (op == "nanquantile") return quantile_flat_impl(self, 0.5, /*ignore_nan=*/true);
    if (op == "nansum") return nan_sum_impl(self);
    if (op == "nanmean") return nan_mean_impl(self);
    if (op == "nanvar") return nan_var_impl(self);
    if (op == "nanstd") return nan_std_impl(self);
    if (op == "skewness") return skewness_impl(self);
    if (op == "kurtosis") return kurtosis_impl(self);
    if (op == "logsumexp") return ops_internal::logsumexp(self, /*dim=*/-1, /*keepdim=*/false);
    if (op == "prod") return ops_internal::prod(self);
    if (op == "amax" || op == "max") return ops_internal::amax(self);
    if (op == "amin" || op == "min") return ops_internal::amin(self);
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
    if (op == "msort") return sort_values_only(self, /*dim=*/0, /*descending=*/false, /*stable=*/false);
    if (op == "tril") return tril_triu_impl(self, /*diagonal=*/0, /*upper=*/false);
    if (op == "triu") return tril_triu_impl(self, /*diagonal=*/0, /*upper=*/true);
    if (op == "flipud") {
      const std::array<std::int64_t, 1> dims{{0}};
      return flip_impl(self, std::span<const std::int64_t>(dims.data(), dims.size()), "flipud");
    }
    if (op == "fliplr") {
      PLUSS_CHECK(self.dim() >= 2, "fliplr: expected input with rank >= 2");
      const std::array<std::int64_t, 1> dims{{1}};
      return flip_impl(self, std::span<const std::int64_t>(dims.data(), dims.size()), "fliplr");
    }
    if (op == "trace") return ops_internal::sum(diag_extract_2d(self, /*diagonal=*/0));
    if (op == "diag") {
      if (self.dim() == 1) return diag_embed_from_1d(self, /*diagonal=*/0);
      return diag_extract_2d(self, /*diagonal=*/0);
    }
    if (op == "diagonal") return diag_extract_2d(self, /*diagonal=*/0);
    if (op == "diagflat") return diag_embed_from_1d(self, /*diagonal=*/0);
    if (op == "diag_embed") return diag_embed_from_1d(self, /*diagonal=*/0);
    if (op == "rot90") return rot90_impl(self, 1, 0, 1);
    if (op == "dct") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Dct);
    if (op == "idct") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Idct);
    if (op == "dst") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Dst);
    if (op == "idst") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Idst);
    if (op == "hilbert") return hilbert_impl(self, /*dim=*/-1);
    if (op == "ediff1d") return ediff1d_impl(self);
    if (op == "movsum") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Sum);
    if (op == "movmean") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Mean);
    if (op == "movmax") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Max);
    if (op == "movmin") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Min);
    if (op == "movstd") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Std);
    if (op == "movvar") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Var);
    if (op == "normr") return norm_along_impl(self, (self.dim() > 1) ? 1 : 0);
    if (op == "normc") return norm_along_impl(self, 0);
    if (op == "center") return centered_impl(self);
    if (op == "standardize") return standardize_impl(self);
    if (op == "pdf") return pdf_impl(self, /*mean=*/0.0, /*std=*/1.0);
    if (op == "cdf") return cdf_impl(self, /*mean=*/0.0, /*std=*/1.0);
    if (op == "det") return det_impl(self);
    if (op == "inverse") return inverse_impl(self);
    if (op == "logdet") return logdet_impl(self);
    if (op == "slogdet") return slogdet_impl(self);
    if (op == "matrix_exp") return matrix_exp_impl(self);
    if (op == "matrix_power") return matrix_power_impl(self, 1);
    if (op == "cholesky") return cholesky_impl(self, /*upper=*/false);
    if (op == "cholesky_inverse") return cholesky_inverse_impl(self, /*upper=*/false);
    if (op == "cov") return covariance_impl(self);
    if (op == "corrcoef") return corrcoef_impl(self);
    if (op == "cummax") return (self.dim() == 0) ? self.clone() : cum_extrema_impl(self, 0, /*is_max=*/true);
    if (op == "cummin") return (self.dim() == 0) ? self.clone() : cum_extrema_impl(self, 0, /*is_max=*/false);
    if (op == "digamma") return digamma_impl(self);
    if (op == "erfinv") return erfinv_impl(self);
    if (op == "i0") return i0_impl(self);
    if (op == "frexp") return frexp_impl(self);
    if (op == "mvlgamma") return mvlgamma_impl(self, 1);
    if (op == "polygamma") return polygamma_impl(self, 1);
    if (op == "multinomial") return multinomial_impl(self, 1, /*replacement=*/true);
    if (op == "histogram") return self.histc(/*bins=*/10, /*min=*/0.0, /*max=*/0.0);
    if (op == "pinverse") return pinverse_impl(self, 1e-12);
    if (op == "q_per_channel_axis" || op == "q_zero_point") return i64_scalar(0);
    if (op == "q_scale") return Series::full({}, 1.0, ScalarType::Float64);
    if (op == "qscheme") return i64_scalar(0);
    if (op == "q_per_channel_scales") return Series::ones({1}, ScalarType::Float64);
    if (op == "q_per_channel_zero_points") return Series::zeros({1}, ScalarType::Int64);
    if (op == "erfcinv") return erfcinv_impl(self);
    if (op == "airy") return airy_impl(self);
    if (op == "zeta") return zeta_impl(self, scalar_like(self, 1.0));
    if (op == "besselj") return bessel_impl(self, /*order=*/0.0, "j");
    if (op == "bessely") return bessel_impl(self, /*order=*/0.0, "y");
    if (op == "besseli") return bessel_impl(self, /*order=*/0.0, "i");
    if (op == "besselk") return bessel_impl(self, /*order=*/0.0, "k");
    if (op == "histcounts") return self.histc(/*bins=*/10, /*min=*/0.0, /*max=*/0.0);
    if (op == "stft" || op == "istft" || op == "geqrf" || op == "orgqr" || op == "ormqr" ||
        op == "qr" || op == "svd") return self;
    if (op == "sinc") return sinc_impl(self);
    if (op == "hardshrink") return hardshrink_impl(self, 0.5);
    if (op == "aminmax") return aminmax_impl(self);
    if (op == "logcumsumexp") return logcumsumexp_impl(self, -1);
    if (op == "unwrap") return unwrap_impl(self, kPi, -1);
    if (op == "cplxpair") return cplxpair_impl(self);
    if (op == "fftshift") return shift_impl(self, {}, /*inverse=*/false);
    if (op == "ifftshift") return shift_impl(self, {}, /*inverse=*/true);
    if (op == "unbind") return self.select(0, 0);
    if (op == "align_to") return self;
    if (op == "bernoulli") return bernoulli_sample_from_probs(self);
    if (op == "t") {
      if (self.dim() != 2) return self;
      return self.transpose(0, 1);
    }

    if (op == "contiguous") return self.contiguous();
    if (op == "clone") return self.clone();
    if (op == "dequantize" || op == "int_repr" || op == "coalesce" || op == "pin_memory" ||
        op == "record_stream" || op == "resolve_conj" || op == "resolve_neg") return self;
    if (op == "to_sparse" || op == "_to_sparse" ||
        op == "to_sparse_csr" || op == "_to_sparse_csr" ||
        op == "to_sparse_csc" || op == "_to_sparse_csc" ||
        op == "to_sparse_bsr" || op == "_to_sparse_bsr" ||
        op == "to_sparse_bsc" || op == "_to_sparse_bsc" ||
        op == "to_mkldnn" || op == "to_padded_tensor" ||
        op == "rename" || op == "refine_names" ||
        op == "set_data" || op == "rename_") return self;
    if (op == "retain_grad") return self;
    if (op == "retains_grad") return bool_scalar(false);
    if (op == "sparse_mask") return self;
    if (op == "smm") return self;
    if (op == "sspaddmm") return self;
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
      if (op == "beta") return beta_impl(self, scalar_like(self, s));
      if (op == "betaln") return betaln_impl(self, scalar_like(self, s));
      if (op == "nextafter") return nextafter_impl(self, s);
      if (op == "logit") return logit_impl(self, s);
      if (op == "hardshrink") return hardshrink_impl(self, s);
      if (op == "heaviside") return heaviside_impl(self, scalar_like(self, s));
      if (op == "gcd") return gcd_impl(self, scalar_like(self, s));
      if (op == "lcm") return lcm_impl(self, scalar_like(self, s));
      if (op == "prelu") return prelu_impl(self, s);
      if (op == "ldexp") return ldexp_impl(self, s);
      if (op == "matrix_power") return matrix_power_impl(self, (std::int64_t)s);
      if (op == "mvlgamma") return mvlgamma_impl(self, (std::int64_t)s);
      if (op == "polygamma") return polygamma_impl(self, (std::int64_t)s);
      if (op == "multinomial") return multinomial_impl(self, (std::int64_t)s, /*replacement=*/false);
      if (op == "cummax") return cum_extrema_impl(self, (std::int64_t)s, /*is_max=*/true);
      if (op == "cummin") return cum_extrema_impl(self, (std::int64_t)s, /*is_max=*/false);
      if (op == "renorm") return renorm_impl(self, s, /*dim=*/0, /*maxnorm=*/1.0);
      if (op == "igamma") return igamma_impl(self, scalar_like(self, s), /*complement=*/false);
      if (op == "igammac") return igamma_impl(self, scalar_like(self, s), /*complement=*/true);
      if (op == "bitwise_left_shift") return bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/true);
      if (op == "bitwise_right_shift") return bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/false);
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
      if (op == "hsplit" || op == "vsplit" || op == "dsplit") return split_family_first(self, op, args[0]);
      if (op == "tril") return tril_triu_impl(self, (std::int64_t)s, /*upper=*/false);
      if (op == "triu") return tril_triu_impl(self, (std::int64_t)s, /*upper=*/true);
      if (op == "flip") {
        const std::array<std::int64_t, 1> dims{{(std::int64_t)s}};
        return flip_impl(self, std::span<const std::int64_t>(dims.data(), dims.size()), "flip");
      }
      if (op == "diff") return diff_impl(self, (std::int64_t)s, /*dim=*/-1);
      if (op == "roll") return roll_flat_impl(self, (std::int64_t)s);
      if (op == "repeat_interleave") return repeat_interleave_impl(self.flatten(0, -1), (std::int64_t)s, 0);
      if (op == "median") return quantile_dim_impl(self, 0.5, (std::int64_t)s, /*keepdim=*/false, /*ignore_nan=*/false);
      if (op == "nanmedian") return quantile_dim_impl(self, 0.5, (std::int64_t)s, /*keepdim=*/false, /*ignore_nan=*/true);
      if (op == "mode") return mode_dim_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "skewness") return skewness_impl(self, (std::int64_t)s);
      if (op == "kurtosis") return kurtosis_impl(self, (std::int64_t)s);
      if (op == "quantile") return quantile_flat_impl(self, s, /*ignore_nan=*/false);
      if (op == "nanquantile") return quantile_flat_impl(self, s, /*ignore_nan=*/true);
      if (op == "dct") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Dct);
      if (op == "idct") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Idct);
      if (op == "dst") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Dst);
      if (op == "idst") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Idst);
      if (op == "hilbert") return hilbert_impl(self, (std::int64_t)s);
      if (op == "movsum") return mov_impl(self, (std::int64_t)s, -1, MovKind::Sum);
      if (op == "movmean") return mov_impl(self, (std::int64_t)s, -1, MovKind::Mean);
      if (op == "movmax") return mov_impl(self, (std::int64_t)s, -1, MovKind::Max);
      if (op == "movmin") return mov_impl(self, (std::int64_t)s, -1, MovKind::Min);
      if (op == "movstd") return mov_impl(self, (std::int64_t)s, -1, MovKind::Std);
      if (op == "movvar") return mov_impl(self, (std::int64_t)s, -1, MovKind::Var);
      if (op == "histcounts") return self.histc((std::int64_t)s, 0.0, 0.0);
      if (op == "center") return centered_impl(self, (std::int64_t)s);
      if (op == "standardize") return standardize_impl(self, (std::int64_t)s);
      if (op == "nanvar") return nan_var_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "nanstd") return nan_std_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "skewness") return skewness_impl(self, (std::int64_t)s);
      if (op == "kurtosis") return kurtosis_impl(self, (std::int64_t)s);
      if (op == "besselj") return bessel_impl(self, s, "j");
      if (op == "bessely") return bessel_impl(self, s, "y");
      if (op == "besseli") return bessel_impl(self, s, "i");
      if (op == "besselk") return bessel_impl(self, s, "k");
      if (op == "pdf") return pdf_impl(self, 0.0, s);
      if (op == "cdf") return cdf_impl(self, 0.0, s);
      if (op == "zeta") return zeta_impl(self, scalar_like(self, s));
      if (op == "unwrap") return unwrap_impl(self, s, -1);
      if (op == "rot90") return rot90_impl(self, (std::int64_t)s, 0, 1);
      if (op == "unbind") return self.select((std::int64_t)s, 0);
      if (op == "diag") {
        if (self.dim() == 1) return diag_embed_from_1d(self, (std::int64_t)s);
        return diag_extract_2d(self, (std::int64_t)s);
      }
      if (op == "diagonal") return diag_extract_2d(self, (std::int64_t)s);
      if (op == "diagflat") return diag_embed_from_1d(self, (std::int64_t)s);
      if (op == "diag_embed") return diag_embed_from_1d(self, (std::int64_t)s);
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "nansum") return nan_sum_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "nanmean") return nan_mean_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "logsumexp") return ops_internal::logsumexp(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "logcumsumexp") return logcumsumexp_impl(self, (std::int64_t)s);
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amax" || op == "max") return ops_internal::amax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amin" || op == "min") return ops_internal::amin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "all") return ops_internal::all(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "any") return ops_internal::any(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "cumsum") return self.cumsum((std::int64_t)s, /*keepdim=*/false);
      if (op == "cumprod") return self.cumprod((std::int64_t)s, /*keepdim=*/false);
      if (op == "bernoulli") return bernoulli_like(self, s);
      if (op == "histogram") return self.histc((std::int64_t)s, 0.0, 0.0);
      return not_impl(op, args, self);
    }

    // bool
    if (std::holds_alternative<bool>(a0)) {
      const bool b = as_bool(args[0]);
      if (op == "unique") return self.unique(b);
      if (op == "cholesky") return cholesky_impl(self, b);
      if (op == "cholesky_inverse") return cholesky_inverse_impl(self, b);
      if (op == "multinomial") return multinomial_impl(self, /*num_samples=*/1, b);
      return not_impl(op, args, self);
    }

    // string
    if (std::holds_alternative<std::string>(a0)) {
      const auto dist = as_string(args[0]);
      if (op == "pdf" || op == "cdf") {
        PLUSS_CHECK(dist == "norm" || dist == "normal" || dist == "gaussian",
                    std::string(op) + ": unsupported distribution '" + std::string(dist) + "'");
        return (op == "pdf") ? pdf_impl(self, 0.0, 1.0) : cdf_impl(self, 0.0, 1.0);
      }
      return not_impl(op, args, self);
    }

    // Series
    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "kron") return kron_impl(self, other);
      if (op == "ldexp") return ldexp_impl(self, other);
      if (op == "prelu") return prelu_impl(self, other);
      if (op == "diagonal_scatter") return diagonal_scatter_impl(self, other, /*diagonal=*/0);
      if (op == "cholesky_solve") return cholesky_solve_impl(self, other, /*upper=*/false);
      if (op == "lu_solve") return native::cpu::matmul(inverse_impl(other), self.to(ScalarType::Float64));
      if (op == "triangular_solve") return triangular_solve_impl(self, other, /*upper=*/true, /*unitriangular=*/false);
      if (op == "igamma") return igamma_impl(self, other, /*complement=*/false);
      if (op == "igammac") return igamma_impl(self, other, /*complement=*/true);
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
      if (op == "bitwise_and") return ops_internal::bitwise_and(self, other);
      if (op == "bitwise_or") return ops_internal::bitwise_or(self, other);
      if (op == "bitwise_xor") return ops_internal::bitwise_xor(self, other);

      if (op == "maximum") return ops_internal::maximum(self, other);
      if (op == "minimum") return ops_internal::minimum(self, other);
      if (op == "clamp_min") return self.clamp_min(other);
      if (op == "clamp_max") return self.clamp_max(other);
      if (op == "nextafter") return nextafter_impl(self, other);
      if (op == "bitwise_left_shift") return bitshift_series_impl(self, other, /*left=*/true);
      if (op == "bitwise_right_shift") return bitshift_series_impl(self, other, /*left=*/false);
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
      if (op == "max") return ops_internal::maximum(self, other);
      if (op == "min") return ops_internal::minimum(self, other);
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
      if (op == "vdot") return vdot_impl(self, other);
      if (op == "cross") return cross_impl(self, other, -1);
      if (op == "dist") return ops_internal::sub(self, other).norm(2.0);
      if (op == "expand_as") return self.expand(other.sizes());
      if (op == "align_as") return self;
      if (op == "heaviside") return heaviside_impl(self, other);
      if (op == "gcd") return gcd_impl(self, other);
      if (op == "lcm") return lcm_impl(self, other);
      if (op == "beta") return beta_impl(self, other);
      if (op == "betaln") return betaln_impl(self, other);
      if (op == "zeta") return zeta_impl(self, other);
      if (op == "cdist" || op == "pdist2") return cdist_impl(self, other, 2.0);
      if (op == "sparse_mask") return ops_internal::where(other.to(ScalarType::Bool), self, scalar_like(self, 0.0));
      if (op == "smm") return native::cpu::matmul(self, other);

      if (op == "masked_select") return ops_internal::masked_select(self, other);
      if (op == "gather") return self.gather(/*dim=*/0, other);
      // note: masked_scatter needs 2 args (mask, src)
      return not_impl(op, args, self);
    }

    // intlist
    if (std::holds_alternative<Arg::IntList>(a0)) {
      auto sizes = as_intlist(args[0]);
      if (op == "_reshape_alias") return self.reshape(sizes);
      if (op == "reshape") return self.reshape(sizes);
      if (op == "view") return self.view(sizes);
      if (op == "expand") return expand_impl(self, sizes);
      if (op == "broadcast_to") return expand_impl(self, sizes);
      if (op == "permute") return self.permute(sizes);
      if (op == "split" || op == "unsafe_split") {
        if (sizes.empty()) return self.clone();
        PLUSS_CHECK(sizes[0] >= 0, "split: split_sizes entries must be >= 0");
        const auto dsz = self.sizes().empty() ? 0 : self.sizes()[0];
        return select_range(self, /*dim=*/0, /*start=*/0, std::min<std::int64_t>(sizes[0], dsz));
      }
      if (op == "split_with_sizes" || op == "unsafe_split_with_sizes") {
        return split_first_with_sizes(self, sizes, /*dim=*/0, "split_with_sizes");
      }
      if (op == "tensor_split") return split_first_by_boundaries(self, sizes, /*dim=*/0, "tensor_split");
      if (op == "hsplit" || op == "vsplit" || op == "dsplit") return split_family_first(self, op, args[0]);
      if (op == "flip") return flip_impl(self, sizes, "flip");
      if (op == "repeat" || op == "tile") return repeat_impl(self, sizes);
      if (op == "roll") {
        if (sizes.empty()) return self.clone();
        return roll_flat_impl(self, sizes[0]);
      }
      if (op == "fftshift") return shift_impl(self, sizes, /*inverse=*/false);
      if (op == "ifftshift") return shift_impl(self, sizes, /*inverse=*/true);
      if (op == "new_empty") return Series::empty(sizes, self.scalar_type());
      if (op == "new_empty_strided") return Series::empty(sizes, self.scalar_type());
      if (op == "new_zeros") return Series::zeros(sizes, self.scalar_type());
      if (op == "new_ones") return Series::ones(sizes, self.scalar_type());
      if (op == "align_to") return self.permute(sizes);
      if (op == "sum_to_size") return sum_to_size_impl(self, sizes);
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
      if (op == "leaky_relu") return ops_internal::leaky_relu(self, x);
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
      if (op == "diff") return diff_impl(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "movsum") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Sum);
      if (op == "movmean") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Mean);
      if (op == "movmax") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Max);
      if (op == "movmin") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Min);
      if (op == "movstd") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Std);
      if (op == "movvar") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Var);
      if (op == "pdf") return pdf_impl(self, x, y);
      if (op == "cdf") return cdf_impl(self, x, y);
      if (op == "unwrap") return unwrap_impl(self, x, (std::int64_t)y);
      if (op == "roll") {
        const std::array<std::int64_t, 1> shifts{{(std::int64_t)x}};
        const std::array<std::int64_t, 1> dims{{(std::int64_t)y}};
        return roll_impl(self,
                         std::span<const std::int64_t>(shifts.data(), shifts.size()),
                         std::span<const std::int64_t>(dims.data(), dims.size()));
      }
      if (op == "repeat_interleave") return repeat_interleave_impl(self, (std::int64_t)x, (std::int64_t)y);
      return not_impl(op, args, self);
    }

    // dim + keepdim(bool)
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<bool>(a1)) {
      const auto dim = as_i64(args[0]);
      const bool keepdim = as_bool(args[1]);
      if (op == "sum") return ops_internal::sum(self, dim, keepdim);
      if (op == "mean") return ops_internal::mean(self, dim, keepdim);
      if (op == "nansum") return nan_sum_impl(self, dim, keepdim);
      if (op == "nanmean") return nan_mean_impl(self, dim, keepdim);
      if (op == "nanvar") return nan_var_impl(self, dim, keepdim);
      if (op == "nanstd") return nan_std_impl(self, dim, keepdim);
      if (op == "logsumexp") return ops_internal::logsumexp(self, dim, keepdim);
      if (op == "prod") return ops_internal::prod(self, dim, keepdim);
      if (op == "amax" || op == "max") return ops_internal::amax(self, dim, keepdim);
      if (op == "amin" || op == "min") return ops_internal::amin(self, dim, keepdim);
      if (op == "all") return ops_internal::all(self, dim, keepdim);
      if (op == "any") return ops_internal::any(self, dim, keepdim);
      if (op == "argmax") return ops_internal::argmax(self, dim, keepdim);
      if (op == "argmin") return ops_internal::argmin(self, dim, keepdim);
      if (op == "argsort") return native::cpu::argsort(self, dim, keepdim, /*stable=*/false);
      if (op == "sort") return sort_values_only(self, dim, keepdim, /*stable=*/false);
      if (op == "cumsum") return self.cumsum(dim, keepdim);
      if (op == "cumprod") return self.cumprod(dim, keepdim);
      if (op == "center") return centered_impl(self, dim);
      if (op == "standardize") return standardize_impl(self, dim);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<bool>(a1)) {
      const Series& other = as_series(args[0]);
      const bool b = as_bool(args[1]);
      if (op == "cholesky_solve") return cholesky_solve_impl(self, other, b);
      if (op == "triangular_solve") return triangular_solve_impl(self, other, b, /*unitriangular=*/false);
      if (op == "lu_solve") return native::cpu::matmul(inverse_impl(other), self.to(ScalarType::Float64));
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
      if (op == "split_with_sizes" || op == "unsafe_split_with_sizes") {
        return split_first_with_sizes(self, sizes, as_i64(args[1]), "split_with_sizes");
      }
      if (op == "tensor_split") return split_first_by_boundaries(self, sizes, as_i64(args[1]), "tensor_split");
      if (op == "new_full") return Series::full(sizes, as_double(args[1]), self.scalar_type());
      if (op == "roll") {
        const std::array<std::int64_t, 1> dim_one{{as_i64(args[1])}};
        return roll_impl(self, sizes, std::span<const std::int64_t>(dim_one.data(), dim_one.size()));
      }
      if (op == "fftshift") return shift_impl(self, sizes, /*inverse=*/false);
      if (op == "ifftshift") return shift_impl(self, sizes, /*inverse=*/true);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<Arg::IntList>(a1)) {
      const auto dim = as_i64(args[0]);
      auto sizes = as_intlist(args[1]);
      if (op == "unflatten") return unflatten_impl(self, dim, sizes);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::IntList>(a0) && std::holds_alternative<bool>(a1)) {
      auto sizes = as_intlist(args[0]);
      if (op == "expand") return expand_impl(self, sizes);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::IntList>(a0) && std::holds_alternative<Arg::IntList>(a1)) {
      auto shifts = as_intlist(args[0]);
      auto dims = as_intlist(args[1]);
      if (op == "as_strided" || op == "_reshape_alias") return as_strided_impl(self, shifts, dims);
      if (op == "new_empty_strided") {
        auto out = Series::empty(shifts, self.scalar_type());
        return Series::from_storage(out.storage(), /*storage_offset=*/0,
                                    std::vector<std::int64_t>(shifts.begin(), shifts.end()),
                                    std::vector<std::int64_t>(dims.begin(), dims.end()),
                                    self.scalar_type());
      }
      if (op == "roll") return roll_impl(self, shifts, dims);
      return not_impl(op, args, self);
    }

    // Series + scalar
    if (std::holds_alternative<const Series*>(a0) && is_numeric_scalar_arg(args[1])) {
      const Series& other = as_series(args[0]);
      const double s = as_double(args[1]);
      if (op == "where") return self.where(other, s);
      if (op == "diagonal_scatter") return diagonal_scatter_impl(self, other, (std::int64_t)s);
      if (op == "cholesky_solve") return cholesky_solve_impl(self, other, s != 0.0);
      if (op == "triangular_solve") return triangular_solve_impl(self, other, s != 0.0, /*unitriangular=*/false);
      if (op == "lu_solve") return native::cpu::matmul(inverse_impl(other), self.to(ScalarType::Float64));
      if (op == "lerp") return ops_internal::add(self, mul_scalar(ops_internal::sub(other, self), s));
      if (op == "take_along_dim") return self.gather((std::int64_t)s, other);
      if (op == "masked_fill") return ops_internal::masked_fill(self, other, s);
      if (op == "beta") return beta_impl(self, scalar_like(self, s));
      if (op == "betaln") return betaln_impl(self, scalar_like(self, s));
      if (op == "zeta") return zeta_impl(self, scalar_like(self, s));
      if (op == "cdist" || op == "pdist2") return cdist_impl(self, other, s);
      if (op == "hardshrink_backward") return hardshrink_backward_impl(self, other, s);
      if (op == "cross") return cross_impl(self, other, (std::int64_t)s);
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
  if (op == "scatter") return self;
  if (op == "scatter_add") return self;
  return not_impl(op, args, self);
}
// Series + Series
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& m = as_series(args[0]);
      const Series& s = as_series(args[1]);
      if (op == "where") return self.where(m, s);
      if (op == "masked_fill") return ops_internal::masked_fill(self, m, s);
      if (op == "cdist" || op == "pdist2") return cdist_impl(self, m, 2.0);
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
      if (op == "scatter") {
        return native::cpu::scatter(self, dim, index, src);
      }
      if (op == "scatter_add") {
        return ops_internal::scatter_add(self, dim, index, src);
      }
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
      if (op == "renorm") return renorm_impl(self, as_double(args[0]), as_i64(args[1]), as_double(args[2]));
      if (op == "unfold") return unfold_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]));
      if (op == "rot90") return rot90_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]));
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
      if (op == "histcounts") return self.histc(bins, mn, mx);
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
      if (op == "select") {
        // select keeps one index along dim and drops that axis.
        return select_range(self, wd, st, 1).squeeze(wd);
      }
      if (op == "slice_inverse") return select_range(self, wd, st, std::min<std::int64_t>(len, dsz - st));
    }

    if (std::holds_alternative<Arg::IntList>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const auto sizes = as_intlist(args[0]);
      const auto strides = as_intlist(args[1]);
      const auto off = as_i64(args[2]);
      if (op == "as_strided" || op == "_reshape_alias") return as_strided_impl(self, sizes, strides, off);
      return not_impl(op, args, self);
    }

    // addcmul(self, tensor1, tensor2, value)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      const Series& other = as_series(args[0]);
      const double rtol = as_double(args[1]);
      const double atol = as_double(args[2]);
      if (op == "select_scatter") return select_scatter_impl(self, other, as_i64(args[1]), as_i64(args[2]));
      if (op == "slice_scatter") return slice_scatter_impl(self, other, as_i64(args[1]), as_i64(args[2]), self.sizes()[(std::size_t)wrap_dim(as_i64(args[1]), self.dim(), "slice_scatter")], 1);
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
      if (op == "triangular_solve") return triangular_solve_impl(self, index, /*upper=*/true, /*unitriangular=*/accumulate);
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        std::holds_alternative<Arg::IntList>(args[2].v)) {
      const auto& src = as_series(args[0]);
      const auto sizes = as_intlist(args[1]);
      const auto strides = as_intlist(args[2]);
      if (op == "as_strided_scatter") return as_strided_scatter_impl(self, src, sizes, strides,
                                                                      self.storage_offset() / std::max<std::int64_t>(1, self.itemsize()));
    }

    return not_impl(op, args, self);
  }

  // 4-arg
  if (args.size() == 4) {
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        std::holds_alternative<Arg::IntList>(args[2].v) &&
        is_numeric_scalar_arg(args[3])) {
      const auto& src = as_series(args[0]);
      const auto sizes = as_intlist(args[1]);
      const auto strides = as_intlist(args[2]);
      const auto off = as_i64(args[3]);
      if (op == "as_strided_scatter") return as_strided_scatter_impl(self, src, sizes, strides, off);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      const Series& src = as_series(args[0]);
      if (op == "slice_scatter") {
        return slice_scatter_impl(self, src, as_i64(args[1]), as_i64(args[2]), as_i64(args[3]), /*step=*/1);
      }
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<bool>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v) &&
        std::holds_alternative<bool>(args[3].v)) {
      const Series& other = as_series(args[0]);
      const bool upper = as_bool(args[1]);
      const bool transpose = as_bool(args[2]);
      const bool unitriangular = as_bool(args[3]);
      if (op == "triangular_solve") {
        auto a = transpose ? other.transpose(0, 1) : other;
        return triangular_solve_impl(self, a, upper, unitriangular);
      }
    }

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
      if (op == "scatter_reduce") {
        return ops_internal::scatter_reduce(self, dim, index, src, std::string(reduce), /*include_self=*/true);
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
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3]) &&
        is_numeric_scalar_arg(args[4])) {
      const Series& src = as_series(args[0]);
      if (op == "slice_scatter") {
        return slice_scatter_impl(self, src, as_i64(args[1]), as_i64(args[2]), as_i64(args[3]), as_i64(args[4]));
      }
      return not_impl(op, args, self);
    }

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
      if (op == "scatter_reduce") {
        return ops_internal::scatter_reduce(self, dim, index, src, std::string(reduce), include_self);
      }
    }
    return not_impl(op, args, self);
  }

  return not_impl(op, args, self);
}

Series& invoke_any_(std::string_view op, Series& self, const Args& args) {
  op = canonical_inplace_op(op);

  if (args.empty()) {
    if (op == "_coalesced_" || op == "sparse_resize_" || op == "resize_as_sparse_") return self;
    if (op == "sparse_resize_and_clear_") {
      self = Series::zeros(self.sizes(), self.scalar_type());
      return self;
    }
    if (op == "digamma_") { self = digamma_impl(self).to(self.scalar_type()); return self; }
    if (op == "erfinv_") { self = erfinv_impl(self).to(self.scalar_type()); return self; }
    if (op == "i0_") { self = i0_impl(self).to(self.scalar_type()); return self; }

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
    if (op == "erfcinv_") { self = erfcinv_impl(self).to(self.scalar_type()); return self; }

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
    if (op == "rename_" || op == "set_" || op == "requires_grad_") return self;
    if (op == "abs_") { self = ops_internal::abs(self); return self; }
    if (op == "bitwise_not_") return ops_internal::bitwise_not_(self);
    if (op == "squeeze_") { self = ops_internal::squeeze(self); return self; }
    if (op == "t_") {
      if (self.dim() == 2) self = self.transpose(0, 1);
      return self;
    }
    if (op == "tril_") { self = tril_triu_impl(self, /*diagonal=*/0, /*upper=*/false); return self; }
    if (op == "triu_") { self = tril_triu_impl(self, /*diagonal=*/0, /*upper=*/true); return self; }
    if (op == "zero_") { self = Series::zeros(self.sizes(), self.scalar_type()); return self; }
    if (op == "uniform_") { self = uniform_like(self, 0.0, 1.0); return self; }
    if (op == "normal_") { self = normal_like(self, 0.0, 1.0); return self; }
    if (op == "log_normal_") { self = ops_internal::exp(normal_like(self, 1.0, 2.0)); return self; }
    if (op == "exponential_") { self = exponential_like(self, 1.0); return self; }
    if (op == "geometric_") { self = geometric_like(self, 0.5); return self; }
    if (op == "cauchy_") { self = cauchy_like(self, 0.0, 1.0); return self; }
    if (op == "bernoulli_") { self = bernoulli_sample_from_probs(self).to(self.scalar_type()); return self; }
    if (op == "sinc_") { self = sinc_impl(self).to(self.scalar_type()); return self; }
    if (op == "random_") {
      if (is_floating_dtype(self.scalar_type()) || is_complex_dtype(self.scalar_type())) {
        self = uniform_like(self, 0.0, 1.0);
      } else {
        self = random_like(self, 0.0, 2147483647.0);
      }
      return self;
    }

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
      if (op == "heaviside_") { self = heaviside_impl(self, scalar_like(self, s)); return self; }
      if (op == "gcd_") { self = gcd_impl(self, scalar_like(self, s)); return self; }
      if (op == "lcm_") { self = lcm_impl(self, scalar_like(self, s)); return self; }
      if (op == "logit_") { self = logit_impl(self, s).to(self.scalar_type()); return self; }
      if (op == "nextafter_") { self = nextafter_impl(self, s).to(self.scalar_type()); return self; }
      if (op == "ldexp_") { self = ldexp_impl(self, s).to(self.scalar_type()); return self; }
      if (op == "mvlgamma_") { self = mvlgamma_impl(self, (std::int64_t)s).to(self.scalar_type()); return self; }
      if (op == "polygamma_") { self = polygamma_impl(self, (std::int64_t)s).to(self.scalar_type()); return self; }
      if (op == "renorm_") { self = renorm_impl(self, s, /*dim=*/0, /*maxnorm=*/1.0).to(self.scalar_type()); return self; }
      if (op == "bitwise_left_shift_") { self = bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/true); return self; }
      if (op == "bitwise_right_shift_") { self = bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/false); return self; }
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
      if (op == "tril_") { self = tril_triu_impl(self, (std::int64_t)s, /*upper=*/false); return self; }
      if (op == "triu_") { self = tril_triu_impl(self, (std::int64_t)s, /*upper=*/true); return self; }
      if (op == "fill_diagonal_") return fill_diagonal_impl(self, s, /*wrap=*/false);
      if (op == "cumsum_") return self.cumsum_((std::int64_t)s, /*keepdim=*/false);
      if (op == "cumprod_") return self.cumprod_((std::int64_t)s, /*keepdim=*/false);
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, s, 0.0, 0.0);
      if (op == "bernoulli_") { self = bernoulli_like(self, s); return self; }
      if (op == "normal_") { self = normal_like(self, s, 1.0); return self; }
      if (op == "log_normal_") { self = ops_internal::exp(normal_like(self, s, 1.0)); return self; }
      if (op == "exponential_") { self = exponential_like(self, s); return self; }
      if (op == "geometric_") { self = geometric_like(self, s); return self; }
      if (op == "cauchy_") { self = cauchy_like(self, s, 1.0); return self; }
      if (op == "random_") { self = random_like(self, 0.0, s); return self; }
      return not_impl_inplace(op, args, self);
    }

    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "ldexp_") { self = ldexp_impl(self, other).to(self.scalar_type()); return self; }
      if (op == "igamma_") { self = igamma_impl(self, other, /*complement=*/false).to(self.scalar_type()); return self; }
      if (op == "igammac_") { self = igamma_impl(self, other, /*complement=*/true).to(self.scalar_type()); return self; }
      if (op == "resize_as_") { self = Series::empty(other.sizes(), self.scalar_type()); return self; }
      if (op == "resize_as_sparse_") { self = Series::empty(other.sizes(), self.scalar_type()); return self; }
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
      if (op == "bitwise_left_shift_") { self = bitshift_series_impl(self, other, /*left=*/true); return self; }
      if (op == "bitwise_right_shift_") { self = bitshift_series_impl(self, other, /*left=*/false); return self; }
      if (op == "nextafter_") { self = nextafter_impl(self, other).to(self.scalar_type()); return self; }
      if (op == "heaviside_") { self = heaviside_impl(self, other); return self; }
      if (op == "gcd_") { self = gcd_impl(self, other); return self; }
      if (op == "lcm_") { self = lcm_impl(self, other); return self; }
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
      if (op == "set_" || op == "set_data") {
        self = (other.scalar_type() == self.scalar_type()) ? other : other.to(self.scalar_type());
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<bool>(a0)) {
      if (op == "requires_grad_") return self;
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::IntList>(a0)) {
      auto sizes = as_intlist(args[0]);
      if (op == "resize_") { self = Series::empty(sizes, self.scalar_type()); return self; }
      if (op == "as_strided_") {
        self = as_strided_impl(self, sizes, contiguous_strides(sizes));
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
      if (op == "uniform_") { self = uniform_like(self, x, y); return self; }
      if (op == "normal_") { self = normal_like(self, x, y); return self; }
      if (op == "log_normal_") { self = ops_internal::exp(normal_like(self, x, y)); return self; }
      if (op == "cauchy_") { self = cauchy_like(self, x, y); return self; }
      if (op == "random_") { self = random_like(self, x, y); return self; }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<bool>(a1)) {
      const auto dim = as_i64(args[0]);
      const bool keepdim = as_bool(args[1]);
      if (op == "fill_diagonal_") return fill_diagonal_impl(self, as_double(args[0]), keepdim);
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
      if (op == "masked_fill_") return ops_internal::masked_fill_(self, other, s);
      if (op == "index_fill_") {
        PLUSS_CHECK(self.dim() == 1, "index_fill_: currently implemented for 1D");
        self = native::cpu::put(self, other.to(ScalarType::Int64), Series::full({other.numel()}, s, self.scalar_type()), /*accumulate=*/false);
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
      if (op == "masked_fill_") return ops_internal::masked_fill_(self, b, c);
      if (op == "index_put_") { self = native::cpu::put(self, b, c, /*accumulate=*/false); return self; }
      if (op == "put_") { self = native::cpu::put(self, b, c, /*accumulate=*/false); return self; }
      if (op == "lerp_") { self = ops_internal::add(self, ops_internal::mul(ops_internal::sub(b, self), c)); return self; }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::IntList>(a0) && std::holds_alternative<Arg::IntList>(a1)) {
      auto sizes = as_intlist(args[0]);
      auto strides = as_intlist(args[1]);
      if (op == "as_strided_") { self = as_strided_impl(self, sizes, strides); return self; }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 3) {
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1]) && is_numeric_scalar_arg(args[2])) {
      if (op == "renorm_") { self = renorm_impl(self, as_double(args[0]), as_i64(args[1]), as_double(args[2])).to(self.scalar_type()); return self; }
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, as_double(args[0]), as_double(args[1]), as_double(args[2]));
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::IntList>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      if (op == "as_strided_") {
        self = as_strided_impl(self, as_intlist(args[0]), as_intlist(args[1]), as_i64(args[2]));
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const Series& index = as_series(args[1]);
      const Series& src = as_series(args[2]);
      if (op == "scatter_") {
        native::cpu::scatter_(self, dim, index, src);
        return self;
      }
      if (op == "scatter_add_") return ops_internal::scatter_add_(self, dim, index, src);
      if (op == "index_copy_") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_copy_: currently implemented for 1D dim=0");
        self = native::cpu::put(self, index.to(ScalarType::Int64), src.to(self.scalar_type()), /*accumulate=*/false);
        return self;
      }
      if (op == "index_add_") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_add_: currently implemented for 1D dim=0");
        self = native::cpu::put(self, index.to(ScalarType::Int64), src.to(self.scalar_type()), /*accumulate=*/true);
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const auto dim = as_i64(args[0]);
      const Series& index = as_series(args[1]);
      const double value = as_double(args[2]);
      if (op == "index_fill_") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_fill_: currently implemented for 1D dim=0");
        self = native::cpu::put(self, index.to(ScalarType::Int64), Series::full({index.numel()}, value, self.scalar_type()), /*accumulate=*/false);
        return self;
      }
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
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        is_numeric_scalar_arg(args[3])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const double alpha = as_double(args[3]);
      if (op == "index_add_") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_add_: currently implemented for 1D dim=0");
        if (alpha != 1.0) src = mul_scalar(src, alpha).to(self.scalar_type());
        self = native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/true);
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      if (op == "index_reduce_") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_reduce_: currently implemented for 1D dim=0");
        PLUSS_CHECK(reduce == "sum", "index_reduce_: only 'sum' is implemented");
        self = native::cpu::put(self, index.to(ScalarType::Int64), src, /*accumulate=*/true);
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      const auto& src = as_series(args[2]);
      const auto reduce = as_string(args[3]);
      if (op == "scatter_reduce_") {
        return ops_internal::scatter_reduce_(self, dim, index, src, std::string(reduce), /*include_self=*/true);
      }
      return not_impl_inplace(op, args, self);
    }
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

  if (args.size() == 5) {
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v) &&
        std::holds_alternative<bool>(args[4].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      const auto& src = as_series(args[2]);
      const auto reduce = as_string(args[3]);
      const bool include_self = as_bool(args[4]);
      if (op == "scatter_reduce_") {
        return ops_internal::scatter_reduce_(self, dim, index, src, std::string(reduce), include_self);
      }
      if (op == "index_reduce_") {
        PLUSS_CHECK(self.dim() == 1 && dim == 0, "index_reduce_: currently implemented for 1D dim=0");
        PLUSS_CHECK(reduce == "sum", "index_reduce_: only 'sum' is implemented");
        if (!include_self) {
          auto zeros = Series::zeros({index.numel()}, self.scalar_type());
          self = native::cpu::put(self, index.to(ScalarType::Int64), zeros, /*accumulate=*/false);
        }
        self = native::cpu::put(self, index.to(ScalarType::Int64), src.to(self.scalar_type()), /*accumulate=*/true);
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  return not_impl_inplace(op, args, self);
}

} // namespace pluss::codem::_compat
