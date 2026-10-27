#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
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
  } catch (const std::invalid_argument &) {
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
}
