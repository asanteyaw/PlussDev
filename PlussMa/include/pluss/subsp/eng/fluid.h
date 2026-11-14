#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::fluid {

namespace detail {
struct TransportEngine;
}

enum class NodeKind { Junction, Reservoir, Tank };

enum class LossModel { DarcyWeisbach, HazenWilliams };

enum class ValveKind { Throttle, Flow, Reduce, Sustain };

enum class DemandMode { Fixed, Pressure };

enum class Device { Pump, Valve };

enum class Sense { Below, Above };

enum class Act { Open, Close, Set };

enum class QualityMode { Constituent, Age };

struct Nodes {
  std::vector<NodeKind> kind;
  Series elevation; // [..., node], m
  Series demand;    // [..., node], m^3/s; positive leaves a junction
  Series head;      // [..., node], m; fixed at reservoirs, initial elsewhere
  Series emitter;   // optional [..., node], (m^3/s) / m^exponent
};

struct Tanks {
  Series area;      // [..., tank], m^2
  Series min_level; // [..., tank], m above bottom elevation
  Series max_level; // [..., tank], m above bottom elevation
  std::vector<bool> overflow;
};

struct Pipes {
  std::vector<std::int64_t> from;
  std::vector<std::int64_t> to;
  Series length;    // [..., pipe], m
  Series diameter;  // [..., pipe], m
  Series roughness; // [..., pipe], m for Darcy; C for Hazen--Williams
  Series minor;     // optional [..., pipe], dimensionless K
};

struct Pumps {
  std::vector<std::int64_t> from;
  std::vector<std::int64_t> to;
  Series shutoff_head; // [..., pump], m at unit speed
  Series resistance;   // [..., pump], m/(m^3/s)^exponent
  Series exponent;     // [..., pump], positive curve exponent
  Series speed;        // optional [..., pump], relative speed; defaults to 1
};

struct Valves {
  std::vector<ValveKind> kind;
  std::vector<std::int64_t> from;
  std::vector<std::int64_t> to;
  Series diameter; // [..., valve], m
  Series minor;    // [..., valve], dimensionless K when open/throttled
  Series setting;  // flow (m^3/s) or pressure head (m); unused by Throttle
};

struct Net {
  Nodes nodes;
  Tanks tanks;
  Pipes pipes;
  Pumps pumps;
  Valves valves;
};

struct Liquid {
  double density{998.2};                 // kg/m^3
  double kinematic_viscosity{1.0038e-6}; // m^2/s
  double gravity{9.80665};               // m/s^2
};

struct Options {
  LossModel loss{LossModel::DarcyWeisbach};
  DemandMode demand_mode{DemandMode::Fixed};
  double min_pressure{0.0};
  double full_pressure{15.0};
  double demand_exponent{0.5};
  double emitter_exponent{0.5};
  double head_tolerance{1e-9};
  double flow_tolerance{1e-11};
  double min_gradient{1e-9};
  std::int64_t max_iterations{40};
  bool fail_on_nonconvergence{true};
};

struct SteadyResult {
  Series head;            // [..., node], m
  Series pressure;        // [..., node], Pa gauge
  Series flow;            // [..., link], m^3/s; pipes, then pumps, then valves
  Series velocity;        // [..., link], m/s; zero for zero-length pumps
  Series head_loss;       // [..., link], m; negative across a gaining pump
  Series friction_factor; // [..., link], Darcy factor; zero for non-Darcy links
  Series balance;         // [..., node], m^3/s; inflow - outflow - demand
  Series energy_residual; // [..., link], equation residual
  Series converged;       // [...] bool
  Series demand;          // [..., node], delivered ordinary demand
  Series emitter;         // [..., node], pressure-driven emitter outflow
  std::int64_t iterations{};
};

struct Action {
  Device device{Device::Pump};
  std::int64_t index{}; // local pump or valve index
  Act act{Act::Open};
  double value{}; // relative pump speed or valve setting for Set
};

struct TimeControl {
  double at{}; // seconds from the start of the run
  Action action;
};

struct StateControl {
  std::int64_t node{};
  Sense sense{Sense::Below};
  double threshold{}; // tank level for tanks; pressure head otherwise, m
  Action action;
};

struct Schedule {
  double duration{};   // seconds
  double step{3600.0}; // maximum hydraulic step, seconds
  double pattern_step{3600.0};
  Series demand; // optional [period, node] demand multipliers, cyclic
  Series pump;   // optional [period, pump] speed multipliers, cyclic
  Series valve;  // optional [period, valve] absolute settings, cyclic
  std::vector<TimeControl> time;
  std::vector<StateControl> state;
};

