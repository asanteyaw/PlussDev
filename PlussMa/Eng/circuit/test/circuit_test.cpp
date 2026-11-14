#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace circuit = pluss::subsp::eng::circuit;

namespace {

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<double> &expected,
           double tolerance = 1e-9) {
  const auto actual = value.host<double>();
  expect(actual.size() == expected.size(), "result size mismatch");
  for (std::size_t i = 0; i < actual.size(); ++i)
    expect(std::abs(actual[i] - expected[i]) <= tolerance,
           "numeric result mismatch");
}

void expect_error(const std::function<void()> &call,
                  const std::string &message) {
  try {
    call();
  } catch (const pluss::Error &) {
    return;
  }
  throw std::runtime_error(message);
}

circuit::Branches branches(std::initializer_list<std::int64_t> positive,
                           std::initializer_list<std::int64_t> negative,
                           pluss::Series value) {
  return {{positive}, {negative}, std::move(value)};
}

circuit::Controls controls(std::initializer_list<std::int64_t> positive,
                           std::initializer_list<std::int64_t> negative,
                           std::initializer_list<std::int64_t> control_positive,
                           std::initializer_list<std::int64_t> control_negative,
                           pluss::Series gain) {
  return {{positive},
          {negative},
          {control_positive},
          {control_negative},
          std::move(gain)};
}

circuit::Amps amps(std::initializer_list<std::int64_t> output,
                   std::initializer_list<std::int64_t> reference,
                   std::initializer_list<std::int64_t> positive_input,
                   std::initializer_list<std::int64_t> negative_input) {
  return {{output}, {reference}, {positive_input}, {negative_input}};
}

} // namespace

// ---- transient analysis ---------------------------------------------------------------------------------------
namespace {

pluss::Series grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return pluss::series::make(t);
}

// RC low-pass: 5 V source, 1 kOhm, 1 uF (tau = 1 ms), output node 2.
circuit::Net rc_net(double resistance = 1000.0) {
  circuit::Net net;
  net.nodes = 3;
  net.voltage_sources = branches({1}, {0}, pluss::series::make({5.0}));
  net.resistors = branches({1}, {2}, pluss::series::make({resistance}));
  net.capacitors = branches({2}, {0}, pluss::series::make({1.0e-6}));
  return net;
}

double rc_error(circuit::Scheme scheme, double step, int count) {
  const auto run = circuit::tran(circuit::make(rc_net()), grid(step, count), {}, {}, circuit::TranOptions{scheme});
  const auto v = run.voltage.host<double>();
  double worst = 0.0;
  for (int k = 0; k < count; ++k)
    worst = std::max(worst, std::abs(v[static_cast<std::size_t>(k) * 3 + 2] - 5.0 * (1.0 - std::exp(-k * step / 1e-3))));
  return worst;
}

