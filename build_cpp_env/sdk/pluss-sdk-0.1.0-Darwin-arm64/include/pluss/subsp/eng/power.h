#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/math/qp.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::power {

namespace detail {
struct AcKernel;
}

enum class BusKind { PQ, PV, Slack };

enum class MeasureKind { FromP, ToP, BusP, Angle, FromQ, ToQ, BusQ, Magnitude };

enum class TraceStop { Target, Nose };

struct Base {
  double power{};   // three-phase apparent-power base [VA]
  double voltage{}; // line-to-line voltage base [V]
};

struct Buses {
  std::vector<BusKind> kind;
  Series p{};  // fixed active injection; loads are negative [p.u.]
  Series q{};  // fixed reactive injection; loads are negative [p.u.]
  Series vm{}; // initial/fixed voltage magnitude [p.u.]
  Series va{}; // initial/fixed voltage angle [rad]
  Series g{};  // shunt conductance [p.u. at vm = 1]
  Series b{};  // shunt susceptance [p.u. at vm = 1]
};

struct Gens {
  std::vector<std::int64_t> bus;
  Series p{}; // active generation [p.u.]
  Series q{}; // reactive generation or initial set point [p.u.]
  std::vector<bool> active;
};

struct Lines {
  std::vector<std::int64_t> from;
  std::vector<std::int64_t> to;
  Series r{};      // series resistance [p.u.]
  Series x{};      // series reactance [p.u.]
  Series charge{}; // total charging susceptance [p.u.]
  Series tap{};    // off-nominal magnitude ratio
  Series shift{};  // phase shift [rad]
  std::vector<bool> active;
};

struct Net {
  Base base;
  Buses buses;
  Gens gens;
  Lines lines;
};

struct Options {
  double tolerance{1e-10};
  std::int64_t max_iterations{20};
};

struct DcResult {
  Series angle;
  Series injection;
  Series from_power;
  Series to_power;
  Series residual;
};

struct AcResult {
  Series voltage;
  Series magnitude;
  Series angle;
  Series injection;
  Series from_power;
  Series to_power;
  Series residual;
  Series converged;
  std::int64_t iterations{};
};

struct LodfResult {
  Series factor; // [..., monitored line, outage line]
  Series valid;  // [..., outage line]
};

struct OutageResult {
  Series base_flow; // [..., line]
  Series flow;      // [..., outage line, monitored line]
  Series valid;     // [..., outage line]
};

struct GenCost {
  Series lower;       // active-power lower bound [p.u.]
  Series upper;       // active-power upper bound [p.u.]
  Series linear;      // coefficient of p [cost / p.u.]
  Series quadratic;   // coefficient of p^2 [cost / p.u.^2]
  Series constant;    // optional constant active-power cost
  Series q_linear;    // optional coefficient of q [cost / p.u.]
  Series q_quadratic; // optional coefficient of q^2 [cost / p.u.^2]
  Series q_constant;  // optional constant reactive-power cost
};

struct DispatchOptions {
  bool secure{false};
  double outage_tolerance{1e-10};
  pluss::subsp::math::qp::Options qp{};
};

struct DispatchResult {
  Series generation;   // [..., generator]
  Series angle;        // [..., bus]
  Series injection;    // [..., bus]
  Series flow;         // [..., line]
  Series post_flow;    // [..., outage line, monitored line], undefined if off
  Series outage_valid; // [..., outage line], undefined if security is off
  Series objective;
  bool converged{};
  std::int64_t iterations{};
  double primal_residual{};
  double dual_residual{};
};

struct Limits {
  Series q_lower;  // generator reactive-power lower bound [p.u.]
  Series q_upper;  // generator reactive-power upper bound [p.u.]
  Series vm_lower; // bus voltage-magnitude lower bound [p.u.]
  Series vm_upper; // bus voltage-magnitude upper bound [p.u.]
  // Optional branch apparent-power rating [p.u.]; undefined disables it.
  Series flow;
};