struct RunResult {
  Series time;            // [sample], seconds
  Series head;            // [sample, ..., node], m
  Series pressure;        // [sample, ..., node], Pa gauge
  Series flow;            // [sample, ..., link], m^3/s
  Series velocity;        // [sample, ..., link], m/s
  Series head_loss;       // [sample, ..., link], m
  Series friction_factor; // [sample, ..., link]
  Series balance;         // [sample, ..., node], m^3/s
  Series energy_residual; // [sample, ..., link]
  Series demand;          // [sample, ..., node], delivered demand
  Series emitter;         // [sample, ..., node], emitter outflow
  Series tank_level;      // [sample, ..., tank], m above bottom
  Series spill;           // [sample, ..., tank], overflow discharge, m^3/s
  Series open;            // [sample, ..., link], bool
  Series converged;       // [sample, ...], bool
  Series iterations;      // [sample], Int64
};

struct Quality {
  QualityMode mode{QualityMode::Constituent};
  Series initial;   // optional for Age; [..., node]
  Series source;    // optional boundary/injection concentration [..., node]
  Series pipe_rate; // optional first-order rate [..., pipe], 1/s
  Series tank_rate; // optional first-order rate [..., tank], 1/s
};

struct RouteOptions {
  double step{300.0};
  double courant{0.9};
  double flow_tolerance{1e-12};
  std::int64_t cells{16};
};

struct TransportResult {
  RunResult hydraulic;
  Series node;         // [sample, ..., node]
  Series link;         // [sample, ..., link], volume-average for pipes
  Series tank;         // [sample, ..., tank]
  Series mass;         // [sample, ...], stored concentration-volume
  Series mass_error;   // [sample, ...], cumulative conservation residual
};

struct Heat {
  Series initial;      // [..., node], K
  Series source;       // optional fixed reservoir/injection temperature, K
  Series ambient;      // optional [..., pipe], K; required where ua > 0
  Series ua;           // optional [..., pipe], W/K; defaults to zero
  Series tank_ambient; // optional [..., tank], K; required where tank_ua > 0
  Series tank_ua;      // optional [..., tank], W/K; defaults to zero
  double cp{4180.0};   // J/(kg K), constant over this call
};

struct HeatResult {
  RunResult hydraulic;
  Series node;         // [sample, ..., node], K
  Series link;         // [sample, ..., link], K
  Series tank;         // [sample, ..., tank], K
  Series energy;       // [sample, ...], stored sensible energy, J
  Series energy_error; // [sample, ...], cumulative energy residual, J
};

class Network final {
public:
  [[nodiscard]] std::int64_t nodes() const noexcept { return nodes_; }
  [[nodiscard]] std::int64_t pipes() const noexcept { return pipes_; }
  [[nodiscard]] std::int64_t pumps() const noexcept { return pumps_; }
  [[nodiscard]] std::int64_t valves() const noexcept { return valves_; }
  [[nodiscard]] std::int64_t links() const noexcept {
    return pipes_ + pumps_ + valves_;
  }
  [[nodiscard]] std::int64_t junctions() const noexcept { return junctions_; }
  [[nodiscard]] std::int64_t reservoirs() const noexcept { return reservoirs_; }
  [[nodiscard]] std::int64_t tanks() const noexcept { return tanks_; }
  [[nodiscard]] const std::vector<std::int64_t> &batch_shape() const noexcept {
    return batch_shape_;
  }
  [[nodiscard]] ScalarType dtype() const noexcept { return dtype_; }
  [[nodiscard]] Platform platform() const noexcept { return head_.platform(); }
  [[nodiscard]] const Liquid &liquid() const noexcept { return liquid_; }

private:
  friend struct detail::TransportEngine;
  friend Network network(const Net &, const Liquid &);
  friend SteadyResult steady(const Network &, const Options &);
  friend RunResult run(const Network &, const Schedule &, const Options &);

