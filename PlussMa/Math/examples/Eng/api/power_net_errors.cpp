#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net two_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  report("no slack bus          ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::PQ, power::BusKind::PQ}; (void)power::make(n); });
  report("two slack buses       ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::Slack, power::BusKind::Slack}; (void)power::make(n); });
  report("an isolated bus       ", [] { auto n = two_bus(); n.lines.active = {false}; (void)power::make(n); });
  report("zero line impedance   ", [] { auto n = two_bus(); n.lines.r = vec({0.0}); n.lines.x = vec({0.0}); (void)power::make(n); });
  report("generator off the grid", [] { auto n = two_bus(); n.gens.bus = {7}; n.gens.p = vec({0.1}); n.gens.q = vec({0.0}); (void)power::make(n); });

  // a 50 MW load written as 50 where per unit was meant: accepted, and the answer is nonsense
  power::Net watts = two_bus();
  watts.buses.p = vec({0.0, -50.0});
  watts.buses.q = vec({0.0, -20.0});
  const auto r = power::ac(power::make(watts));
  std::cout << "a load of 50 per unit (meant 0.5): accepted, converged " << r.converged.item<bool>() << ", load-bus voltage " << flat(r.magnitude)[1] << "\n";
}