struct OpfOptions {
  std::int64_t max_iterations{160};
  double constraint_tolerance{1e-8};
  double optimality_tolerance{1e-8};
};

struct OpfResult {
  Series generation_p; // [..., generator]
  Series generation_q; // [..., generator]
  Series voltage;      // [..., bus], complex p.u.
  Series magnitude;    // [..., bus]
  Series angle;        // [..., bus], radians
  Series injection;    // [..., bus], complex p.u.
  Series from_power;   // [..., line], complex p.u.
  Series to_power;     // [..., line], complex p.u.
  Series objective;    // [...], one cost per scenario
  Series p_price;      // [..., bus], active-balance multiplier
  Series q_price;      // [..., bus], reactive-balance multiplier
  Series from_shadow;  // [..., line], from-end rating multiplier
  Series to_shadow;    // [..., line], to-end rating multiplier
  bool converged{};
  std::int64_t iterations{};
  double constraint_violation{};
  double first_order_optimality{};
  std::string message;
};

struct Measurements {
  std::vector<MeasureKind> kind;
  std::vector<std::int64_t> index;
  Series value;  // [..., measurement], angles in radians and powers in p.u.
  Series stddev; // [..., measurement], strictly positive
};

struct EstimateOptions {
  double tolerance{1e-8};
  double observability_tolerance{1e-10};
  std::int64_t max_iterations{20};
};

struct EstimateResult {
  Series voltage;             // [..., bus], complex
  Series magnitude;           // [..., bus]
  Series angle;               // [..., bus], radians
  Series injection;           // [..., bus], complex p.u.
  Series from_power;          // [..., line], complex p.u.
  Series to_power;            // [..., line], complex p.u.
  Series predicted;           // [..., measurement]
  Series residual;            // value - predicted
  Series normalized_residual; // residual / residual standard deviation
  Series objective;           // sum((residual / stddev)^2)
  Series covariance;          // [..., state, state]: Va(non-reference), then Vm
  Series converged;           // [...] bool
  std::int64_t iterations{};
};

struct LoadPath {
  Series p; // target net active bus injection at loading factor 1 [p.u.]
  Series q; // target net reactive bus injection at loading factor 1 [p.u.]
};

struct TraceOptions {
  TraceStop stop{TraceStop::Target};
  double target{1.0};
  double step{0.05};
  double step_min{1e-4};
  double step_max{0.2};
  double adapt_tolerance{1e-3};
  double adapt_damping{0.7};
  double tolerance{1e-9};
  std::int64_t max_steps{400};
  std::int64_t max_iterations{12};
  bool adaptive{true};
};

struct TraceResult {
  Series voltage;    // [..., point, bus], complex
  Series magnitude;  // [..., point, bus]
  Series angle;      // [..., point, bus], radians
  Series injection;  // [..., point, bus], complex p.u.
  Series from_power; // [..., point, line], complex p.u.
  Series to_power;   // [..., point, line], complex p.u.
  Series factor;     // [..., point], loading factor
  Series tangent;    // [..., point, state + 1]
  Series residual;   // [..., point]
  Series valid;      // [..., point] bool; false after a scenario has stopped
  Series reached;    // [...] bool; requested target or nose was reached
  std::int64_t points{};
};

class Grid final {
public:
  std::int64_t buses() const noexcept { return buses_; }
  std::int64_t gens() const noexcept { return gens_; }
  std::int64_t lines() const noexcept { return lines_; }
  const std::vector<std::int64_t> &batch_shape() const noexcept {
    return batch_shape_;
  }
  double base_power() const noexcept { return base_.power; }
  const Base &base() const noexcept { return base_; }
  ScalarType dtype() const noexcept { return dtype_; }
  Platform platform() const noexcept { return p_.platform(); }
  const Series &ybus() const noexcept { return ybus_; }
  const Series &yfrom() const noexcept { return yfrom_; }
  const Series &yto() const noexcept { return yto_; }

private:
  friend struct detail::AcKernel;
  friend Grid make(const Net &);
  friend DcResult dc(const Grid &);
  friend AcResult ac(const Grid &, const Options &);
  friend Series ptdf(const Grid &);
  friend LodfResult lodf(const Grid &, double);
  friend OutageResult outages(const Grid &, double);
  friend DispatchResult dispatch(const Grid &, const GenCost &, const Series &,
                                 const DispatchOptions &);
  friend OpfResult opf(const Grid &, const GenCost &, const Limits &,
                       const OpfOptions &);
  friend EstimateResult estimate(const Grid &, const Measurements &,
                                 const EstimateOptions &);
  friend TraceResult trace(const Grid &, const LoadPath &,
                           const TraceOptions &);