  Network(std::int64_t node_count, std::int64_t pipe_count,
          std::int64_t pump_count, std::int64_t valve_count,
          std::int64_t junction_count, std::int64_t reservoir_count,
          std::int64_t tank_count, std::vector<NodeKind> node_kind,
          std::vector<std::int64_t> tank_of_node,
          std::vector<ValveKind> valve_kind,
          std::vector<std::int64_t> batch_shape, ScalarType dtype,
          Liquid liquid, Series elevation, Series demand, Series head,
          Series emitter, Series tank_area, Series tank_min, Series tank_max,
          Series tank_overflow, Series tank_pick, Series tank_put,
          Series length, Series diameter, Series roughness, Series minor,
          Series incidence, Series energy, Series junction_pick,
          Series junction_put, Series fixed_put, Series pump_mask,
          Series valve_mask, Series pump_head, Series pump_resistance,
          Series pump_exponent, Series pump_speed, Series valve_diameter,
          Series valve_minor, Series valve_setting, Series normal_head_map,
          Series control_head_map, Series flow_valve_mask,
          Series pressure_valve_mask, Series closed_mask)
      : nodes_(node_count), pipes_(pipe_count), junctions_(junction_count),
        reservoirs_(reservoir_count), tanks_(tank_count), pumps_(pump_count),
        valves_(valve_count), node_kind_(std::move(node_kind)),
        tank_of_node_(std::move(tank_of_node)),
        valve_kind_(std::move(valve_kind)),
        batch_shape_(std::move(batch_shape)), dtype_(dtype), liquid_(liquid),
        elevation_(std::move(elevation)), demand_(std::move(demand)),
        head_(std::move(head)), emitter_(std::move(emitter)),
        tank_area_(std::move(tank_area)), tank_min_(std::move(tank_min)),
        tank_max_(std::move(tank_max)),
        tank_overflow_(std::move(tank_overflow)),
        tank_pick_(std::move(tank_pick)), tank_put_(std::move(tank_put)),
        length_(std::move(length)), diameter_(std::move(diameter)),
        roughness_(std::move(roughness)), minor_(std::move(minor)),
        incidence_(std::move(incidence)), energy_(std::move(energy)),
        junction_pick_(std::move(junction_pick)),
        junction_put_(std::move(junction_put)),
        fixed_put_(std::move(fixed_put)), pump_mask_(std::move(pump_mask)),
        valve_mask_(std::move(valve_mask)), pump_head_(std::move(pump_head)),
        pump_resistance_(std::move(pump_resistance)),
        pump_exponent_(std::move(pump_exponent)),
        pump_speed_(std::move(pump_speed)),
        valve_diameter_(std::move(valve_diameter)),
        valve_minor_(std::move(valve_minor)),
        valve_setting_(std::move(valve_setting)),
        normal_head_map_(std::move(normal_head_map)),
        control_head_map_(std::move(control_head_map)),
        flow_valve_mask_(std::move(flow_valve_mask)),
        pressure_valve_mask_(std::move(pressure_valve_mask)),
        closed_mask_(std::move(closed_mask)) {}

  std::int64_t nodes_{};
  std::int64_t pipes_{};
  std::int64_t junctions_{};
  std::int64_t reservoirs_{};
  std::int64_t tanks_{};
  std::int64_t pumps_{};
  std::int64_t valves_{};
  std::vector<NodeKind> node_kind_;
  std::vector<std::int64_t> tank_of_node_;
  std::vector<ValveKind> valve_kind_;
  std::vector<std::int64_t> batch_shape_;
  ScalarType dtype_{ScalarType::Float64};
  Liquid liquid_{};
  Series elevation_;
  Series demand_;
  Series head_;
  Series emitter_;
  Series tank_area_;     // [..., tank]
  Series tank_min_;      // [..., tank]
  Series tank_max_;      // [..., tank]
  Series tank_overflow_; // [tank]
  Series tank_pick_;     // [tank, node]
  Series tank_put_;      // [node, tank]
  Series length_;
  Series diameter_;
  Series roughness_;
  Series minor_;
  Series incidence_;     // [node, pipe]: -1 at from, +1 at to
  Series energy_;        // [pipe, node]: +1 at from, -1 at to
  Series junction_pick_; // [junction, node]
  Series junction_put_;  // [node, junction]
  Series fixed_put_;     // [node, node], diagonal reservoir/tank mask
  Series pump_mask_;     // [link]
  Series valve_mask_;    // [link]
  Series pump_head_;     // [..., link]
  Series pump_resistance_;
  Series pump_exponent_;
  Series pump_speed_;
  Series valve_diameter_;
  Series valve_minor_;
  Series valve_setting_;
  Series normal_head_map_;     // [link, node]
  Series control_head_map_;    // [link, node]
  Series flow_valve_mask_;     // [link]
  Series pressure_valve_mask_; // [link]
  Series closed_mask_;         // [..., link]
  Series flow_guess_;          // optional [..., link], internal warm start
};

[[nodiscard]] Network network(const Net &net, const Liquid &liquid = {});

[[nodiscard]] SteadyResult steady(const Network &network,
                                  const Options &options = {});

[[nodiscard]] RunResult run(const Network &network, const Schedule &schedule,
                            const Options &options = {});

[[nodiscard]] TransportResult
transport(const Network &network, const Schedule &schedule,
          const Quality &quality, const RouteOptions &route_options = {},
          const Options &hydraulic_options = {});

[[nodiscard]] HeatResult
heat(const Network &network, const Schedule &schedule, const Heat &thermal,
     const RouteOptions &route_options = {},
     const Options &hydraulic_options = {});

} // namespace pluss::subsp::eng::fluid
