#include "pluss/subsp/eng/lti/pole_zero.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::lti {
namespace {

using Complex = std::complex<double>;
using RootGroup = std::vector<Complex>;

ScalarType complex_dtype(ScalarType left, ScalarType right) {
  const ScalarType common = codem::promote_types(left, right);
  return common == ScalarType::Float64 || common == ScalarType::Complex128
             ? ScalarType::Complex128
             : ScalarType::Complex64;
}

void validate_roots(const Series &value, const char *name) {
  PLUSS_CHECK(value.defined(),
              std::string("eng::lti::zpk: ") + name + " must be defined");
  PLUSS_CHECK(value.dim() == 1,
              std::string("eng::lti::zpk: ") + name + " must be a vector");
  PLUSS_CHECK(value.dtype().is_floating() || value.dtype().is_complex(),
              std::string("eng::lti::zpk: ") + name +
                  " must have a floating or complex dtype");
  const bool finite = value.dtype().is_complex()
                          ? value.real().isfinite().all().item<bool>() &&
                                value.imag().isfinite().all().item<bool>()
                          : value.isfinite().all().item<bool>();
  PLUSS_CHECK(finite,
              std::string("eng::lti::zpk: ") + name + " must be finite");
}

std::vector<Complex> host_roots(const Series &value) {
  return value.to(Platform::CPU, ScalarType::Complex128).to_vec<Complex>();
}

double root_scale(const Complex value) {
  return std::max(1.0, std::abs(value));
}

bool near_real(const Complex value) {
  return std::abs(value.imag()) <=
         64.0 * std::numeric_limits<double>::epsilon() * root_scale(value);
}

bool conjugate_system(const std::vector<Complex> &roots) {
  std::vector<bool> used(roots.size(), false);
  for (std::size_t i = 0; i < roots.size(); ++i) {
    if (used[i])
      continue;
    if (near_real(roots[i])) {
      used[i] = true;
      continue;
    }
    std::size_t best = roots.size();
    double distance = std::numeric_limits<double>::infinity();
    for (std::size_t j = i + 1; j < roots.size(); ++j) {
      if (used[j])
        continue;
      const double candidate = std::abs(roots[j] - std::conj(roots[i]));
      if (candidate < distance) {
        distance = candidate;
        best = j;
      }
    }
    const double tolerance =
        256.0 * std::numeric_limits<double>::epsilon() * root_scale(roots[i]);
    if (best == roots.size() || distance > tolerance)
      return false;
    used[i] = true;
    used[best] = true;
  }
  return true;
}

std::vector<RootGroup> conjugate_groups(const std::vector<Complex> &roots,
                                        bool real_system) {
  if (!real_system) {
    std::vector<RootGroup> groups;
    for (std::size_t i = 0; i < roots.size(); i += 2) {
      RootGroup group{roots[i]};
      if (i + 1 < roots.size())
        group.push_back(roots[i + 1]);
      groups.push_back(std::move(group));
    }
    return groups;
  }

  std::vector<Complex> real;
  std::vector<Complex> complex;
  real.reserve(roots.size());
  complex.reserve(roots.size());
  for (const Complex root : roots) {
    if (near_real(root))
      real.emplace_back(root.real(), 0.0);
    else if (root.imag() > 0.0)
      complex.push_back(root);
  }
  std::sort(real.begin(), real.end(), [](Complex left, Complex right) {
    return std::abs(left) > std::abs(right);
  });
  std::sort(complex.begin(), complex.end(), [](Complex left, Complex right) {
    return std::abs(left) > std::abs(right);
  });

  std::vector<RootGroup> groups;
  groups.reserve((roots.size() + 1) / 2);
  for (const Complex root : complex)
    groups.push_back({root, std::conj(root)});
  for (std::size_t i = 0; i < real.size(); i += 2) {
    RootGroup group{real[i]};
    if (i + 1 < real.size())
      group.push_back(real[i + 1]);
    groups.push_back(std::move(group));
  }
  return groups;
}

double group_radius(const RootGroup &group) {
  double radius = 0.0;
  for (const Complex root : group)
    radius = std::max(radius, std::abs(root));
  return radius;
}

double group_distance(const RootGroup &left, const RootGroup &right) {
  if (right.empty())
    return std::numeric_limits<double>::infinity();
  if (left.size() == 1 || right.size() == 1)
    return std::abs(left.front() - right.front());
  const double direct =
      std::abs(left[0] - right[0]) + std::abs(left[1] - right[1]);
  const double crossed =
      std::abs(left[0] - right[1]) + std::abs(left[1] - right[0]);
  return std::min(direct, crossed);
}

std::array<Complex, 3> coefficients(const RootGroup &roots) {
  if (roots.empty())
    return {Complex{1.0, 0.0}, Complex{}, Complex{}};
  if (roots.size() == 1)
    return {Complex{1.0, 0.0}, -roots[0], Complex{}};
  return {Complex{1.0, 0.0}, -(roots[0] + roots[1]), roots[0] * roots[1]};
}

} // namespace

