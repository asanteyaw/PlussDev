#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit, with the given load
static power::Net two_bus(double active, double reactive) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -reactive});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(two_bus(0.0, 0.0));
  power::LoadPath path;
  path.p = vec({0.0, -1.0});
  path.q = vec({0.0, -0.4});
  const auto run = [&](const char *label, power::TraceOptions options) {
    options.target = 2.0;
    try {
      const auto r = power::trace(grid, path, options);
      out.writeln(label, ": ", r.points, " points, reached ", r.reached.item<bool>(), ", last factor ", flat(r.factor).back(), ", voltage ", flat(r.magnitude.select(-1, 1)).back());
    } catch (const std::exception &e) {
      std::cout << label << ": " << e.what() << "\n";
    }
  };
  run("defaults (adaptive)        ", power::TraceOptions{});
  power::TraceOptions fixed;
  fixed.adaptive = false;
  fixed.step = 0.1;
  run("fixed step 0.1             ", fixed);
  power::TraceOptions wide;
  wide.step_max = 0.5;
  run("step_max 0.5               ", wide);
  power::TraceOptions oversized;
  oversized.step = 0.5;
  run("first step above step_max  ", oversized);
  power::TraceOptions few;
  few.max_steps = 3;
  run("max_steps 3                ", few);
}
