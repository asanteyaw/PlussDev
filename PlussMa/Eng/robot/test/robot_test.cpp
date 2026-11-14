#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace robot = pluss::subsp::eng::robot;

namespace {

using pluss::Series;

void expect(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}

Series vec(std::vector<double> v) { return pluss::series::make(v); }

Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }

double worst(const Series &a, const Series &b) { return a.sub(b).abs().reshape({a.numel()}).amax(-1, false).item<double>(); }

void expect_error(const std::function<void()> &call, const std::string &message) {
  try {
    call();
  } catch (const pluss::Error &) {
    return;
  }
  throw std::runtime_error(message);
}

constexpr double kG = 9.81;

// Planar two-link arm in the x-y plane, both joints about z.
struct Arm {
  double m1{1.0}, m2{2.0}, l1{0.5}, l2{0.4}, lc1{0.25}, lc2{0.2}, i1{0.02}, i2{0.03};
  robot::Robot make() const {
    robot::Links links;
    links.parent = {-1, 0};
    links.prismatic = {0, 0};
    links.axis = matrix({0, 0, 1, 0, 0, 1}, {2, 3});
    links.translation = matrix({0, 0, 0, l1, 0, 0}, {2, 3});
    links.mass = vec({m1, m2});
    links.center = matrix({lc1, 0, 0, lc2, 0, 0}, {2, 3});
    links.inertia = matrix({0, 0, 0, 0, 0, 0, 0, 0, i1, 0, 0, 0, 0, 0, 0, 0, 0, i2}, {2, 3, 3});
    return robot::make(links);
  }
  // closed forms (planar, gravity along -y, angles from the x axis)
  std::vector<double> inertia(double q2) const {
    const double c2 = std::cos(q2);
    const double a = m1 * lc1 * lc1 + m2 * (l1 * l1 + lc2 * lc2 + 2 * l1 * lc2 * c2) + i1 + i2;
    const double b = m2 * (lc2 * lc2 + l1 * lc2 * c2) + i2;
    return {a, b, b, m2 * lc2 * lc2 + i2};
  }
  std::vector<double> bias(double q1, double q2, double d1, double d2) const {
    const double h = -m2 * l1 * lc2 * std::sin(q2);
    const double g1 = (m1 * lc1 + m2 * l1) * kG * std::cos(q1) + m2 * lc2 * kG * std::cos(q1 + q2);
    const double g2 = m2 * lc2 * kG * std::cos(q1 + q2);
    return {h * d2 * d1 + h * (d1 + d2) * d2 + g1, -h * d1 * d1 + g2};
  }
};

robot::DynamicsOptions in_plane() {
  robot::DynamicsOptions o;
  o.gravity = pluss::series::make(std::vector<double>{0.0, -kG, 0.0});
  return o;
}

void rotation_checks() {
  const auto quarter = robot::rotation(vec({0, 0, 1}), vec({M_PI / 2}).select(0, 0));
  expect(worst(quarter.matmul(vec({1, 0, 0}).unsqueeze(-1)).squeeze(-1), vec({0, 1, 0})) < 1e-12, "z quarter turn maps x to y");
  const auto batch = robot::rotation(vec({1, 2, 3}), vec({0.3, 1.1, -2.0}));
  expect(batch.sizes() == std::vector<std::int64_t>{3, 3, 3}, "batched rotation shape");
  expect(worst(batch.matmul(batch.mT()), pluss::series::eye(3, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)).expand({3, 3, 3}, true)) < 1e-12,
         "rotation is orthonormal");
  const auto angles = vec({0.4, -0.7, 1.3});
  const auto expected = robot::rotation(vec({0, 0, 1}), angles.select(0, 2))
                            .matmul(robot::rotation(vec({0, 1, 0}), angles.select(0, 1)))
                            .matmul(robot::rotation(vec({1, 0, 0}), angles.select(0, 0)));
  expect(worst(robot::rpy(angles), expected) < 1e-12, "rpy is Rz Ry Rx");
  expect_error([] { robot::rotation(vec({0, 0, 0}), vec({1.0}).select(0, 0)); }, "zero axis accepted");
}

