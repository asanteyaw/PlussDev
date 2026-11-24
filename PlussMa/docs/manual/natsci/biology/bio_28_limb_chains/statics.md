# Geometry and holding torques

[Back to Biology 28: Limb Chains](../bio_28_limb_chains.md)

You have a limb described by the masses and lengths of its segments, and you want to know where each joint and each centre of mass is for a given posture, and what torque each joint must supply to hold the posture still, with or without a weight in the hand. This page builds a two-link arm, places it, finds its holding torques and adds an external load.

## How it works

A `chain::Chain` is an open chain of $n$ rigid links, fixed at a joint at the origin, in a vertical plane with gravity along $-y$. Each link has a mass $m_i$, a length $\ell_i$, a centre-of-mass distance $c_i$ from its proximal joint, and an inertia $I_i$ about its centre of mass. Joint angles are *relative*, anatomical angles $q_i$: the absolute orientation of link $i$ is the running sum, measured counter-clockwise from the $+x$ axis,

```{math}
\theta_i = q_1 + q_2 + \dots + q_i
```

`chain::make(mass, length, com, inertia)` builds the chain from four 1-D Series, and `chain::forward_kinematics(c, q)` returns the joint positions `joint_x`, `joint_y` (base, then the far end of each link) and the centre-of-mass positions `com_x`, `com_y`. The torque $\tau_i$ that the actuator at joint $i$ applies to link $i$ (positive counter-clockwise) comes from `chain::inverse_dynamics(c, q, qdot, qddot, end_force)`, the recursive Newton-Euler algorithm. For a posture held still, set the velocity and acceleration to zero. An optional `end_force` $(F_x, F_y)$ acts on the far end of the last link, for a held weight or a ground reaction. Everything is batched: `q` may have shape `[..., n]` with any leading axes, and every function evaluates all postures in one call. The programs use an arm with an upper arm of `2.0` kg and `0.30` m and a forearm with hand of `1.5` kg and `0.25` m, whose centres of mass lie at `0.13` m and `0.11` m from their proximal joints.

## Where the joints are

The program places the arm with the shoulder at `30` degrees above horizontal and the elbow bent a further `45` degrees, and prints where the joints and the centres of mass lie, and checks the forearm against its known length and direction.

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
  // joint angles are relative: the shoulder at 30 degrees above horizontal, the elbow bent a further 45 degrees
  const auto q = vec({30.0 * deg, 45.0 * deg});
  const auto k = chain::forward_kinematics(limb, q);
  out.writeln("links                         ", std::vector<double>{static_cast<double>(limb.links())});
  out.writeln("joint x (base, elbow, hand)   ", flat(k.joint_x));
  out.writeln("joint y (base, elbow, hand)   ", flat(k.joint_y));
  out.writeln("centre of mass x              ", flat(k.com_x));
  out.writeln("centre of mass y              ", flat(k.com_y));
  // absolute angle of the forearm is the sum of the two relative angles: 75 degrees, so the hand is 0.25 m from the elbow along that direction
  const auto jx = flat(k.joint_x), jy = flat(k.joint_y);
  out.writeln("forearm length from the joints ", std::vector<double>{std::hypot(jx[2] - jx[1], jy[2] - jy[1])});
  out.writeln("forearm direction (degrees)    ", std::vector<double>{std::atan2(jy[2] - jy[1], jx[2] - jx[1]) / deg});
}
```

Output

```text
links                         [2.0000]
joint x (base, elbow, hand)   [0.0000, 0.2598, 0.3245]
joint y (base, elbow, hand)   [0.0000, 0.1500, 0.3915]
centre of mass x              [0.1126, 0.2883]
centre of mass y              [0.0650, 0.2563]
forearm length from the joints [0.2500]
forearm direction (degrees)    [75.0000]
```

The elbow is at $(0.2598, 0.1500) = 0.30\,(\cos 30^\circ, \sin 30^\circ)$ and the hand at $(0.3245, 0.3915)$. The distance from elbow to hand is `0.2500`, the length of the forearm, and its direction is `75` degrees, the sum of the two relative angles, as it should be. The centre of mass of the upper arm at $x = 0.1126$ is $0.13\cos 30^\circ$.

## Many postures in one call

The program gives `q` a leading axis, seven shoulder angles from `0` to `90` degrees with the elbow fixed at `60`, and reads the hand position and the reach of the arm for all of them at once.

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
  // seven arm poses at once: the shoulder swings from 0 to 90 degrees while the elbow stays at 60 degrees. q has shape [poses, 2]
  const auto shoulder = pluss::series::linspace(0.0, 90.0 * deg, 7, pluss::ScalarType::Float64);
  const auto q = shoulder.unsqueeze(-1).unite(std::vector<pluss::Series>{shoulder.mul(0.0).add(60.0 * deg).unsqueeze(-1)}, 1);
  const auto k = chain::forward_kinematics(limb, q);
  out.writeln("shoulder angle (degrees)   ", flat(shoulder.div(deg)));
  out.writeln("hand x                     ", flat(k.joint_x.select(1, 2)));
  out.writeln("hand y                     ", flat(k.joint_y.select(1, 2)));
  // distance of the hand from the shoulder
  out.writeln("reach                      ", flat(k.joint_x.select(1, 2).square().add(k.joint_y.select(1, 2).square()).sqrt()));
}
```

