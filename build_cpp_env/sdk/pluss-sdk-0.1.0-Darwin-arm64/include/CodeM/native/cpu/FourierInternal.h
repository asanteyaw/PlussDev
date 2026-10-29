#pragma once

#include "CodeM/core/Errors.h"
#include "MSharp/Parallel.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <numbers>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::codem::native::cpu::fourier_detail {

inline bool is_power_of_two(std::int64_t n) noexcept {
  return n > 0 && (n & (n - 1)) == 0;
}

inline std::int64_t next_power_of_two(std::int64_t n) {
  PLUSS_CHECK(n > 0, "fft: transform length must be positive");
  std::int64_t out = 1;
  while (out < n) {
    PLUSS_CHECK(out <= std::numeric_limits<std::int64_t>::max() / 2,
                "fft: transform length is too large");
    out <<= 1;
  }
  return out;
}

// exp(sign * i * pi * a / b) for integers a >= 0, b > 0 and sign = +1 or -1.
// The angle is reduced to the first octant with integer arithmetic and the
// other octants are obtained by exact symmetries, so that multiples of pi / 2
// give exactly 0 and +-1 (not 6e-17) and symmetric angles give bit-identical
// magnitudes; the one trigonometric evaluation per factor is rounded once to
// Real.
template <class Real>
std::complex<Real> unit_root(std::uint64_t a, std::uint64_t b, bool negative) {
  a %= 2 * b;
  const std::uint64_t doubled = 2 * a;
  const std::uint64_t quadrant = doubled / b; // multiples of pi / 2
  const std::uint64_t remainder =
      doubled -
      quadrant * b; // angle = quadrant * pi / 2 + pi * remainder / (2 b)
  long double x;
  long double y;
  const long double unit =
      std::numbers::pi_v<long double> / (2.0L * static_cast<long double>(b));
  if (2 * remainder >
      b) { // beyond pi / 4 inside the quadrant: use the complementary angle
    const long double c = unit * static_cast<long double>(b - remainder);
    x = std::sin(c);
    y = std::cos(c);
  } else {
    const long double c = unit * static_cast<long double>(remainder);
    x = std::cos(c);
    y = std::sin(c);
  }
  long double re = x;
  long double im = y;
  switch (quadrant & 3) {
  case 1:
    re = -y;
    im = x;
    break;
  case 2:
    re = -x;
    im = -y;
    break;
  case 3:
    re = y;
    im = -x;
    break;
  default:
    break;
  }
  return std::complex<Real>(static_cast<Real>(re),
                            static_cast<Real>(negative ? -im : im));
}

// Bounded, thread-safe cache of immutable per-length data (twiddle tables,
// chirp plans). FFT-heavy workflows reuse lengths heavily, while unbounded
// user-controlled lengths must not grow process memory without limit.
template <class Value> class PlanCache final {
public:
  explicit PlanCache(std::size_t capacity) : capacity_(capacity) {}

  template <class Make>
  std::shared_ptr<const Value> get(std::uint64_t key, Make &&make) {
    {
      std::lock_guard lock(mutex_);
      if (const auto found = plans_.find(key); found != plans_.end())
        return found->second;
    }
    std::shared_ptr<const Value> created = make();
    std::lock_guard lock(mutex_);
    if (const auto found = plans_.find(key); found != plans_.end())
      return found->second;
    if (order_.size() == capacity_) {
      plans_.erase(order_.front());
      order_.erase(order_.begin());
    }
    order_.push_back(key);
    plans_.emplace(key, created);
    return created;
  }

private:
  std::mutex mutex_{};
  std::size_t capacity_;
  std::unordered_map<std::uint64_t, std::shared_ptr<const Value>> plans_{};
  std::vector<std::uint64_t> order_{};
};

// Complex product without the NaN-recovery slow path of
// std::complex::operator*, which keeps the butterflies inlined and
// vectorizable.
template <class Real>
inline std::complex<Real> cmul(std::complex<Real> a,
                               std::complex<Real> b) noexcept {
  return {a.real() * b.real() - a.imag() * b.imag(),
          a.real() * b.imag() + a.imag() * b.real()};
}

template <class Real>
inline std::complex<Real>
cmul_conj(std::complex<Real> a, std::complex<Real> b) noexcept { // a * conj(b)
  return {a.real() * b.real() + a.imag() * b.imag(),
          a.imag() * b.real() - a.real() * b.imag()};
}

