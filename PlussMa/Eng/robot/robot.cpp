#include "pluss/subsp/eng/robot.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/qp.h"
#include "../detail/series_util.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace pluss::subsp::eng::robot {

// Read access to the compiled constants for the free functions below.
struct RobotAccess {
  static const Series &axis(const Robot &r) { return r.axis_; }
  static const Series &offset_rotation(const Robot &r) { return r.offset_rotation_; }
  static const Series &offset_position(const Robot &r) { return r.offset_position_; }
  static const Series &mass(const Robot &r) { return r.mass_; }
  static const Series &center(const Robot &r) { return r.center_; }
  static const Series &inertia(const Robot &r) { return r.inertia_; }
  static const std::vector<std::int64_t> &prismatic(const Robot &r) { return r.prismatic_; }
};

namespace {

using Shape = std::vector<std::int64_t>;
using detail::max_abs;

Config config(ScalarType dtype, Platform platform) { return Config{}.with_dtype(dtype).with_platform(platform); }

Shape leading(const Series &value, std::int64_t trailing) {
  return Shape(value.sizes().begin(), value.sizes().end() - trailing);
}

Shape with(Shape shape, std::initializer_list<std::int64_t> tail) {
  shape.insert(shape.end(), tail.begin(), tail.end());
  return shape;
}

// Cross product of vectors [..., 3] that may differ in their leading axes (broadcast).
Shape leading_of(const Series &q) { return Shape(q.sizes().begin(), q.sizes().end() - 1); }

Series vec_cross(const Series &a, const Series &b) {
  const Series a0 = a.select(-1, 0), a1 = a.select(-1, 1), a2 = a.select(-1, 2);
  const Series b0 = b.select(-1, 0), b1 = b.select(-1, 1), b2 = b.select(-1, 2);
  const Series x = a1.mul(b2).sub(a2.mul(b1)), y = a2.mul(b0).sub(a0.mul(b2)), z = a0.mul(b1).sub(a1.mul(b0));
  return x.unsqueeze(-1).unite({y.unsqueeze(-1), z.unsqueeze(-1)}, -1);
}

// R v for rotation matrices [..., 3, 3] and vectors [..., 3].
Series apply(const Series &rotation_matrix, const Series &v) {
  return rotation_matrix.matmul(v.unsqueeze(-1)).squeeze(-1);
}

Series dot(const Series &a, const Series &b) { return a.mul(b).sum(-1); }

Series zeros_like_batch(const Shape &batch, std::int64_t tail, ScalarType dtype, Platform platform) {
  return pluss::series::zeros(with(batch, {tail}), config(dtype, platform));
}

void check_values(const Series &value, const char *name, std::vector<std::int64_t> tail, std::int64_t n, ScalarType &dtype,
                  Platform &platform, bool &have) {
  std::vector<std::int64_t> want{n};
  want.insert(want.end(), tail.begin(), tail.end());
  PLUSS_CHECK(value.dtype().is_floating() && value.sizes() == want,
              std::string("eng::robot::make: ") + name + " must be a real floating array with one row per link");
  PLUSS_CHECK(value.isfinite().all().item<bool>(), std::string("eng::robot::make: ") + name + " must be finite");
  if (!have) {
    dtype = value.scalar_type();
    platform = value.platform();
    have = true;
  } else {
    PLUSS_CHECK(dtype == value.scalar_type() && platform == value.platform(),
                "eng::robot::make: every value array must share dtype and platform");
  }
}

} // namespace

Series rotation(const Series &axis, const Series &angle) {
  PLUSS_CHECK(axis.defined() && axis.dtype().is_floating() && axis.dim() >= 1 && axis.size(-1) == 3,
              "eng::robot::rotation: axis must be a real floating [..., 3] array");
  PLUSS_CHECK(angle.defined() && angle.dtype().is_floating(), "eng::robot::rotation: angle must be a real floating array");
  const Series length = axis.mul(axis).sum(-1).sqrt();
  PLUSS_CHECK(length.gt(0.0).all().item<bool>(), "eng::robot::rotation: axis must not be zero");
  const Series k = axis.div(length.unsqueeze(-1));
  const Series c = angle.cos(), s = angle.sin();
  const Series x = k.select(-1, 0), y = k.select(-1, 1), z = k.select(-1, 2);
  const Series t = c.neg().add(1.0);
  auto cell = [&](const Series &value) { return value.unsqueeze(-1); };
  const Series row0 = cell(t.mul(x).mul(x).add(c)).unite({cell(t.mul(x).mul(y).sub(s.mul(z))), cell(t.mul(x).mul(z).add(s.mul(y)))}, -1);
  const Series row1 = cell(t.mul(x).mul(y).add(s.mul(z))).unite({cell(t.mul(y).mul(y).add(c)), cell(t.mul(y).mul(z).sub(s.mul(x)))}, -1);
  const Series row2 = cell(t.mul(x).mul(z).sub(s.mul(y))).unite({cell(t.mul(y).mul(z).add(s.mul(x))), cell(t.mul(z).mul(z).add(c))}, -1);
  return row0.unsqueeze(-2).unite({row1.unsqueeze(-2), row2.unsqueeze(-2)}, -2);
}

Series rpy(const Series &angles) {
  PLUSS_CHECK(angles.defined() && angles.dtype().is_floating() && angles.dim() >= 1 && angles.size(-1) == 3,
              "eng::robot::rpy: angles must be a real floating [..., 3] array");
  const Config cfg = config(angles.scalar_type(), angles.platform());
  const Shape lead = leading(angles, 1);
  auto axis_of = [&](double x, double y, double z) {
    return pluss::series::make(std::vector<double>{x, y, z}).to(angles.scalar_type()).to(angles.platform());
  };
  (void)cfg;
  (void)lead;
  const Series rx = rotation(axis_of(1, 0, 0), angles.select(-1, 0));
  const Series ry = rotation(axis_of(0, 1, 0), angles.select(-1, 1));
  const Series rz = rotation(axis_of(0, 0, 1), angles.select(-1, 2));
  return rz.matmul(ry).matmul(rx);
}