void transient_checks() {
  // RC step response against 5 (1 - exp(-t/tau)), both schemes, and second-order convergence when the step halves.
  const auto rc_run = circuit::tran(circuit::make(rc_net()), grid(1e-4, 51));
  expect(rc_run.voltage.sizes() == std::vector<std::int64_t>{51, 3}, "tran voltage shape is wrong");
  expect(rc_run.source_current.sizes() == std::vector<std::int64_t>{51, 1}, "tran source current shape is wrong");
  expect(rc_run.residual.sizes() == std::vector<std::int64_t>{51}, "tran residual shape is wrong");
  expect(rc_run.residual.amax().item<double>() < 1e-12, "tran residual is too large");
  const auto trap_coarse = rc_error(circuit::Scheme::Trapezoid, 1e-4, 51);
  const auto trap_fine = rc_error(circuit::Scheme::Trapezoid, 5e-5, 101);
  expect(trap_coarse < 2e-3 && trap_coarse / trap_fine > 3.5 && trap_coarse / trap_fine < 4.5,
         "trapezoid is not second order on the RC step");
  const auto gear_coarse = rc_error(circuit::Scheme::Gear2, 1e-4, 51);
  const auto gear_fine = rc_error(circuit::Scheme::Gear2, 5e-5, 101);
  expect(gear_coarse < 3e-2 && gear_coarse / gear_fine > 3.0 && gear_coarse / gear_fine < 4.5,
         "Gear2 is not second order on the RC step");

  // The source starts at its full value: the first sample is the consistent state with the capacitor at rest.
  const auto first = rc_run.voltage.slice(0, 0, 1).host<double>();
  close(rc_run.voltage.slice(0, 0, 1), {0.0, 5.0, 0.0}, 1e-12);
  expect(first.size() == 3, "first sample size");

  // A charged capacitor discharges through the resistor from its start voltage.
  circuit::Net discharge = rc_net();
  discharge.voltage_sources.value = pluss::series::make({0.0});
  circuit::Start charged;
  charged.capacitor_voltage = pluss::series::make({4.0});
  const auto dis = circuit::tran(circuit::make(discharge), grid(1e-4, 31), {}, charged);
  const auto dv = dis.voltage.host<double>();
  expect(std::abs(dv[2] - 4.0) < 1e-12, "capacitor start voltage was not honoured");
  expect(std::abs(dv[30 * 3 + 2] - 4.0 * std::exp(-3.0)) < 2e-3, "RC discharge is wrong");

  // RL step: i(t) = (V/R)(1 - exp(-R t / L)); inductor current starts at its start value.
  circuit::Net rl;
  rl.nodes = 3;
  rl.voltage_sources = branches({1}, {0}, pluss::series::make({5.0}));
  rl.resistors = branches({1}, {2}, pluss::series::make({10.0}));
  rl.inductors = branches({2}, {0}, pluss::series::make({0.01}));
  const auto rl_run = circuit::tran(circuit::make(rl), grid(1e-4, 31));
  const auto il = rl_run.inductor_current.host<double>();
  double rl_err = 0.0;
  for (int k = 0; k < 31; ++k) rl_err = std::max(rl_err, std::abs(il[static_cast<std::size_t>(k)] - 0.5 * (1.0 - std::exp(-k * 1e-4 / 1e-3))));
  expect(rl_err < 1e-3, "RL step current is wrong");
  circuit::Start with_current;
  with_current.inductor_current = pluss::series::make({0.25});
  close(circuit::tran(circuit::make(rl), grid(1e-4, 2), {}, with_current).inductor_current.slice(0, 0, 1), {0.25}, 1e-12);

  // Lossless LC: the trapezoid rule conserves the oscillation amplitude, Gear2 damps it.
  circuit::Net lc;
  lc.nodes = 2;
  lc.inductors = branches({1}, {0}, pluss::series::make({1.0e-3}));
  lc.capacitors = branches({1}, {0}, pluss::series::make({1.0e-6}));
  circuit::Start lc_start;
  lc_start.capacitor_voltage = pluss::series::make({1.0});
  const double period = 2.0 * std::acos(-1.0) * std::sqrt(1.0e-3 * 1.0e-6);
  const auto lc_trap = circuit::tran(circuit::make(lc), grid(period / 200.0, 801), {}, lc_start);
  const auto lt = lc_trap.voltage.select(-1, 1).host<double>();
  const double peak = *std::max_element(lt.begin(), lt.end());
  expect(std::abs(peak - 1.0) < 1e-9 && *std::max_element(lt.begin() + 600, lt.end()) > 0.99999,
         "trapezoid did not conserve the LC amplitude");
  const double w0 = 1.0 / std::sqrt(1.0e-3 * 1.0e-6);
  expect(std::abs(lt[200] - std::cos(w0 * 200.0 * period / 200.0)) < 2e-2, "LC oscillation phase is wrong");
  const auto lc_gear = circuit::tran(circuit::make(lc), grid(period / 20.0, 81), {}, lc_start,
                                     circuit::TranOptions{circuit::Scheme::Gear2});
  const auto lg = lc_gear.voltage.select(-1, 1).host<double>();
  expect(*std::max_element(lg.begin() + 60, lg.end()) < 0.9, "Gear2 should damp the lossless oscillation");

  // A sinusoidal drive settles to the phasor magnitude 1/sqrt(2) of the RC corner (omega = 1/tau).
  std::vector<double> sine;
  for (int k = 0; k < 2001; ++k) sine.push_back(std::sin(1000.0 * k * 1e-4));
  circuit::Net lowpass = rc_net();
  circuit::Drive wave;
  wave.voltage = pluss::series::make(sine, {2001, 1});
  const auto sinus = circuit::tran(circuit::make(lowpass), grid(1e-4, 2001), wave);
  const auto sv = sinus.voltage.select(-1, 2).host<double>();
  double settled = 0.0;
  for (int k = 1200; k < 2001; ++k) settled = std::max(settled, std::abs(sv[static_cast<std::size_t>(k)]));
  expect(std::abs(settled - 1.0 / std::sqrt(2.0)) < 5e-3, "sinusoidal steady state does not match the AC response");

  // Current-driven resistor (no capacitors): the voltage follows the drive exactly.
  circuit::Net current_driven;
  current_driven.nodes = 2;
  current_driven.resistors = branches({1}, {0}, pluss::series::make({10.0}));
  current_driven.current_sources = branches({1}, {0}, pluss::series::make({1.0}));
  circuit::Drive ramp;
  ramp.current = pluss::series::make({0.0, 1.0, 2.0, 3.0}, {4, 1});
  const auto resistive = circuit::tran(circuit::make(current_driven), grid(1.0, 4), ramp);
  close(resistive.voltage.select(-1, 1), {0.0, -10.0, -20.0, -30.0}, 1e-12);

  // Parameter batch: the second lane has tau = 2 ms.
  circuit::Net batched = rc_net();
  batched.resistors.value = pluss::series::make(std::vector<double>{1000.0, 2000.0}, {2, 1});
  const auto lanes = circuit::tran(circuit::make(batched), grid(1e-4, 51));
  expect(lanes.voltage.sizes() == std::vector<std::int64_t>{2, 51, 3}, "tran batch shape is wrong");
  const auto lv = lanes.voltage.host<double>();
  expect(std::abs(lv[(51 + 50) * 3 + 2] - 5.0 * (1.0 - std::exp(-2.5))) < 2e-3, "batched lane is wrong");

  // Float32 stays Float32.
  circuit::Net single = rc_net();
  single.voltage_sources.value = single.voltage_sources.value.to(pluss::ScalarType::Float32);
  single.resistors.value = single.resistors.value.to(pluss::ScalarType::Float32);
  single.capacitors.value = single.capacitors.value.to(pluss::ScalarType::Float32);
  const auto f32 = circuit::tran(circuit::make(single), grid(1e-4, 51).to(pluss::ScalarType::Float32));
  expect(f32.voltage.scalar_type() == pluss::ScalarType::Float32, "Float32 circuit returned another dtype");
  expect(std::abs(f32.voltage.select(-1, 2).select(0, 50).item<float>() - 4.96645f) < 1e-3f, "Float32 value is wrong");

  // Ideal-op-amp integrator: vout = -(1/RC) t for a 1 V input (1 kOhm, 1 uF -> -1000 V/s).
  circuit::Net integrator;
  integrator.nodes = 4;
  integrator.voltage_sources = branches({1}, {0}, pluss::series::make({1.0}));
  integrator.resistors = branches({1}, {2}, pluss::series::make({1000.0}));
  integrator.capacitors = branches({2}, {3}, pluss::series::make({1.0e-6}));
  integrator.amps = amps({3}, {0}, {0}, {2});
  const auto ramp_out = circuit::tran(circuit::make(integrator), grid(1e-4, 11));
  close(ramp_out.voltage.select(-1, 3), {0.0, -0.1, -0.2, -0.3, -0.4, -0.5, -0.6, -0.7, -0.8, -0.9, -1.0}, 1e-9);
  expect(ramp_out.amp_current.sizes() == std::vector<std::int64_t>{11, 1}, "amp current shape is wrong");

  // A controlled source charging a capacitor: v2 = -gm R V (1 - exp(-t / RC)).
  circuit::Net gm;
  gm.nodes = 3;
  gm.resistors = branches({2}, {0}, pluss::series::make({10.0}));
  gm.capacitors = branches({2}, {0}, pluss::series::make({1.0e-6}));
  gm.voltage_sources = branches({1}, {0}, pluss::series::make({2.0}));
  gm.vccs = controls({2}, {0}, {1}, {0}, pluss::series::make({0.5}));
  const auto gm_run = circuit::tran(circuit::make(gm), grid(1e-6, 51));
  close(gm_run.vccs_current, std::vector<double>(51, 1.0), 1e-12);
  expect(std::abs(gm_run.voltage.select(-1, 2).select(0, 50).item<double>() + 10.0 * (1.0 - std::exp(-5.0))) < 5e-2,
         "VCCS transient is wrong");

  // Reverse-mode sensitivity of the final voltage to the resistance agrees with a central difference.
  const double r0 = 1000.0;
  auto tracked = pluss::series::make({r0});
  tracked.tracks_slope_(true);
  circuit::Net net = rc_net();
  net.resistors.value = tracked;
  const auto traced = circuit::tran(circuit::make(net), grid(1e-4, 21));
  traced.voltage.select(-1, 2).select(0, 20).reverse();
  auto final_at = [&](double r) {
    circuit::Net n = rc_net(r);
    return circuit::tran(circuit::make(n), grid(1e-4, 21)).voltage.select(-1, 2).select(0, 20).item<double>();
  };
  const double fd = (final_at(r0 + 0.5) - final_at(r0 - 0.5)) / 1.0;
  expect(std::abs(tracked.sensitivity().items<double>()[0] - fd) < 1e-6 * std::abs(fd) + 1e-9, "transient sensitivity is wrong");

  // Argument checks.
  const auto compiled_rc = circuit::make(rc_net());
  expect_error([&] { (void)circuit::tran(compiled_rc, pluss::series::make({0.0})); }, "one time sample was accepted");
  expect_error([&] { (void)circuit::tran(compiled_rc, pluss::series::make({0.0, 1.0, 3.0})); }, "uneven time grid was accepted");
  expect_error([&] { (void)circuit::tran(compiled_rc, pluss::series::make({1.0, 0.0})); }, "decreasing time was accepted");
  expect_error([&] {
    circuit::Drive bad;
    bad.voltage = pluss::series::make({1.0, 2.0, 3.0}, {3, 1});
    (void)circuit::tran(compiled_rc, grid(1.0, 2), bad);
  }, "a drive with the wrong number of rows was accepted");
  expect_error([&] {
    circuit::Start bad;
    bad.capacitor_voltage = pluss::series::make({1.0, 2.0});
    (void)circuit::tran(compiled_rc, grid(1.0, 2), {}, bad);
  }, "a start vector with the wrong length was accepted");
  circuit::Net clash;
  clash.nodes = 3;
  clash.voltage_sources = branches({1, 1}, {0, 0}, pluss::series::make({1.0, 2.0}));
  clash.resistors = branches({1}, {2}, pluss::series::make({1.0}));
  clash.capacitors = branches({2}, {0}, pluss::series::make({1.0}));
  bool explained = false;
  try {
    (void)circuit::tran(circuit::make(clash), grid(1.0, 3));
  } catch (const pluss::Error &error) {
    explained = error.message().find("two sources fixing the same voltage") != std::string::npos;
  }
  expect(explained, "conflicting sources were not explained");
  // a source directly across an inductor is fine in a transient: the current ramps as V t / L
  circuit::Net across;
  across.nodes = 2;
  across.voltage_sources = branches({1}, {0}, pluss::series::make({1.0}));
  across.inductors = branches({1}, {0}, pluss::series::make({0.5}));
  const auto ramped = circuit::tran(circuit::make(across), grid(0.5, 4));
  close(ramped.inductor_current, {0.0, 1.0, 2.0, 3.0}, 1e-12);
}

} // namespace

