#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// reservoir (60 m) -> pipe -> junction 1 (demand 0.04) -> pipe -> tank; the tank bottom is at 20 m, its surface starts at 23 m
static fluid::Net with_tank(double area = 400.0, bool overflow = false, double initial_head = 23.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 10.0, 20.0});
  net.nodes.demand = vec({0.0, 0.04, 0.0});
  net.nodes.head = vec({60.0, 50.0, initial_head});
  net.tanks.area = vec({area});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {overflow};
  net.pipes.from = {0, 1};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({800.0, 400.0});
  net.pipes.diameter = vec({0.15, 0.20});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

// a pump from a low reservoir (10 m) into junction 1, which draws 0.03 m3/s and is joined to a 200 m2 tank by a pipe
static fluid::Net pumped(double area = 200.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 12.0, 20.0});
  net.nodes.demand = vec({0.0, 0.03, 0.0});
  net.nodes.head = vec({10.0, 30.0, 23.0});
  net.tanks.area = vec({area});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {false};
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = vec({60.0});
  net.pumps.resistance = vec({1500.0});
  net.pumps.exponent = vec({2.0});
  net.pumps.speed = vec({1.0});
  net.pipes.from = {1};
  net.pipes.to = {2};
  net.pipes.length = vec({300.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  fluid::Schedule base;
  base.duration = 24.0 * 3600.0;
  base.step = 1800.0;
  base.pattern_step = 24.0 * 3600.0;
  const fluid::Action close{fluid::Device::Pump, 0, fluid::Act::Close, 0.0};
  const fluid::Action open{fluid::Device::Pump, 0, fluid::Act::Open, 0.0};
  attempt("a tank that starts above its maximum level", [&] { (void)fluid::run(fluid::network(with_tank(400.0, false, 29.0)), base); });
  attempt("a demand pattern with two columns for three nodes", [&] {
    auto s = base;
    s.demand = pluss::series::make(std::vector<double>{0.0, 1.0}, {1, 2});
    (void)fluid::run(fluid::network(with_tank()), s);
  });
  attempt("a negative demand multiplier", [&] {
    auto s = base;
    s.demand = pluss::series::make(std::vector<double>{0.0, -1.0, 0.0}, {1, 3});
    (void)fluid::run(fluid::network(with_tank()), s);
  });
  attempt("thresholds the wrong way round", [&] {
    auto s = base;
    s.state = {{2, fluid::Sense::Below, 5.0, close}, {2, fluid::Sense::Above, 2.0, open}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("a rule for pump 3 of 1", [&] {
    auto s = base;
    s.state = {{2, fluid::Sense::Above, 5.0, {fluid::Device::Pump, 3, fluid::Act::Close, 0.0}}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("an action before the run starts", [&] {
    auto s = base;
    s.time = {{-5.0, close}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("a pump speed of -1", [&] {
    auto s = base;
    s.time = {{3600.0, {fluid::Device::Pump, 0, fluid::Act::Set, -1.0}}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("close above 5 m and never reopen", [&] {
    auto s = base;
    s.state = {{2, fluid::Sense::Above, 5.0, close}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
}
