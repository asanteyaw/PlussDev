#pragma once

// Lambert W function (principal branch, x >= 0), batched: every lane advances in the same Series ops.
//
// W(x) solves w e^w = x. In log form, with z = ln x, it solves  w + ln w = z, a concave increasing equation, so Newton's method started
// to the left of the root converges monotonically: w <- w - w (w + ln w - z) / (w + 1). The start is x/(1+x) for z < 1 and z - ln z
// otherwise (both lie below the root). Working with z avoids the overflow of e^z for the large arguments of integrated
// Michaelis-Menten progress curves.

#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"

#include <cmath>

namespace pluss::subsp::math::special {

inline Series lambert_w0_from_log(const Series &z, int iterations = 40) {
  const Series x = z.clamp(-700.0, 700.0).exp();
  const Series large = z.gt(1.0);
  Series w = large.where(z.sub(z.clamp(1.0, 1e300).log()), x.div(x.add(1.0)));
  w = w.clamp(1e-300, 1e300);
  for (int i = 0; i < iterations; ++i) {
    const Series step = w.mul(w.add(w.log()).sub(z)).div(w.add(1.0));
    Series next = w.sub(step);
    next = next.gt(0.0).where(next, w.mul(0.5)); // safeguard (never triggered from a left start)
    w = next.clamp(1e-300, 1e300);
  }
  return w;
}

// W0(x) for x >= 0
inline Series lambert_w0(const Series &x) { return x.eq(0.0).where(x.mul(0.0), lambert_w0_from_log(x.clamp(1e-300, 1e300).log())); }

} // namespace pluss::subsp::math::special
