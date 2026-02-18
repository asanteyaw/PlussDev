#include "CodeM/native/cpu/SignalDistanceKernels.h"

#include "CodeM/core/Complex.h"
#include "CodeM/core/Errors.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace pluss::codem::native::cpu {

namespace {

constexpr double kPi = 3.14159265358979323846;

inline std::int64_t wrap_dim_local(std::int64_t dim, std::int64_t nd, const char* name) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, std::string(name) + ": dim out of range");
  return dim;
}

inline bool is_pow2(std::int64_t n) {
  return n > 0 && (n & (n - 1)) == 0;
}

void fft_radix2(std::vector<Complex128>& a, bool inverse) {
  const auto n = static_cast<std::int64_t>(a.size());
  for (std::int64_t i = 1, j = 0; i < n; ++i) {
    std::int64_t bit = n >> 1;
    while (j & bit) {
      j ^= bit;
      bit >>= 1;
    }
    j ^= bit;
    if (i < j) std::swap(a[(std::size_t)i], a[(std::size_t)j]);
  }

  for (std::int64_t len = 2; len <= n; len <<= 1) {
    const double ang = (inverse ? 2.0 : -2.0) * kPi / static_cast<double>(len);
    const Complex128 wlen(std::cos(ang), std::sin(ang));
    for (std::int64_t i = 0; i < n; i += len) {
      Complex128 w(1.0, 0.0);
      const auto half = len >> 1;
      for (std::int64_t j = 0; j < half; ++j) {
        const Complex128 u = a[(std::size_t)(i + j)];
        const Complex128 v = a[(std::size_t)(i + j + half)] * w;
        a[(std::size_t)(i + j)] = u + v;
        a[(std::size_t)(i + j + half)] = u - v;
        w *= wlen;
      }
    }
  }

  if (inverse) {
    const double inv_n = 1.0 / static_cast<double>(n);
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i = 0; i < n; ++i) {
      a[(std::size_t)i] *= inv_n;
    }
  }
}

void dft_fallback(std::vector<Complex128>& a, bool inverse) {
  const auto n = static_cast<std::int64_t>(a.size());
  std::vector<Complex128> src = a;
  const double sign = inverse ? 1.0 : -1.0;
  for (std::int64_t k = 0; k < n; ++k) {
    Complex128 acc(0.0, 0.0);
    for (std::int64_t t = 0; t < n; ++t) {
      const double ang = sign * 2.0 * kPi * static_cast<double>(k * t) / static_cast<double>(n);
      acc += src[(std::size_t)t] * Complex128(std::cos(ang), std::sin(ang));
    }
    a[(std::size_t)k] = acc;
  }
  if (inverse) {
    const double inv_n = 1.0 / static_cast<double>(n);
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i = 0; i < n; ++i) a[(std::size_t)i] *= inv_n;
  }
}

inline void fft_any(std::vector<Complex128>& a, bool inverse) {
  if (is_pow2(static_cast<std::int64_t>(a.size()))) fft_radix2(a, inverse);
  else dft_fallback(a, inverse);
}

Series trig_transform_fallback(const Series& x, std::int64_t dim, TrigTransformKind kind) {
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());

  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];

  std::vector<double> table((std::size_t)(n * n), 0.0);
  for (std::int64_t k = 0; k < n; ++k) {
    for (std::int64_t t = 0; t < n; ++t) {
      const double td = static_cast<double>(t);
      const double kd = static_cast<double>(k);
      if (kind == TrigTransformKind::Dct) {
        table[(std::size_t)(k * n + t)] = std::cos(kPi * (td + 0.5) * kd / static_cast<double>(n));
      } else if (kind == TrigTransformKind::Dst) {
        table[(std::size_t)(k * n + t)] = std::sin(kPi * (td + 0.5) * (kd + 1.0) / static_cast<double>(n));
      } else if (kind == TrigTransformKind::Idct) {
        if (t == 0) table[(std::size_t)(k * n + t)] = 0.5;
        else table[(std::size_t)(k * n + t)] = std::cos(kPi * td * (kd + 0.5) / static_cast<double>(n));
      } else {
        table[(std::size_t)(k * n + t)] = std::sin(kPi * (td + 1.0) * (kd + 0.5) / static_cast<double>(n));
      }
    }
  }

  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      for (std::int64_t k = 0; k < n; ++k) {
        const double* row = table.data() + (k * n);
        double acc = 0.0;
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
        for (std::int64_t t = 0; t < n; ++t) {
          const auto off = (o * n + t) * inner + i;
          acc += ip[off] * row[t];
        }
        if (kind == TrigTransformKind::Idct || kind == TrigTransformKind::Idst) {
          acc *= 2.0 / static_cast<double>(n);
        }
        op[(o * n + k) * inner + i] = acc;
      }
    }
  }
  return out;
}

} // namespace