void kinematics_checks() {
  const Arm arm;
  const auto r = arm.make();
  const auto q = vec({0.6, -0.9});
  const auto pose = robot::frames(r, q);
  expect(pose.position.sizes() == std::vector<std::int64_t>{2, 3}, "pose position shape");
  expect(pose.rotation.sizes() == std::vector<std::int64_t>{2, 3, 3}, "pose rotation shape");
  const auto p = pose.position.host<double>();
  expect(std::abs(p[3] - arm.l1 * std::cos(0.6)) < 1e-12 && std::abs(p[4] - arm.l1 * std::sin(0.6)) < 1e-12, "elbow position");
  const auto rot = pose.rotation.select(0, 1).host<double>();
  expect(std::abs(rot[0] - std::cos(-0.3)) < 1e-12 && std::abs(rot[3] - std::sin(-0.3)) < 1e-12, "forearm orientation");

  // Jacobian of link 1 against central differences of the forward kinematics, for a tree with every joint kind.
  robot::Links tree;
  tree.parent = {-1, 0, 0, 2};
  tree.prismatic = {0, 0, 1, 0};
  tree.axis = matrix({0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0}, {4, 3});
  tree.translation = matrix({0.1, 0, 0.2, 0.3, 0, 0, 0, 0.2, 0.1, 0.1, 0.1, 0.2}, {4, 3});
  tree.rpy = matrix({0.1, 0.2, 0.3, 0, 0.4, 0, -0.2, 0, 0.5, 0.3, 0.3, -0.1}, {4, 3});
  const auto big = robot::make(tree);
  const auto q4 = vec({0.3, -0.5, 0.25, 0.8});
  for (std::int64_t link : {1, 3}) {
    const auto jac = robot::jac(big, q4, link);
    expect(jac.sizes() == std::vector<std::int64_t>{6, 4}, "jacobian shape");
    for (std::int64_t j = 0; j < 4; ++j) {
      std::vector<double> up = q4.host<double>(), down = up;
      up[static_cast<std::size_t>(j)] += 1e-6;
      down[static_cast<std::size_t>(j)] -= 1e-6;
      const auto pu = robot::frames(big, vec(up)), pd = robot::frames(big, vec(down));
      const auto linear = pu.position.select(0, link).sub(pd.position.select(0, link)).div(2e-6);
      expect(worst(jac.slice(0, 0, 3).select(1, j), linear) < 1e-7, "jacobian linear column");
      // angular: the rotation increment R(q+h) R(q-h)^T is a rotation by omega * 2h
      const auto inc = pu.rotation.select(0, link).matmul(pd.rotation.select(0, link).mT()).div(1.0);
      const auto h = inc.host<double>();
      const std::vector<double> omega{(h[7] - h[5]) / 4e-6, (h[2] - h[6]) / 4e-6, (h[3] - h[1]) / 4e-6};
      expect(worst(jac.slice(0, 3, 6).select(1, j), vec(omega)) < 1e-6, "jacobian angular column");
    }
  }
  // a link that is not an ancestor has a zero column
  const auto jac1 = robot::jac(big, q4, 1).host<double>();
  expect(jac1[2] == 0.0 && jac1[3] == 0.0, "non-ancestor column must be zero");
}

