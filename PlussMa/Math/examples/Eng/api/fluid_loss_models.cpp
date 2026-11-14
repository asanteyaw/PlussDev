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

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand = 0.02) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double q = 0.05, D = 0.30, L = 1000.0, g = 9.80665, kPi = 3.14159265358979323846;
  const double v = 4.0 * q / (kPi * D * D);
  fluid::Options darcy;
  fluid::Options hazen;
  hazen.loss = fluid::LossModel::HazenWilliams;
  auto pipe = [&](double rough, double minor) {
    auto net = one_pipe(q);
    net.nodes.head = vec({100.0, 90.0});
    net.nodes.elevation = vec({0.0, 0.0});
    net.pipes.roughness = vec({rough});
    net.pipes.minor = vec({minor});
    return fluid::network(net);
  };
  for (double eps : {1e-5, 1e-4, 1e-3}) {
    const auto r = fluid::steady(pipe(eps, 0.0), darcy);
    out.writeln("Darcy-Weisbach, roughness ", eps, " m: friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0], " m");
  }
  for (double C : {80.0, 130.0, 150.0}) {
    const auto r = fluid::steady(pipe(C, 0.0), hazen);
    const double formula = 10.67 * L * std::pow(q, 1.852) / (std::pow(C, 1.852) * std::pow(D, 4.8704));
    out.writeln("Hazen-Williams, C = ", C, ": head loss ", flat(r.head_loss)[0], ", formula ", formula, ", friction field ", flat(r.friction_factor)[0]);
  }
  const double base = flat(fluid::steady(pipe(1e-4, 0.0), darcy).head_loss)[0];
  out.writeln("minor loss K = 20: head loss ", flat(fluid::steady(pipe(1e-4, 20.0), darcy).head_loss)[0], ", base + K v^2/2g = ", base + 20.0 * v * v / (2.0 * g));
}