PoleZero zpk(const Series &zeros, const Series &poles, const Series &gain,
             double sample_time) {
  validate_roots(zeros, "zeros");
  validate_roots(poles, "poles");
  PLUSS_CHECK(gain.defined(), "eng::lti::zpk: gain must be defined");
  PLUSS_CHECK(gain.numel() == 1, "eng::lti::zpk: gain must contain one value");
  PLUSS_CHECK(gain.dtype().is_floating() || gain.dtype().is_complex(),
              "eng::lti::zpk: gain must have a floating or complex dtype");
  const bool finite_gain = gain.dtype().is_complex()
                               ? gain.real().isfinite().all().item<bool>() &&
                                     gain.imag().isfinite().all().item<bool>()
                               : gain.isfinite().all().item<bool>();
  PLUSS_CHECK(finite_gain, "eng::lti::zpk: gain must be finite");
  PLUSS_CHECK(zeros.platform() == poles.platform() &&
                  zeros.platform() == gain.platform(),
              "eng::lti::zpk: zeros, poles, and gain must share one platform");
  PLUSS_CHECK(
      zeros.numel() <= poles.numel(),
      "eng::lti::zpk: a proper system cannot have more zeros than poles");
  PLUSS_CHECK(std::isfinite(sample_time) && sample_time >= 0.0,
              "eng::lti::zpk: sample_time must be finite and non-negative");

  ScalarType dtype = complex_dtype(zeros.scalar_type(), poles.scalar_type());
  dtype = complex_dtype(dtype, gain.scalar_type());
  return PoleZero(zeros.to(dtype), poles.to(dtype), gain.reshape({}).to(dtype),
                  sample_time);
}

bool PoleZero::is_stable() const {
  if (poles_.numel() == 0)
    return true;
  if (is_disc())
    return poles_.abs().lt(1.0).all().item<bool>();
  return poles_.real().lt(0.0).all().item<bool>();
}

Series to_sos(const PoleZero &system) {
  PLUSS_CHECK(system.is_disc(),
              "eng::lti::to_sos: only discrete systems can be realized by "
              "the delay-form SOS filter");
  PLUSS_CHECK(system.zeros().numel() == system.poles().numel(),
              "eng::lti::to_sos: the discrete system must explicitly carry "
              "one zero for each pole");
  auto zeros = host_roots(system.zeros());
  auto poles = host_roots(system.poles());
  const Complex gain =
      system.gain().to(Platform::CPU, ScalarType::Complex128).item<Complex>();
  const bool real_system =
      near_real(gain) && conjugate_system(zeros) && conjugate_system(poles);

  if (poles.size() % 2 != 0) {
    poles.emplace_back(0.0, 0.0);
    zeros.emplace_back(0.0, 0.0);
  }
  auto pole_groups = conjugate_groups(poles, real_system);
  auto zero_groups = conjugate_groups(zeros, real_system);
  std::sort(pole_groups.begin(), pole_groups.end(),
            [](const RootGroup &left, const RootGroup &right) {
              return group_radius(left) > group_radius(right);
            });

  std::vector<Complex> values;
  values.reserve(pole_groups.size() * 6);
  for (std::size_t section = 0; section < pole_groups.size(); ++section) {
    const auto &pole_group = pole_groups[section];
    const auto nearest =
        std::min_element(zero_groups.begin(), zero_groups.end(),
                         [&](const RootGroup &left, const RootGroup &right) {
                           return group_distance(pole_group, left) <
                                  group_distance(pole_group, right);
                         });
    PLUSS_CHECK(nearest != zero_groups.end(),
                "eng::lti::to_sos: root pairing failed");
    auto numerator = coefficients(*nearest);
    const auto denominator = coefficients(pole_group);
    if (section == 0) {
      for (Complex &coefficient : numerator)
        coefficient *= gain;
    }
    values.insert(values.end(), numerator.begin(), numerator.end());
    values.insert(values.end(), denominator.begin(), denominator.end());
    zero_groups.erase(nearest);
  }

  const auto rows = static_cast<std::int64_t>(pole_groups.size());
  Series result = series::make(values, {rows, 6}).to(system.poles().platform());
  if (real_system)
    result =
        result.real().to(system.poles().scalar_type() == ScalarType::Complex128
                             ? ScalarType::Float64
                             : ScalarType::Float32);
  return result;
}

} // namespace pluss::subsp::eng::lti