  DcResult solve_dc(const Series &specified) const;

  Grid(Base base, std::int64_t buses, std::int64_t gens, std::int64_t lines,
       std::vector<std::int64_t> batch_shape, ScalarType dtype, Series p,
       Series q, Series vm, Series va, Series ybus, Series yfrom, Series yto,
       Series from_map, Series to_map, Series angle_map, Series pq_map,
       Series fixed_p, Series fixed_q, Series gen_map, Series gen_active,
       Series gen_p, Series gen_q, Series line_active, Series bbus,
       Series bfrom, Series bus_shift, Series from_shift)
      : base_(base), buses_(buses), gens_(gens), lines_(lines),
        batch_shape_(std::move(batch_shape)), dtype_(dtype), p_(std::move(p)),
        q_(std::move(q)), vm_(std::move(vm)), va_(std::move(va)),
        ybus_(std::move(ybus)), yfrom_(std::move(yfrom)), yto_(std::move(yto)),
        from_map_(std::move(from_map)), to_map_(std::move(to_map)),
        angle_map_(std::move(angle_map)), pq_map_(std::move(pq_map)),
        fixed_p_(std::move(fixed_p)), fixed_q_(std::move(fixed_q)),
        gen_map_(std::move(gen_map)), gen_active_(std::move(gen_active)),
        gen_p_(std::move(gen_p)), gen_q_(std::move(gen_q)),
        line_active_(std::move(line_active)), bbus_(std::move(bbus)),
        bfrom_(std::move(bfrom)), bus_shift_(std::move(bus_shift)),
        from_shift_(std::move(from_shift)) {}

  Base base_;
  std::int64_t buses_{};
  std::int64_t gens_{};
  std::int64_t lines_{};
  std::vector<std::int64_t> batch_shape_;
  ScalarType dtype_{};
  Series p_;
  Series q_;
  Series vm_;
  Series va_;
  Series ybus_;
  Series yfrom_;
  Series yto_;
  Series from_map_;
  Series to_map_;
  Series angle_map_;
  Series pq_map_;
  Series fixed_p_;
  Series fixed_q_;
  Series gen_map_;
  Series gen_active_;
  Series gen_p_;
  Series gen_q_;
  Series line_active_;
  Series bbus_;
  Series bfrom_;
  Series bus_shift_;
  Series from_shift_;
};

// Three-phase bases use line-to-line voltage and total apparent power.
double zbase(const Base &base);
double ybase(const Base &base);
double ibase(const Base &base);
Series pu(const Series &value, double base);
Series si(const Series &value, double base);

Grid make(const Net &net);
DcResult dc(const Grid &grid);
AcResult ac(const Grid &grid, const Options &options = {});
Series ptdf(const Grid &grid);
LodfResult lodf(const Grid &grid, double tolerance = 1e-10);
OutageResult outages(const Grid &grid, double tolerance = 1e-10);
DispatchResult dispatch(const Grid &grid, const GenCost &cost,
                        const Series &line_limit,
                        const DispatchOptions &options = {});
OpfResult opf(const Grid &grid, const GenCost &cost, const Limits &limits,
              const OpfOptions &options = {});
EstimateResult estimate(const Grid &grid, const Measurements &measurements,
                        const EstimateOptions &options = {});
TraceResult trace(const Grid &grid, const LoadPath &path,
                  const TraceOptions &options = {});

} // namespace pluss::subsp::eng::power