Robot make(const Links &links) {
  const auto n = static_cast<std::int64_t>(links.parent.size());
  PLUSS_CHECK(n >= 1, "eng::robot::make: a robot needs at least one link");
  PLUSS_CHECK(static_cast<std::int64_t>(links.prismatic.size()) == n,
              "eng::robot::make: prismatic needs one entry per link");
  for (std::int64_t i = 0; i < n; ++i) {
    const auto p = links.parent[static_cast<std::size_t>(i)];
    PLUSS_CHECK(p >= -1 && p < i, "eng::robot::make: parent[i] must be -1 or the index of an earlier link");
    const auto kind = links.prismatic[static_cast<std::size_t>(i)];
    PLUSS_CHECK(kind == 0 || kind == 1, "eng::robot::make: prismatic must be 0 (revolute) or 1 (prismatic)");
  }
  ScalarType dtype = ScalarType::Float64;
  Platform platform = Platform::CPU;
  bool have = false;
  PLUSS_CHECK(links.axis.defined() && links.translation.defined(), "eng::robot::make: axis and translation are required");
  check_values(links.axis, "axis", {3}, n, dtype, platform, have);
  check_values(links.translation, "translation", {3}, n, dtype, platform, have);
  if (links.rpy.defined()) check_values(links.rpy, "rpy", {3}, n, dtype, platform, have);
  const bool has_mass = links.mass.defined();
  if (has_mass) {
    check_values(links.mass, "mass", {}, n, dtype, platform, have);
    PLUSS_CHECK(links.mass.ge(0.0).all().item<bool>(), "eng::robot::make: mass must be non-negative");
    if (links.center.defined()) check_values(links.center, "center", {3}, n, dtype, platform, have);
    if (links.inertia.defined()) check_values(links.inertia, "inertia", {3, 3}, n, dtype, platform, have);
  } else {
    PLUSS_CHECK(!links.center.defined() && !links.inertia.defined(), "eng::robot::make: center and inertia need mass");
  }
  for (std::int64_t i = 0; i < n; ++i) {
    const Series length = links.axis.select(0, i).abs().sum(-1);
    PLUSS_CHECK(length.item<double>() > 0.0, "eng::robot::make: every joint axis must be non-zero");
  }
  if (links.inertia.defined()) {
    const Series asymmetry = links.inertia.sub(links.inertia.mT()).abs().amax(-1, false).amax(-1, false);
    PLUSS_CHECK(asymmetry.amax().item<double>() <= 1e-12 * (1.0 + links.inertia.abs().amax().item<double>()),
                "eng::robot::make: every inertia matrix must be symmetric");
  }

  Robot robot;
  const Config cfg = config(dtype, platform);
  robot.links_ = n;
  robot.dtype_ = dtype;
  robot.has_mass_ = has_mass;
  robot.parent_ = links.parent;
  robot.prismatic_ = links.prismatic;
  // unit axes
  const Series length = links.axis.mul(links.axis).sum(-1).sqrt().unsqueeze(-1);
  robot.axis_ = links.axis.div(length);
  robot.offset_position_ = links.translation;
  robot.offset_rotation_ = links.rpy.defined() ? rpy(links.rpy)
                                               : pluss::series::eye(3, cfg).unsqueeze(0).expand({n, 3, 3}, true);
  if (has_mass) {
    robot.mass_ = links.mass;
    robot.center_ = links.center.defined() ? links.center : pluss::series::zeros({n, 3}, cfg);
    robot.inertia_ = links.inertia.defined() ? links.inertia : pluss::series::zeros({n, 3, 3}, cfg);
  }
  return robot;
}

namespace {

struct Frames {
  std::vector<Series> rpc;     // rotation of link frame i in its parent, [..., 3, 3]
  std::vector<Series> offset;  // origin of link frame i in its parent, [..., 3]
  Shape batch;
};

Shape batch_of(const Robot &robot, const Series &q, const char *who, const char *name = "q") {
  PLUSS_CHECK(q.defined() && q.dtype().is_floating() && q.dim() >= 1 && q.size(-1) == robot.links(),
              std::string("eng::robot::") + who + ": " + name + " must be a real floating [..., " +
                  std::to_string(robot.links()) + "] array");
  PLUSS_CHECK(q.platform() == robot.platform(),
              std::string("eng::robot::") + who + ": " + name + " and the robot must share a platform");
  return leading(q, 1);
}

Frames link_frames(const Robot &robot, const Series &q) {
  const Shape batch = leading(q, 1);
  const ScalarType dtype = robot.dtype();
  const Platform platform = robot.platform();
  const Series eye = pluss::series::zeros(with(batch, {3, 3}), config(dtype, platform)).add(pluss::series::eye(3, config(dtype, platform)));
  Frames out;
  out.batch = batch;
  const auto &prismatic = RobotAccess::prismatic(robot);
  for (std::int64_t i = 0; i < robot.links(); ++i) {
    const Series offset_rotation = RobotAccess::offset_rotation(robot).select(0, i);
    const Series offset_position = RobotAccess::offset_position(robot).select(0, i);
    const Series axis = RobotAccess::axis(robot).select(0, i);
    const Series angle = q.select(-1, i).to(dtype);
    if (prismatic[static_cast<std::size_t>(i)] == 0) {
      out.rpc.push_back(offset_rotation.matmul(rotation(axis, angle)));
      out.offset.push_back(zeros_like_batch(batch, 3, dtype, platform).add(offset_position));
    } else {
      out.rpc.push_back(eye.matmul(offset_rotation));
      out.offset.push_back(offset_position.add(apply(offset_rotation, axis).mul(angle.unsqueeze(-1))));
    }
  }
  return out;
}

// The one place that composes link frames down the tree: world rotation and position of every link's frame.
struct World {
  std::vector<Series> rotation;
  std::vector<Series> position;
};

World world_frames(const Robot &robot, const Frames &f) {
  World w;
  for (std::int64_t i = 0; i < robot.links(); ++i) {
    const auto parent = robot.parent()[static_cast<std::size_t>(i)];
    const Series &local_rotation = f.rpc[static_cast<std::size_t>(i)];
    const Series &local_offset = f.offset[static_cast<std::size_t>(i)];
    if (parent < 0) {
      w.rotation.push_back(local_rotation);
      w.position.push_back(local_offset);
    } else {
      const Series parent_rotation = w.rotation[static_cast<std::size_t>(parent)];   // a copy: push_back may reallocate
      const Series parent_position = w.position[static_cast<std::size_t>(parent)];
      w.rotation.push_back(parent_rotation.matmul(local_rotation));
      w.position.push_back(parent_position.add(apply(parent_rotation, local_offset)));
    }
  }
  return w;
}

Series world_axis_origin(const Robot &robot, std::int64_t joint, const std::vector<Series> &world_rotation) {
  // the joint axis in the world frame: the joint frame's rotation (before the joint motion) applied to the axis
  const auto parent = robot.parent()[static_cast<std::size_t>(joint)];
  const Series offset = RobotAccess::offset_rotation(robot).select(0, joint);
  const Series axis = RobotAccess::axis(robot).select(0, joint);
  const Series local = apply(offset, axis);
  if (parent < 0) return local;
  return apply(world_rotation[static_cast<std::size_t>(parent)], local);
}

Series gravity_vector(const Robot &robot, const DynamicsOptions &options) {
  const Config cfg = config(robot.dtype(), robot.platform());
  if (!options.gravity.defined()) return pluss::series::make(std::vector<double>{0.0, 0.0, -9.81}).to(robot.dtype()).to(robot.platform());
  PLUSS_CHECK(options.gravity.dtype().is_floating() && options.gravity.sizes() == Shape{3},
              "eng::robot: gravity must be a real floating vector of three components");
  return options.gravity.to(robot.dtype()).to(robot.platform());
}

void need_mass(const Robot &robot, const char *who) {
  PLUSS_CHECK(robot.has_mass(), std::string("eng::robot::") + who + ": the robot has no mass properties; set mass (and "
                                                                    "optionally center and inertia) in Links");
}

} // namespace

