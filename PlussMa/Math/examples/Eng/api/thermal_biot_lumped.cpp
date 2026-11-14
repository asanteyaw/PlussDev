#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A plate heated on one face cools through the other. A lumped body (C = rho c L A, G = h A) against a ten-cell mesh.
  const double thickness = 0.02, area = 1.0, h = 20.0, rho = 2700.0, c = 900.0;
  const auto run_for = [&](double k) {
    th::Layers layers;
    layers.cells = {10};
    layers.thickness = vec({thickness});
    layers.conductivity = vec({k});
    layers.density = vec({rho});
    layers.specific_heat = vec({c});
    th::Slab w = th::mesh(layers, vec({area}).reshape({}));
    const std::int64_t ambient = w.net.nodes;
    w.net.nodes += 1;
    w.net.links.a.push_back(w.outer);
    w.net.links.b.push_back(ambient);
    w.net.links.conductance = w.net.links.conductance.unite({vec({h * area})}, -1);
    w.net.fixed = {{ambient}, vec({0.0})};
    th::Start start;
    start.temperature = vec(std::vector<double>(10, 100.0));
    const double tau = rho * c * thickness / h;
    const auto run = th::tran(w.net, vec(grid(tau / 50.0, 51)), {}, start);
    return std::make_pair(flat(run.temperature.select(1, 1)), flat(run.temperature.select(1, 10)));
  };
  const double tau = rho * c * thickness / h;
  const double bi = h * thickness / 200.0;
  const auto [near_face, far_face] = run_for(200.0);
  out.writeln("k = 200: Bi =", bi, " time constant", tau, "s");
  out.writeln("  at one time constant: face next to the heat source", near_face[50], " cooled face", far_face[50], " lumped", 100.0 * std::exp(-1.0));
  const double slow = h * thickness / 0.5;
  const auto [near_slow, far_slow] = run_for(0.5);
  out.writeln("k = 0.5: Bi =", slow);
  out.writeln("  at one time constant: face next to the heat source", near_slow[50], " cooled face", far_slow[50], " lumped", 100.0 * std::exp(-1.0));
}
