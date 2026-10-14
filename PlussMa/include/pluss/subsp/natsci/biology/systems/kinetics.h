#pragma once

// Closed-form enzyme / regulation rate laws, elementwise over Series.
// Sources: Wikipedia "Michaelis-Menten kinetics" (v = Vmax a/(Km + a),
// Km = (k-1 + kcat)/k+1 under the steady-state assumption), "Hill equation
// (biochemistry)" (activation k L^n/(K^n + L^n), repression k K^n/(K^n + L^n)).

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::systems::kinetics {

using dynamics::Coeff;

inline Series michaelis_menten(const Series &s, const Coeff &vmax,
                               const Coeff &km) {
  dynamics::require_positive(km, s, "michaelis_menten: Km");
  return vmax.on(s).mul(s).div(km.on(s).add(s));
}

// Briggs-Haldane Km from the elementary constants.
inline Series michaelis_constant(const Series &k_minus, const Series &kcat,
                                 const Series &k_plus) {
  return k_minus.add(kcat).div(k_plus);
}

inline Series competitive_inhibition(const Series &s, const Series &inhibitor,
                                     const Coeff &vmax, const Coeff &km,
                                     const Coeff &ki) {
  dynamics::require_positive(km, s, "competitive_inhibition: Km");
  dynamics::require_positive(ki, s, "competitive_inhibition: Ki");
  const Series apparent = km.on(s).mul(inhibitor.div(ki.on(s)).add(1.0));
  return vmax.on(s).mul(s).div(apparent.add(s));
}

inline Series hill_activation(const Series &l, const Coeff &vmax,
                              const Coeff &k_half, const Coeff &n) {
  dynamics::require_positive(k_half, l, "hill: K");
  dynamics::require_positive(n, l, "hill: n");
  const Series ln = l.pow(n.on(l));
  return vmax.on(l).mul(ln).div(k_half.on(l).pow(n.on(l)).add(ln));
}

inline Series hill_repression(const Series &l, const Coeff &vmax,
                              const Coeff &k_half, const Coeff &n) {
  dynamics::require_positive(k_half, l, "hill: K");
  dynamics::require_positive(n, l, "hill: n");
  const Series kn = k_half.on(l).pow(n.on(l));
  return vmax.on(l).mul(kn).div(kn.add(l.pow(n.on(l))));
}

} // namespace pluss::subsp::natsci::biology::systems::kinetics