Pose frames(const Robot &robot, const Series &q) {
  const Shape batch = batch_of(robot, q, "frames");
  const Frames f = link_frames(robot, q);
  const auto n = robot.links();
  const World world = world_frames(robot, f);
  const std::vector<Series> &rotations = world.rotation, &positions = world.position;
  std::vector<Series> r_rest, p_rest;
  for (std::int64_t i = 1; i < n; ++i) {
    r_rest.push_back(rotations[static_cast<std::size_t>(i)].unsqueeze(-3));
    p_rest.push_back(positions[static_cast<std::size_t>(i)].unsqueeze(-2));
  }
  (void)batch;
  return {rotations[0].unsqueeze(-3).unite(r_rest, -3), positions[0].unsqueeze(-2).unite(p_rest, -2)};
}

Series jac(const Robot &robot, const Series &q, std::int64_t link) {
  batch_of(robot, q, "jac");
  const auto n = robot.links();
  PLUSS_CHECK(link >= 0 && link < n, "eng::robot::jac: link is outside the robot");
  const Frames f = link_frames(robot, q);
  const Shape batch = f.batch;
  const Config cfg = config(robot.dtype(), robot.platform());
  const World world = world_frames(robot, f);
  const std::vector<Series> &rotations = world.rotation, &positions = world.position;
  std::vector<bool> ancestor(static_cast<std::size_t>(n), false);
  for (std::int64_t i = link; i >= 0; i = robot.parent()[static_cast<std::size_t>(i)]) ancestor[static_cast<std::size_t>(i)] = true;
  const Series none = zeros_like_batch(batch, 3, robot.dtype(), robot.platform());
  const auto &prismatic = RobotAccess::prismatic(robot);
  std::vector<Series> columns;
  for (std::int64_t j = 0; j < n; ++j) {
    if (!ancestor[static_cast<std::size_t>(j)]) {
      columns.push_back(none.unite({none}, -1).unsqueeze(-1));
      continue;
    }
    const Series axis = world_axis_origin(robot, j, rotations).add(none);
    // the joint's frame origin, before its own motion, is the parent's origin plus the fixed offset
    Series linear, angular;
    if (prismatic[static_cast<std::size_t>(j)] == 1) {
      linear = axis;
      angular = none;
    } else {
      const auto parent = robot.parent()[static_cast<std::size_t>(j)];
      const Series fixed = RobotAccess::offset_position(robot).select(0, j);
      const Series origin = parent < 0 ? none.add(fixed)
                                       : positions[static_cast<std::size_t>(parent)].add(apply(rotations[static_cast<std::size_t>(parent)], none.add(fixed)));
      linear = vec_cross(axis, positions[static_cast<std::size_t>(link)].sub(origin));
      angular = axis;
    }
    columns.push_back(linear.unite({angular}, -1).unsqueeze(-1));
  }
  (void)cfg;
  std::vector<Series> rest(columns.begin() + 1, columns.end());
  return columns[0].unite(rest, -1);
}