// ---- semiconductor devices ------------------------------------------------------------------------------------
namespace {

constexpr double kVt = 0.02585;
constexpr double kGmin = 1e-18;

// Diode voltage of a source V behind R, by bisection on the exact scalar equation.
double diode_reference(double volts, double r, double is, double n = 1.0) {
  double lo = -volts - 1.0, hi = std::max(volts, 1.0) + 1.0;
  if (volts < 0) lo = std::min(volts, -1.0) - 1.0, hi = 1.0;
  for (int i = 0; i < 200; ++i) {
    const double mid = 0.5 * (lo + hi);
    const double f = (volts - mid) / r - is * (std::exp(mid / (n * kVt)) - 1.0) - kGmin * mid;
    (f > 0 ? lo : hi) = mid;
  }
  return 0.5 * (lo + hi);
}

// Source (node 1) - R (1 -> 2) - diode (2 -> 0).
circuit::Net diode_net(double volts = 5.0, double r = 1000.0, double is = 1e-14) {
  circuit::Net net;
  net.nodes = 3;
  net.voltage_sources = branches({1}, {0}, pluss::series::make({volts}));
  net.resistors = branches({1}, {2}, pluss::series::make({r}));
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = pluss::series::make({is});
  return net;
}

// NPN common emitter: Vcc at node 1, Rc 1->2 (collector), Vb at node 3 through Rb 3->4 (base), emitter grounded.
circuit::Net amp_net(std::int64_t polarity, double scale = 1.0) {
  circuit::Net net;
  net.nodes = 5;
  net.voltage_sources = branches({1, 3}, {0, 0}, pluss::series::make({10.0 * scale, 5.0 * scale}));
  net.resistors = branches({1, 3}, {2, 4}, pluss::series::make({1000.0, 100000.0}));
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {polarity};
  net.transistors.saturation = pluss::series::make({1e-14});
  net.transistors.forward_gain = pluss::series::make({100.0});
  return net;
}

void device_checks() {
  // Diode behind a resistor: the Newton solution matches the exact scalar equation, and KCL closes.
  const auto forward = circuit::op(circuit::make(diode_net()));
  const double vd = diode_reference(5.0, 1000.0, 1e-14);
  expect(std::abs(forward.voltage.host<double>()[2] - vd) < 1e-9, "forward diode voltage is wrong");
  expect(std::abs(forward.diode_current.items<double>()[0] - (5.0 - vd) / 1000.0) < 1e-9, "diode current is wrong");
  expect(forward.residual.item<double>() < 1e-9, "diode residual is too large");
  const auto count = forward.iterations.items<std::int64_t>()[0];
  expect(count >= 2 && count < 40, "diode iteration count is implausible");

  // Reverse bias leaves the saturation current plus the gmin leakage across the junction.
  const auto reverse = circuit::op(circuit::make(diode_net(-5.0)));
  const double vr = diode_reference(-5.0, 1000.0, 1e-14);
  expect(std::abs(reverse.voltage.host<double>()[2] - vr) < 1e-9, "reverse diode voltage is wrong");
  expect(std::abs(reverse.diode_current.items<double>()[0] - (1e-14 * (std::exp(vr / kVt) - 1.0) + kGmin * vr)) < 1e-15,
         "reverse diode current is wrong");

  // A swept source is a batch, and each lane matches its own scalar solve (the ideality factor is honoured).
  circuit::Net swept = diode_net(0.0, 1000.0, 1e-12);
  swept.voltage_sources.value = pluss::series::make({0.2, 0.8, 2.0, 12.0}).reshape({4, 1});
  swept.resistors.value = pluss::series::make({1000.0}).reshape({1});
  swept.diodes.ideality = pluss::series::make({1.7});
  const auto lanes = circuit::op(circuit::make(swept)).voltage.host<double>();
  const double drive[] = {0.2, 0.8, 2.0, 12.0};
  for (int k = 0; k < 4; ++k)
    expect(std::abs(lanes[static_cast<std::size_t>(k) * 3 + 2] - diode_reference(drive[k], 1000.0, 1e-12, 1.7)) < 1e-9,
           "swept diode lane is wrong");

  // Transistor: forward-active gain, terminal currents sum to the emitter current, and a PNP mirror reproduces it.
  const auto npn = circuit::op(circuit::make(amp_net(1)));
  const double ic = npn.collector_current.items<double>()[0], ib = npn.base_current.items<double>()[0];
  expect(ib > 1e-6 && ic / ib > 95.0 && ic / ib < 100.5, "transistor current gain is wrong");
  expect(npn.residual.item<double>() < 1e-9, "transistor residual is too large");
  const auto pnp = circuit::op(circuit::make(amp_net(-1, -1.0)));
  const auto vn = npn.voltage.host<double>(), vp = pnp.voltage.host<double>();
  for (std::size_t i = 0; i < vn.size(); ++i) expect(std::abs(vn[i] + vp[i]) < 1e-9, "PNP is not the mirror of the NPN");
  expect(std::abs(pnp.collector_current.items<double>()[0] + ic) < 1e-12, "PNP collector current is wrong");

  // Early effect: the collector current grows with the collector voltage by 1 + vce / VA, and Newton still balances.
  circuit::Net early = amp_net(1);
  early.transistors.early_voltage = pluss::series::make({50.0});
  const auto flat_gain = circuit::op(circuit::make(amp_net(1)));
  const auto early_gain = circuit::op(circuit::make(early));
  const double vce = early_gain.voltage.host<double>()[2] - 0.0, vbe = early_gain.voltage.host<double>()[4];
  expect(early_gain.residual.item<double>() < 1e-9, "Early residual");
  expect(early_gain.collector_current.items<double>()[0] > flat_gain.collector_current.items<double>()[0] &&
             vce > 1.0 && vbe > 0.6,
         "Early effect raises the collector current");
  expect_error([&] { circuit::Net bad = amp_net(1); bad.transistors.early_voltage = pluss::series::make({-5.0}); circuit::make(bad); }, "negative Early voltage accepted");

  // Newton does not break the reverse-mode sensitivity: d v_diode / d R against a central difference.
  auto tracked = pluss::series::make({1000.0});
  tracked.tracks_slope_(true);
  circuit::Net net = diode_net();
  net.resistors.value = tracked;
  circuit::op(circuit::make(net)).voltage.select(-1, 2).reverse();
  const double fd = (diode_reference(5.0, 1000.5, 1e-14) - diode_reference(5.0, 999.5, 1e-14)) / 1.0;
  expect(std::abs(tracked.sensitivity().items<double>()[0] - fd) < 1e-6 * std::abs(fd) + 1e-12,
         "device sensitivity is wrong");

  // Small signal: a divider of R and the diode's dynamic resistance n Vt / Id.
  const auto bias = circuit::op(circuit::make(diode_net()));
  const double rd = kVt / bias.diode_current.items<double>()[0];
  circuit::Net small = diode_net(1.0);
  const auto response = circuit::ac(circuit::make(small), pluss::series::make({0.0, 10.0}), bias);
  const auto gain = response.voltage.host<std::complex<double>>();
  expect(std::abs(gain[2].real() - rd / (1000.0 + rd)) < 1e-6, "small-signal diode gain is wrong");
  expect_error([&] { circuit::ac(circuit::make(small), pluss::series::make({1.0})); },
               "ac without an operating point was accepted");

  // A reverse-biased diode is open: the RC step response is the linear one, and the residual stays tiny.
  circuit::Net blocked = rc_net();
  blocked.nodes = 4;
  blocked.diodes.positive = {3};
  blocked.diodes.negative = {2};
  blocked.diodes.saturation = pluss::series::make({1e-14});
  blocked.resistors.positive = {1, 3};
  blocked.resistors.negative = {2, 0};
  blocked.resistors.value = pluss::series::make({1000.0, 1e6});
  const auto off = circuit::tran(circuit::make(blocked), grid(1e-4, 31));
  const auto vo = off.voltage.host<double>();
  expect(std::abs(vo[30 * 4 + 2] - 5.0 * (1.0 - std::exp(-3.0))) < 6e-3, "off-state diode disturbed the RC response");
  expect(off.residual.amax().item<double>() < 1e-8, "device transient residual is too large");

  // Half-wave rectifier into an RC load: the output follows the positive peaks less a diode drop and never goes
  // negative; both schemes agree.
  circuit::Net rect;
  rect.nodes = 3;
  rect.voltage_sources = branches({1}, {0}, pluss::series::make({0.0}));
  rect.diodes.positive = {1};
  rect.diodes.negative = {2};
  rect.diodes.saturation = pluss::series::make({1e-12});
  rect.resistors = branches({2}, {0}, pluss::series::make({1000.0}));
  rect.capacitors = branches({2}, {0}, pluss::series::make({1e-6}));
  const int samples = 401;
  const double step = 5e-5;
  std::vector<double> wave;
  for (int k = 0; k < samples; ++k) wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
  circuit::Drive source;
  source.voltage = pluss::series::make(wave).reshape({samples, 1});
  const auto trap = circuit::tran(circuit::make(rect), grid(step, samples), source);
  circuit::TranOptions gear;
  gear.scheme = circuit::Scheme::Gear2;
  const auto g2 = circuit::tran(circuit::make(rect), grid(step, samples), source, {}, gear);
  const auto out_trap = trap.voltage.select(-1, 2).host<double>();
  const auto out_gear = g2.voltage.select(-1, 2).host<double>();
  double peak = 0.0, floor_v = 0.0, gap = 0.0;
  for (int k = 0; k < samples; ++k) {
    peak = std::max(peak, out_trap[static_cast<std::size_t>(k)]);
    floor_v = std::min(floor_v, out_trap[static_cast<std::size_t>(k)]);
    gap = std::max(gap, std::abs(out_trap[static_cast<std::size_t>(k)] - out_gear[static_cast<std::size_t>(k)]));
  }
  expect(peak > 4.0 && peak < 4.7 && floor_v > -1e-3, "rectifier output is wrong");
  expect(gap < 0.15, "rectifier schemes disagree");
  expect(trap.residual.amax().item<double>() < 1e-8 && g2.residual.amax().item<double>() < 1e-8,
         "rectifier residual is too large");
  expect(trap.iterations.sizes() == std::vector<std::int64_t>{samples}, "iteration count shape is wrong");

  // A diode directly across an ideal source carries an absurd current: refused, with the reason.
  circuit::Net across;
  across.nodes = 2;
  across.voltage_sources = branches({1}, {0}, pluss::series::make({5.0}));
  across.diodes.positive = {1};
  across.diodes.negative = {0};
  across.diodes.saturation = pluss::series::make({1e-14});
  bool explained = false;
  try {
    circuit::op(circuit::make(across));
  } catch (const pluss::Error &error) {
    explained = error.message().find("max_current") != std::string::npos;
  }
  expect(explained, "an unlimited diode current was not reported");
  circuit::SolveOptions allowed;
  allowed.max_current = 1e80;
  expect(circuit::op(circuit::make(across), allowed).diode_current.items<double>()[0] > 1e60, "max_current knob");

  // A coarse step no longer overshoots the source: the rectifier peak stays below the 5 V peak of the source.
  {
    const int coarse = 9;
    std::vector<double> wave;
    for (int k = 0; k < coarse; ++k) wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-4));
    circuit::Drive drive;
    drive.voltage = pluss::series::make(wave).reshape({coarse, 1});
    const auto run = circuit::tran(circuit::make(rect), grid(5e-4, coarse), drive);
    expect(run.voltage.select(-1, 2).amax().item<double>() < 5.0, "coarse step overshoots the source");
    expect(run.refinements.sum().item<std::int64_t>() > 0, "coarse step was not subdivided");
  }

  // Errors: too few iterations, bad solver options, bad devices.
  circuit::SolveOptions few;
  few.max_iterations = 2;
  expect_error([&] { circuit::op(circuit::make(diode_net()), few); }, "non-convergence was not reported");
  circuit::SolveOptions bad;
  bad.thermal_voltage = 0.0;
  expect_error([&] { circuit::op(circuit::make(diode_net()), bad); }, "zero thermal voltage was accepted");
  circuit::Net no_sat = diode_net();
  no_sat.diodes.saturation = pluss::series::make({0.0});
  expect_error([&] { circuit::make(no_sat); }, "zero saturation current was accepted");
  circuit::Net bad_polarity = amp_net(1);
  bad_polarity.transistors.polarity = {2};
  expect_error([&] { circuit::make(bad_polarity); }, "bad polarity was accepted");
  circuit::Net shorted = amp_net(1);
  shorted.transistors.emitter = {2};
  expect_error([&] { circuit::make(shorted); }, "collector tied to emitter was accepted");
}

} // namespace

