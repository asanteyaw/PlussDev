#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the three-bus network of chapter 1: a slack, a generator bus held at 1.02, and a load bus
static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

// ten measurements taken from the exact solution of the network, each disturbed by a fixed error of the stated size:
// voltage magnitudes at every bus, net active power at buses 1 and 2, active power entering lines 0 and 2 at their sending ends,
// active power entering line 1 at its receiving end, reactive power entering line 1 at its sending end, and the angle at bus 2
static power::Measurements measurements(const power::AcResult &truth, double gross_error = 0.0) {
  const auto vm = flat(truth.magnitude), va = flat(truth.angle), injected = flat(truth.injection.real());
  const auto from_p = flat(truth.from_power.real()), from_q = flat(truth.from_power.imag()), to_p = flat(truth.to_power.real());
  power::Measurements m;
  m.kind = {power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::BusP, power::MeasureKind::BusP,
            power::MeasureKind::FromP, power::MeasureKind::FromP, power::MeasureKind::ToP, power::MeasureKind::FromQ, power::MeasureKind::Angle};
  m.index = {0, 1, 2, 1, 2, 0, 2, 1, 1, 2};
  std::vector<double> value = {vm[0], vm[1], vm[2], injected[1], injected[2], from_p[0], from_p[2], to_p[1], from_q[1], va[2]};
  const std::vector<double> error = {0.002, -0.001, 0.0015, 0.004, -0.003, 0.002, -0.002, 0.003, -0.002, 0.001};
  for (std::size_t i = 0; i < value.size(); ++i) value[i] += error[i];
  value[5] += gross_error;
  m.value = vec(value);
  m.stddev = vec(std::vector<double>(value.size(), 0.01));
  return m;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto truth = power::ac(grid);

  const auto clean = power::estimate(grid, measurements(truth));
  out.writeln("normalised residuals, clean data: ", flat(clean.normalized_residual));
  const auto covariance = flat(clean.covariance);   // [state][state], angles of buses 1 and 2, then three magnitudes
  std::vector<double> deviation;
  for (int i = 0; i < 5; ++i) deviation.push_back(std::sqrt(covariance[5 * i + i]));
  out.writeln("standard deviation of the estimated state: ", deviation, "   (every measurement had sigma = 0.01)");

  const auto bad = power::estimate(grid, measurements(truth, 0.3));
  out.writeln("with a 0.3 error on measurement 5 (active power entering line 0): J = ", flat(bad.objective), " against ", flat(clean.objective));
  out.writeln("normalised residuals: ", flat(bad.normalized_residual));
  out.writeln("a chi-square test with 5 degrees of freedom at 95 percent rejects J above 11.07: clean data ", flat(clean.objective)[0] > 11.07 ? "rejected" : "accepted", ", bad data ", flat(bad.objective)[0] > 11.07 ? "rejected" : "accepted");
}
