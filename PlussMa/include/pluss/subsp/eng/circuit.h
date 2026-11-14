#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <memory>
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

// Junction diodes. Current flows from `positive` (anode) to `negative` (cathode): i = Is (exp(v / (n Vt)) - 1).
// `ideality` is the emission coefficient n; an undefined member means 1. `junction_capacitance` is the zero-bias depletion
// capacitance Cj0 [F]: constant when `junction_potential` is undefined, otherwise voltage dependent, Cj0 / (1 - v / Vj)^m
// with `grading` m (default 0.5), continued linearly above half the potential. `transit_time` [s] adds diffusion charge tau i.
struct Diodes {
  std::vector<std::int64_t> positive;
  std::vector<std::int64_t> negative;
  Series saturation{};
  Series ideality{};
  Series junction_capacitance{};
  Series junction_potential{};
  Series grading{};
  Series transit_time{};
};

// Bipolar transistors, Ebers-Moll transport model: two junctions and the two current gains, with no Early effect,
// charge storage or terminal resistance. `polarity` is +1 for NPN and -1 for PNP. An undefined `reverse_gain` means 1.
// `early_voltage` [V] adds the Early effect (transport current scaled by 1 + vce / VA); undefined means none.
struct Transistors {
  std::vector<std::int64_t> collector;
  std::vector<std::int64_t> base;
  std::vector<std::int64_t> emitter;
  std::vector<std::int64_t> polarity;
  Series saturation{};
  Series forward_gain{};
  Series reverse_gain{};
  Series early_voltage{};
  Series base_emitter_capacitance{};
  Series base_collector_capacitance{};
};

// Enhancement MOSFETs, level-1 square-law model; the body is tied to the source unless `bulk` is given, and then the threshold
// moves with the source-bulk voltage: Vth = Vth0 + gamma (sqrt(phi + vsb) - sqrt(phi)) with `body_effect` gamma [sqrt(V)] and
// `surface_potential` phi [V] (default 0.6). `polarity` is +1 for NMOS and -1 for PMOS;
// `transconductance` is mu Cox W / L [A/V^2], `threshold` the magnitude of the threshold voltage [V] and `channel_modulation`
// the optional lambda [1/V] (undefined means 0). The gate capacitances are constant and optional.
struct Mosfets {
  std::vector<std::int64_t> drain;
  std::vector<std::int64_t> gate;
  std::vector<std::int64_t> source;
  std::vector<std::int64_t> polarity;
  std::vector<std::int64_t> bulk;
  Series transconductance{};
  Series threshold{};
  Series channel_modulation{};
  Series body_effect{};
  Series surface_potential{};
  Series gate_source_capacitance{};
  Series gate_drain_capacitance{};
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
  Diodes diodes;
  Transistors transistors;
  Mosfets mosfets;
};

struct Result {
  Series voltage;
  Series source_current;
  Series inductor_current;
  Series vccs_current;
  Series vcvs_current;
  Series amp_current;
  Series diode_current;
  Series collector_current;
  Series base_current;
  Series drain_current;
  Series iterations;
  Series refinements;
  Series residual;
};

// Device capacitances (`junction_capacitance`, the transistor and MOSFET capacitances) are linear capacitors appended to the
// net's capacitors in the order diodes, transistors, MOSFETs; `Start::capacitor_voltage` follows that order.
// Newton iteration for circuits with diodes or transistors. Junction voltages are limited between iterations so the
// exponentials cannot overflow; `gmin` is a conductance placed across every junction. A solve that fails to converge is
// retried by gmin stepping and, for operating points, by source stepping. A device current above `max_current` is an
// error: nothing in the circuit limits it. In `tran` a time step whose solution changes by more than `step_tolerance`
// (relative) when taken as two half steps is subdivided, down to `max_refinement` halvings.
struct SolveOptions {
  double thermal_voltage{0.02585};
  double tolerance{1e-10};
  std::int64_t max_iterations{100};
  double gmin{1e-18};
  double max_current{1e6};
  double step_tolerance{1e-3};
  std::int64_t max_refinement{8};
};

// Time-stepping rule of `tran`.
enum class Scheme { Trapezoid, Gear2 };

// Time-varying source values, [..., time, elements] with the time axis matching the `tran` time vector. An undefined
// member holds the value stored in the circuit constant over the run.
struct Drive {
  Series voltage{};
  Series current{};
};

// Energy-storage state at the first time sample, [..., elements]. An undefined member starts at rest (zero).
struct Start {
  Series capacitor_voltage{};
  Series inductor_current{};
};

struct TranOptions {
  Scheme scheme{Scheme::Trapezoid};
  SolveOptions solver{};
};

struct Devices;

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
  std::int64_t diodes() const noexcept { return diode_count_; }
  std::int64_t transistors() const noexcept { return transistor_count_; }
  std::int64_t mosfets() const noexcept { return mosfet_count_; }
  ScalarType dtype() const noexcept { return dtype_; }
  Platform platform() const noexcept { return resistance_.platform(); }

private:
  friend struct Stamps;
  friend Circuit make(const Net &);
  friend Result op(const Circuit &, const SolveOptions &);
  friend Result ac(const Circuit &, const Series &);
  friend Result ac(const Circuit &, const Series &, const Result &, const SolveOptions &);
  friend Result tran(const Circuit &, const Series &, const Drive &,
                     const Start &, const TranOptions &);

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
          std::int64_t vcvs_count, std::int64_t amp_count,
          std::shared_ptr<const Devices> devices, std::int64_t diode_count, std::int64_t transistor_count,
          std::int64_t mosfet_count)
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
        amp_count_(amp_count), devices_(std::move(devices)), diode_count_(diode_count),
        transistor_count_(transistor_count), mosfet_count_(mosfet_count) {}

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
  std::shared_ptr<const Devices> devices_;
  std::int64_t diode_count_{};
  std::int64_t transistor_count_{};
  std::int64_t mosfet_count_{};
};

// Validates topology and compiles incidence matrices once. All non-empty
// value groups must share dtype/platform and compatible leading batch axes.
Circuit make(const Net &net);

// DC operating point. Capacitors are open and inductors are ideal shorts represented by MNA branch-current
// unknowns. Circuits with diodes or transistors are solved by Newton iteration.
Result op(const Circuit &circuit, const SolveOptions &options = {});

// Small-signal phasor response at non-negative angular frequencies [rad/s].
// The frequency axis is inserted immediately before the final node/branch axis.
Result ac(const Circuit &circuit, const Series &omega);

// Small-signal response of a circuit with diodes or transistors, linearised about `operating_point` (the result of
// `op` on the biased circuit). The stored source values are the small-signal excitation.
Result ac(const Circuit &circuit, const Series &omega, const Result &operating_point, const SolveOptions &options = {});

// Transient response on a uniform time grid [s]. Every Result member gains a time axis immediately before its final
// element axis; `residual` is the defect of the implicit step equation at each sample.
Result tran(const Circuit &circuit, const Series &time, const Drive &drive = {},
            const Start &start = {}, const TranOptions &options = {});

} // namespace pluss::subsp::eng::circuit