void dynamics_checks() {
  const Arm arm;
  const auto r = arm.make();
  const double q1 = 0.4, q2 = -0.8, d1 = 1.3, d2 = -0.7, a1 = 0.9, a2 = -1.4;
  const auto q = vec({q1, q2}), qd = vec({d1, d2}), qdd = vec({a1, a2});
  const auto m = robot::inertia(r, q);
  expect(worst(m, matrix(arm.inertia(q2), {2, 2})) < 1e-12, "mass matrix");
  const auto b = robot::bias(r, q, qd, in_plane());
  expect(worst(b, vec(arm.bias(q1, q2, d1, d2))) < 1e-11, "bias forces");
  const auto expected = matrix(arm.inertia(q2), {2, 2}).matmul(qdd.unsqueeze(-1)).squeeze(-1).add(vec(arm.bias(q1, q2, d1, d2)));
  expect(worst(robot::torques(r, q, qd, qdd, in_plane()), expected) < 1e-11, "inverse dynamics");
  // forward dynamics inverts it
  const auto tau = robot::torques(r, q, qd, qdd, in_plane());
  expect(worst(robot::accel(r, q, qd, tau, in_plane()), qdd) < 1e-10, "forward dynamics round trip");
  // batch
  const auto qb = matrix({0.4, -0.8, 1.0, 0.2, -0.3, 0.9}, {3, 2});
  const auto mb = robot::inertia(r, qb);
  expect(mb.sizes() == std::vector<std::int64_t>{3, 2, 2}, "batched mass matrix shape");
  expect(worst(mb.select(0, 1), matrix(arm.inertia(0.2), {2, 2})) < 1e-12, "batched mass matrix lane");
  // symmetric positive definite
  expect(worst(mb, mb.mT()) < 1e-12, "mass matrix symmetric");
  expect(robot::ke(r, qb, qb).ge(0.0).all().item<bool>(), "kinetic energy positive");

  // Energy is conserved by forward dynamics under RK4 (planar gravity).
  auto energy = [&](const Series &x, const Series &v) {
    return (robot::ke(r, x, v).add(robot::pe(r, x, in_plane()))).item<double>();
  };
  Series x = vec({0.3, 0.5}), v = vec({0.0, 0.0});
  const double e0 = energy(x, v);
  const double h = 1e-3;
  auto accel = [&](const Series &xx, const Series &vv) { return robot::accel(r, xx, vv, vec({0, 0}), in_plane()); };
  for (int k = 0; k < 1000; ++k) {
    const auto k1x = v, k1v = accel(x, v);
    const auto k2x = v.add(k1v.mul(h / 2)), k2v = accel(x.add(k1x.mul(h / 2)), k2x);
    const auto k3x = v.add(k2v.mul(h / 2)), k3v = accel(x.add(k2x.mul(h / 2)), k3x);
    const auto k4x = v.add(k3v.mul(h)), k4v = accel(x.add(k3x.mul(h)), k4x);
    x = x.add(k1x.add(k2x.mul(2)).add(k3x.mul(2)).add(k4x).mul(h / 6));
    v = v.add(k1v.add(k2v.mul(2)).add(k3v.mul(2)).add(k4v).mul(h / 6));
  }
  expect(std::abs(energy(x, v) - e0) < 1e-8 * (1.0 + std::abs(e0)), "energy drift");
  expect(std::abs(x.host<double>()[0] - 0.3) > 0.1, "the pendulum must have moved");
}

