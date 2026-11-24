# Moving limbs

[Back to Biology 28: Limb Chains](../bio_28_limb_chains.md)

A limb that is not held still accelerates according to the torques on it. You have muscle torques, or a control rule, and you want to know how the arm moves, how hard each joint is to accelerate, how much of a joint torque is spent on the velocity of the other joints, and whether a controller can hold the arm where you want it. This page computes the mass matrix, the velocity-dependent forces, the accelerations, and the full motion over time.

## How it works

The equations of motion of a chain are

```{math}
M(q)\,\ddot q + h(q, \dot q) = \tau + J^{\top}F
```

where $M(q)$ is the symmetric positive-definite mass matrix, $h$ collects the Coriolis, centrifugal and gravity terms, $\tau$ holds the joint torques and $J^\top F$ is the effect of the end-point force. `chain::mass_matrix(c, q)` returns $M$ with shape `[..., n, n]`, `chain::bias_forces(c, q, qd)` returns $h$, and `chain::forward_dynamics(c, q, qd, tau, end_force)` solves for $\ddot q = M^{-1}(\tau + J^\top F - h)$. `chain::energy(c, q, qd)` gives the kinetic energy $\tfrac12\dot q^\top M\dot q$ and the potential energy $\sum m_i g\,y_i$ of the centres of mass, with a `total()`. `chain::simulate(c, torque, q0, qd0, t0, t1, steps, end_force)` integrates the motion by fixed-step RK4; `torque(t, q, qd)` is a function that returns the joint torques, so it can be a feedback law, and the result `Path` holds `times`, `q` and `qd` at every step.

## The mass matrix

