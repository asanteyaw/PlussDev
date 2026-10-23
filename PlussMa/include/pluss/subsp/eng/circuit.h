#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::circuit {

// Values use the final axis for elements. Leading axes are parameter-sweep
// batches. Node zero is always the reference node.
struct Branches {
  std::vector<std::int64_t> positive;
  std::vector<std::int64_t> negative;
  Series value{};
};

// Four-terminal voltage-controlled elements. Current and voltage orientation
// follows positive -> negative; control voltage is control_positive minus
// control_negative.
struct Controls {
  std::vector<std::int64_t> positive;
  std::vector<std::int64_t> negative;
  std::vector<std::int64_t> control_positive;
  std::vector<std::int64_t> control_negative;
  Series gain{};
};

// Infinite-gain ideal op amps. Each output/reference pair supplies the current
// needed to enforce positive_input == negative_input.
struct Amps {
  std::vector<std::int64_t> output;
  std::vector<std::int64_t> reference;
  std::vector<std::int64_t> positive_input;
  std::vector<std::int64_t> negative_input;
};

struct Net {
  std::int64_t nodes{};
  Branches resistors;
  Branches capacitors;
  Branches inductors;
  Branches current_sources;
  Branches voltage_sources;
  Controls vccs;
  Controls vcvs;
  Amps amps;
};

struct Result {
  Series voltage;
  Series source_current;
  Series inductor_current;
  Series vccs_current;
  Series vcvs_current;
  Series amp_current;
  Series residual;
};

class Circuit final {
public:
  std::int64_t nodes() const noexcept { return nodes_; }
  std::int64_t unknowns() const noexcept {
    return nodes_ - 1 + voltage_count_ + inductor_count_ + vcvs_count_ +
           amp_count_;
  }
  const std::vector<std::int64_t> &batch_shape() const noexcept {
    return batch_shape_;
  }
  ScalarType dtype() const noexcept { return dtype_; }
  Platform platform() const noexcept { return resistance_.platform(); }

private:
  friend Circuit make(const Net &);
  friend Result op(const Circuit &);
  friend Result ac(const Circuit &, const Series &);

  Circuit(std::int64_t nodes, std::vector<std::int64_t> batch_shape,
          ScalarType dtype, Series resistor_incidence, Series resistance,
          Series capacitor_incidence, Series capacitance,
          Series inductor_incidence, Series inductance,
          Series current_incidence, Series current, Series voltage_incidence,
          Series voltage, Series vccs_output_incidence,
          Series vccs_control_incidence, Series vccs_gain,
          Series vcvs_output_incidence, Series vcvs_control_incidence,
          Series vcvs_gain, Series amp_output_incidence,
          Series amp_input_incidence, std::int64_t voltage_count,
          std::int64_t inductor_count, std::int64_t vccs_count,
          std::int64_t vcvs_count, std::int64_t amp_count)
      : nodes_(nodes), batch_shape_(std::move(batch_shape)), dtype_(dtype),
        resistor_incidence_(std::move(resistor_incidence)),
        resistance_(std::move(resistance)),
        capacitor_incidence_(std::move(capacitor_incidence)),
        capacitance_(std::move(capacitance)),
        inductor_incidence_(std::move(inductor_incidence)),
        inductance_(std::move(inductance)),
        current_incidence_(std::move(current_incidence)),
        current_(std::move(current)),
        voltage_incidence_(std::move(voltage_incidence)),
        voltage_(std::move(voltage)),
        vccs_output_incidence_(std::move(vccs_output_incidence)),
        vccs_control_incidence_(std::move(vccs_control_incidence)),
        vccs_gain_(std::move(vccs_gain)),
        vcvs_output_incidence_(std::move(vcvs_output_incidence)),
        vcvs_control_incidence_(std::move(vcvs_control_incidence)),
        vcvs_gain_(std::move(vcvs_gain)),
        amp_output_incidence_(std::move(amp_output_incidence)),
        amp_input_incidence_(std::move(amp_input_incidence)),
        voltage_count_(voltage_count), inductor_count_(inductor_count),
        vccs_count_(vccs_count), vcvs_count_(vcvs_count),
        amp_count_(amp_count) {}

  std::int64_t nodes_{};
  std::vector<std::int64_t> batch_shape_;
  ScalarType dtype_{};
  Series resistor_incidence_;
  Series resistance_;
  Series capacitor_incidence_;
  Series capacitance_;
  Series inductor_incidence_;
  Series inductance_;
  Series current_incidence_;
  Series current_;
  Series voltage_incidence_;
  Series voltage_;
  Series vccs_output_incidence_;
  Series vccs_control_incidence_;
  Series vccs_gain_;
  Series vcvs_output_incidence_;
  Series vcvs_control_incidence_;
  Series vcvs_gain_;
  Series amp_output_incidence_;
  Series amp_input_incidence_;
  std::int64_t voltage_count_{};
  std::int64_t inductor_count_{};
  std::int64_t vccs_count_{};
  std::int64_t vcvs_count_{};
  std::int64_t amp_count_{};
};

// Validates topology and compiles incidence matrices once. All non-empty
// value groups must share dtype/platform and compatible leading batch axes.
Circuit make(const Net &net);

// Linear DC operating point. Capacitors are open and inductors are ideal
// shorts represented by MNA branch-current unknowns.
Result op(const Circuit &circuit);

// Small-signal phasor response at non-negative angular frequencies [rad/s].
// The frequency axis is inserted immediately before the final node/branch axis.
Result ac(const Circuit &circuit, const Series &omega);

} // namespace pluss::subsp::eng::circuit