namespace {

// Velocities and accelerations of every link frame (in the link frame), propagated from the base outwards. The one place
// that implements the kinematic recursion: the dynamics and the constraint code both use it.
struct Motion {
  std::vector<Series> omega, alpha, accel;
};

Motion propagate(const Robot &robot, const Frames &f, const Series &qd, const Series &qdd, const Series &base_accel) {
  const auto n = robot.links();
  const ScalarType dtype = robot.dtype();
  const Series zero = zeros_like_batch(f.batch, 3, dtype, robot.platform());
  const auto &prismatic = RobotAccess::prismatic(robot);
  Motion m;
  m.omega.resize(static_cast<std::size_t>(n));
  m.alpha.resize(static_cast<std::size_t>(n));
  m.accel.resize(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto parent = robot.parent()[static_cast<std::size_t>(i)];
    const Series w_p = parent < 0 ? zero : m.omega[static_cast<std::size_t>(parent)];
    const Series a_p = parent < 0 ? zero : m.alpha[static_cast<std::size_t>(parent)];
    const Series v_p = parent < 0 ? base_accel : m.accel[static_cast<std::size_t>(parent)];
    const Series &rpc = f.rpc[static_cast<std::size_t>(i)];
    const Series &d = f.offset[static_cast<std::size_t>(i)];
    const Series rt = rpc.mT();
    const Series z = RobotAccess::axis(robot).select(0, i);
    const Series rate = qd.select(-1, i).to(dtype).unsqueeze(-1).mul(z);
    const Series acc = qdd.select(-1, i).to(dtype).unsqueeze(-1).mul(z);
    const Series w_r = apply(rt, w_p);
    const Series a_r = apply(rt, a_p);
    const Series carried = apply(rt, v_p.add(vec_cross(a_p, d)).add(vec_cross(w_p, vec_cross(w_p, d))));
    if (prismatic[static_cast<std::size_t>(i)] == 0) {
      m.omega[static_cast<std::size_t>(i)] = w_r.add(rate);
      m.alpha[static_cast<std::size_t>(i)] = a_r.add(vec_cross(w_r, rate)).add(acc);
      m.accel[static_cast<std::size_t>(i)] = carried;
    } else {
      m.omega[static_cast<std::size_t>(i)] = w_r;
      m.alpha[static_cast<std::size_t>(i)] = a_r;
      m.accel[static_cast<std::size_t>(i)] = carried.add(vec_cross(w_r, rate).mul(2.0)).add(acc);
    }
  }
  return m;
}

// Recursive Newton-Euler for q, qd, qdd [..., n] with the base accelerating against gravity.
Series rnea(const Robot &robot, const Series &q, const Series &qd, const Series &qdd, const Series &gravity) {
  const Frames f = link_frames(robot, q);
  const Shape batch = f.batch;
  const auto n = robot.links();
  const ScalarType dtype = robot.dtype();
  const Platform platform = robot.platform();
  const Series zero = zeros_like_batch(batch, 3, dtype, platform);
  const auto &prismatic = RobotAccess::prismatic(robot);
  const Motion motion = propagate(robot, f, qd, qdd, zero.sub(gravity));
  std::vector<Series> force(static_cast<std::size_t>(n)), moment(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    if (robot.has_mass()) {
      const Series m = RobotAccess::mass(robot).select(0, i);
      const Series c = RobotAccess::center(robot).select(0, i);
      const Series inertia = RobotAccess::inertia(robot).select(0, i);
      const Series &w = motion.omega[static_cast<std::size_t>(i)];
      const Series &a = motion.alpha[static_cast<std::size_t>(i)];
      const Series centre_accel = motion.accel[static_cast<std::size_t>(i)].add(vec_cross(a, c)).add(vec_cross(w, vec_cross(w, c)));
      const Series big_f = centre_accel.mul(m);
      const Series big_n = apply(inertia, a).add(vec_cross(w, apply(inertia, w)));
      force[static_cast<std::size_t>(i)] = big_f;
      moment[static_cast<std::size_t>(i)] = big_n.add(vec_cross(c, big_f));
    } else {
      force[static_cast<std::size_t>(i)] = zero;
      moment[static_cast<std::size_t>(i)] = zero;
    }
  }
  std::vector<Series> torque(static_cast<std::size_t>(n));
  for (std::int64_t i = n - 1; i >= 0; --i) {
    const Series z = RobotAccess::axis(robot).select(0, i);
    torque[static_cast<std::size_t>(i)] = prismatic[static_cast<std::size_t>(i)] == 0 ? dot(moment[static_cast<std::size_t>(i)], z)
                                                                                      : dot(force[static_cast<std::size_t>(i)], z);
    const auto parent = robot.parent()[static_cast<std::size_t>(i)];
    if (parent >= 0) {
      const Series carried_force = apply(f.rpc[static_cast<std::size_t>(i)], force[static_cast<std::size_t>(i)]);
      force[static_cast<std::size_t>(parent)] = force[static_cast<std::size_t>(parent)].add(carried_force);
      moment[static_cast<std::size_t>(parent)] =
          moment[static_cast<std::size_t>(parent)]
              .add(apply(f.rpc[static_cast<std::size_t>(i)], moment[static_cast<std::size_t>(i)]))
              .add(vec_cross(f.offset[static_cast<std::size_t>(i)], carried_force));
    }
  }
  std::vector<Series> rest;
  for (std::int64_t i = 1; i < n; ++i) rest.push_back(torque[static_cast<std::size_t>(i)].unsqueeze(-1));
  return torque[0].unsqueeze(-1).unite(rest, -1);
}

} // namespace

Series torques(const Robot &robot, const Series &q, const Series &qd, const Series &qdd,
                        const DynamicsOptions &options) {
  need_mass(robot, "torques");
  batch_of(robot, q, "torques");
  batch_of(robot, qd, "torques", "qd");
  batch_of(robot, qdd, "torques", "qdd");
  return rnea(robot, q, qd, qdd, gravity_vector(robot, options));
}

Series bias(const Robot &robot, const Series &q, const Series &qd, const DynamicsOptions &options) {
  need_mass(robot, "bias");
  batch_of(robot, q, "bias");
  batch_of(robot, qd, "bias", "qd");
  return rnea(robot, q, qd, qd.mul(0.0), gravity_vector(robot, options));
}

Series inertia(const Robot &robot, const Series &q) {
  need_mass(robot, "inertia");
  batch_of(robot, q, "inertia");
  const auto n = robot.links();
  const Config cfg = config(robot.dtype(), robot.platform());
  const Series no_gravity = pluss::series::zeros({3}, cfg);
  const Series still = q.mul(0.0);
  std::vector<Series> columns;
  for (std::int64_t j = 0; j < n; ++j) {
    std::vector<double> unit(static_cast<std::size_t>(n), 0.0);
    unit[static_cast<std::size_t>(j)] = 1.0;
    const Series direction = pluss::series::make(unit).to(robot.dtype()).to(robot.platform());
    columns.push_back(rnea(robot, q, still, still.add(direction), no_gravity).unsqueeze(-1));
  }
  std::vector<Series> rest(columns.begin() + 1, columns.end());
  return columns[0].unite(rest, -1);
}

Series accel(const Robot &robot, const Series &q, const Series &qd, const Series &tau,
                        const DynamicsOptions &options) {
  need_mass(robot, "accel");
  batch_of(robot, tau, "accel", "tau");
  const Series drift = bias(robot, q, qd, options);
  const Series mass = inertia(robot, q);
  return mass.linearalg.lu_solve(tau.to(robot.dtype()).sub(drift).unsqueeze(-1)).squeeze(-1);
}

Series ke(const Robot &robot, const Series &q, const Series &qd) {
  const Series mass = inertia(robot, q);
  batch_of(robot, qd, "ke", "qd");
  return mass.matmul(qd.to(robot.dtype()).unsqueeze(-1)).squeeze(-1).mul(qd.to(robot.dtype())).sum(-1).mul(0.5);
}

