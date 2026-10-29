#pragma once

// Moran birth-death process with a mutant of relative fitness r among N
// individuals. From i mutants:
//   up   = r i/(r i + N - i) * (N - i)/N,   down = (N - i)/(r i + N - i) * i/N.
// gamma_k = down/up = 1/r for every k, so the birth-death absorption formula
// gives fixation from i mutants rho_i = (1 - r^-i)/(1 - r^-N) (r != 1; i/N at
// r = 1), the classic result for a well-mixed population (e.g. the
// birth-death well-mixed fixation probability (1-1/r)/(1-1/r^N) cited in the
// graph-Moran literature). The exact chain is built and solved independently.

#include "pluss/subsp/natsci/biology/genetics/markov.h"

namespace pluss::subsp::natsci::biology::genetics {

inline Series moran_transition(std::int64_t n, double r) {
  dynamics::require(n >= 2 && r > 0.0 && std::isfinite(r),
                    "moran: need N >= 2 and positive finite relative fitness");
  const Series i = pluss::series::arange(0, n + 1, 1, pluss::ScalarType::Float64);
  const Series denom = i.mul(r).add(static_cast<double>(n)).sub(i);
  const Series up = i.mul(r).div(denom).mul(i.neg().add(static_cast<double>(n)).div(static_cast<double>(n)));
  const Series down = i.neg().add(static_cast<double>(n)).div(denom).mul(i.div(static_cast<double>(n)));
  const Series stay = up.add(down).neg().add(1.0);
  const Series states = pluss::series::arange(0, n + 1, 1, pluss::ScalarType::Float64);
  const Series row = states.unsqueeze(-1), col = states.unsqueeze(0);
  const Series is_up = col.sub(row).eq(1.0).to(pluss::ScalarType::Float64);
  const Series is_dn = col.sub(row).eq(-1.0).to(pluss::ScalarType::Float64);
  const Series is_stay = col.sub(row).eq(0.0).to(pluss::ScalarType::Float64);
  return is_up.mul(up.unsqueeze(-1)).add(is_dn.mul(down.unsqueeze(-1)))
      .add(is_stay.mul(stay.unsqueeze(-1)));
}

inline Series moran_fixation(double r, std::int64_t n, const Series &start) {
  if (std::abs(r - 1.0) < 1e-12)
    return start.div(static_cast<double>(n));
  const Series num = start.mul(-std::log(r)).exp().neg().add(1.0);
  return num.div(1.0 - std::pow(r, -static_cast<double>(n)));
}

} // namespace pluss::subsp::natsci::biology::genetics