// Immutable data of a power-of-two transform. The transform is a Stockham
// autosort FFT built from radix-4 stages (plus one radix-2 stage when log2(n)
// is odd): no bit-reversal pass, and every stage reads and writes contiguous
// runs. Each stage owns a contiguous table of its twiddles exp(-2 pi i j p /
// L), each taken from one directly evaluated root table (never by repeated
// multiplication, whose rounding error grows with the number of steps).
template <class Real> struct Stockham final {
  struct Stage {
    std::int64_t length{}; // length of the sequences that remain at this stage
    bool radix4{};
    std::vector<std::complex<Real>>
        twiddle{}; // radix-4: w^p, w^2p, w^3p for p < length / 4
  };
  std::int64_t length{};
  std::vector<Stage> stages{};
};

template <class Real>
std::shared_ptr<const Stockham<Real>> stockham_plan(std::int64_t n) {
  static PlanCache<Stockham<Real>> cache(6);
  return cache.get(static_cast<std::uint64_t>(n), [n] {
    auto plan = std::make_shared<Stockham<Real>>();
    plan->length = n;
    if (n > 1) {
      // exp(-2 pi i k / n) for k < n / 2, evaluated directly; the other half
      // follows from w^(k + n/2) = -w^k.
      std::vector<std::complex<Real>> root(static_cast<std::size_t>(n / 2));
      pluss::msharp::parallel_for(
          0, n / 2, 4096, [&](std::int64_t begin, std::int64_t end) {
            for (std::int64_t k = begin; k < end; ++k) {
              root[static_cast<std::size_t>(k)] =
                  unit_root<Real>(static_cast<std::uint64_t>(2 * k),
                                  static_cast<std::uint64_t>(n), true);
            }
          });
      const auto full = [&](std::int64_t k) {
        k %= n;
        return k < n / 2 ? root[static_cast<std::size_t>(k)]
                         : -root[static_cast<std::size_t>(k - n / 2)];
      };
      for (std::int64_t length = n; length > 1;) {
        typename Stockham<Real>::Stage stage;
        stage.length = length;
        const std::int64_t step = n / length;
        if (length >= 4) {
          stage.radix4 = true;
          const std::int64_t quarter = length / 4;
          stage.twiddle.resize(static_cast<std::size_t>(3 * quarter));
          for (std::int64_t p = 0; p < quarter; ++p) {
            for (std::int64_t j = 1; j <= 3; ++j) {
              stage.twiddle[static_cast<std::size_t>(3 * p + j - 1)] =
                  full(j * p * step);
            }
          }
          length /= 4;
        } else {
          length = 1; // a final radix-2 stage has only the trivial twiddle
        }
        plan->stages.push_back(std::move(stage));
      }
    }
    return std::shared_ptr<const Stockham<Real>>(std::move(plan));
  });
}

// Runs the stages over `lanes` interleaved sequences stored as data[p * lanes +
// lane]. The result ends up in either `x` or `y`; the returned pointer says
// which. The inverse transform is unscaled.
template <class Real, bool Inverse>
std::complex<Real> *stockham_run(std::complex<Real> *x, std::complex<Real> *y,
                                 std::int64_t lanes,
                                 const Stockham<Real> &plan) {
  using C = std::complex<Real>;
  std::int64_t s = lanes;
  for (const auto &stage : plan.stages) {
    if (stage.radix4) {
      const std::int64_t quarter = stage.length / 4;
      for (std::int64_t p = 0; p < quarter; ++p) {
        const C w1 =
            Inverse ? std::conj(stage.twiddle[static_cast<std::size_t>(3 * p)])
                    : stage.twiddle[static_cast<std::size_t>(3 * p)];
        const C w2 =
            Inverse
                ? std::conj(stage.twiddle[static_cast<std::size_t>(3 * p + 1)])
                : stage.twiddle[static_cast<std::size_t>(3 * p + 1)];
        const C w3 =
            Inverse
                ? std::conj(stage.twiddle[static_cast<std::size_t>(3 * p + 2)])
                : stage.twiddle[static_cast<std::size_t>(3 * p + 2)];
        const C *xa = x + s * p;
        const C *xb = x + s * (p + quarter);
        const C *xc = x + s * (p + 2 * quarter);
        const C *xd = x + s * (p + 3 * quarter);
        C *y0 = y + s * (4 * p);
        C *y1 = y0 + s;
        C *y2 = y1 + s;
        C *y3 = y2 + s;
        for (std::int64_t q = 0; q < s; ++q) {
          const C a = xa[q], b = xb[q], c = xc[q], d = xd[q];
          const C apc = a + c, amc = a - c, bpd = b + d, bmd = b - d;
          const C jbmd(-bmd.imag(), bmd.real()); // j * (b - d)
          y0[q] = apc + bpd;
          y1[q] = cmul(Inverse ? amc + jbmd : amc - jbmd, w1);
          y2[q] = cmul(apc - bpd, w2);
          y3[q] = cmul(Inverse ? amc - jbmd : amc + jbmd, w3);
        }
      }
      s *= 4;
    } else {
      const C *xa = x;
      const C *xb = x + s;
      for (std::int64_t q = 0; q < s; ++q) {
        const C a = xa[q], b = xb[q];
        y[q] = a + b;
        y[s + q] = a - b;
      }
      s *= 2;
    }
    std::swap(x, y);
  }
  return x;
}

