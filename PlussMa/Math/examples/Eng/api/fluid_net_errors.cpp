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
  attempt("a pipe pair with no reservoir", [] {
    auto net = one_pipe(0.01);
    net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
    net.nodes.elevation = vec({0.0, 5.0, 5.0, 5.0});
    net.nodes.demand = vec({0.0, 0.01, 0.01, 0.01});
    net.nodes.head = vec({50.0, 49.0, 49.0, 49.0});
    net.pipes.from = {0, 2};
    net.pipes.to = {1, 3};
    net.pipes.length = vec({1000.0, 1000.0});
    net.pipes.diameter = vec({0.30, 0.30});
    net.pipes.roughness = vec({1e-4, 1e-4});
    net.pipes.minor = vec({0.0, 0.0});
    (void)fluid::network(net);
  });
  attempt("a demand at the reservoir", [] { auto net = one_pipe(); net.nodes.demand = vec({0.02, 0.0}); (void)fluid::network(net); });
  attempt("a zero diameter", [] { auto net = one_pipe(); net.pipes.diameter = vec({0.0}); (void)fluid::network(net); });
  attempt("a pipe from a node to itself", [] { auto net = one_pipe(); net.pipes.to = {0}; (void)fluid::network(net); });
  attempt("a pipe to node 7 of 2", [] { auto net = one_pipe(); net.pipes.to = {7}; (void)fluid::network(net); });
  attempt("a negative roughness", [] { auto net = one_pipe(); net.pipes.roughness = vec({-1.0}); (void)fluid::network(net); });
  attempt("integer lengths", [] { auto net = one_pipe(); net.pipes.length = pluss::series::make(std::vector<int>{1000}); (void)fluid::network(net); });
  attempt("mixed precision", [] { auto net = one_pipe(); net.pipes.length = net.pipes.length.to(pluss::ScalarType::Float32); (void)fluid::network(net); });
}