Series pe(const Robot &robot, const Series &q, const DynamicsOptions &options) {
  need_mass(robot, "pe");
  batch_of(robot, q, "pe");
  const Frames f = link_frames(robot, q);
  const Series gravity = gravity_vector(robot, options);
  Series total;
  const World world = world_frames(robot, f);
  for (std::int64_t i = 0; i < robot.links(); ++i) {
    const Series centre = world.position[static_cast<std::size_t>(i)].add(apply(world.rotation[static_cast<std::size_t>(i)], RobotAccess::center(robot).select(0, i)));
    const Series term = dot(centre, gravity).mul(RobotAccess::mass(robot).select(0, i)).neg();
    total = total.defined() ? total.add(term) : term;
  }
  return total;
}


namespace {

// Skew-symmetric matrix [r]x of vectors [..., 3], [..., 3, 3].
Series skew(const Series &r) {
  const Series zero = r.select(-1, 0).mul(0.0);
  const Series x = r.select(-1, 0), y = r.select(-1, 1), z = r.select(-1, 2);
  auto row = [&](const Series &a, const Series &b, const Series &c) { return a.unsqueeze(-1).unite({b.unsqueeze(-1), c.unsqueeze(-1)}, -1).unsqueeze(-2); };
  return row(zero, z.neg(), y).unite({row(z, zero, x.neg()), row(y.neg(), x, zero)}, -2);
}

// Per-point kinematics: world position, the point Jacobian [..., 3, n] and the bias acceleration J' qd [..., 3].
struct PointKinematics {
  Series position, jacobian, bias;
};

PointKinematics point_kinematics(const Robot &robot, const Series &q, const Series &qd, std::int64_t link, const Series &point) {
  PLUSS_CHECK(link >= 0 && link < robot.links(), "eng::robot: a constraint refers to a link outside the robot");
  const Frames f = link_frames(robot, q);
  const World world = world_frames(robot, f);
  const Series &rotation = world.rotation[static_cast<std::size_t>(link)];
  const Series offset = apply(rotation, point);
  const Series rows = jac(robot, q, link);
  const Series linear = rows.slice(-2, 0, 3), angular = rows.slice(-2, 3, 6);
  PointKinematics k;
  k.position = world.position[static_cast<std::size_t>(link)].add(offset);
  k.jacobian = linear.sub(skew(offset).matmul(angular));
  const Series zero = zeros_like_batch(f.batch, 3, robot.dtype(), robot.platform());
  const Motion motion = propagate(robot, f, qd, qd.mul(0.0), zero);
  const Series w = apply(rotation, motion.omega[static_cast<std::size_t>(link)]);
  const Series a = apply(rotation, motion.alpha[static_cast<std::size_t>(link)]);
  const Series o = apply(rotation, motion.accel[static_cast<std::size_t>(link)]);
  k.bias = o.add(vec_cross(a, offset)).add(vec_cross(w, vec_cross(w, offset)));
  return k;
}

void check_points(const std::vector<std::int64_t> &link, const Series &point, const char *who) {
  PLUSS_CHECK(point.defined() && point.dtype().is_floating() && point.dim() == 2 && point.size(0) == static_cast<std::int64_t>(link.size()) && point.size(1) == 3,
              std::string("eng::robot::") + who + ": point must be a real floating [m, 3] array with one row per link entry");
}

Series stack_rows(const std::vector<Series> &rows, std::int64_t dim) {
  return rows.size() == 1 ? rows[0] : rows[0].unite(std::vector<Series>(rows.begin() + 1, rows.end()), dim);
}

} // namespace

Series locate(const Robot &robot, const Series &q, const std::vector<std::int64_t> &link, const Series &point) {
  batch_of(robot, q, "locate");
  check_points(link, point, "locate");
  const Frames f = link_frames(robot, q);
  const World world = world_frames(robot, f);
  std::vector<Series> rows;
  for (std::size_t i = 0; i < link.size(); ++i) {
    PLUSS_CHECK(link[i] >= 0 && link[i] < robot.links(), "eng::robot::locate: link is outside the robot");
    rows.push_back(world.position[static_cast<std::size_t>(link[i])].add(apply(world.rotation[static_cast<std::size_t>(link[i])], point.select(0, static_cast<std::int64_t>(i)))).unsqueeze(-2));
  }
  return stack_rows(rows, -2);
}

Series gap(const Robot &robot, const Contacts &contacts, const Series &q) {
  batch_of(robot, q, "gap");
  check_points(contacts.link, contacts.point, "gap");
  const auto m = static_cast<std::int64_t>(contacts.link.size());
  PLUSS_CHECK(contacts.normal.defined() && contacts.normal.sizes() == Shape({m, 3}) && contacts.offset.defined() && contacts.offset.sizes() == Shape({m}),
              "eng::robot::gap: normal must be [m, 3] and offset [m]");
  const Series where = locate(robot, q, contacts.link, contacts.point);
  return where.mul(contacts.normal.to(robot.dtype())).sum(-1).sub(contacts.offset.to(robot.dtype()));
}