// Bluestein's chirp-z algorithm for lengths that are not powers of two: a
// length-n transform becomes a circular convolution of power-of-two length m >=
// 2n - 1. The chirp exp(sign * i * pi * x^2 / n) depends on x^2 only modulo 2n,
// so the exponent is reduced exactly in integer arithmetic before it is turned
// into an angle; without that the angle loses all precision for large n.
template <class Real> struct BluesteinPlan final {
  std::int64_t length{};
  std::int64_t convolution_length{};
  std::shared_ptr<const Stockham<Real>> table{};
  std::vector<std::complex<Real>> chirp{};
  std::vector<std::complex<Real>> kernel_spectrum{};
};

template <class Real>
std::shared_ptr<const BluesteinPlan<Real>> bluestein_plan(std::int64_t n,
                                                          bool inverse) {
  PLUSS_CHECK(n <= std::numeric_limits<std::int64_t>::max() / 4,
              "fft: transform length is too large");
  static PlanCache<BluesteinPlan<Real>> cache(8);
  const std::uint64_t key = (static_cast<std::uint64_t>(n) << 1) |
                            static_cast<std::uint64_t>(inverse);
  return cache.get(key, [n, inverse] {
    auto plan = std::make_shared<BluesteinPlan<Real>>();
    plan->length = n;
    plan->convolution_length = next_power_of_two(2 * n - 1);
    plan->table = stockham_plan<Real>(plan->convolution_length);
    plan->chirp.resize(static_cast<std::size_t>(n));
    const std::int64_t m = plan->convolution_length;
    const unsigned __int128 modulus =
        static_cast<unsigned __int128>(2) * static_cast<unsigned __int128>(n);
    pluss::msharp::parallel_for(
        0, n, 4096, [&](std::int64_t begin, std::int64_t end) {
          for (std::int64_t lane = begin; lane < end; ++lane) {
            const unsigned __int128 x = static_cast<unsigned __int128>(lane);
            const auto reduced = static_cast<std::uint64_t>((x * x) % modulus);
            plan->chirp[static_cast<std::size_t>(lane)] = unit_root<Real>(
                reduced, static_cast<std::uint64_t>(n), !inverse);
          }
        });
    std::vector<std::complex<Real>> kernel(
        static_cast<std::size_t>(m), std::complex<Real>{Real{0}, Real{0}});
    std::vector<std::complex<Real>> scratch(static_cast<std::size_t>(m));
    for (std::int64_t lane = 0; lane < n; ++lane) {
      const std::complex<Real> value =
          std::conj(plan->chirp[static_cast<std::size_t>(lane)]);
      kernel[static_cast<std::size_t>(lane)] = value;
      if (lane != 0)
        kernel[static_cast<std::size_t>(m - lane)] = value;
    }
    auto *result = stockham_run<Real, false>(kernel.data(), scratch.data(), 1,
                                             *plan->table);
    plan->kernel_spectrum.assign(result, result + m);
    return std::shared_ptr<const BluesteinPlan<Real>>(std::move(plan));
  });
}

// Everything a transform of one length needs, fetched once per call and shared
// (read-only) by all worker threads.
template <class Real> struct Plan final {
  std::int64_t length{};
  std::shared_ptr<const Stockham<Real>> power_of_two{};
  std::shared_ptr<const BluesteinPlan<Real>> forward{};
  std::shared_ptr<const BluesteinPlan<Real>> inverse{};
};

template <class Real> Plan<Real> make_plan(std::int64_t n) {
  PLUSS_CHECK(n > 0, "fft: transform length must be positive");
  Plan<Real> plan;
  plan.length = n;
  if (is_power_of_two(n)) {
    plan.power_of_two = stockham_plan<Real>(n);
  } else {
    plan.forward = bluestein_plan<Real>(n, false);
    plan.inverse = bluestein_plan<Real>(n, true);
  }
  return plan;
}

