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
  attempt("Hazen-Williams with C = 0", [] {
    auto net = one_pipe();
    net.pipes.roughness = vec({0.0});
    fluid::Options options;
    options.loss = fluid::LossModel::HazenWilliams;
    (void)fluid::steady(fluid::network(net), options);
  });
  attempt("full pressure below the minimum", [] {
    fluid::Options options;
    options.demand_mode = fluid::DemandMode::Pressure;
    options.min_pressure = 20.0;
    options.full_pressure = 10.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("a demand exponent of zero", [] {
    fluid::Options options;
    options.demand_exponent = 0.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("a head tolerance of zero", [] {
    fluid::Options options;
    options.head_tolerance = 0.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("an iteration limit of zero", [] {
    fluid::Options options;
    options.max_iterations = 0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
}
