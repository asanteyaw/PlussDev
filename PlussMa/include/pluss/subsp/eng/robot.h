#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <vector>

namespace pluss::subsp::eng::robot {

// Rotation matrix of a rotation by `angle` [rad] about `axis`: axis [..., 3] (need not be unit length) and angle [...]
// broadcast, giving [..., 3, 3].
Series rotation(const Series &axis, const Series &angle);

// Rotation matrix of roll-pitch-yaw angles [..., 3] about the fixed x, y and z axes in that order: R = Rz Ry Rx.
Series rpy(const Series &angles);

// An articulated robot as a tree of rigid links, one joint per link. Link i hangs from `parent[i]` (-1 for the base) and
// parent[i] < i. Its joint frame sits at `translation[i]` and `rpy[i]` in the parent frame when the joint is at zero; a
// revolute joint turns the link about `axis[i]`, a prismatic joint slides it along `axis[i]`, both expressed in the joint
// frame. Mass properties are in the link's own frame. All values are real floating Series of one dtype and platform.
struct Links {
  std::vector<std::int64_t> parent;
  std::vector<std::int64_t> prismatic;
  Series axis{};
  Series translation{};
  Series rpy{};
  Series mass{};
  Series center{};
  Series inertia{};
};

// Link frames in the world, for configurations [..., n]: rotation [..., n, 3, 3] and position [..., n, 3].
struct Pose {
  Series rotation;
  Series position;
};

// Gravity as a vector in the base frame; undefined means (0, 0, -9.81).
struct DynamicsOptions {
  Series gravity{};
};

// `lower` and `upper` are optional joint limits [links]; the iteration never leaves them.
struct IkOptions {
  std::int64_t max_iterations{100};
  double tolerance{1e-8};
  double damping{1e-3};
  double orientation_weight{1.0};
  Series lower{};
  Series upper{};
};

struct IkResult {
  Series q;
  Series error;
  Series iterations;
  Series converged;
};

class Robot final {
public:
  std::int64_t links() const noexcept { return links_; }
  ScalarType dtype() const noexcept { return dtype_; }
  Platform platform() const noexcept { return axis_.platform(); }
  bool has_mass() const noexcept { return has_mass_; }
  const std::vector<std::int64_t> &parent() const noexcept { return parent_; }

private:
  friend struct RobotAccess;
  friend Robot make(const Links &);
  Robot() = default;

  std::int64_t links_{};
  ScalarType dtype_{};
  bool has_mass_{};
  std::vector<std::int64_t> parent_;
  std::vector<std::int64_t> prismatic_;
  Series axis_, offset_rotation_, offset_position_, mass_, center_, inertia_;
};

// Validates the description and compiles the constant transforms once.
Robot make(const Links &links);

// Frames of every link for configurations q [..., n].
Pose frames(const Robot &robot, const Series &q);

// Geometric Jacobian of the origin of `link`'s frame in the world frame, [..., 6, n]: three rows of linear velocity and
// three of angular velocity per unit joint rate.
Series jac(const Robot &robot, const Series &q, std::int64_t link);

// Joint forces and torques [..., n] for the given motion (recursive Newton-Euler).
Series torques(const Robot &robot, const Series &q, const Series &qd, const Series &qdd,
                        const DynamicsOptions &options = {});

// Joint-space inertia matrix [..., n, n].
Series inertia(const Robot &robot, const Series &q);

// Everything in the dynamics except the inertial term: Coriolis, centrifugal and gravity forces [..., n].
Series bias(const Robot &robot, const Series &q, const Series &qd, const DynamicsOptions &options = {});

// Joint accelerations [..., n] under joint forces `tau`.
Series accel(const Robot &robot, const Series &q, const Series &qd, const Series &tau,
                        const DynamicsOptions &options = {});

// Kinetic energy 0.5 qd' M qd and gravitational potential energy, both [...].
Series ke(const Robot &robot, const Series &q, const Series &qd);
Series pe(const Robot &robot, const Series &q, const DynamicsOptions &options = {});

// Damped least-squares inverse kinematics so that `link`'s frame reaches `target_position` [..., 3] and, when defined,
// `target_rotation` [..., 3, 3], starting from q0 [..., n].
IkResult ik(const Robot &robot, std::int64_t link, const Series &target_position,
                            const Series &target_rotation, const Series &q0, const IkOptions &options = {});

// Frictionless unilateral contacts of points fixed to links with planes. Contact i is a point `point[i]` [3] in the frame of
// link `link[i]` and the half-space n . x >= offset, with the unit normal `normal[i]` [3] and `offset[i]` in the world. The
// contact acts (pushes along the normal, never pulls) while its gap n . x - offset is at most `contact_threshold`. An optional
// `friction` [m] is the Coulomb coefficient of every contact (undefined or zero: frictionless): the tangential force is limited
// to friction times the normal force along two tangent directions (the pyramid approximation of the friction cone).
struct Contacts {
  std::vector<std::int64_t> link;
  Series point{};
  Series normal{};
  Series offset{};
  Series friction{};
};

// Bilateral constraints that hold a point of a link at rest in the world: three equations per pin (a pin joint to the
// ground, or a point held fixed), with free force components.
struct Pins {
  std::vector<std::int64_t> link;
  Series point{};
};

struct ConstraintOptions {
  DynamicsOptions dynamics{};
  double contact_threshold{1e-6};
  std::int64_t friction_iterations{12};
};

struct ConstrainedResult {
  Series acceleration;
  Series contact_force;
  Series pin_force;
  Series friction_force;
  Series gap;
};

struct ImpactResult {
  Series velocity;
  Series contact_impulse;
  Series pin_impulse;
  Series friction_impulse;
};

// World position of points fixed to links, `point` [m, 3] in the link frames, [..., m, 3].
Series locate(const Robot &robot, const Series &q, const std::vector<std::int64_t> &link, const Series &point);

// Gap n . x - offset of every contact, [..., m].
Series gap(const Robot &robot, const Contacts &contacts, const Series &q);

// Joint accelerations [..., n] of a robot under joint forces `tau` that also satisfies the pins (their points do not
// accelerate) and the contacts (contacts in range push with the non-negative normal forces that keep the point from
// accelerating into the plane, complementarity holds). The forces are returned: `contact_force` [..., m] along the
// normals and `pin_force` [..., p, 3] in the world. The contact problem is the linear complementarity problem of the
// Delassus matrix, solved as a quadratic program by `math::qp` (accurate to its tolerances), so the batch is solved at once.
// Either record may be left empty.
ConstrainedResult constrain(const Robot &robot, const Contacts &contacts, const Pins &pins, const Series &q,
                                       const Series &qd, const Series &tau, const ConstraintOptions &options = {});

// Velocities after an impact: the contacts in range that approach their planes (normal velocity below zero) receive non-negative
// impulses along their normals so that the normal velocity after is `restitution` times the approach speed (0: a plastic impact,
// 1: elastic), the pins stop their points, and friction (when set) limits the tangential impulse to friction times the normal
// one. Contacts that are out of range or not approaching, and joints not touched by a constraint, are unaffected except through
// the inertia. Result: the joint velocities after [..., n], the normal impulses [..., m], the pin impulses [..., p, 3] and the
// world friction impulses [..., m, 3].
ImpactResult impact(const Robot &robot, const Contacts &contacts, const Pins &pins, const Series &q, const Series &qd,
                    double restitution, const ConstraintOptions &options = {});

} // namespace pluss::subsp::eng::robot