// Per-thread scratch buffers, grown on demand and reused across transforms.
template <class Real> struct Workspace final {
  std::vector<std::complex<Real>> first{};
  std::vector<std::complex<Real>> second{};
};

// Transforms `lanes` interleaved sequences (data[p * lanes + lane], p <
// plan.length) and returns a pointer to the result, which is either `data` or a
// buffer inside `workspace`. Neither direction is scaled.
template <class Real>
std::complex<Real> *
transform_lanes(std::complex<Real> *data, std::int64_t lanes, bool inverse,
                const Plan<Real> &plan, Workspace<Real> &workspace) {
  using C = std::complex<Real>;
  const std::int64_t n = plan.length;
  if (plan.power_of_two) {
    if (n == 1)
      return data;
    workspace.second.resize(static_cast<std::size_t>(n * lanes));
    return inverse ? stockham_run<Real, true>(data, workspace.second.data(),
                                              lanes, *plan.power_of_two)
                   : stockham_run<Real, false>(data, workspace.second.data(),
                                               lanes, *plan.power_of_two);
  }

  const auto &bluestein = inverse ? *plan.inverse : *plan.forward;
  const std::int64_t m = bluestein.convolution_length;
  workspace.first.assign(static_cast<std::size_t>(m * lanes),
                         C{Real{0}, Real{0}});
  workspace.second.resize(static_cast<std::size_t>(m * lanes));
  C *a = workspace.first.data();
  for (std::int64_t p = 0; p < n; ++p) {
    const C chirp = bluestein.chirp[static_cast<std::size_t>(p)];
    for (std::int64_t q = 0; q < lanes; ++q)
      a[p * lanes + q] = cmul(data[p * lanes + q], chirp);
  }
  C *b = workspace.second.data();
  C *spectrum = stockham_run<Real, false>(a, b, lanes, *bluestein.table);
  C *other = spectrum == a ? b : a;
  for (std::int64_t p = 0; p < m; ++p) {
    const C kernel = bluestein.kernel_spectrum[static_cast<std::size_t>(p)];
    for (std::int64_t q = 0; q < lanes; ++q)
      spectrum[p * lanes + q] = cmul(spectrum[p * lanes + q], kernel);
  }
  C *conv = stockham_run<Real, true>(spectrum, other, lanes, *bluestein.table);
  const Real scale = Real{1} / static_cast<Real>(m);
  for (std::int64_t p = 0; p < n; ++p) {
    const C chirp = bluestein.chirp[static_cast<std::size_t>(p)];
    for (std::int64_t q = 0; q < lanes; ++q) {
      data[p * lanes + q] = cmul(conv[p * lanes + q], chirp) * scale;
    }
  }
  return data;
}

// Convenience form for callers that transform a single buffer; the inverse
// carries 1 / n.
template <class Real>
void transform(std::vector<std::complex<Real>> &values, bool inverse,
               const Plan<Real> &plan) {
  const auto n = static_cast<std::int64_t>(values.size());
  PLUSS_CHECK(n == plan.length, "fft: internal plan length mismatch");
  Workspace<Real> workspace;
  auto *result = transform_lanes(values.data(), 1, inverse, plan, workspace);
  const Real scale = inverse ? Real{1} / static_cast<Real>(n) : Real{1};
  for (std::int64_t i = 0; i < n; ++i)
    values[static_cast<std::size_t>(i)] = result[i] * scale;
}

template <class Real>
void transform(std::vector<std::complex<Real>> &values, bool inverse) {
  transform(values, inverse,
            make_plan<Real>(static_cast<std::int64_t>(values.size())));
}

// exp(-2 pi i k / n) for k <= n / 2, used to split the packed transform of a
// real signal.
template <class Real>
std::shared_ptr<const std::vector<std::complex<Real>>>
real_split_roots(std::int64_t n) {
  static PlanCache<std::vector<std::complex<Real>>> cache(6);
  return cache.get(static_cast<std::uint64_t>(n), [n] {
    auto roots = std::make_shared<std::vector<std::complex<Real>>>(
        static_cast<std::size_t>(n / 2 + 1));
    pluss::msharp::parallel_for(
        0, n / 2 + 1, 4096, [&](std::int64_t begin, std::int64_t end) {
          for (std::int64_t k = begin; k < end; ++k) {
            (*roots)[static_cast<std::size_t>(k)] =
                unit_root<Real>(static_cast<std::uint64_t>(2 * k),
                                static_cast<std::uint64_t>(n), true);
          }
        });
    return std::shared_ptr<const std::vector<std::complex<Real>>>(
        std::move(roots));
  });
}

} // namespace pluss::codem::native::cpu::fourier_detail