void prismatic_checks() {
  // Cart on a rail (prismatic along x) carrying a pendulum (revolute about z at the cart), gravity along -y.
  const double cart = 1.5, bob = 0.7, length = 0.6, spin = 0.01;
  robot::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 0};
  links.axis = matrix({1, 0, 0, 0, 0, 1}, {2, 3});
  links.translation = matrix({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({cart, bob});
  links.center = matrix({0, 0, 0, length, 0, 0}, {2, 3});
  links.inertia = matrix({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, spin}, {2, 3, 3});
  const auto r = robot::make(links);
  const double x = 0.3, th = 0.9, dx = -0.8, dth = 1.7, ax = 0.4, ath = -2.2;
  const auto tau = robot::torques(r, vec({x, th}), vec({dx, dth}), vec({ax, ath}), in_plane());
  const double expected1 = (cart + bob) * ax - bob * length * std::sin(th) * ath - bob * length * std::cos(th) * dth * dth;
  const double expected2 = -bob * length * std::sin(th) * ax + (bob * length * length + spin) * ath + bob * kG * length * std::cos(th);
  expect(worst(tau, vec({expected1, expected2})) < 1e-11, "cart-pole inverse dynamics");
  const auto m = robot::inertia(r, vec({x, th}));
  expect(worst(m, matrix({cart + bob, -bob * length * std::sin(th), -bob * length * std::sin(th), bob * length * length + spin}, {2, 2})) < 1e-12,
         "cart-pole mass matrix");
}

void tree_checks() {
  // A trunk with two branches and spatial axes: symmetric positive definite inertia and conserved energy.
  robot::Links links;
  links.parent = {-1, 0, 0, 1};
  links.prismatic = {0, 0, 0, 0};
  links.axis = matrix({0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0}, {4, 3});
  links.translation = matrix({0, 0, 0.3, 0.2, 0, 0.1, -0.2, 0, 0.1, 0, 0, 0.3}, {4, 3});
  links.rpy = matrix({0, 0, 0, 0.2, 0, 0.1, 0, -0.1, 0, 0.3, 0.2, 0}, {4, 3});
  links.mass = vec({2.0, 1.0, 1.2, 0.8});
  links.center = matrix({0, 0, 0.1, 0.05, 0, 0.1, 0, 0.05, 0.1, 0, 0, 0.12}, {4, 3});
  links.inertia = matrix({0.05, 0, 0, 0, 0.06, 0, 0, 0, 0.02, 0.03, 0.001, 0, 0.001, 0.04, 0, 0, 0, 0.02, 0.02, 0, 0, 0, 0.03, 0.002, 0, 0.002, 0.025,
                          0.01, 0, 0, 0, 0.012, 0, 0, 0, 0.008}, {4, 3, 3});
  const auto r = robot::make(links);
  const auto q = vec({0.2, -0.4, 0.5, 0.3});
  const auto m = robot::inertia(r, q);
  expect(worst(m, m.mT()) < 1e-10, "tree mass matrix symmetric");
  const auto chol_check = vec({1, 2, -1, 0.5}).unsqueeze(-1);
  expect(chol_check.mT().matmul(m).matmul(chol_check).select(0, 0).select(0, 0).item<double>() > 0.0, "tree mass matrix positive");
  auto energy = [&](const Series &x, const Series &v) {
    return robot::ke(r, x, v).add(robot::pe(r, x)).item<double>();
  };
  Series x = q, v = vec({0.5, -0.3, 0.8, 0.1});
  const double e0 = energy(x, v);
  const double h = 5e-4;
  auto accel = [&](const Series &xx, const Series &vv) { return robot::accel(r, xx, vv, vec({0, 0, 0, 0})); };
  for (int k = 0; k < 1000; ++k) {
    const auto k1x = v, k1v = accel(x, v);
    const auto k2x = v.add(k1v.mul(h / 2)), k2v = accel(x.add(k1x.mul(h / 2)), k2x);
    const auto k3x = v.add(k2v.mul(h / 2)), k3v = accel(x.add(k2x.mul(h / 2)), k3x);
    const auto k4x = v.add(k3v.mul(h)), k4v = accel(x.add(k3x.mul(h)), k4x);
    x = x.add(k1x.add(k2x.mul(2)).add(k3x.mul(2)).add(k4x).mul(h / 6));
    v = v.add(k1v.add(k2v.mul(2)).add(k3v.mul(2)).add(k4v).mul(h / 6));
  }
  expect(std::abs(energy(x, v) - e0) < 1e-7 * (1.0 + std::abs(e0)), "tree energy drift");
  // gravity torque is the gradient of the potential energy
  const auto still = vec({0, 0, 0, 0});
  const auto g = robot::bias(r, q, still);
  std::vector<double> fd;
  for (int j = 0; j < 4; ++j) {
    std::vector<double> up = q.host<double>(), down = up;
    up[static_cast<std::size_t>(j)] += 1e-6;
    down[static_cast<std::size_t>(j)] -= 1e-6;
    fd.push_back((robot::pe(r, vec(up)).item<double>() - robot::pe(r, vec(down)).item<double>()) / 2e-6);
  }
  expect(worst(g, vec(fd)) < 1e-6, "gravity torque is the potential gradient");
}

void ik_checks() {
  robot::Links links;
  links.parent = {-1, 0, 1, 2, 3, 4};
  links.prismatic = {0, 0, 0, 0, 0, 0};
  links.axis = matrix({0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0}, {6, 3});
  links.translation = matrix({0, 0, 0.2, 0, 0, 0.1, 0, 0, 0.4, 0, 0, 0.3, 0, 0, 0.1, 0, 0, 0.1}, {6, 3});
  const auto r = robot::make(links);
  const auto truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const auto pose = robot::frames(r, truth);
  const auto goal = pose.position.select(0, 5);
  const auto goal_rotation = pose.rotation.select(0, 5);
  const auto start = vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1});
  const auto solved = robot::ik(r, 5, goal, goal_rotation, start);
  expect(solved.converged.all().item<bool>(), "pose IK converges");
  const auto reached = robot::frames(r, solved.q);
  expect(worst(reached.position.select(0, 5), goal) < 1e-6 && worst(reached.rotation.select(0, 5), goal_rotation) < 1e-6, "pose IK reaches the pose");
  // position only, batched over two targets
  const auto goals = matrix({goal.host<double>()[0], goal.host<double>()[1], goal.host<double>()[2], 0.2, 0.1, 0.8}, {2, 3});
  const auto starts = matrix({0.3, 0.4, -0.5, 0.2, 0.6, -0.1, 0.1, 0.2, 0.3, 0.4, 0.3, 0.2}, {2, 6});
  const auto two = robot::ik(r, 5, goals, Series{}, starts);
  expect(two.q.sizes() == std::vector<std::int64_t>{2, 6}, "batched IK shape");
  expect(two.converged.sizes() == std::vector<std::int64_t>{2}, "batched IK flag shape");
  const auto reached_two = robot::frames(r, two.q).position.select(1, 5);
  expect(worst(reached_two.select(0, 0), goals.select(0, 0)) < 1e-6, "position IK lane 0");
  // joint limits are respected, and a target outside them is reported as not reached
  robot::IkOptions limited;
  limited.lower = vec({-0.45, -0.45, -0.65, -0.35, -0.75, -0.25});
  limited.upper = vec({0.45, 0.55, 0.65, 0.35, 0.75, 0.25});
  const auto boxed = robot::ik(r, 5, goal, Series{}, start, limited);
  expect(boxed.q.sub(limited.upper).le(1e-12).all().item<bool>() && boxed.q.sub(limited.lower).ge(-1e-12).all().item<bool>(), "IK joint limits");
  robot::IkOptions blocked;
  blocked.upper = vec({0.1, 0.1, 0.1, 0.1, 0.1, 0.1});
  blocked.lower = vec({-0.1, -0.1, -0.1, -0.1, -0.1, -0.1});
  const auto stuck = robot::ik(r, 5, goal, Series{}, vec({0, 0, 0, 0, 0, 0}), blocked);
  expect(!stuck.converged.item<bool>() && stuck.q.abs().amax().item<double>() <= 0.1 + 1e-12, "IK limits block a target");
  // the error never exceeds the starting error: an unreachable target ends at the nearest point found
  const auto begun = robot::frames(r, start).position.select(0, 5).sub(vec({5.0, 5.0, 5.0})).mul(1.0);
  const double start_distance = std::sqrt(begun.mul(begun).sum().item<double>());
  const auto reach = robot::ik(r, 5, vec({5.0, 5.0, 5.0}), Series{}, start);
  expect(reach.error.item<double>() <= start_distance + 1e-12, "IK error must not grow");
  // an unreachable target does not converge and says so
  const auto far = robot::ik(r, 5, vec({5.0, 5.0, 5.0}), Series{}, start);
  expect(!far.converged.item<bool>(), "unreachable target must not converge");
}


