#pragma once

#include <MSharp/util/complex.h>
#include <PlussQit/NumericUtils.h>

namespace pluss::native {
inline namespace CPU_CAPABILITY {

// custom min and max to be used in logcumsumexp for complex arguments
template <typename scalar_t>
std::pair<MSharp::complex<scalar_t>, MSharp::complex<scalar_t>> _logcumsumexp_minmax(MSharp::complex<scalar_t> x, MSharp::complex<scalar_t> y) {
  if (pluss::_isnan(y)) {  // either real is nan or imag is nan
    return std::make_pair(y, y);
  } else if (pluss::_isnan(x)) {  // either real is nan or imag is nan
    return std::make_pair(x, x);
  } else {
    return (x.real() < y.real()) ? std::make_pair(x, y) : std::make_pair(y, x);
  }
}

template <typename scalar_t>
scalar_t _log_add_exp_helper(scalar_t x, scalar_t y) {
  // Reference : https://www.tensorflow.org/api_docs/python/tf/math/cumulative_logsumexp
  scalar_t min = pluss::_isnan(y) ? y : std::min(x, y); // std::min returns first arg if one of the args is nan
  scalar_t max = pluss::_isnan(y) ? y : std::max(x, y); // std::max returns first arg if one of the args is nan
  if (min != max || std::isfinite(min)) {
    // nan will be propagated here
    return std::log1p(std::exp(min - max)) + max;
  } else {
    // special case to correctly handle infinite cases
    return x;
  }
}

template <typename scalar_t>
MSharp::complex<scalar_t> _log_add_exp_helper(const MSharp::complex<scalar_t>& x, const MSharp::complex<scalar_t>& y) {
  auto [min, max] = _logcumsumexp_minmax<scalar_t>(x, y);
  auto min_real = std::real(min);
  auto max_real = std::real(max);

  if (pluss::_isnan(min)) {  // either real is nan or imag is nan
    // handling the "infectious" NaNs
    return {std::numeric_limits<scalar_t>::quiet_NaN(), std::numeric_limits<scalar_t>::quiet_NaN()};
  } else if (!std::isfinite(min_real) && (min_real == max_real)) {
    if (min_real < 0) {
      // handle the -inf case, the imaginary part here does not really matter as the exp(value)
      // will be around 0.0 and the angle (i.e. the imaginary part) cannot be determined.
      // It does not matter if we're taking the exp of this value
      return min;
    } else {
      // handle the +inf case, we don't need the special precision for log1p for small values
      // and to avoid producing nan in case of real(max) == real(min) == +inf
      return std::log(std::exp(min) + std::exp(max));
    }
  } else {
    return std::log1p(std::exp(min - max)) + max;
  }
}

} // end namespace
} //end pluss::native