Output

```text
shoulder angle (degrees)   [0.0000, 15.0000, 30.0000, 45.0000, 60.0000, 75.0000, 90.0000]
hand x                     [0.4250, 0.3545, 0.2598, 0.1474, 0.0250, -0.0991, -0.2165]
hand y                     [0.2165, 0.3191, 0.4000, 0.4536, 0.4763, 0.4666, 0.4250]
reach                      [0.4770, 0.4770, 0.4770, 0.4770, 0.4770, 0.4770, 0.4770]
```

The hand rises and swings back as the shoulder lifts: from $(0.4250, 0.2165)$ at `0` degrees to $(-0.2165, 0.4250)$ at `90`. The reach, the distance from shoulder to hand, is `0.4770` in every posture, because a fixed elbow angle fixes the shape of the arm and the shoulder only rotates it. A check like that is a good test of any posture table.

## Holding torques

The program asks for the torques that hold the arm still in three postures, and checks the horizontal one by hand: the weight of each link times the horizontal distance of its centre of mass from the joint.

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
  // torques that hold the arm still (no velocity, no acceleration) against its own weight, for the arm stretched out horizontally and for the arm hanging down
  const auto zero = vec({0.0, 0.0});
  const auto horizontal = vec({0.0, 0.0}), hanging = vec({-M_PI / 2.0, 0.0}), bent = vec({0.0, M_PI / 2.0});
  out.writeln("horizontal, straight   tau (shoulder, elbow) = ", flat(chain::inverse_dynamics(limb, horizontal, zero, zero)));
  out.writeln("hanging straight down  tau (shoulder, elbow) = ", flat(chain::inverse_dynamics(limb, hanging, zero, zero)));
  out.writeln("horizontal, forearm up tau (shoulder, elbow) = ", flat(chain::inverse_dynamics(limb, bent, zero, zero)));
  // by hand for the horizontal arm: the elbow carries the forearm weight at its centre of mass, the shoulder carries both
  const double g = limb.gravity;
  const double elbow = 1.5 * g * 0.11, shoulder = 2.0 * g * 0.13 + 1.5 * g * (0.30 + 0.11);
  out.writeln("by hand (shoulder, elbow)  = ", std::vector<double>{shoulder, elbow});
}
```

Output

```text
horizontal, straight   tau (shoulder, elbow) = [8.5808, 1.6181]
hanging straight down  tau (shoulder, elbow) = [0.0000, 0.0000]
horizontal, forearm up tau (shoulder, elbow) = [6.9627, 0.0000]
by hand (shoulder, elbow)  = [8.5808, 1.6181]
```

Held straight out, the arm needs `8.5808` N m at the shoulder and `1.6181` N m at the elbow. By hand, the elbow carries the forearm's weight at its centre of mass, $1.5\,g\,(0.11) = 1.6181$, and the shoulder carries both links, $2.0\,g\,(0.13) + 1.5\,g\,(0.30 + 0.11) = 8.5808$. Hanging straight down, gravity acts along the arm and the torques are `0.0000`. With the forearm raised vertically the elbow torque vanishes (the forearm's weight now acts through the elbow) and the shoulder needs `6.9627` N m for the whole arm's weight at the shoulder's lever arms.

## Holding a weight

The program holds the arm out horizontally with a `2` kg weight in the hand, a downward force $(0, -2g)$ at the end of the last link, with and without the weight, and compares the extra torque with the product of the weight and its lever arm.

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
  // the arm held out horizontally, now holding a 2 kg weight in the hand: the load is a force of (0, -2 g) newtons at the hand
  const double weight = 2.0 * limb.gravity;
  const auto q = vec({0.0, 0.0}), zero = vec({0.0, 0.0});
  const auto load = vec({0.0, -weight});
  const auto without = chain::inverse_dynamics(limb, q, zero, zero);
  const auto with = chain::inverse_dynamics(limb, q, zero, zero, load);
  out.writeln("torques without the weight (shoulder, elbow) ", flat(without));
  out.writeln("torques with the 2 kg weight                 ", flat(with));
  out.writeln("extra torque                                    ", flat(with.sub(without)));
  out.writeln("end_point_torques of the load alone             ", flat(chain::end_point_torques(limb, q, load)));
  // by hand: the weight acts at the hand, 0.25 m beyond the elbow and 0.55 m beyond the shoulder
  out.writeln("by hand (shoulder, elbow)                       ", std::vector<double>{weight * 0.55, weight * 0.25});
}
```

