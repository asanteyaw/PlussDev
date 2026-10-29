#pragma once

// Millard (2012) muscle curves: C^2-smooth quintic-Bezier force-length, force-velocity and tendon curves, as constructed by OpenSim's SmoothSegmentedFunctionFactory
// (OpenSim/Common, sha256 prefixes 83c25616 and 79bfeb65 for the factory and the Bezier toolkit; curve defaults from OpenSim/Actuators {Active,Fiber,Tendon}ForceLength
// and ForceVelocity curve classes; all read from the GitHub source). The curves are `math::bezier::Curve` values; `curves()` packages them as the muscle.h `Curves`
// family so the same muscle-tendon machinery (equilibrium, dynamics, musculoskeletal coupling) runs with either the De Groote-Fregly closed forms or these.
//
//   active force-length  five Bezier corners through (x0, ylow), (x01), (x1s), (1, 1), (x23), (x3, ylow); defaults min 0.4441, transition 0.73, max 1.8123,
//                        shallow-ascending slope 0.8616, minimum value 0.1, curviness 1 (scaled c = 0.1 + 0.8 curviness)
//   force-velocity      four corners (-1, 0), (-0.9, .), (0, 1, slope 5), (0.9, .), (1, fmaxE = 1.4); concentric / eccentric curviness 0.6 / 0.9
//   passive fibre       two corners from (1 + e0, 0) to (1 + e1, 1): strain at zero force 0, at unit force 0.7, low-force stiffness 0.2, unit-force stiffness 2 / (e1 - e0), curviness 0.75
//   tendon              two corners from (1, 0) to the toe end: strain at unit force 0.049, unit-force stiffness 1.375 / e1, toe force 2/3, curviness 0.5
// Millard's paper (2013, J. Biomech. Eng.) was not obtained; the construction is verified by the curves' defining key points, C^2 continuity at the segment joins,
// finite-difference derivatives and an independent numpy/scipy re-implementation of the construction and of the u(x) inversion.

#include "pluss/subsp/math/bezier.h"
#include "pluss/subsp/natsci/biology/biomechanics/muscle.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace pluss::subsp::natsci::biology::biomechanics::millard {

namespace bz = ::pluss::subsp::math::bezier;
namespace muscle = ::pluss::subsp::natsci::biology::biomechanics::muscle;

[[nodiscard]] constexpr double scale_curviness(double curviness) noexcept { return 0.1 + 0.8 * curviness; }

struct ActiveParams final {
  double min_length{0.47 - 0.0259};
  double transition_length{0.73};
  double max_length{1.8123};
  double shallow_slope{0.8616};
  double minimum_value{0.1};
  double curviness{1.0};
};
[[nodiscard]] inline bz::Curve active_force_length(const ActiveParams &q = {}) {
  const double x0 = q.min_length, x1 = q.transition_length, x2 = 1.0, x3 = q.max_length, ylow = q.minimum_value, c = scale_curviness(q.curviness);
  const double delta = 0.05 * x2, xs = x2 - delta;
  const double y0 = 0.0, dydx0 = 0.0, y1 = 1.0 - q.shallow_slope * (xs - x1), dydx01 = 1.25 * (y1 - y0) / (x1 - x0);
  const double x01 = x0 + 0.5 * (x1 - x0), y01 = y0 + 0.5 * (y1 - y0), x1s = x1 + 0.5 * (xs - x1), y1s = y1 + 0.5 * (1.0 - y1);
  const double x23 = (x2 + delta) + 0.5 * (x3 - (x2 + delta)), y23 = 0.5 * (1.0 + 0.0), dydx23 = (0.0 - 1.0) / ((x3 - delta) - (x2 + delta));
  return {.segments = {bz::corner(x0, ylow, dydx0, x01, y01, dydx01, c), bz::corner(x01, y01, dydx01, x1s, y1s, q.shallow_slope, c), bz::corner(x1s, y1s, q.shallow_slope, x2, 1.0, 0.0, c),
                       bz::corner(x2, 1.0, 0.0, x23, y23, dydx23, c), bz::corner(x23, y23, dydx23, x3, ylow, 0.0, c)},
          .slope_below = 0.0, .slope_above = 0.0};
}