namespace {

// The constraint rows of pins and contacts at a state, shared by the dynamics and the impact law. Row order: three per pin,
// one normal row per contact, and (when a contact has friction) two tangent rows per contact.
struct ConstraintRows {
  Series jacobian;      // [..., k, n]
  Series bias;          // [..., k]: the row accelerations when the joint accelerations are zero
  Series gap;           // [..., mc]
  Series active;        // [..., mc] 1 where the contact is in range
  Series friction;      // [mc] (zero where frictionless) or undefined
  std::int64_t pins{}, contacts{};
  bool frictional{};
  Shape batch;
};

// Two unit tangent vectors of a (unit) normal, as a pair of [3] Series.
std::pair<Series, Series> tangent_pair(const Series &normal) {
  const std::vector<double> n = normal.to(Platform::CPU, ScalarType::Float64).items<double>();
  const std::vector<double> helper = std::abs(n[0]) < 0.9 ? std::vector<double>{1, 0, 0} : std::vector<double>{0, 1, 0};
  const Series a = pluss::series::make(helper).to(normal.scalar_type()).to(normal.platform());
  const Series along = a.sub(normal.mul(a.mul(normal).sum(-1)));
  const Series t1 = along.div(along.mul(along).sum(-1).sqrt());
  return {t1, vec_cross(normal, t1)};
}

ConstraintRows build_rows(const Robot &robot, const Contacts &contacts, const Pins &pins, const Series &q, const Series &qd,
                          double threshold, const char *who) {
  const auto mc = static_cast<std::int64_t>(contacts.link.size()), mp = static_cast<std::int64_t>(pins.link.size());
  const ScalarType dtype = robot.dtype();
  const Platform platform = robot.platform();
  ConstraintRows rows;
  rows.batch = leading_of(q);
  rows.pins = mp;
  rows.contacts = mc;
  if (mp > 0) check_points(pins.link, pins.point, who);
  std::vector<Series> jac, bias;
  for (std::int64_t i = 0; i < mp; ++i) {
    const PointKinematics k = point_kinematics(robot, q, qd, pins.link[static_cast<std::size_t>(i)], pins.point.select(0, i));
    jac.push_back(k.jacobian);
    bias.push_back(k.bias);
  }
  rows.gap = pluss::series::zeros(with(rows.batch, {mc}), config(dtype, platform));
  rows.active = rows.gap;
  if (mc > 0) {
    check_points(contacts.link, contacts.point, who);
    rows.gap = gap(robot, contacts, q);
    rows.active = rows.gap.le(threshold).to(dtype);
    Series mu;
    if (contacts.friction.defined()) {
      PLUSS_CHECK(contacts.friction.dtype().is_floating() && contacts.friction.sizes() == Shape({mc}) && contacts.friction.ge(0.0).all().item<bool>(),
                  std::string("eng::robot::") + who + ": friction must be a non-negative real floating vector with one value per contact");
      mu = contacts.friction.to(dtype).to(platform);
      rows.frictional = mu.gt(0.0).any().item<bool>();
    }
    rows.friction = mu;
    std::vector<Series> normal_jac, normal_bias, tangent_jac, tangent_bias;
    for (std::int64_t i = 0; i < mc; ++i) {
      const PointKinematics k = point_kinematics(robot, q, qd, contacts.link[static_cast<std::size_t>(i)], contacts.point.select(0, i));
      const Series normal = contacts.normal.select(0, i).to(dtype);
      normal_jac.push_back(normal.unsqueeze(-1).mT().matmul(k.jacobian));
      normal_bias.push_back(k.bias.mul(normal).sum(-1).unsqueeze(-1));
      if (rows.frictional) {
        const auto [t1, t2] = tangent_pair(normal);
        for (const Series &t : {t1, t2}) {
          tangent_jac.push_back(t.unsqueeze(-1).mT().matmul(k.jacobian));
          tangent_bias.push_back(k.bias.mul(t).sum(-1).unsqueeze(-1));
        }
      }
    }
    for (const Series &j : normal_jac) jac.push_back(j);
    for (const Series &j : tangent_jac) jac.push_back(j);
    for (const Series &b : normal_bias) bias.push_back(b);
    for (const Series &b : tangent_bias) bias.push_back(b);
  }
  // pin biases are [..., 3]; contact biases [..., 1]: all join along the last axis
  rows.jacobian = stack_rows(jac, -2);
  rows.bias = stack_rows(bias, -1);
  return rows;
}

// Solves the (mixed) linear complementarity problem of the rows as a box-constrained quadratic program: pin forces free, normal
// forces non-negative (and zero for contacts that are not `active`), tangent forces limited to friction times the normal force of
// the previous round (a fixed-point iteration that settles in a few rounds). Returns the forces [..., k].
Series solve_forces(const Series &delassus, const Series &linear, const ConstraintRows &rows, std::int64_t friction_rounds,
                    ScalarType dtype, Platform platform) {
  const std::int64_t mp = rows.pins, mc = rows.contacts, k = delassus.size(-1);
  const Config cfg = config(dtype, platform);
  pluss::NoGradGuard no_grad;
  math::qp::Options solver;
  solver.max_iters = 20000;
  solver.abs_tol = 1e-9;
  solver.rel_tol = 1e-9;
  const Series identity = pluss::series::eye(k, cfg);
  auto bounds = [&](const Series &tangent_limit) {
    std::vector<Series> low, high;
    if (mp > 0) {
      low.push_back(pluss::series::zeros(with(rows.batch, {mp * 3}), cfg).add(-1.0e12));
      high.push_back(pluss::series::zeros(with(rows.batch, {mp * 3}), cfg).add(1.0e12));
    }
    if (mc > 0) {
      low.push_back(pluss::series::zeros(with(rows.batch, {mc}), cfg));
      high.push_back(rows.active.mul(1.0e12));
      if (rows.frictional) {
        low.push_back(tangent_limit.neg());
        high.push_back(tangent_limit);
      }
    }
    return std::make_pair(stack_rows(low, -1), stack_rows(high, -1));
  };
  Series limit;
  if (rows.frictional) limit = pluss::series::zeros(with(rows.batch, {2 * mc}), cfg);
  Series force;
  const std::int64_t rounds = rows.frictional ? std::max<std::int64_t>(friction_rounds, 1) : 1;
  for (std::int64_t round = 0; round < rounds; ++round) {
    const auto [low, high] = bounds(limit);
    force = math::qp::solve(delassus, linear, identity, low, high, solver).x;
    if (rows.frictional) {
      const Series normals = force.slice(-1, mp * 3, mp * 3 + mc);
      const Series cone = normals.clamp_min(0.0).mul(rows.friction);
      const Series next = cone.unite({cone}, -1);
      const bool settled = max_abs(next.sub(limit)) <= 1e-7 * (1.0 + max_abs(next));
      limit = next;
      if (settled) break;
    }
  }
  return force;
}

// Splits forces [..., k] into pin, normal and world friction parts for the results.
struct SplitForces {
  Series pin, normal, friction;
};

SplitForces split_forces(const Robot &robot, const Contacts &contacts, const ConstraintRows &rows, const Series &force) {
  const std::int64_t mp = rows.pins, mc = rows.contacts;
  const ScalarType dtype = robot.dtype();
  const Platform platform = robot.platform();
  SplitForces out;
  out.pin = mp > 0 ? force.slice(-1, 0, mp * 3).reshape(with(rows.batch, {mp, 3})) : pluss::series::zeros(with(rows.batch, {0, 3}), config(dtype, platform));
  out.normal = mc > 0 ? force.slice(-1, mp * 3, mp * 3 + mc) : pluss::series::zeros(with(rows.batch, {0}), config(dtype, platform));
  out.friction = pluss::series::zeros(with(rows.batch, {mc, 3}), config(dtype, platform));
  if (mc > 0 && rows.frictional) {
    std::vector<Series> perContact;
    for (std::int64_t i = 0; i < mc; ++i) {
      const auto [t1, t2] = tangent_pair(contacts.normal.select(0, i).to(dtype));
      const Series f1 = force.select(-1, mp * 3 + mc + 2 * i).unsqueeze(-1), f2 = force.select(-1, mp * 3 + mc + 2 * i + 1).unsqueeze(-1);
      perContact.push_back(f1.mul(t1).add(f2.mul(t2)).unsqueeze(-2));
    }
    out.friction = stack_rows(perContact, -2);
  }
  return out;
}

}  // namespace