The program prints the mass matrix of the arm with the elbow at `0`, `90` and `150` degrees, then checks on a batch of six elbow angles that the matrix is symmetric and positive definite.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace chain = biology::biomechanics::chain;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static const double deg = M_PI / 180.0;
// an arm of two links: upper arm (2.0 kg, 0.30 m) and forearm with hand (1.5 kg, 0.25 m); the centre of mass of each lies a little short of the middle
static chain::Chain arm() { return chain::make(vec({2.0, 1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto limb = arm();
  // the mass matrix M(q): how hard it is to accelerate each joint, and how the joints couple. Elbow straight, bent to 90 degrees, and folded back to 150 degrees
  for (double elbow : {0.0, 90.0, 150.0}) {
    const auto m = chain::mass_matrix(limb, vec({0.0, elbow * deg}));
    out.writeln("elbow ", std::vector<double>{elbow}, " degrees   M = ", flat(m));
  }
  // M is symmetric and positive definite for every pose: check on a batch of poses
  const auto elbow = pluss::series::linspace(0.0, 150.0 * deg, 6, pluss::ScalarType::Float64);
  const auto q = elbow.mul(0.0).unsqueeze(-1).unite(std::vector<pluss::Series>{elbow.unsqueeze(-1)}, 1);
  const auto m = chain::mass_matrix(limb, q);
  out.writeln("largest asymmetry over the batch ", m.sub(m.transpose(-1, -2)).abs().max().item<double>());
  out.writeln("shoulder inertia M11 along the batch ", flat(m.select(-2, 0).select(-1, 0)));
  out.writeln("determinant along the batch          ", flat(m.select(-2, 0).select(-1, 0).mul(m.select(-2, 1).select(-1, 1)).sub(m.select(-2, 0).select(-1, 1).square())));
}
```

Output

```text
elbow [0.0000] degrees   M = [0.3160, 0.0776, 0.0776, 0.0282]
elbow [90.0000] degrees   M = [0.2169, 0.0282, 0.0282, 0.0282]
elbow [150.0000] degrees   M = [0.1312, -0.0147, -0.0147, 0.0282]
largest asymmetry over the batch 0.0000
shoulder inertia M11 along the batch [0.3160, 0.3027, 0.2664, 0.2169, 0.1674, 0.1312]
determinant along the batch          [0.0029, 0.0035, 0.0047, 0.0053, 0.0047, 0.0035]
```

With the arm straight, shoulder inertia is `0.3160` and the coupling is `0.0776`; as the elbow folds, the forearm comes closer to the shoulder and the shoulder inertia falls to `0.2169` and then `0.1312`, while the coupling goes through `0.0282` and becomes negative (`-0.0147`) when the forearm folds back past the upper arm. The elbow's own inertia `0.0282` never changes, since it is the forearm's inertia about its own joint. Over the batch the matrix is exactly symmetric (largest asymmetry `0.0000`), and the determinant `0.0029` to `0.0053` is positive, so the matrix is positive definite for every angle.

## Accelerations from torques

The program releases the arm from rest, horizontal, with no muscle torque, checks that the equation of motion balances, applies only the shoulder part of the holding torque, and finally turns accelerations into torques and back.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace chain = biology::biomechanics::chain;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static const double deg = M_PI / 180.0;
// an arm of two links: upper arm (2.0 kg, 0.30 m) and forearm with hand (1.5 kg, 0.25 m); the centre of mass of each lies a little short of the middle
static chain::Chain arm() { return chain::make(vec({2.0, 1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto limb = arm();
  // release the arm from rest, held out horizontally, with no muscle torque: forward dynamics gives the accelerations
  const auto q = vec({0.0, 0.0}), zero = vec({0.0, 0.0});
  const auto accel = chain::forward_dynamics(limb, q, zero, zero);
  out.writeln("released from rest: (shoulder, elbow) acceleration [rad/s^2] ", flat(accel));
  // check: M qddot + h = tau with tau = 0
  const auto m = chain::mass_matrix(limb, q);
  out.writeln("M qddot + h (should be 0)                                     ", flat(m.matmul(accel.unsqueeze(-1)).squeeze(-1).add(chain::bias_forces(limb, q, zero))));
  // apply only the shoulder part of the holding torque and leave the elbow without a muscle torque
  const auto hold = chain::inverse_dynamics(limb, q, zero, zero);
  const auto tau = vec({flat(hold)[0], 0.0});
  out.writeln("shoulder torque only: accelerations                          ", flat(chain::forward_dynamics(limb, q, zero, tau)));
  // inverse and forward are each other's inverse: take the torques needed for chosen accelerations and ask for the accelerations back
  const auto wanted = vec({2.0, -1.0});
  const auto need = chain::inverse_dynamics(limb, q, zero, wanted);
  out.writeln("torque needed for accelerations (2, -1)                       ", flat(need));
  out.writeln("accelerations that torque produces                            ", flat(chain::forward_dynamics(limb, q, zero, need)));
}
```

Output

```text
released from rest: (shoulder, elbow) acceleration [rad/s^2] [-40.4629, 54.1331]
M qddot + h (should be 0)                                     [0.0000, 0.0000]
shoulder torque only: accelerations                          [43.8634, -178.4755]
torque needed for accelerations (2, -1)                       [9.1351, 1.7452]
accelerations that torque produces                            [2.0000, -1.0000]
```

Released from rest the shoulder accelerates downward at `-40.46` rad/s$^2$ and the elbow at `+54.13`, as the lighter forearm lags behind the falling upper arm. The check $M\ddot q + h = 0$ gives `0.0000` for both joints. If only the shoulder is supported, the unsupported elbow collapses at `-178.48` rad/s$^2$, and the reaction pushes the shoulder up at `43.86`. The last pair confirms that inverse and forward dynamics undo each other: the torques `(9.1351, 1.7452)` needed for accelerations `(2, -1)` return exactly `(2.0000, -1.0000)`.

## Velocity-dependent forces

The program prints the bias forces at one posture for three shoulder speeds, and divides the speed-dependent part by the speed squared.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace chain = biology::biomechanics::chain;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static const double deg = M_PI / 180.0;
// an arm of two links: upper arm (2.0 kg, 0.30 m) and forearm with hand (1.5 kg, 0.25 m); the centre of mass of each lies a little short of the middle
static chain::Chain arm() { return chain::make(vec({2.0, 1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto limb = arm();
  // Coriolis and centrifugal forces: bias_forces h(q, qdot) at one pose for three joint speeds. With zero speed only gravity is left
  const auto q = vec({0.0, 60.0 * deg});
  for (double speed : {0.0, 2.0, 4.0})
    out.writeln("shoulder speed ", std::vector<double>{speed}, " rad/s   h = ", flat(chain::bias_forces(limb, q, vec({speed, 0.0}))));
  // the speed-dependent part is quadratic in the speeds: remove gravity (the speed-0 value) and divide by speed squared
  const auto h0 = flat(chain::bias_forces(limb, q, vec({0.0, 0.0})));
  for (double speed : {2.0, 4.0}) {
    const auto h = flat(chain::bias_forces(limb, q, vec({speed, 0.0})));
    out.writeln("(h - h0) / speed^2 at ", std::vector<double>{speed}, " ", std::vector<double>{(h[0] - h0[0]) / (speed * speed), (h[1] - h0[1]) / (speed * speed)});
  }
}
```

Output

```text
shoulder speed [0.0000] rad/s   h = [7.7718, 0.8090]
shoulder speed [2.0000] rad/s   h = [7.7718, 0.9805]
shoulder speed [4.0000] rad/s   h = [7.7718, 1.4949]
(h - h0) / speed^2 at [2.0000] [0.0000, 0.0429]
(h - h0) / speed^2 at [4.0000] [0.0000, 0.0429]
```

At zero speed the bias forces are the gravity torques, `(7.7718, 0.8090)`. As the shoulder turns, only the elbow torque changes, `0.9805` at `2` rad/s and `1.4949` at `4` rad/s: the forearm swings outward and the centrifugal force must be resisted. Dividing the change by the speed squared gives `0.0429` at both speeds, so the effect is exactly quadratic in the speed, and `0.0429` is $m_2 \ell_1 c_2 \sin q_2 = 1.5 (0.30)(0.11)\sin 60^\circ$.

## A passive swing and its energy

The program releases the arm from horizontal, with zero torque, and integrates it for one second in `200` steps, printing kinetic, potential and total energy at five times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace chain = biology::biomechanics::chain;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static const double deg = M_PI / 180.0;
// an arm of two links: upper arm (2.0 kg, 0.30 m) and forearm with hand (1.5 kg, 0.25 m); the centre of mass of each lies a little short of the middle
static chain::Chain arm() { return chain::make(vec({2.0, 1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto limb = arm();
  // a passive swing: release from horizontal with no muscle torque and integrate for one second. Energy should stay constant
  const auto torque = [](double, const pluss::Series &, const pluss::Series &) { return vec({0.0, 0.0}); };
  const auto q0 = vec({0.0, 0.0}), qd0 = vec({0.0, 0.0});
  const auto path = chain::simulate(limb, torque, q0, qd0, 0.0, 1.0, 200);
  for (std::size_t i : {0, 50, 100, 150, 200}) {
    const auto e = chain::energy(limb, path.q[i], path.qd[i]);
    out.writeln("t = ", std::vector<double>{path.times[i]}, " q ", flat(path.q[i]), "  kinetic ", flat(e.kinetic), "  potential ", flat(e.potential), "  total ", flat(e.total()));
  }
}
```

Output

```text
t = [0.000000] q [0.000000, 0.000000]  kinetic [0.000000]  potential [0.000000]  total [0.000000]
t = [0.250000] q [-1.030830, 0.700441]  kinetic [6.497039]  potential [-6.497039]  total [0.000000]
t = [0.500000] q [-2.518298, -0.176730]  kinetic [4.763044]  potential [-4.763044]  total [0.000000]
t = [0.750000] q [-3.118020, 0.141682]  kinetic [0.430298]  potential [-0.430298]  total [-0.000000]
t = [1.000000] q [-1.749241, -0.608778]  kinetic [7.994238]  potential [-7.994237]  total [0.000001]
```

The arm swings freely through large angles: the shoulder reaches `-3.12` rad at `0.75` s. Kinetic and potential energy trade places (kinetic `6.497039` while the potential energy is `-6.497039`), and their sum stays at zero to within $10^{-6}$ for the whole second. Energy conservation is a strong check on both the equations and the integration.

## Holding a posture with feedback

The program holds the arm at shoulder `20` degrees and elbow `60` degrees with a proportional-derivative law on top of the holding torque, $\tau = \tau_{hold} + k_p(q^* - q) - k_d\dot q$, starting with the elbow straight.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace chain = biology::biomechanics::chain;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static const double deg = M_PI / 180.0;
// an arm of two links: upper arm (2.0 kg, 0.30 m) and forearm with hand (1.5 kg, 0.25 m); the centre of mass of each lies a little short of the middle
static chain::Chain arm() { return chain::make(vec({2.0, 1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto limb = arm();
  // hold the forearm at 60 degrees against gravity with a proportional-derivative controller on both joints plus the holding torque.
  // tau = hold(q*) + kp (q* - q) - kd qdot. Start with the shoulder already at its target and the elbow straight.
  const auto target = vec({20.0 * deg, 60.0 * deg}), zero = vec({0.0, 0.0});
  const auto hold = chain::inverse_dynamics(limb, target, zero, zero);
  const double kp = 80.0, kd = 3.0;
  const auto torque = [&](double, const pluss::Series &q, const pluss::Series &qd) { return hold.add(target.sub(q).mul(kp)).sub(qd.mul(kd)); };
  const auto path = chain::simulate(limb, torque, vec({20.0 * deg, 0.0}), zero, 0.0, 2.0, 1000);
  for (std::size_t i : {0, 50, 125, 250, 500, 1000})
    out.writeln("t = ", std::vector<double>{path.times[i]}, "  joint angles (degrees) ", flat(path.q[i].div(deg)));
  out.writeln("final error (degrees)    ", flat(target.sub(path.q.back()).div(deg)));
}
```

Output

```text
t = [0.0000]  joint angles (degrees) [20.0000, 0.0000]
t = [0.1000]  joint angles (degrees) [17.6933, 55.7766]
t = [0.2500]  joint angles (degrees) [23.6847, 60.7837]
t = [0.5000]  joint angles (degrees) [19.5901, 59.8976]
t = [1.0000]  joint angles (degrees) [20.0620, 60.0136]
t = [2.0000]  joint angles (degrees) [19.9997, 59.9999]
final error (degrees)    [0.0003, 0.0001]
```

The elbow reaches `55.78` degrees in `0.1` s and settles at `60.01` degrees by `1` s; after `2` s the error is `0.0003` and `0.0001` degrees. The overshoot at `0.25` s (`23.68` degrees at the shoulder) is the reaction to the forearm's quick motion. The feedback law is just a Series function, so any torque law can be used.

## Decisions

- Use `forward_dynamics` when you know the torques and need the motion, and `inverse_dynamics` when you know the motion (from a recording, say) and need the torques.
- Use `mass_matrix` and `bias_forces` directly when you build your own controller or optimiser; they are the parts of the equation of motion.
- Pick the number of `steps` so that the time step is small compared with the fastest time scale of the limb: stiff feedback needs a small step.
- Check `energy` on a passive swing whenever you change the step or the parameters.

## Pitfalls

RK4 with a step that is too large loses energy or gains it. The program repeats the passive swing with `4`, `10`, `50` and `200` steps and prints the drift of the total energy after one second.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace chain = biology::biomechanics::chain;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static const double deg = M_PI / 180.0;
// an arm of two links: upper arm (2.0 kg, 0.30 m) and forearm with hand (1.5 kg, 0.25 m); the centre of mass of each lies a little short of the middle
static chain::Chain arm() { return chain::make(vec({2.0, 1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01})); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto limb = arm();
  // the same passive swing with too few steps: energy is not conserved when the step is too coarse
  const auto torque = [](double, const pluss::Series &, const pluss::Series &) { return vec({0.0, 0.0}); };
  const auto q0 = vec({0.0, 0.0}), qd0 = vec({0.0, 0.0});
  const double e0 = flat(chain::energy(limb, q0, qd0).total())[0];
  for (std::int64_t steps : {4, 10, 50, 200}) {
    const auto path = chain::simulate(limb, torque, q0, qd0, 0.0, 1.0, steps);
    const double e = flat(chain::energy(limb, path.q.back(), path.qd.back()).total())[0];
    out.writeln("steps ", std::vector<double>{static_cast<double>(steps)}, " energy drift after one second ", std::vector<double>{e - e0});
  }
}
```

Output

```text
steps [4.0000] energy drift after one second [-2.0869]
steps [10.0000] energy drift after one second [0.5123]
steps [50.0000] energy drift after one second [0.0001]
steps [200.0000] energy drift after one second [0.0000]
```

With `4` steps the total energy drifts by `-2.0869`, and with `10` by `0.5123`: the swing is completely wrong. With `50` steps the drift is `0.0001`, and with `200` it is below the printed precision. A passive swing is an easy test of a step size before you use it on a driven motion.

## Where next

Go back to [Geometry and holding torques](statics.md) for placement and loads, to [Biology 11: Muscle](../bio_11_muscle.md) for the muscle model that produces the torque, and to [Biology 13: Blood Flow](../bio_13_blood_flow.md) for the other biomechanics module.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="statics.html">
    <span class="label">Previous</span>
    <span class="title">Geometry and holding torques</span>
  </a>
</div>
<!-- apiref:nav:end -->
