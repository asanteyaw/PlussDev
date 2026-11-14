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

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(line());
  fluid::Schedule schedule;
  schedule.duration = 3600.0;
  schedule.step = 600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  attempt("a constituent with no initial quality", [&] { fluid::Quality q; (void)fluid::transport(network, schedule, q); });
  attempt("age with a decay rate", [&] { fluid::Quality q; q.mode = fluid::QualityMode::Age; q.pipe_rate = vec({-1e-4}); (void)fluid::transport(network, schedule, q); });
  attempt("a negative concentration", [&] { fluid::Quality q; q.initial = vec({-1.0, 0.0}); (void)fluid::transport(network, schedule, q); });
  attempt("an initial field of the wrong width", [&] { fluid::Quality q; q.initial = vec({1.0}); (void)fluid::transport(network, schedule, q); });
  attempt("a Courant limit of 1.5", [&] { fluid::Quality q; q.initial = vec({1.0, 0.0}); fluid::RouteOptions r; r.courant = 1.5; (void)fluid::transport(network, schedule, q, r); });
  attempt("zero cells", [&] { fluid::Quality q; q.initial = vec({1.0, 0.0}); fluid::RouteOptions r; r.cells = 0; (void)fluid::transport(network, schedule, q, r); });
  attempt("a quality step of zero", [&] { fluid::Quality q; q.initial = vec({1.0, 0.0}); fluid::RouteOptions r; r.step = 0.0; (void)fluid::transport(network, schedule, q, r); });
}