ConstrainedResult constrain(const Robot &robot, const Contacts &contacts, const Pins &pins, const Series &q,
                                       const Series &qd, const Series &tau, const ConstraintOptions &options) {
  need_mass(robot, "constrain");
  const Shape batch = batch_of(robot, q, "constrain");
  batch_of(robot, qd, "constrain", "qd");
  batch_of(robot, tau, "constrain", "tau");
  PLUSS_CHECK(std::isfinite(options.contact_threshold) && options.contact_threshold >= 0.0 && options.friction_iterations >= 1,
              "eng::robot::constrain: contact_threshold must be non-negative and friction_iterations at least 1");
  const auto mc = static_cast<std::int64_t>(contacts.link.size()), mp = static_cast<std::int64_t>(pins.link.size());
  const ScalarType dtype = robot.dtype();
  const Platform platform = robot.platform();
  const Series mass = inertia(robot, q);
  const Series push = tau.to(dtype).sub(bias(robot, q, qd, options.dynamics));
  const Series free_accel = mass.linearalg.lu_solve(push.unsqueeze(-1)).squeeze(-1);
  ConstrainedResult result;
  if (mc == 0 && mp == 0) {
    result.acceleration = free_accel;
    result.contact_force = pluss::series::zeros(with(batch, {0}), config(dtype, platform));
    result.pin_force = pluss::series::zeros(with(batch, {0, 3}), config(dtype, platform));
    result.friction_force = pluss::series::zeros(with(batch, {0, 3}), config(dtype, platform));
    result.gap = result.contact_force;
    return result;
  }
  const ConstraintRows rows = build_rows(robot, contacts, pins, q, qd, options.contact_threshold, "constrain");
  const Series minv_jt = mass.linearalg.lu_solve(rows.jacobian.mT());                       // M^-1 J^T, [..., n, k]
  const Series delassus = rows.jacobian.matmul(minv_jt);
  const Series sym = delassus.add(delassus.mT()).mul(0.5);
  const Series linear = rows.jacobian.matmul(free_accel.unsqueeze(-1)).squeeze(-1).add(rows.bias);   // row accelerations with no force
  const Series force = solve_forces(sym.detach(), linear.detach(), rows, options.friction_iterations, dtype, platform);
  const SplitForces parts = split_forces(robot, contacts, rows, force);
  result.acceleration = free_accel.add(minv_jt.matmul(force.unsqueeze(-1)).squeeze(-1));
  result.contact_force = parts.normal;
  result.pin_force = parts.pin;
  result.friction_force = parts.friction;
  result.gap = rows.gap;
  return result;
}

ImpactResult impact(const Robot &robot, const Contacts &contacts, const Pins &pins, const Series &q, const Series &qd,
                    double restitution, const ConstraintOptions &options) {
  need_mass(robot, "impact");
  const Shape batch = batch_of(robot, q, "impact");
  batch_of(robot, qd, "impact", "qd");
  PLUSS_CHECK(std::isfinite(restitution) && restitution >= 0.0 && restitution <= 1.0, "eng::robot::impact: restitution must lie in [0, 1]");
  PLUSS_CHECK(std::isfinite(options.contact_threshold) && options.contact_threshold >= 0.0 && options.friction_iterations >= 1,
              "eng::robot::impact: contact_threshold must be non-negative and friction_iterations at least 1");
  const auto mc = static_cast<std::int64_t>(contacts.link.size()), mp = static_cast<std::int64_t>(pins.link.size());
  const ScalarType dtype = robot.dtype();
  const Platform platform = robot.platform();
  const Series velocity = qd.to(dtype);
  ImpactResult result;
  if (mc == 0 && mp == 0) {
    result.velocity = velocity;
    result.contact_impulse = pluss::series::zeros(with(batch, {0}), config(dtype, platform));
    result.pin_impulse = pluss::series::zeros(with(batch, {0, 3}), config(dtype, platform));
    result.friction_impulse = pluss::series::zeros(with(batch, {0, 3}), config(dtype, platform));
    return result;
  }
  ConstraintRows rows = build_rows(robot, contacts, pins, q, qd, options.contact_threshold, "impact");
  const Series mass = inertia(robot, q);
  const Series minv_jt = mass.linearalg.lu_solve(rows.jacobian.mT());
  const Series delassus = rows.jacobian.matmul(minv_jt);
  const Series sym = delassus.add(delassus.mT()).mul(0.5);
  // row velocities before; only contacts in range that approach their plane take part, and their target is the rebound
  const Series approach = rows.jacobian.matmul(velocity.unsqueeze(-1)).squeeze(-1);
  Series target = approach;
  if (mc > 0) {
    const Series normal_speed = approach.slice(-1, mp * 3, mp * 3 + mc);
    const Series approaching = normal_speed.lt(0.0).to(dtype);
    rows.active = rows.active.mul(approaching);
    const Series rebound = normal_speed.mul(approaching).mul(restitution);
    std::vector<Series> parts;
    if (mp > 0) parts.push_back(approach.slice(-1, 0, mp * 3));
    parts.push_back(normal_speed.add(rebound));
    if (rows.frictional) parts.push_back(approach.slice(-1, mp * 3 + mc, mp * 3 + mc + 2 * mc));
    target = stack_rows(parts, -1);
  }
  const Series force = solve_forces(sym.detach(), target.detach(), rows, options.friction_iterations, dtype, platform);
  const SplitForces parts = split_forces(robot, contacts, rows, force);
  result.velocity = velocity.add(minv_jt.matmul(force.unsqueeze(-1)).squeeze(-1));
  result.contact_impulse = parts.normal;
  result.pin_impulse = parts.pin;
  result.friction_impulse = parts.friction;
  return result;
}