Output

```text
torques without the weight (shoulder, elbow) [8.5808, 1.6181]
torques with the 2 kg weight                 [19.3681, 6.5214]
extra torque                                    [10.7873, 4.9033]
end_point_torques of the load alone             [-10.7873, -4.9033]
by hand (shoulder, elbow)                       [10.7873, 4.9033]
```

The weight, `19.61` N, adds `10.7873` N m at the shoulder and `4.9033` N m at the elbow, and by hand these are the weight times its horizontal distances from the joints, `0.55` m and `0.25` m. `end_point_torques` returns the torques the load itself puts on the joints, $-10.7873$ and $-4.9033$: the load turns the arm clockwise, and the actuators must supply the same amount counter-clockwise.

## Decisions

- Use relative joint angles, as a clinician measures them. If you hold absolute segment angles instead, convert them to relative ones by differences first.
- Pass postures on a leading axis of `q` instead of looping: the same call serves a motion-capture trial of thousands of frames.
- Pass external loads as `end_force`, never as extra mass, when the load is a weight carried in the hand: the mass of a held object enters the dynamics of the arm as a force at the hand.
- Choose `com` and `inertia` from the segment you model: they are inputs, not derived from the length.

## Pitfalls

Malformed limbs are rejected when you build them or use them. The program tries a negative mass, lengths of the wrong size, and three joint angles for a two-link arm.

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
  // malformed limbs and states are rejected with a message
  try {
    chain::make(vec({2.0, -1.5}), vec({0.30, 0.25}), vec({0.13, 0.11}), vec({0.02, 0.01}));
  } catch (const pluss::Error &e) {
    out.writeln("negative mass: ", e.message());
  }
  try {
    chain::make(vec({2.0, 1.5}), vec({0.30}), vec({0.13, 0.11}), vec({0.02, 0.01}));
  } catch (const pluss::Error &e) {
    out.writeln("lengths of the wrong size: ", e.message());
  }
  const auto limb = arm();
  try {
    chain::inverse_dynamics(limb, vec({0.0, 0.0, 0.0}), vec({0.0, 0.0, 0.0}), vec({0.0, 0.0, 0.0}));
  } catch (const pluss::Error &e) {
    out.writeln("three joint angles for a two-link arm: ", e.message());
  }
}
```

Output

```text
negative mass: natsci: chain::make: positive masses and lengths, non-negative inertias
lengths of the wrong size: natsci: chain::make: mass, length, com and inertia must be 1-D of equal length
three joint angles for a two-link arm: natsci: chain: q, qdot, qddot must have one entry per joint
```

Each message names the function and the rule. A mass or length must be positive, the four parameter lists must have one entry for every link, and the state must have one entry for every joint.

## Where next

Continue with [Moving limbs](motion.md) for the accelerations that torques produce and for time integration. [Biology 11: Muscle](../bio_11_muscle.md) supplies the muscle forces that become the torques here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="motion.html">
    <span class="label">Next</span>
    <span class="title">Moving limbs</span>
  </a>
</div>
<!-- apiref:nav:end -->