Series cdist_kernel(const Series& a, const Series& b, double p) {
  PLUSS_CHECK(a.dim() == 2 && b.dim() == 2, "cdist/pdist2: expected 2D inputs");
  PLUSS_CHECK(a.sizes()[1] == b.sizes()[1], "cdist/pdist2: feature dimension mismatch");

  auto aa = a.to(ScalarType::Float64).contiguous();
  auto bb = b.to(ScalarType::Float64).contiguous();
  const auto m = aa.sizes()[0];
  const auto n = bb.sizes()[0];
  const auto d = aa.sizes()[1];

  auto out = Series::empty({m, n}, ScalarType::Float64);
  auto* op = reinterpret_cast<double*>(out.raw_data());

  if (p == 2.0) {
    const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
    const auto* bp = reinterpret_cast<const double*>(bb.raw_data());
    for (std::int64_t i = 0; i < m; ++i) {
      const auto* ar = ap + i * d;
      for (std::int64_t j = 0; j < n; ++j) {
        const auto* br = bp + j * d;
        double acc = 0.0;
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
        for (std::int64_t k = 0; k < d; ++k) {
          const double dv = ar[k] - br[k];
          acc += dv * dv;
        }
        op[i * n + j] = std::sqrt(acc);
      }
    }
    return out;
  }

  const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const double*>(bb.raw_data());
  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t j = 0; j < n; ++j) {
      double acc = 0.0;
      double mx = 0.0;
      const auto* ar = ap + i * d;
      const auto* br = bp + j * d;
      if (std::isinf(p)) {
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
        for (std::int64_t k = 0; k < d; ++k) mx = std::max(mx, std::abs(ar[k] - br[k]));
        op[i * n + j] = mx;
      } else if (p == 1.0) {
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
        for (std::int64_t k = 0; k < d; ++k) acc += std::abs(ar[k] - br[k]);
        op[i * n + j] = acc;
      } else {
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
        for (std::int64_t k = 0; k < d; ++k) acc += std::pow(std::abs(ar[k] - br[k]), p);
        op[i * n + j] = std::pow(acc, 1.0 / p);
      }
    }
  }
  return out;
}