IkResult ik(const Robot &robot, std::int64_t link, const Series &target_position,
                            const Series &target_rotation, const Series &q0, const IkOptions &options) {
  const Shape batch = batch_of(robot, q0, "ik", "q0");
  const auto n = robot.links();
  PLUSS_CHECK(link >= 0 && link < n, "eng::robot::ik: link is outside the robot");
  PLUSS_CHECK(target_position.defined() && target_position.dtype().is_floating() && target_position.dim() >= 1 &&
                  target_position.size(-1) == 3 && target_position.platform() == robot.platform(),
              "eng::robot::ik: target_position must be a real floating [..., 3] array");
  const bool orient = target_rotation.defined();
  if (orient)
    PLUSS_CHECK(target_rotation.dtype().is_floating() && target_rotation.dim() >= 2 && target_rotation.size(-1) == 3 &&
                    target_rotation.size(-2) == 3 && target_rotation.platform() == robot.platform(),
                "eng::robot::ik: target_rotation must be a real floating [..., 3, 3] array");
  PLUSS_CHECK(options.max_iterations >= 1 && std::isfinite(options.tolerance) && options.tolerance > 0.0 &&
                  std::isfinite(options.damping) && options.damping > 0.0 && std::isfinite(options.orientation_weight) &&
                  options.orientation_weight >= 0.0,
              "eng::robot::ik: max_iterations must be at least 1, tolerance and damping positive, "
              "orientation_weight non-negative");
  for (const Series *limit : {&options.lower, &options.upper})
    if (limit->defined())
      PLUSS_CHECK(limit->dtype().is_floating() && limit->sizes() == Shape{n} && limit->platform() == robot.platform(),
                  "eng::robot::ik: joint limits must be real floating vectors with one value per link");
  if (options.lower.defined() && options.upper.defined())
    PLUSS_CHECK(options.lower.le(options.upper).all().item<bool>(), "eng::robot::ik: every lower limit must not exceed its upper limit");
  pluss::NoGradGuard no_grad;
  const ScalarType dtype = robot.dtype();
  const Config cfg = config(dtype, robot.platform());
  const std::int64_t rows = orient ? 6 : 3;
  auto clamp = [&](Series x) {
    if (options.lower.defined()) x = x.maximum(options.lower.to(dtype));
    if (options.upper.defined()) x = x.minimum(options.upper.to(dtype));
    return x;
  };
  Series q = clamp(q0.to(dtype).detach());
  const Series goal = target_position.to(dtype);
  const Series goal_rotation = orient ? target_rotation.to(dtype) : Series{};
  const Series identity = pluss::series::eye(rows, cfg);
  const Series weight = pluss::series::make(std::vector<double>{1, 1, 1, options.orientation_weight, options.orientation_weight, options.orientation_weight})
                            .to(dtype).to(robot.platform());
  // task error and its norm at joint values x: position error, then the weighted sine-axis orientation error
  auto task_error = [&](const Series &x) {
    const Pose pose = frames(robot, x);
    Series error = goal.sub(pose.position.select(-2, link));
    if (orient) {
      const Series rel = goal_rotation.matmul(pose.rotation.select(-3, link).mT());
      auto entry = [&](std::int64_t r, std::int64_t c) { return rel.select(-2, r).select(-1, c); };
      const Series rot_error = entry(2, 1).sub(entry(1, 2)).unsqueeze(-1)
                                   .unite({entry(0, 2).sub(entry(2, 0)).unsqueeze(-1), entry(1, 0).sub(entry(0, 1)).unsqueeze(-1)}, -1)
                                   .mul(0.5 * options.orientation_weight);
      error = error.unite({rot_error}, -1);
    }
    return error;
  };
  auto norm_of = [](const Series &error) { return error.mul(error).sum(-1).sqrt(); };
  // Levenberg-Marquardt damping per target: a step that does not reduce the error is rejected and the damping raised, one
  // that does is accepted and the damping lowered, so the error never grows and an unreachable target ends at the
  // nearest configuration the iteration finds.
  Series lambda = pluss::series::zeros(leading_of(q), cfg).add(options.damping * options.damping);
  Series error = task_error(q);
  Series error_norm = norm_of(error);
  std::int64_t used = 0;
  for (std::int64_t it = 0; it <= options.max_iterations; ++it) {
    used = it;
    if (error_norm.amax().item<double>() <= options.tolerance) break;
    if (it == options.max_iterations) break;
    Series rows = jac(robot, q, link);
    rows = orient ? rows.slice(-2, 0, 6).mul(weight.unsqueeze(-1)) : rows.slice(-2, 0, 3);
    const Series system = rows.matmul(rows.mT()).add(identity.mul(lambda.unsqueeze(-1).unsqueeze(-1)));
    const Series step = rows.mT().matmul(system.linearalg.lu_solve(error.unsqueeze(-1))).squeeze(-1);
    const Series trial = clamp(q.add(step));
    const Series trial_norm = norm_of(task_error(trial));
    const Series better = trial_norm.lt(error_norm).to(dtype);
    q = q.add(trial.sub(q).mul(better.unsqueeze(-1)));
    lambda = better.mul(lambda.mul(0.5)).add(better.neg().add(1.0).mul(lambda.mul(4.0))).clamp_min(1e-14).clamp_max(1e12);
    error = task_error(q);
    error_norm = norm_of(error);
  }
  const Series flag = error_norm.le(options.tolerance);
  return {q, error_norm, pluss::series::make(std::vector<double>{static_cast<double>(used)}).to(ScalarType::Int64).to(robot.platform()),
          flag};
}

} // namespace pluss::subsp::eng::robot