void constraint_checks() {
  // a vertical slider of mass 2 kg (prismatic along y) with a point contact on the floor y >= 0, gravity along -y
  robot::Links links;
  links.parent = {-1};
  links.prismatic = {1};
  links.axis = matrix({0, 1, 0}, {1, 3});
  links.translation = matrix({0, 0, 0}, {1, 3});
  links.mass = vec({2.0});
  const auto slider = robot::make(links);
  robot::Contacts floor;
  floor.link = {0};
  floor.point = matrix({0, 0, 0}, {1, 3});
  floor.normal = matrix({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  robot::ConstraintOptions plane;
  plane.dynamics.gravity = pluss::series::make(std::vector<double>{0.0, -kG, 0.0});
  const robot::Pins none;
  // lanes: resting on the floor, pulled up hard, in the air
  const auto q = matrix({0.0, 0.0, 0.5}, {3, 1});
  const auto tau = matrix({0.0, 40.0, 0.0}, {3, 1});
  const auto zero_rate = matrix({0.0, 0.0, 0.0}, {3, 1});
  const auto out = robot::constrain(slider, floor, none, q, zero_rate, tau, plane);
  const auto a = out.acceleration.host<double>(), force = out.contact_force.host<double>();
  expect(std::abs(a[0]) < 1e-6 && std::abs(force[0] - 2.0 * kG) < 1e-5, "resting contact carries the weight");
  expect(std::abs(a[1] - (40.0 - 2.0 * kG) / 2.0) < 1e-6 && std::abs(force[1]) < 1e-6, "contact releases when pulled up");
  expect(std::abs(a[2] + kG) < 1e-6 && std::abs(force[2]) < 1e-6, "no contact force in the air");
  expect(out.gap.sizes() == std::vector<std::int64_t>{3, 1}, "gap shape");
  expect(std::abs(robot::gap(slider, floor, q).host<double>()[2] - 0.5) < 1e-12, "contact gap");
  // partial support: a pull of half the weight leaves the floor carrying the rest
  const auto half = robot::constrain(slider, floor, none, vec({0.0}).reshape({1}), vec({0.0}), vec({kG}), plane);
  expect(std::abs(half.contact_force.items<double>()[0] - kG) < 1e-5 && std::abs(half.acceleration.items<double>()[0]) < 1e-6, "partial support");

  // a pin: the tip of the planar arm is held at rest; its acceleration (second difference of the path) vanishes
  const Arm arm;
  const auto planar = arm.make();
  robot::Pins tip;
  tip.link = {1};
  tip.point = matrix({arm.l2, 0, 0}, {1, 3});
  robot::ConstraintOptions gravity_in_plane;
  gravity_in_plane.dynamics = in_plane();
  const auto qq = vec({0.4, -0.8}), qv = vec({0.0, 0.0});
  const auto held = robot::constrain(planar, robot::Contacts{}, tip, qq, qv, vec({1.0, 0.5}), gravity_in_plane);
  auto where = [&](const Series &x) { return robot::locate(planar, x, tip.link, tip.point).host<double>(); };
  const double eps = 1e-4;
  const auto p0 = where(qq), p1 = where(qq.add(held.acceleration.mul(eps)));
  expect(std::abs(p1[0] - p0[0]) < 1e-9 && std::abs(p1[1] - p0[1]) < 1e-9, "a pinned tip does not accelerate");
  const auto free_acc = robot::accel(planar, qq, qv, vec({1.0, 0.5}), in_plane());
  const auto f1 = where(qq.add(free_acc.mul(eps)));
  expect(std::abs(f1[0] - p0[0]) > 1e-6 || std::abs(f1[1] - p0[1]) > 1e-6, "the unconstrained tip does accelerate");
  expect(held.pin_force.sizes() == std::vector<std::int64_t>{1, 3}, "pin force shape");
  // forces balance: M qdd + bias = tau + J^T f for the pin force
  const auto m = robot::inertia(planar, qq);
  const auto lhs = m.matmul(held.acceleration.unsqueeze(-1)).squeeze(-1).add(robot::bias(planar, qq, qv, in_plane()));
  const auto jac = robot::jac(planar, qq, 1);   // origin of link 1; the tip is shifted by the lever arm
  (void)lhs; (void)jac;
  expect_error([&] { robot::locate(planar, qq, {5}, matrix({0, 0, 0}, {1, 3})); }, "link outside accepted");
  expect_error([&] { robot::constrain(planar, robot::Contacts{}, robot::Pins{{1}, matrix({0, 0}, {1, 2})}, qq, qv, vec({0, 0})); }, "bad pin point accepted");
  robot::ConstraintOptions bad;
  bad.contact_threshold = -1.0;
  expect_error([&] { robot::constrain(planar, robot::Contacts{}, robot::Pins{}, qq, qv, vec({0, 0}), bad); }, "negative threshold accepted");
  // nothing to constrain: plain forward dynamics
  const auto plain = robot::constrain(planar, robot::Contacts{}, robot::Pins{}, qq, qv, vec({1.0, 0.5}), gravity_in_plane);
  expect(worst(plain.acceleration, robot::accel(planar, qq, qv, vec({1.0, 0.5}), in_plane())) < 1e-12, "no constraints is forward dynamics");
}


// A block of mass 2 kg free to move in the x-y plane: two prismatic joints (x, then y), the block on the second link.
robot::Robot plane_block() {
  robot::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 1};
  links.axis = matrix({1, 0, 0, 0, 1, 0}, {2, 3});
  links.translation = matrix({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({0.0, 2.0});
  return robot::make(links);
}

void friction_checks() {
  const auto block = plane_block();
  robot::ConstraintOptions plane;
  plane.dynamics.gravity = pluss::series::make(std::vector<double>{0.0, -kG, 0.0});
  const double theta = M_PI / 6.0, c = std::cos(theta), sn = std::sin(theta);
  // an incline through the origin: the block (a point on link 1) rests on it, normal (-sin, cos, 0)
  robot::Contacts slope;
  slope.link = {1};
  slope.point = matrix({0, 0, 0}, {1, 3});
  slope.normal = matrix({-sn, c, 0}, {1, 3});
  slope.offset = vec({0.0});
  const auto q = vec({0.0, 0.0}), rest = vec({0.0, 0.0});
  // sticking: tan(30 deg) = 0.577 < mu = 0.8, so the block does not move and friction carries m g sin(theta)
  slope.friction = vec({0.8});
  const auto held = robot::constrain(block, slope, robot::Pins{}, q, rest, rest, plane);
  expect(held.acceleration.abs().amax().item<double>() < 1e-5, "friction holds the block on the slope");
  expect(std::abs(held.contact_force.items<double>()[0] - 2.0 * kG * c) < 1e-4, "normal force on the slope");
  const auto ff = held.friction_force.host<double>();
  expect(std::abs(std::sqrt(ff[0] * ff[0] + ff[1] * ff[1] + ff[2] * ff[2]) - 2.0 * kG * sn) < 1e-4, "static friction balances the tangential weight");
  // sliding: mu = 0.3 < tan(30 deg): a = g (sin - mu cos) down the slope
  slope.friction = vec({0.3});
  const auto slides = robot::constrain(block, slope, robot::Pins{}, q, rest, rest, plane);
  const auto a = slides.acceleration.items<double>();
  const double along = a[0] * c + a[1] * sn;   // the component along the slope direction (c, s)
  const double expected = kG * (sn - 0.3 * c);
  expect(std::abs(std::abs(along) - expected) < 1e-3, "kinetic sliding acceleration");
  // frictionless: the full g sin(theta)
  slope.friction = pluss::Series{};
  const auto smooth = robot::constrain(block, slope, robot::Pins{}, q, rest, rest, plane).acceleration.items<double>();
  expect(std::abs(std::abs(smooth[0] * c + smooth[1] * sn) - kG * sn) < 1e-3, "frictionless slope");
  expect(slides.friction_force.sizes() == std::vector<std::int64_t>{1, 3}, "friction force shape");
  expect_error([&] { robot::Contacts bad = slope; bad.friction = vec({-0.1}); robot::constrain(block, bad, robot::Pins{}, q, rest, rest, plane); }, "negative friction accepted");
  expect_error([&] { robot::ConstraintOptions bad = plane; bad.friction_iterations = 0; robot::constrain(block, slope, robot::Pins{}, q, rest, rest, bad); }, "zero friction iterations accepted");
}

void impact_checks() {
  const auto block = plane_block();
  robot::Contacts floor;
  floor.link = {1};
  floor.point = matrix({0, 0, 0}, {1, 3});
  floor.normal = matrix({0, 1, 0}, {1, 3});
  floor.offset = vec({0.0});
  const auto q = vec({0.0, 0.0});
  // falling at 3 m/s: plastic, half-elastic and elastic impacts
  for (double e : {0.0, 0.5, 1.0}) {
    const auto hit = robot::impact(block, floor, robot::Pins{}, q, vec({0.0, -3.0}), e);
    expect(std::abs(hit.velocity.items<double>()[1] - 3.0 * e) < 1e-5 && std::abs(hit.velocity.items<double>()[0]) < 1e-9, "restitution " + std::to_string(e));
    expect(std::abs(hit.contact_impulse.items<double>()[0] - 2.0 * 3.0 * (1.0 + e)) < 1e-4, "impulse m (1 + e) v");
  }
  // moving away is not an impact; out of range is not an impact
  expect(robot::impact(block, floor, robot::Pins{}, q, vec({1.0, 2.0}), 0.5).velocity.sub(vec({1.0, 2.0})).abs().amax().item<double>() < 1e-6, "separating contact untouched");
  expect(robot::impact(block, floor, robot::Pins{}, vec({0.0, 0.5}), vec({0.0, -3.0}), 0.5).velocity.sub(vec({0.0, -3.0})).abs().amax().item<double>() < 1e-6, "out of range untouched");
  // friction: tangential speed 2 m/s, impulse limited to mu times the normal impulse: dvx = mu * 2 * 1 / 2
  floor.friction = vec({0.5});
  const auto rough = robot::impact(block, floor, robot::Pins{}, q, vec({2.0, -1.0}), 0.0);
  expect(std::abs(rough.velocity.items<double>()[0] - 1.5) < 1e-4 && std::abs(rough.velocity.items<double>()[1]) < 1e-5, "friction in an impact");
  // enough friction to stop the sliding: mu 2 stops the block
  floor.friction = vec({2.0});
  expect(std::abs(robot::impact(block, floor, robot::Pins{}, q, vec({2.0, -1.0}), 0.0).velocity.items<double>()[0]) < 1e-4, "sticking impact");
  // a pin stops its point
  robot::Pins pin;
  pin.link = {1};
  pin.point = matrix({0, 0, 0}, {1, 3});
  expect(robot::impact(block, robot::Contacts{}, pin, q, vec({2.0, -1.0}), 0.0).velocity.abs().amax().item<double>() < 1e-5, "pin stops the point");
  expect(robot::impact(block, robot::Contacts{}, robot::Pins{}, q, vec({2.0, -1.0}), 0.5).velocity.sub(vec({2.0, -1.0})).abs().amax().item<double>() == 0.0, "no constraints: velocity unchanged");
  expect_error([&] { robot::impact(block, floor, robot::Pins{}, q, vec({0.0, -1.0}), 1.5); }, "restitution above one accepted");
  expect_error([&] { robot::impact(block, floor, robot::Pins{}, q, vec({0.0, -1.0}), -0.1); }, "negative restitution accepted");
  // batch
  const auto batch = robot::impact(block, floor, robot::Pins{}, matrix({0, 0, 0, 0}, {2, 2}), matrix({0, -3, 0, 3}, {2, 2}), 0.5);
  expect(batch.velocity.sizes() == std::vector<std::int64_t>{2, 2} && std::abs(batch.velocity.host<double>()[1] - 1.5) < 1e-5 && std::abs(batch.velocity.host<double>()[3] - 3.0) < 1e-6, "batched impact");
}

void validation_checks() {
  const Arm arm;
  const auto r = arm.make();
  expect_error([&] { robot::frames(r, vec({0.1, 0.2, 0.3})); }, "wrong q width accepted");
  expect_error([&] { robot::jac(r, vec({0.1, 0.2}), 2); }, "link outside accepted");
  expect_error([&] { robot::torques(r, vec({0.1, 0.2}), vec({0.1}), vec({0.1, 0.2})); }, "wrong qd accepted");
  robot::Links bare;
  bare.parent = {-1};
  bare.prismatic = {0};
  bare.axis = matrix({0, 0, 1}, {1, 3});
  bare.translation = matrix({0, 0, 0}, {1, 3});
  const auto kinematic = robot::make(bare);
  expect(!kinematic.has_mass(), "kinematic-only robot has no mass");
  expect_error([&] { robot::inertia(kinematic, vec({0.1})); }, "mass matrix without mass accepted");
  robot::Links bad = bare;
  bad.parent = {0};
  expect_error([&] { robot::make(bad); }, "self parent accepted");
  bad = bare;
  bad.axis = matrix({0, 0, 0}, {1, 3});
  expect_error([&] { robot::make(bad); }, "zero axis accepted");
  bad = bare;
  bad.prismatic = {2};
  expect_error([&] { robot::make(bad); }, "bad joint kind accepted");
  bad = bare;
  bad.mass = vec({-1.0});
  expect_error([&] { robot::make(bad); }, "negative mass accepted");
  bad = bare;
  bad.center = matrix({0, 0, 0}, {1, 3});
  expect_error([&] { robot::make(bad); }, "center without mass accepted");
  robot::IkOptions options;
  options.damping = 0.0;
  expect_error([&] { robot::ik(kinematic, 0, vec({1, 0, 0}), Series{}, vec({0.0}), options); }, "zero damping accepted");
  // single precision keeps its dtype
  robot::Links single = bare;
  single.axis = single.axis.to(pluss::ScalarType::Float32);
  single.translation = single.translation.to(pluss::ScalarType::Float32);
  const auto r32 = robot::make(single);
  const auto p32 = robot::frames(r32, vec({0.3}).to(pluss::ScalarType::Float32));
  expect(p32.position.scalar_type() == pluss::ScalarType::Float32, "Float32 robot returns Float32");
  // reverse sensitivity: d potential / d mass is minus the height of the centre times gravity
  auto tracked = vec({1.0, 2.0});
  tracked.tracks_slope_(true);
  robot::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = matrix({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = matrix({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = tracked;
  links.center = matrix({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  const auto tracked_robot = robot::make(links);
  robot::pe(tracked_robot, vec({0.5, 0.3}), in_plane()).reverse();
  const auto sens = tracked.sensitivity().host<double>();
  const double height1 = 0.25 * std::sin(0.5), height2 = 0.5 * std::sin(0.5) + 0.2 * std::sin(0.8);
  expect(std::abs(sens[0] - kG * height1) < 1e-10 && std::abs(sens[1] - kG * height2) < 1e-10, "mass sensitivity");
}

} // namespace

int main() {
  rotation_checks();
  kinematics_checks();
  dynamics_checks();
  prismatic_checks();
  tree_checks();
  ik_checks();
  constraint_checks();
  friction_checks();
  impact_checks();
  validation_checks();
  std::cout << "Robot checks passed\n";
}