Series trig_transform(const Series& self, std::int64_t dim, TrigTransformKind kind) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim_local(dim, x.dim(), "dct/idct/dst/idst");
  const auto n = x.sizes()[(std::size_t)dim];

  if (n <= 1 || !is_pow2(2 * n)) {
    return trig_transform_fallback(x, dim, kind);
  }

  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  const auto nd = x.dim();
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];

  const auto fft_n = 2 * n;
  std::vector<Complex128> buf((std::size_t)fft_n);
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      if (kind == TrigTransformKind::Dct) {
        for (std::int64_t t = 0; t < n; ++t) {
          const double v = ip[(o * n + t) * inner + i];
          buf[(std::size_t)t] = Complex128(v, 0.0);
          buf[(std::size_t)(fft_n - 1 - t)] = Complex128(v, 0.0);
        }
        fft_any(buf, /*inverse=*/false);
        for (std::int64_t k = 0; k < n; ++k) {
          const double theta = kPi * static_cast<double>(k) / static_cast<double>(fft_n);
          const Complex128 phase(std::cos(theta), -std::sin(theta));
          const double v = 0.5 * std::real(buf[(std::size_t)k] * phase);
          op[(o * n + k) * inner + i] = v;
        }
      } else if (kind == TrigTransformKind::Idct) {
        std::fill(buf.begin(), buf.end(), Complex128(0.0, 0.0));
        for (std::int64_t k = 0; k < n; ++k) {
          const double theta = kPi * static_cast<double>(k) / static_cast<double>(fft_n);
          const Complex128 phase(std::cos(theta), std::sin(theta));
          const double xk = ip[(o * n + k) * inner + i];
          buf[(std::size_t)k] = phase * (2.0 * xk);
        }
        for (std::int64_t k = 1; k < n; ++k) {
          buf[(std::size_t)(fft_n - k)] = std::conj(buf[(std::size_t)k]);
        }
        fft_any(buf, /*inverse=*/true);
        for (std::int64_t t = 0; t < n; ++t) {
          op[(o * n + t) * inner + i] = std::real(buf[(std::size_t)t]);
        }
      } else if (kind == TrigTransformKind::Dst) {
        for (std::int64_t t = 0; t < n; ++t) {
          const double v = ip[(o * n + t) * inner + i];
          buf[(std::size_t)t] = Complex128(v, 0.0);
          buf[(std::size_t)(fft_n - 1 - t)] = Complex128(-v, 0.0);
        }
        fft_any(buf, /*inverse=*/false);
        for (std::int64_t k = 0; k < n; ++k) {
          const std::int64_t m = k + 1;
          const double theta = kPi * static_cast<double>(m) / static_cast<double>(fft_n);
          const Complex128 phase(std::cos(theta), -std::sin(theta));
          const double v = -0.5 * std::imag(buf[(std::size_t)m] * phase);
          op[(o * n + k) * inner + i] = v;
        }
      } else {
        std::fill(buf.begin(), buf.end(), Complex128(0.0, 0.0));
        for (std::int64_t m = 1; m <= n; ++m) {
          const double theta = kPi * static_cast<double>(m) / static_cast<double>(fft_n);
          const Complex128 phase(std::cos(theta), std::sin(theta));
          const double xm = ip[(o * n + (m - 1)) * inner + i];
          buf[(std::size_t)m] = phase * Complex128(0.0, -2.0 * xm);
        }
        for (std::int64_t m = 1; m < n; ++m) {
          buf[(std::size_t)(fft_n - m)] = std::conj(buf[(std::size_t)m]);
        }
        fft_any(buf, /*inverse=*/true);
        for (std::int64_t t = 0; t < n; ++t) {
          op[(o * n + t) * inner + i] = std::real(buf[(std::size_t)t]);
        }
      }
    }
  }
  return out;
}

Series hilbert_transform(const Series& self, std::int64_t dim) {
  Series x = [&]() -> Series {
    if (self.scalar_type() == ScalarType::Complex128) {
      return self.contiguous();
    }
    auto out = Series::empty(self.sizes(), ScalarType::Complex128);
    if (self.scalar_type() == ScalarType::Complex64) {
      auto in = self.contiguous();
      const auto n = in.numel();
      const auto* ip = reinterpret_cast<const Complex64*>(in.raw_data());
      auto* op = reinterpret_cast<Complex128*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i = 0; i < n; ++i) {
        op[i] = Complex128(static_cast<double>(std::real(ip[i])), static_cast<double>(std::imag(ip[i])));
      }
      return out;
    }
    auto r = self.to(ScalarType::Float64).contiguous();
    const auto n = r.numel();
    const auto* rp = reinterpret_cast<const double*>(r.raw_data());
    auto* op = reinterpret_cast<Complex128*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i = 0; i < n; ++i) op[i] = Complex128(rp[i], 0.0);
    return out;
  }();

  dim = wrap_dim_local(dim, x.dim(), "hilbert");
  auto out = Series::empty(x.sizes(), ScalarType::Complex128);
  const auto* ip = reinterpret_cast<const Complex128*>(x.raw_data());
  auto* op = reinterpret_cast<Complex128*>(out.raw_data());
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];

  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];

  std::vector<Complex128> spec((std::size_t)n);
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      for (std::int64_t t = 0; t < n; ++t) {
        spec[(std::size_t)t] = ip[(o * n + t) * inner + i];
      }
      fft_any(spec, /*inverse=*/false);
      for (std::int64_t k = 0; k < n; ++k) {
        double h = 0.0;
        if (k == 0) h = 1.0;
        else if (n % 2 == 0 && k == n / 2) h = 1.0;
        else if (k < (n + 1) / 2) h = 2.0;
        spec[(std::size_t)k] *= h;
      }
      fft_any(spec, /*inverse=*/true);
      for (std::int64_t t = 0; t < n; ++t) {
        op[(o * n + t) * inner + i] = spec[(std::size_t)t];
      }
    }
  }
  return out;
}

} // namespace pluss::codem::native::cpu