struct VelocityParams final {
  double max_eccentric_multiplier{1.4};
  double concentric_slope_at_vmax{0.0};
  double concentric_slope_near_vmax{0.25};
  double isometric_slope{5.0};
  double eccentric_slope_at_vmax{0.0};
  double eccentric_slope_near_vmax{0.15};
  double concentric_curviness{0.6};
  double eccentric_curviness{0.9};
};
[[nodiscard]] inline bz::Curve force_velocity(const VelocityParams &q = {}) {
  const double cc = scale_curviness(q.concentric_curviness), ce = scale_curviness(q.eccentric_curviness);
  const double xc = -1.0, yc = 0.0, xnc = -0.9, ync = yc + 0.5 * q.concentric_slope_near_vmax * (xnc - xc) + 0.5 * q.concentric_slope_at_vmax * (xnc - xc);
  const double xiso = 0.0, yiso = 1.0, xe = 1.0, ye = q.max_eccentric_multiplier, xne = 0.9, yne = ye + 0.5 * q.eccentric_slope_near_vmax * (xne - xe) + 0.5 * q.eccentric_slope_at_vmax * (xne - xe);
  return {.segments = {bz::corner(xc, yc, q.concentric_slope_at_vmax, xnc, ync, q.concentric_slope_near_vmax, cc), bz::corner(xnc, ync, q.concentric_slope_near_vmax, xiso, yiso, q.isometric_slope, cc),
                       bz::corner(xiso, yiso, q.isometric_slope, xne, yne, q.eccentric_slope_near_vmax, ce), bz::corner(xne, yne, q.eccentric_slope_near_vmax, xe, ye, q.eccentric_slope_at_vmax, ce)},
          .slope_below = q.concentric_slope_at_vmax, .slope_above = q.eccentric_slope_at_vmax};
}

struct PassiveParams final {
  double strain_at_zero_force{0.0};
  double strain_at_one_force{0.7};
  double low_force_stiffness{0.2};
  double stiffness_at_one_force{-1.0}; // default 2 / (e1 - e0)
  double curviness{0.75};
};
[[nodiscard]] inline bz::Curve passive_force_length(const PassiveParams &q = {}) {
  const double e0 = q.strain_at_zero_force, e1 = q.strain_at_one_force, k_low = q.low_force_stiffness, k_iso = q.stiffness_at_one_force > 0.0 ? q.stiffness_at_one_force : 2.0 / (e1 - e0), c = scale_curviness(q.curviness);
  const double x_zero = 1.0 + e0, x_iso = 1.0 + e1, delta = std::min(0.1 * (1.0 / k_iso), 0.1 * (x_iso - x_zero)), x_low = x_zero + delta, x_foot = x_zero + 0.5 * (x_low - x_zero);
  const double y_low = 0.0 + k_low * (x_low - x_foot);
  return {.segments = {bz::corner(x_zero, 0.0, 0.0, x_low, y_low, k_low, c), bz::corner(x_low, y_low, k_low, x_iso, 1.0, k_iso, c)}, .slope_below = 0.0, .slope_above = k_iso};
}

struct TendonParams final {
  double strain_at_one_force{0.049};
  double stiffness_at_one_force{-1.0}; // default 1.375 / e1
  double force_at_toe_end{2.0 / 3.0};
  double curviness{0.5};
};
[[nodiscard]] inline bz::Curve tendon_force_length(const TendonParams &q = {}) {
  const double e1 = q.strain_at_one_force, k_iso = q.stiffness_at_one_force > 0.0 ? q.stiffness_at_one_force : 1.375 / e1, c = scale_curviness(q.curviness);
  const double x0 = 1.0, x_iso = 1.0 + e1, y_toe = q.force_at_toe_end, x_toe = (y_toe - 1.0) / k_iso + x_iso;
  const double x_foot = 1.0 + (x_toe - 1.0) / 10.0, y_foot = 0.0, y_toe_mid = 0.5 * y_toe, x_toe_mid = (y_toe_mid - 1.0) / k_iso + x_iso;
  const double slope_mid = (y_toe_mid - y_foot) / (x_toe_mid - x_foot), x_ctrl = x_foot + 0.5 * (x_toe_mid - x_foot), y_ctrl = y_foot + slope_mid * (x_ctrl - x_foot);
  return {.segments = {bz::corner(x0, 0.0, 0.0, x_ctrl, y_ctrl, slope_mid, c), bz::corner(x_ctrl, y_ctrl, slope_mid, x_toe, y_toe, k_iso, c)}, .slope_below = 0.0, .slope_above = k_iso};
}

// The four Millard curves packaged for muscle.h (defaults of OpenSim's Millard2012EquilibriumMuscle).
[[nodiscard]] inline std::shared_ptr<const muscle::Curves> curves(const ActiveParams &a = {}, const VelocityParams &v = {}, const PassiveParams &p = {}, const TendonParams &t = {}) {
  auto act = std::make_shared<const bz::Curve>(active_force_length(a)), fv = std::make_shared<const bz::Curve>(force_velocity(v)), pas = std::make_shared<const bz::Curve>(passive_force_length(p)),
       ten = std::make_shared<const bz::Curve>(tendon_force_length(t));
  return std::make_shared<const muscle::Curves>(muscle::Curves{
      .active = [act](const Series &l) { return bz::value(*act, l); },
      .passive = [pas](const Series &l) { return bz::value(*pas, l); },
      .force_velocity = [fv](const Series &x) { return bz::value(*fv, x); },
      .force_velocity_inverse = [fv](const Series &f) { return bz::inverse(*fv, f); },
      .force_velocity_slope = [fv](const Series &x) { return bz::evaluate(*fv, x).slope; },
      .tendon_length_from_force = [ten](const Series &f) { return bz::inverse(*ten, f); },
      .tendon_stiffness = [ten](const Series &l) { return bz::evaluate(*ten, l).slope; }});
}

} // namespace pluss::subsp::natsci::biology::biomechanics::millard