// Common source: Vdd (node 1), Rd 1 -> 2 (drain), gate source at node 3, source grounded.
circuit::Net mos_net(double vdd, double vgs, double rd, double lambda, std::int64_t polarity = 1) {
  circuit::Net net;
  net.nodes = 4;
  net.voltage_sources = branches({1, 3}, {0, 0}, pluss::series::make({vdd, vgs}));
  net.resistors = branches({1}, {2}, pluss::series::make({rd}));
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = pluss::series::make({2e-3});
  net.mosfets.threshold = pluss::series::make({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = pluss::series::make({lambda});
  return net;
}

void mosfet_checks() {
  // saturation: id = kp vov^2 / 2 = 4 mA, drain at 10 - 4 mA x 500
  const auto sat = circuit::op(circuit::make(mos_net(10.0, 3.0, 500.0, 0.0)));
  expect(std::abs(sat.voltage.host<double>()[2] - 8.0) < 1e-8 && std::abs(sat.drain_current.items<double>()[0] - 4e-3) < 1e-9, "MOSFET saturation");
  expect(sat.residual.item<double>() < 1e-9, "MOSFET residual");
  // triode: 5 vds^2 - 41 vds + 5 = 0
  const auto tri = circuit::op(circuit::make(mos_net(5.0, 5.0, 5000.0, 0.0)));
  expect(std::abs(tri.voltage.host<double>()[2] - (41.0 - std::sqrt(1681.0 - 100.0)) / 10.0) < 1e-8, "MOSFET triode");
  // channel-length modulation: 10 - vd = 2 (1 + 0.05 vd)
  const auto clm = circuit::op(circuit::make(mos_net(10.0, 3.0, 500.0, 0.05)));
  expect(std::abs(clm.voltage.host<double>()[2] - 8.0 / 1.1) < 1e-8, "channel-length modulation");
  // cutoff
  const auto off = circuit::op(circuit::make(mos_net(10.0, 0.5, 500.0, 0.0)));
  expect(std::abs(off.voltage.host<double>()[2] - 10.0) < 1e-6 && std::abs(off.drain_current.items<double>()[0]) < 1e-9, "MOSFET cutoff");
  // PMOS is the mirror image on negated supplies
  const auto pmos = circuit::op(circuit::make(mos_net(-10.0, -3.0, 500.0, 0.0, -1)));
  expect(std::abs(pmos.voltage.host<double>()[2] + 8.0) < 1e-8 && std::abs(pmos.drain_current.items<double>()[0] + 4e-3) < 1e-9, "PMOS mirror");
  // a sweep of gate voltages is a batch
  circuit::Net swept = mos_net(10.0, 0.0, 500.0, 0.0);
  swept.voltage_sources.value = pluss::series::make({10.0, 0.0, 10.0, 2.0, 10.0, 3.0}).reshape({3, 2});
  const auto family = circuit::op(circuit::make(swept)).drain_current.host<double>();
  expect(std::abs(family[0]) < 1e-9 && std::abs(family[1] - 1e-3) < 1e-9 && std::abs(family[2] - 4e-3) < 1e-9, "MOSFET gate sweep");
  // small signal: gain -gm Rd = -2 with gm = kp vov = 4 mS
  const auto bias = circuit::op(circuit::make(mos_net(10.0, 3.0, 500.0, 0.0)));
  const auto response = circuit::ac(circuit::make(mos_net(0.0, 1.0, 500.0, 0.0)), pluss::series::make({0.0}), bias);
  expect(std::abs(response.voltage.host<std::complex<double>>()[2].real() + 2.0) < 1e-6, "common-source gain");
  // gate capacitance is a capacitor: the gate charges through 1 kohm with tau = 1 ms
  circuit::Net charge;
  charge.nodes = 3;
  charge.voltage_sources = branches({1}, {0}, pluss::series::make({5.0}));
  charge.resistors = branches({1}, {2}, pluss::series::make({1000.0}));
  charge.mosfets.drain = {1};
  charge.mosfets.gate = {2};
  charge.mosfets.source = {0};
  charge.mosfets.polarity = {1};
  charge.mosfets.transconductance = pluss::series::make({2e-3});
  charge.mosfets.threshold = pluss::series::make({1.0});
  charge.mosfets.gate_source_capacitance = pluss::series::make({1e-6});
  const auto cg = circuit::tran(circuit::make(charge), grid(1e-4, 31));
  expect(std::abs(cg.voltage.host<double>()[30 * 3 + 2] - 5.0 * (1.0 - std::exp(-3.0))) < 4e-3, "gate capacitance charging");
  expect(cg.residual.amax().item<double>() < 1e-8, "MOSFET transient residual");
  // a diode junction capacitance and the transistor capacitances are appended after the net's capacitors
  circuit::Net both = diode_net();
  both.capacitors = branches({2}, {0}, pluss::series::make({1e-6}));
  both.diodes.junction_capacitance = pluss::series::make({2e-6});
  const auto built = circuit::make(both);
  circuit::Start start;
  start.capacitor_voltage = pluss::series::make({0.5, 0.5});
  expect(circuit::tran(built, grid(1e-4, 3), {}, start).voltage.sizes() == std::vector<std::int64_t>{3, 3}, "device capacitor appended");
  expect_error([&] { circuit::Net bad = mos_net(10.0, 3.0, 500.0, 0.0); bad.mosfets.polarity = {2}; circuit::make(bad); }, "bad MOSFET polarity");
  expect_error([&] { circuit::Net bad = mos_net(10.0, 3.0, 500.0, 0.0); bad.mosfets.transconductance = pluss::series::make({0.0}); circuit::make(bad); }, "zero transconductance");
  expect_error([&] { circuit::Net bad = mos_net(10.0, 3.0, 500.0, 0.0); bad.mosfets.gate_source_capacitance = pluss::series::make({-1e-9}); circuit::make(bad); }, "negative capacitance");
  expect_error([&] { circuit::Net bad = mos_net(10.0, 3.0, 500.0, 0.0); bad.mosfets.source = {2}; circuit::make(bad); }, "drain tied to source");
}

void charge_and_body_checks() {
  // body effect: a source follower, drain at 10 V, gate at 5 V, source through 1 kohm, bulk grounded
  auto follower = [](double gamma, bool bulk) {
    circuit::Net net;
    net.nodes = 4;
    net.voltage_sources = branches({1, 2}, {0, 0}, pluss::series::make({10.0, 5.0}));
    net.resistors = branches({3}, {0}, pluss::series::make({1000.0}));
    net.mosfets.drain = {1};
    net.mosfets.gate = {2};
    net.mosfets.source = {3};
    net.mosfets.polarity = {1};
    if (bulk) net.mosfets.bulk = {0};
    net.mosfets.transconductance = pluss::series::make({2e-3});
    net.mosfets.threshold = pluss::series::make({1.0});
    if (gamma > 0.0) net.mosfets.body_effect = pluss::series::make({gamma});
    return circuit::op(circuit::make(net)).voltage.host<double>()[3];
  };
  auto closed_form = [](double gamma) {
    double lo = 0.0, hi = 5.0;
    for (int i = 0; i < 200; ++i) {
      const double vs = 0.5 * (lo + hi);
      const double vth = 1.0 + gamma * (std::sqrt(0.6 + vs) - std::sqrt(0.6));
      const double vov = std::max(0.0, 5.0 - vs - vth);
      ((vs / 1000.0) > 1e-3 * vov * vov ? hi : lo) = vs;
    }
    return 0.5 * (lo + hi);
  };
  expect(std::abs(follower(0.0, false) - closed_form(0.0)) < 1e-7, "source follower without body effect");
  expect(std::abs(follower(0.0, true) - closed_form(0.0)) < 1e-7, "bulk with zero gamma changes nothing");
  expect(std::abs(follower(0.5, true) - closed_form(0.5)) < 1e-7 && follower(0.5, true) < follower(0.0, true) - 0.05, "body effect lowers the follower output");
  expect(std::abs(follower(0.5, false) - closed_form(0.0)) < 1e-7, "no bulk node: body tied to the source, gamma ignored");
  expect_error([] { circuit::Net net; net.nodes = 3; net.voltage_sources = branches({1}, {0}, pluss::series::make({1.0})); net.resistors = branches({1}, {2}, pluss::series::make({1.0}));
    net.mosfets.drain = {1}; net.mosfets.gate = {2}; net.mosfets.source = {0}; net.mosfets.polarity = {1}; net.mosfets.bulk = {0, 0};
    net.mosfets.transconductance = pluss::series::make({1e-3}); net.mosfets.threshold = pluss::series::make({1.0}); circuit::make(net); }, "bulk of the wrong length accepted");

  // voltage-dependent depletion capacitance: the AC corner follows C(v) = Cj0 (1 - v / Vj)^-m at the bias
  auto junction = [](double source, double cj0) {
    circuit::Net net;
    net.nodes = 3;
    net.voltage_sources = branches({1}, {0}, pluss::series::make({source}));
    net.resistors = branches({1}, {2}, pluss::series::make({1000.0}));
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = pluss::series::make({1e-14});
    net.diodes.junction_capacitance = pluss::series::make({cj0});
    net.diodes.junction_potential = pluss::series::make({0.7});
    return net;
  };
  const double c_bias = 1e-6 / std::sqrt(1.0 + 3.0 / 0.7);
  const auto bias = circuit::op(circuit::make(junction(-3.0, 1e-6)));
  const auto small = circuit::ac(circuit::make(junction(1.0, 1e-6)), pluss::series::make({1.0 / (1000.0 * c_bias)}), bias);
  expect(std::abs(std::abs(small.voltage.host<std::complex<double>>()[2]) - 1.0 / std::sqrt(2.0)) < 1e-4, "AC corner of the depletion capacitance");
  // a small step in time: the time constant is R C(bias)
  std::vector<double> wave;
  for (int k = 0; k < 41; ++k) wave.push_back(k == 0 ? -3.0 : -2.999);
  circuit::Drive step;
  step.voltage = pluss::series::make(wave).reshape({41, 1});
  const double tau = 1000.0 * c_bias;
  const auto run = circuit::tran(circuit::make(junction(-3.0, 1e-6)), grid(tau / 10.0, 41), step);
  const auto vn = run.voltage.host<double>();
  expect(std::abs((vn[40 * 3 + 2] + 3.0) - 0.001 * (1.0 - std::exp(-4.0))) < 3e-5, "depletion capacitance charging time constant");
  expect(run.residual.amax().item<double>() < 1e-8, "charge transient residual");
  // diffusion charge: y = g (1 + j w tau) of a forward-biased diode behind a resistor
  circuit::Net forward = diode_net(5.0, 1000.0, 1e-14);
  forward.diodes.transit_time = pluss::series::make({1e-6});
  const auto fwd_bias = circuit::op(circuit::make(forward));
  const double g = fwd_bias.diode_current.items<double>()[0] / kVt;
  circuit::Net excite = forward;
  excite.voltage_sources = branches({1}, {0}, pluss::series::make({1.0}));
  const double w = 1e5;
  const auto response = circuit::ac(circuit::make(excite), pluss::series::make({w}), fwd_bias).voltage.host<std::complex<double>>()[2];
  const std::complex<double> y(g, w * 1e-6 * g);
  const std::complex<double> expected = 1.0 / (1.0 + 1000.0 * y);
  expect(std::abs(response - expected) < 1e-4, "diffusion capacitance tau g");
  expect_error([] { circuit::Net net = diode_net(); net.diodes.junction_potential = pluss::series::make({-0.7}); circuit::make(net); }, "negative junction potential accepted");
  expect_error([] { circuit::Net net = diode_net(); net.diodes.grading = pluss::series::make({1.0}); circuit::make(net); }, "grading of one accepted");
  expect_error([] { circuit::Net net = diode_net(); net.diodes.transit_time = pluss::series::make({-1e-9}); circuit::make(net); }, "negative transit time accepted");
  // a forward-biased junction charge is conserved by the integrator: charge a diode node from rest, the residual stays small
  circuit::Net pulse = diode_net(3.0, 1000.0, 1e-14);
  pulse.diodes.junction_capacitance = pluss::series::make({1e-9});
  pulse.diodes.junction_potential = pluss::series::make({0.7});
  pulse.diodes.transit_time = pluss::series::make({1e-7});
  std::vector<double> square;
  for (int k = 0; k < 81; ++k) square.push_back(k < 20 ? 0.0 : 3.0);
  circuit::Drive pulse_drive;
  pulse_drive.voltage = pluss::series::make(square).reshape({81, 1});
  const auto switching = circuit::tran(circuit::make(pulse), grid(2e-8, 81), pulse_drive);
  expect(switching.residual.amax().item<double>() < 1e-7 && switching.voltage.select(-1, 2).amax().item<double>() < 0.9, "switching a junction with stored charge");
}

int main() {
  circuit::Net divider;
  divider.nodes = 3;
  divider.resistors =
      branches({1, 2}, {0, 1}, pluss::series::make({1000.0, 1000.0}));
  divider.voltage_sources = branches({2}, {0}, pluss::series::make({10.0}));
  const auto compiled = circuit::make(divider);
  const auto operating = circuit::op(compiled);
  close(operating.voltage, {0.0, 5.0, 10.0});
  close(operating.source_current, {-0.005});
  expect(operating.residual.item<double>() < 1e-12, "DC residual is too large");

  circuit::Net sweep = divider;
  sweep.resistors.value = pluss::series::make(
      std::vector<double>{1000.0, 1000.0, 1000.0, 3000.0}, {2, 2});
  const auto swept = circuit::op(circuit::make(sweep));
  expect(swept.voltage.sizes() == std::vector<std::int64_t>{2, 3},
         "parameter batch shape was not retained");
  close(swept.voltage, {0.0, 5.0, 10.0, 0.0, 2.5, 10.0});

  circuit::Net current_driven;
  current_driven.nodes = 2;
  current_driven.resistors = branches({1}, {0}, pluss::series::make({10.0}));
  current_driven.current_sources =
      branches({1}, {0}, pluss::series::make({1.0}));
  const auto current_point = circuit::op(circuit::make(current_driven));
  close(current_point.voltage, {0.0, -10.0});
  expect(current_point.source_current.numel() == 0 &&
             current_point.inductor_current.numel() == 0,
         "branch-free MNA returned auxiliary currents");

  circuit::Net transconductance;
  transconductance.nodes = 3;
  transconductance.resistors = branches({2}, {0}, pluss::series::make({10.0}));
  transconductance.voltage_sources =
      branches({1}, {0}, pluss::series::make({2.0}));
  transconductance.vccs =
      controls({2}, {0}, {1}, {0}, pluss::series::make({0.5}));
  const auto gm_point = circuit::op(circuit::make(transconductance));
  close(gm_point.voltage, {0.0, 2.0, -10.0});
  close(gm_point.vccs_current, {1.0});

  circuit::Net voltage_gain;
  voltage_gain.nodes = 3;
  voltage_gain.resistors = branches({2}, {0}, pluss::series::make({10.0}));
  voltage_gain.voltage_sources = branches({1}, {0}, pluss::series::make({2.0}));
  voltage_gain.vcvs = controls({2}, {0}, {1}, {0}, pluss::series::make({3.0}));
  const auto gain_point = circuit::op(circuit::make(voltage_gain));
  close(gain_point.voltage, {0.0, 2.0, 6.0});
  close(gain_point.vcvs_current, {-0.6});

  voltage_gain.vcvs.gain =
      pluss::series::make(std::vector<double>{2.0, 3.0}, {2, 1});
  const auto gain_sweep = circuit::op(circuit::make(voltage_gain));
  expect(gain_sweep.voltage.sizes() == std::vector<std::int64_t>{2, 3},
         "controlled-source parameter batch shape is wrong");
  close(gain_sweep.voltage, {0.0, 2.0, 4.0, 0.0, 2.0, 6.0});

  circuit::Net non_inverting;
  non_inverting.nodes = 4;
  non_inverting.resistors =
      branches({2, 3}, {3, 0}, pluss::series::make({9000.0, 1000.0}));
  non_inverting.voltage_sources =
      branches({1}, {0}, pluss::series::make({1.0}));
  non_inverting.amps = amps({2}, {0}, {1}, {3});
  const auto amp_point = circuit::op(circuit::make(non_inverting));
  close(amp_point.voltage, {0.0, 1.0, 10.0, 1.0});
  close(amp_point.amp_current, {-0.001});

  circuit::Net lowpass;
  lowpass.nodes = 3;
  lowpass.resistors = branches({2}, {1}, pluss::series::make({1000.0}));
  lowpass.capacitors = branches({1}, {0}, pluss::series::make({1.0e-6}));
  lowpass.voltage_sources = branches({2}, {0}, pluss::series::make({1.0}));
  const auto response =
      circuit::ac(circuit::make(lowpass), pluss::series::make({0.0, 1000.0}));
  expect(response.voltage.sizes() == std::vector<std::int64_t>{2, 3},
         "AC frequency axis is wrong");
  close(response.voltage.select(-1, 1).abs(), {1.0, 1.0 / std::sqrt(2.0)},
        1e-8);

  lowpass.resistors.value =
      pluss::series::make(std::vector<double>{1000.0, 2000.0}, {2, 1});
  const auto ac_sweep =
      circuit::ac(circuit::make(lowpass), pluss::series::make({0.0, 1000.0}));
  expect(ac_sweep.voltage.sizes() == std::vector<std::int64_t>{2, 2, 3},
         "AC parameter/frequency batch shape is wrong");
  close(ac_sweep.voltage.select(-1, 1).abs(),
        {1.0, 1.0 / std::sqrt(2.0), 1.0, 1.0 / std::sqrt(5.0)}, 1e-8);

  voltage_gain.vcvs.gain = pluss::series::make({3.0});
  const auto gain_ac = circuit::ac(circuit::make(voltage_gain),
                                   pluss::series::make({0.0, 100.0}));
  close(gain_ac.voltage.select(-1, 2).abs(), {6.0, 6.0});

  auto tracked_source = pluss::series::make({10.0});
  tracked_source.tracks_slope_(true);
  divider.voltage_sources.value = tracked_source;
  const auto differentiable = circuit::op(circuit::make(divider));
  differentiable.voltage.select(-1, 1).reverse();
  close(tracked_source.sensitivity(), {0.5}, 1e-9);

  auto tracked_gain = pluss::series::make({3.0});
  tracked_gain.tracks_slope_(true);
  voltage_gain.vcvs.gain = tracked_gain;
  const auto differentiable_gain = circuit::op(circuit::make(voltage_gain));
  differentiable_gain.voltage.select(-1, 2).reverse();
  close(tracked_gain.sensitivity(), {2.0}, 1e-9);

  expect_error(
      [] {
        circuit::Net bad;
        bad.nodes = 2;
        bad.resistors = branches({1}, {2}, pluss::series::make({1.0}));
        (void)circuit::make(bad);
      },
      "out-of-range node was accepted");
  expect_error(
      [] {
        circuit::Net bad;
        bad.nodes = 3;
        bad.resistors = branches({1}, {0}, pluss::series::make({1.0}));
        bad.vccs = controls({2}, {0}, {1, 2}, {0}, pluss::series::make({1.0}));
        (void)circuit::make(bad);
      },
      "mismatched controlled-source terminals were accepted");
  expect_error(
      [] {
        circuit::Net bad;
        bad.nodes = 3;
        bad.resistors = branches({1}, {0}, pluss::series::make({1.0}));
        bad.amps = amps({2}, {0}, {1}, {1});
        (void)circuit::make(bad);
      },
      "degenerate op-amp input constraint was accepted");
  expect_error(
      [&] { (void)circuit::ac(compiled, pluss::series::make({-1.0})); },
      "negative angular frequency was accepted");

  std::cout << "Linear MNA circuit checks passed\n";
  transient_checks();
  device_checks();
  mosfet_checks();
  charge_and_body_checks();
  std::cout << "Transient circuit checks passed\n";
  std::cout << "Semiconductor device checks passed\n";

  // a floating node and two sources fixing different voltages are explained, not reported as a singular matrix
  {
    circuit::Net floating;
    floating.nodes = 3;
    floating.resistors = {{1}, {0}, pluss::series::make({1.0})};
    bool explained = false;
    try {
      (void)circuit::op(circuit::make(floating));
    } catch (const std::exception &error) {
      explained = std::string(error.what()).find("check for a node with no path to ground") != std::string::npos;
    }
    expect(explained, "a floating node was not explained");
    circuit::Net clash;
    clash.nodes = 3;
    clash.voltage_sources = {{1, 1}, {0, 0}, pluss::series::make({1.0, 2.0})};
    clash.resistors = {{1}, {2}, pluss::series::make({1.0})};
    bool clash_explained = false;
    try {
      (void)circuit::op(circuit::make(clash));
    } catch (const std::exception &error) {
      clash_explained = std::string(error.what()).find("two sources fixing the same voltage") != std::string::npos;
    }
    expect(clash_explained, "conflicting voltage sources were not explained");
  }
}
