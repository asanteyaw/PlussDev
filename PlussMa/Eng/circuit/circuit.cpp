#include "pluss/subsp/eng/circuit.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/eng/lti.h"
#include "../detail/series_util.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace pluss::subsp::eng::circuit {
namespace {

using Shape = std::vector<std::int64_t>;

std::int64_t count(const Branches &branches) {
  return static_cast<std::int64_t>(branches.positive.size());
}

std::int64_t count(const Controls &controls) {
  return static_cast<std::int64_t>(controls.positive.size());
}

std::int64_t count(const Amps &amps) {
  return static_cast<std::int64_t>(amps.output.size());
}

Shape leading_shape(const Series &value) {
  Shape shape(value.sizes().begin(), value.sizes().end());
  shape.pop_back();
  return shape;
}

Config config(ScalarType dtype, Platform platform) {
  return Config{}.with_dtype(dtype).with_platform(platform);
}

void validate_endpoints(const std::vector<std::int64_t> &positive,
                        const std::vector<std::int64_t> &negative,
                        std::int64_t nodes, const std::string &role,
                        bool distinct) {
  if (positive.size() != negative.size())
    PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                " endpoint arrays must have equal length");
  for (std::size_t i = 0; i < positive.size(); ++i) {
    const auto p = positive[i];
    const auto n = negative[i];
    if (p < 0 || p >= nodes || n < 0 || n >= nodes)
      PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                  " contains a node outside [0, nodes)");
    if (distinct && p == n)
      PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                  " cannot connect a node to itself");
  }
}

void validate_nodes(const Branches &branches, std::int64_t nodes,
                    const char *role) {
  validate_endpoints(branches.positive, branches.negative, nodes, role, true);
}

void validate_nodes(const Controls &controls, std::int64_t nodes,
                    const char *role) {
  validate_endpoints(controls.positive, controls.negative, nodes, role, true);
  if (controls.control_positive.size() != controls.positive.size() ||
      controls.control_negative.size() != controls.positive.size())
    PLUSS_FAIL(
        std::string("eng::circuit::make: ") + role +
        " output and control arrays must have equal length");
  validate_endpoints(controls.control_positive, controls.control_negative,
                     nodes, std::string(role) + " control", false);
}

void validate_nodes(const Amps &amps, std::int64_t nodes) {
  validate_endpoints(amps.output, amps.reference, nodes, "op amps", true);
  if (amps.positive_input.size() != amps.output.size() ||
      amps.negative_input.size() != amps.output.size())
    PLUSS_FAIL("eng::circuit::make: op-amp output and input "
                                "arrays must have equal length");
  validate_endpoints(amps.positive_input, amps.negative_input, nodes,
                     "op-amp inputs", true);
}

void validate_values(const Branches &branches, const char *role, bool positive,
                     bool nonnegative, ScalarType &dtype, Platform &platform,
                     bool &have_config, Shape &batch) {
  const auto branches_count = count(branches);
  if (branches_count == 0) {
    if (branches.value.defined() && branches.value.numel() != 0)
      PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                  " has values but no endpoints");
    return;
  }
  const Series &value = branches.value;
  if (!value.defined() || value.dim() < 1 || value.size(-1) != branches_count ||
      !value.dtype().is_floating())
    PLUSS_FAIL(
        std::string("eng::circuit::make: ") + role +
        " values must be real floating [..., elements]");
  if (!value.isfinite().all().item<bool>())
    PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                " values must be finite");
  if (positive && !value.gt(0.0).all().item<bool>())
    PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                " values must be positive");
  if (nonnegative && !value.ge(0.0).all().item<bool>())
    PLUSS_FAIL(std::string("eng::circuit::make: ") + role +
                                " values must be non-negative");

  if (!have_config) {
    dtype = value.scalar_type();
    platform = value.platform();
    have_config = true;
  } else if (dtype != value.scalar_type() || platform != value.platform()) {
    PLUSS_FAIL(
        "eng::circuit::make: every value group must share dtype and platform");
  }

  const Shape current = leading_shape(value);
  if (!current.empty()) {
    if (batch.empty())
      batch = current;
    else if (batch != current)
      PLUSS_FAIL("eng::circuit::make: non-scalar parameter "
                                  "batches must have equal leading shapes");
  }
}

void validate_values(const Controls &controls, const char *role,
                     ScalarType &dtype, Platform &platform, bool &have_config,
                     Shape &batch) {
  Branches values_only{controls.positive, controls.negative, controls.gain};
  validate_values(values_only, role, false, false, dtype, platform, have_config,
                  batch);
}

Series incidence(const std::vector<std::int64_t> &positive,
                 const std::vector<std::int64_t> &negative, std::int64_t nodes,
                 ScalarType dtype, Platform platform) {
  const auto branches_count = static_cast<std::int64_t>(positive.size());
  std::vector<double> values(
      static_cast<std::size_t>((nodes - 1) * branches_count), 0.0);
  for (std::int64_t branch = 0; branch < branches_count; ++branch) {
    const auto p = positive[static_cast<std::size_t>(branch)];
    const auto n = negative[static_cast<std::size_t>(branch)];
    if (p != 0)
      values[static_cast<std::size_t>((p - 1) * branches_count + branch)] = 1.0;
    if (n != 0)
      values[static_cast<std::size_t>((n - 1) * branches_count + branch)] =
          -1.0;
  }
  return pluss::series::make(values, {nodes - 1, branches_count})
      .to(dtype)
      .to(platform);
}

Series incidence(const Branches &branches, std::int64_t nodes, ScalarType dtype,
                 Platform platform) {
  return incidence(branches.positive, branches.negative, nodes, dtype,
                   platform);
}

Series values(const Branches &branches, const Shape &batch, ScalarType dtype,
              Platform platform) {
  const auto branches_count = count(branches);
  Shape target = batch;
  target.push_back(branches_count);
  if (branches_count == 0)
    return pluss::series::zeros(target, config(dtype, platform));
  if (leading_shape(branches.value) == batch)
    return branches.value;
  Shape aligned(batch.size(), 1);
  aligned.push_back(branches_count);
  return branches.value.reshape(aligned).expand(target, true);
}

Series values(const Controls &controls, const Shape &batch, ScalarType dtype,
              Platform platform) {
  return values(Branches{controls.positive, controls.negative, controls.gain},
                batch, dtype, platform);
}

Shape matrix_shape(const Shape &batch, std::int64_t rows, std::int64_t cols) {
  Shape shape = batch;
  shape.push_back(rows);
  shape.push_back(cols);
  return shape;
}

Shape vector_shape(const Shape &batch, std::int64_t size) {
  Shape shape = batch;
  shape.push_back(size);
  return shape;
}

Series laplace(const Series &incidence, const Series &weight,
               std::int64_t nodes, const Shape &batch, ScalarType dtype,
               Platform platform) {
  if (incidence.size(-1) == 0)
    return pluss::series::zeros(matrix_shape(batch, nodes - 1, nodes - 1),
                                config(dtype, platform));
  return incidence.mul(weight.unsqueeze(-2)).matmul(incidence.mT());
}

Series inject(const Series &incidence, const Series &value, std::int64_t nodes,
              const Shape &batch, ScalarType dtype, Platform platform) {
  if (incidence.size(-1) == 0)
    return pluss::series::zeros(vector_shape(batch, nodes - 1),
                                config(dtype, platform));
  return incidence.matmul(value.unsqueeze(-1)).squeeze(-1);
}

Series combine_last(std::initializer_list<const Series *> parts,
                    const Shape &batch, std::int64_t rows, ScalarType dtype,
                    Platform platform) {
  Series result = pluss::series::zeros(matrix_shape(batch, rows, 0),
                                       config(dtype, platform));
  for (const Series *part : parts) {
    if (part->size(-1) != 0)
      result = result.unite({*part}, -1);
  }
  return result;
}

Series batch_matrix(const Series &matrix, const Shape &batch) {
  if (batch.empty())
    return matrix;
  Shape aligned(batch.size(), 1);
  aligned.push_back(matrix.size(-2));
  aligned.push_back(matrix.size(-1));
  Shape target = batch;
  target.push_back(matrix.size(-2));
  target.push_back(matrix.size(-1));
  return matrix.reshape(aligned).expand(target, true);
}

Series controlled_nodal(const Series &output, const Series &control,
                        const Series &gain, std::int64_t nodes,
                        const Shape &batch, ScalarType dtype,
                        Platform platform) {
  if (gain.size(-1) == 0)
    return pluss::series::zeros(matrix_shape(batch, nodes - 1, nodes - 1),
                                config(dtype, platform));
  return output.mul(gain.unsqueeze(-2)).matmul(control.mT());
}

Result solve_system(std::int64_t nodes, std::int64_t voltage_count,
                    std::int64_t inductor_count, std::int64_t vccs_count,
                    std::int64_t vcvs_count, std::int64_t amp_count,
                    const Series &branch_kcl, const Series &branch_constraint,
                    const Series &nodal, const Series &rhs_node,
                    const Series &impedance, const Series &rhs_branch,
                    const Series &vccs_control_incidence,
                    const Series &vccs_gain, const Shape &batch,
                    ScalarType dtype, Platform platform) {
  const std::int64_t branches = branch_kcl.size(-1);

  Series matrix = nodal;
  Series rhs = rhs_node;
  if (branches > 0) {
    const Series top = nodal.unite({branch_kcl}, -1);
    const Series bottom = branch_constraint.mT().unite({impedance}, -1);
    matrix = top.unite({bottom}, -2);
    rhs = rhs_node.unite({rhs_branch}, -1);
  }

  const Series solution =
      matrix.linearalg.lu_solve(rhs.unsqueeze(-1)).squeeze(-1);
  const Series defect = matrix.matmul(solution.unsqueeze(-1))
                            .squeeze(-1)
                            .sub(rhs)
                            .abs()
                            .amax(-1, false);
  const auto node_unknowns = nodes - 1;
  const Series node_voltage = solution.slice(-1, 0, node_unknowns);
  const Series ground =
      pluss::series::zeros(vector_shape(batch, 1), config(dtype, platform));
  const Series voltage = ground.unite({node_voltage}, -1);
  const Series branch_current =
      solution.slice(-1, node_unknowns, node_unknowns + branches);
  const Series source_current = branch_current.slice(-1, 0, voltage_count);
  const Series inductor_current =
      branch_current.slice(-1, voltage_count, voltage_count + inductor_count);
  const auto vcvs_begin = voltage_count + inductor_count;
  const Series vcvs_current =
      branch_current.slice(-1, vcvs_begin, vcvs_begin + vcvs_count);
  const auto amp_begin = vcvs_begin + vcvs_count;
  const Series amp_current =
      branch_current.slice(-1, amp_begin, amp_begin + amp_count);
  Series vccs_current = pluss::series::zeros(vector_shape(batch, vccs_count),
                                             config(dtype, platform));
  if (vccs_count > 0) {
    const Series control_voltage = vccs_control_incidence.mT()
                                       .matmul(node_voltage.unsqueeze(-1))
                                       .squeeze(-1);
    vccs_current = control_voltage.mul(vccs_gain);
  }
  return {voltage,      source_current, inductor_current,
          vccs_current, vcvs_current,   amp_current,
          pluss::series::zeros(vector_shape(batch, 0), config(dtype, platform)),
          pluss::series::zeros(vector_shape(batch, 0), config(dtype, platform)),
          pluss::series::zeros(vector_shape(batch, 0), config(dtype, platform)),
          pluss::series::zeros(vector_shape(batch, 0), config(dtype, platform)),
          Series{}, Series{}, defect};
}


// The nodal equations are singular when a node has no path to ground, or when ideal voltage sources and inductors
// form a loop (or put two different voltages across the same pair of nodes). The linear solver reports that as a
// singular matrix, which says nothing about the circuit, so the failure is re-raised with the likely causes.
template <class Solve>
Result checked_solve(const char *operation, Solve &&solve) {
  try {
    return solve();
  } catch (const pluss::Error &error) {
    const std::string cause = error.message();
    PLUSS_FAIL_NO_CONTEXT(std::string("eng::circuit::") + operation +
                                ": the circuit equations are singular; check for a node with no path to ground, and for a loop "
                                "of ideal voltage sources and inductors (or two sources fixing the same voltage). Solver message: " + cause);
  }
}

} // namespace

// Compiled semiconductor-device data. The junction incidences carry the transistor polarity, so a PNP is evaluated
// with the NPN equations on negated junction voltages.
struct Devices {
  Series diode_incidence;
  Series diode_saturation;
  Series diode_ideality;
  Series be_incidence;
  Series bc_incidence;
  Series ic_incidence;
  Series t_saturation;
  Series t_forward;
  Series t_reverse;
  Series t_polarity;
  Series t_inv_early;
  Series gs_incidence, ds_incidence, sb_incidence, mos_kp, mos_vth, mos_lambda, mos_polarity, mos_gamma, mos_phi;
  Series diode_cj0, diode_vj, diode_m, diode_tt;
  bool diode_charge{};
  std::int64_t diodes{};
  std::int64_t transistors{};
  std::int64_t mosfets{};
  bool any() const { return diodes + transistors + mosfets > 0; }
};

// The time-independent stamps shared by the operating point and the transient: the conductance and controlled-source
// nodal block, the branch incidence of KCL and of the branch constraints, and the independent current injections.
struct Stamps {
  Series nodal;
  Series branch_kcl;
  Series branch_constraint;
  Series rhs_node;
  Series vccs_control;

  static Result ac(const Circuit &circuit, const Series &omega, const Result *point, const SolveOptions &options);
  static std::pair<Series, Series> dynamics(const Circuit &circuit, const Stamps &st);
  static Result unpack(const Circuit &circuit, const Stamps &st, const Series &states, const SolveOptions &options);

  static Stamps build(const Circuit &circuit) {
    const auto dtype = circuit.dtype_;
    const auto platform = circuit.platform();
    const Shape &batch = circuit.batch_shape_;
    const auto reduced_nodes = circuit.nodes_ - 1;

    const Series conductance = circuit.resistance_.reciprocal();
    Series nodal = laplace(circuit.resistor_incidence_, conductance, circuit.nodes_, batch, dtype, platform);
    nodal = nodal.add(controlled_nodal(circuit.vccs_output_incidence_, circuit.vccs_control_incidence_,
                                       circuit.vccs_gain_, circuit.nodes_, batch, dtype, platform));
    const Series rhs_node =
        inject(circuit.current_incidence_, circuit.current_, circuit.nodes_, batch, dtype, platform).neg();

    const Series source_kcl = batch_matrix(circuit.voltage_incidence_.to(dtype), batch);
    const Series inductor_kcl = batch_matrix(circuit.inductor_incidence_.to(dtype), batch);
    const Series vcvs_kcl = batch_matrix(circuit.vcvs_output_incidence_.to(dtype), batch);
    const Series amp_kcl = batch_matrix(circuit.amp_output_incidence_.to(dtype), batch);
    const Series vcvs_control = batch_matrix(circuit.vcvs_control_incidence_.to(dtype), batch);
    const Series vcvs_constraint = vcvs_kcl.sub(vcvs_control.mul(circuit.vcvs_gain_.unsqueeze(-2)));
    const Series amp_constraint = batch_matrix(circuit.amp_input_incidence_.to(dtype), batch);
    return {nodal,
            combine_last({&source_kcl, &inductor_kcl, &vcvs_kcl, &amp_kcl}, batch, reduced_nodes, dtype, platform),
            combine_last({&source_kcl, &inductor_kcl, &vcvs_constraint, &amp_constraint}, batch, reduced_nodes,
                         dtype, platform),
            rhs_node, batch_matrix(circuit.vccs_control_incidence_.to(dtype), batch)};
  }
};

namespace {
Stamps stamps(const Circuit &circuit) { return Stamps::build(circuit); }

// Concatenates along the last axis, skipping zero-width parts.
Series cat_columns(const std::vector<Series> &parts) {
  Series out;
  for (const Series &part : parts) {
    if (part.size(-1) == 0) continue;
    out = out.defined() ? out.unite({part}, -1) : part;
  }
  return out.defined() ? out : parts.front();
}

// Concatenates matrices along the row axis, skipping zero-height parts.
Series cat_rows(const std::vector<Series> &parts) {
  Series out;
  for (const Series &part : parts) {
    if (part.size(-2) == 0) continue;
    out = out.defined() ? out.unite({part}, -2) : part;
  }
  return out.defined() ? out : parts.front();
}

Shape timed_shape(const Shape &batch, std::int64_t steps, std::int64_t count) {
  Shape shape = batch;
  shape.push_back(steps);
  shape.push_back(count);
  return shape;
}

// [..., count] start values (or [count] shared by every lane) as one array with the circuit's full batch shape.
Series lane_values(const Series &given, const Shape &batch, std::int64_t count, ScalarType dtype, Platform platform,
                   const char *name) {
  if (!given.defined() || count == 0) {
    PLUSS_CHECK(!given.defined() || given.numel() == 0,
                std::string("eng::circuit::tran: ") + name + " has values but the circuit has no such elements");
    return pluss::series::zeros(vector_shape(batch, count), config(dtype, platform));
  }
  PLUSS_CHECK(given.dtype().is_floating() && given.dim() >= 1 && given.platform() == platform,
              std::string("eng::circuit::tran: ") + name + " must be real floating [..., elements] on the circuit's platform");
  PLUSS_CHECK(given.size(-1) == count, std::string("eng::circuit::tran: ") + name + " needs one value per element (" +
                                           std::to_string(count) + "), got " + std::to_string(given.size(-1)));
  PLUSS_CHECK(given.isfinite().all().item<bool>(), std::string("eng::circuit::tran: ") + name + " must be finite");
  const Shape lead = leading_shape(given);
  PLUSS_CHECK(lead.empty() || lead == batch,
              std::string("eng::circuit::tran: ") + name + " leading axes must match the circuit's parameter batch");
  if (lead == batch) return given.to(dtype);
  Shape aligned(batch.size(), 1);
  aligned.push_back(count);
  return given.to(dtype).reshape(aligned).expand(vector_shape(batch, count), true);
}

// [..., time, count] drive values, or the circuit's constant value repeated over every sample.
Series timed_values(const Series &given, const Series &constant, const Shape &batch, std::int64_t steps,
                    std::int64_t count, ScalarType dtype, Platform platform, const char *name) {
  const Shape target = timed_shape(batch, steps, count);
  if (count == 0) {
    PLUSS_CHECK(!given.defined() || given.numel() == 0,
                std::string("eng::circuit::tran: drive ") + name + " has values but the circuit has no such sources");
    return pluss::series::zeros(target, config(dtype, platform));
  }
  if (!given.defined()) {
    Shape aligned = batch;
    aligned.push_back(1);
    aligned.push_back(count);
    return constant.reshape(aligned).expand(target, true);
  }
  PLUSS_CHECK(given.dtype().is_floating() && given.dim() >= 2 && given.platform() == platform,
              std::string("eng::circuit::tran: drive ") + name +
                  " must be real floating [..., time, sources] on the circuit's platform");
  PLUSS_CHECK(given.size(-2) == steps, std::string("eng::circuit::tran: drive ") + name +
                                           " needs one row per time sample (" + std::to_string(steps) + "), got " +
                                           std::to_string(given.size(-2)));
  PLUSS_CHECK(given.size(-1) == count, std::string("eng::circuit::tran: drive ") + name +
                                           " needs one column per source (" + std::to_string(count) + "), got " +
                                           std::to_string(given.size(-1)));
  PLUSS_CHECK(given.isfinite().all().item<bool>(), std::string("eng::circuit::tran: drive ") + name + " must be finite");
  const Shape lead(given.sizes().begin(), given.sizes().end() - 2);
  PLUSS_CHECK(lead.empty() || lead == batch,
              std::string("eng::circuit::tran: drive ") + name + " leading axes must match the circuit's parameter batch");
  if (lead == batch) return given.to(dtype);
  Shape aligned(batch.size(), 1);
  aligned.push_back(steps);
  aligned.push_back(count);
  return given.to(dtype).reshape(aligned).expand(target, true);
}

Series diagonal_matrix(const Series &diagonal, ScalarType dtype, Platform platform) {
  return diagonal.unsqueeze(-1).mul(pluss::series::eye(diagonal.size(-1), config(dtype, platform)));
}

// Solves M x = rhs for a batch of dense matrices, replacing a bare "singular matrix" with the circuit-level cause.
Series dense_solve(const Series &matrix, const Series &rhs, const char *what, const char *who = "tran") {
  try {
    return matrix.linearalg.lu_solve(rhs);
  } catch (const pluss::Error &) {
    PLUSS_FAIL_NO_CONTEXT(std::string("eng::circuit::") + who + ": " + what);
  }
}

constexpr const char *kStepSingular =
    "the step equations are singular; check for a node with no path to ground, and for a loop of ideal voltage "
    "sources and inductors (or two sources fixing the same voltage)";
constexpr const char *kStartSingular =
    "the starting state cannot be reached; a capacitor connected directly across ideal voltage sources must start at "
    "the voltage they impose, an inductor in series with only current sources must start at the current they impose, "
    "and every node needs a path to ground (a loop of ideal voltage sources, or two sources fixing the same voltage, "
    "also has no solution)";

Series row(const Series &value, std::int64_t index) { return value.slice(-2, index, index + 1).squeeze(-2); }

Series defect(const Series &value) { return value.abs().amax(-1, false); }


// ---- Semiconductor devices -------------------------------------------------------------------------------------

Series pad_matrix(const Series &g, std::int64_t size) {
  const std::int64_t r = g.size(-1);
  if (r == size) return g;
  const Shape lead(g.sizes().begin(), g.sizes().end() - 2);
  const Config cfg = config(g.scalar_type(), g.platform());
  const Series top = g.unite({pluss::series::zeros(matrix_shape(lead, r, size - r), cfg)}, -1);
  return top.unite({pluss::series::zeros(matrix_shape(lead, size - r, size), cfg)}, -2);
}

Series pad_vector(const Series &v, std::int64_t size) {
  const std::int64_t r = v.size(-1);
  if (r == size) return v;
  const Shape lead(v.sizes().begin(), v.sizes().end() - 1);
  return v.unite({pluss::series::zeros(vector_shape(lead, size - r), config(v.scalar_type(), v.platform()))}, -1);
}

using detail::max_abs;

// Device parameters shaped to broadcast against the junction voltages: [..., devices], or [..., 1, devices] when the
// voltages carry a time axis.
struct DeviceParams {
  Series diode_saturation, diode_ideality, saturation, forward, reverse, polarity, inv_early, kp, vth, lambda, mos_polarity, gamma, phi;
  Series cj0, vj, grading, transit;
  bool diode_charge{};
  double charge_scale{0.0};   // 2 / h and the like: the weight of the junction charge in a time step (zero in DC)
};

DeviceParams device_params(const Devices &d, bool timed) {
  auto shape = [&](const Series &value) { return timed ? value.unsqueeze(-2) : value; };
  DeviceParams p;
  if (d.diodes > 0) {
    p.diode_saturation = shape(d.diode_saturation);
    p.diode_ideality = shape(d.diode_ideality);
    if (d.diode_charge) {
      p.diode_charge = true;
      p.cj0 = shape(d.diode_cj0);
      p.vj = shape(d.diode_vj);
      p.grading = shape(d.diode_m);
      p.transit = shape(d.diode_tt);
    }
  }
  if (d.transistors > 0) {
    p.saturation = shape(d.t_saturation);
    p.forward = shape(d.t_forward);
    p.reverse = shape(d.t_reverse);
    p.inv_early = shape(d.t_inv_early);
    p.polarity = d.t_polarity;
  }
  if (d.mosfets > 0) {
    p.kp = shape(d.mos_kp);
    p.vth = shape(d.mos_vth);
    p.lambda = shape(d.mos_lambda);
    p.mos_polarity = d.mos_polarity;
    p.gamma = shape(d.mos_gamma);
    p.phi = shape(d.mos_phi);
  }
  return p;
}

struct Junctions {
  Series vd, vbe, vbc, vgs, vds, vsb;
};

Junctions junctions(const Devices &d, const Series &node) {
  Junctions j;
  if (d.mosfets > 0) {
    j.vgs = node.matmul(d.gs_incidence);
    j.vds = node.matmul(d.ds_incidence);
    j.vsb = node.matmul(d.sb_incidence);
  }
  if (d.diodes > 0) j.vd = node.matmul(d.diode_incidence);
  if (d.transistors > 0) {
    j.vbe = node.matmul(d.be_incidence);
    j.vbc = node.matmul(d.bc_incidence);
  }
  return j;
}

struct Currents {
  Series id, gd, ic, ib, gc_be, gc_bc, gb_be, gb_bc, mid, mgm, mgds, mgmb, qd, cd;
};

// exp of a junction argument. The clamp only guards against overflow (the exponent that the dtype can hold); the
// limiter keeps every iterate far below it.
Series junction_exp(const Series &arg) {
  return arg.clamp_max(arg.scalar_type() == ScalarType::Float32 ? 80.0 : 700.0).exp();
}

Currents evaluate(const Devices &d, const DeviceParams &p, const Junctions &j, const SolveOptions &o) {
  Currents c;
  if (d.diodes > 0) {
    const Series nvt = p.diode_ideality.mul(o.thermal_voltage);
    const Series e = junction_exp(j.vd.div(nvt));
    c.id = p.diode_saturation.mul(e.sub(1.0)).add(j.vd.mul(o.gmin));
    c.gd = p.diode_saturation.mul(e).div(nvt).add(o.gmin);
    if (p.diode_charge) {
      // depletion charge (SPICE form: linear continuation above half the potential) plus diffusion charge tau i
      const Series v = j.vd;
      const Series fcvj = p.vj.mul(0.5);
      const Series below = v.lt(fcvj).to(v.scalar_type());
      const Series x = v.div(p.vj).neg().add(1.0).clamp_min(1e-9);
      const Series one_minus_m = p.grading.neg().add(1.0);
      const Series x_power = x.log().mul(one_minus_m).exp();
      const Series q_low = p.cj0.mul(p.vj).div(one_minus_m).mul(x_power.neg().add(1.0));
      const Series c_low = p.cj0.mul(x.log().mul(p.grading.neg()).exp());
      const Series f2 = p.grading.add(1.0).mul(-std::log(0.5)).exp();   // (1 - fc)^-(1 + m) with fc = 1/2
      const Series q_fc = p.cj0.mul(p.vj).div(one_minus_m).mul(one_minus_m.mul(std::log(0.5)).exp().neg().add(1.0));
      const Series dv = v.sub(fcvj);
      const Series slope_term = p.grading.add(1.0).mul(-0.5).add(1.0);   // 1 - fc (1 + m)
      const Series q_high = q_fc.add(p.cj0.mul(f2).mul(slope_term.mul(dv).add(p.grading.mul(v.mul(v).sub(fcvj.mul(fcvj))).div(p.vj.mul(2.0)))));
      const Series c_high = p.cj0.mul(f2).mul(slope_term.add(p.grading.mul(v).div(p.vj)));
      const Series e_dc = p.diode_saturation.mul(e.sub(1.0));
      const Series g_dc = p.diode_saturation.mul(e).div(nvt);
      c.qd = below.mul(q_low).add(below.neg().add(1.0).mul(q_high)).add(p.transit.mul(e_dc));
      c.cd = below.mul(c_low).add(below.neg().add(1.0).mul(c_high)).add(p.transit.mul(g_dc));
      if (p.charge_scale != 0.0) {
        c.id = c.id.add(c.qd.mul(p.charge_scale));
        c.gd = c.gd.add(c.cd.mul(p.charge_scale));
      }
    }
  }
  if (d.transistors > 0) {
    const double vt = o.thermal_voltage;
    const Series ef = junction_exp(j.vbe.mul(1.0 / vt));
    const Series er = junction_exp(j.vbc.mul(1.0 / vt));
    const Series s = p.saturation;
    const Series inv_bf = p.forward.reciprocal();
    const Series inv_br = p.reverse.reciprocal();
    const Series ir = s.mul(er.sub(1.0));
    const Series transport = s.mul(ef.sub(er));
    const Series early = j.vbe.sub(j.vbc).mul(p.inv_early).add(1.0);
    c.ic = transport.mul(early).sub(ir.mul(inv_br)).sub(j.vbc.mul(o.gmin));
    c.ib = s.mul(ef.sub(1.0)).mul(inv_bf).add(ir.mul(inv_br)).add(j.vbe.add(j.vbc).mul(o.gmin));
    const Series gf = s.mul(ef).mul(1.0 / vt);
    const Series gr = s.mul(er).mul(1.0 / vt);
    c.gc_be = gf.mul(early).add(transport.mul(p.inv_early));
    c.gc_bc = gr.mul(early).add(transport.mul(p.inv_early)).add(gr.mul(inv_br)).neg().sub(o.gmin);
    c.gb_be = gf.mul(inv_bf).add(o.gmin);
    c.gb_bc = gr.mul(inv_br).add(o.gmin);
  }
  if (d.mosfets > 0) {
    // square law in the effective (polarity-folded) frame; a negative drain-source voltage swaps drain and source
    const Series forward = j.vds.ge(0.0).to(j.vds.scalar_type());
    const Series reversed = forward.neg().add(1.0);
    const Series ugs = j.vgs.sub(reversed.mul(j.vds));
    const Series uds = j.vds.abs();
    // body effect: the effective source-bulk voltage (the drain plays the source when the channel is reversed)
    const Series sb_raw = j.vsb.add(reversed.mul(j.vds));
    const Series sb_on = sb_raw.gt(0.0).to(sb_raw.scalar_type());
    const Series root = p.phi.add(sb_raw.clamp_min(0.0)).sqrt();
    const Series vth = p.vth.add(p.gamma.mul(root.sub(p.phi.sqrt())));
    const Series eta = p.gamma.div(root.mul(2.0)).mul(sb_on);   // d vth / d vsb
    const Series vov = ugs.sub(vth);
    const Series on = vov.gt(0.0).to(vov.scalar_type());
    const Series triode = on.mul(uds.lt(vov).to(vov.scalar_type()));
    const Series saturated = on.sub(triode);
    const Series clm = p.lambda.mul(uds).add(1.0);
    const Series half_square = vov.mul(vov).mul(0.5);
    const Series i_tri = p.kp.mul(vov.mul(uds).sub(uds.mul(uds).mul(0.5))).mul(clm);
    const Series i_sat = p.kp.mul(half_square).mul(clm);
    const Series f = triode.mul(i_tri).add(saturated.mul(i_sat));
    const Series fm = triode.mul(p.kp.mul(uds).mul(clm)).add(saturated.mul(p.kp.mul(vov).mul(clm)));
    const Series fds = triode.mul(p.kp.mul(vov.sub(uds)).mul(clm).add(p.kp.mul(vov.mul(uds).sub(uds.mul(uds).mul(0.5))).mul(p.lambda)))
                           .add(saturated.mul(p.kp.mul(half_square).mul(p.lambda)));
    const Series sign = forward.sub(reversed);
    c.mid = sign.mul(f).add(j.vds.mul(o.gmin));
    c.mgm = sign.mul(fm);
    c.mgmb = sign.mul(fm).mul(eta).neg();
    c.mgds = forward.mul(fds).add(reversed.mul(fm.add(fds))).add(o.gmin).add(reversed.mul(sign.mul(fm).mul(eta).neg()));
  }
  return c;
}

// Nodal current leaving every node through the devices, [..., nodes - 1].
Series device_injection(const Devices &d, const Currents &c, const Shape &lead, ScalarType dtype, Platform platform,
                        std::int64_t reduced) {
  Series out = pluss::series::zeros(vector_shape(lead, reduced), config(dtype, platform));
  if (d.diodes > 0) out = out.add(c.id.matmul(d.diode_incidence.mT()));
  if (d.transistors > 0)
    out = out.add(c.ic.matmul(d.ic_incidence.mT())).add(c.ib.matmul(d.be_incidence.mT()));
  if (d.mosfets > 0) out = out.add(c.mid.matmul(d.ds_incidence.mT()));
  return out;
}

// The nodal charge [..., nodes - 1] stored in the devices (the junction charge of diodes): its time derivative is a current.
Series device_charge(const Devices &d, const Currents &c, const Shape &lead, ScalarType dtype, Platform platform, std::int64_t reduced) {
  Series out = pluss::series::zeros(vector_shape(lead, reduced), config(dtype, platform));
  if (d.diodes > 0 && d.diode_charge) out = out.add(c.qd.matmul(d.diode_incidence.mT()));
  return out;
}

// Small-signal capacitance stamp [..., nodes - 1, nodes - 1] of the device charge at the junction voltages of `c`.
Series device_capacitance(const Devices &d, const Currents &c, const Shape &lead, ScalarType dtype, Platform platform, std::int64_t reduced) {
  Series out = pluss::series::zeros(matrix_shape(lead, reduced, reduced), config(dtype, platform));
  if (d.diodes > 0 && d.diode_charge) out = out.add(d.diode_incidence.mul(c.cd.unsqueeze(-2)).matmul(d.diode_incidence.mT()));
  return out;
}

struct Companion {
  Series conductance;
  Series injection;
};

// Newton companion of the devices about junction voltages `j`: conductance stamp G and equivalent source so that the
// device currents are G v + injection.
Companion linearize(const Devices &d, const DeviceParams &p, const Junctions &j, const SolveOptions &o,
                    const Shape &lead, ScalarType dtype, Platform platform, std::int64_t reduced) {
  const Currents c = evaluate(d, p, j, o);
  const Config cfg = config(dtype, platform);
  Series g = pluss::series::zeros(matrix_shape(lead, reduced, reduced), cfg);
  Series inj = pluss::series::zeros(vector_shape(lead, reduced), cfg);
  auto stamp = [&](const Series &out_inc, const Series &weight, const Series &in_inc) {
    return out_inc.mul(weight.unsqueeze(-2)).matmul(in_inc.mT());
  };
  if (d.diodes > 0) {
    g = g.add(stamp(d.diode_incidence, c.gd, d.diode_incidence));
    inj = inj.add(c.id.sub(c.gd.mul(j.vd)).matmul(d.diode_incidence.mT()));
  }
  if (d.transistors > 0) {
    const Series &bc = d.ic_incidence;
    const Series &bb = d.be_incidence;
    g = g.add(stamp(bc, c.gc_be, d.be_incidence)).add(stamp(bc, c.gc_bc, d.bc_incidence))
            .add(stamp(bb, c.gb_be, d.be_incidence)).add(stamp(bb, c.gb_bc, d.bc_incidence));
    const Series eq_c = c.ic.sub(c.gc_be.mul(j.vbe)).sub(c.gc_bc.mul(j.vbc));
    const Series eq_b = c.ib.sub(c.gb_be.mul(j.vbe)).sub(c.gb_bc.mul(j.vbc));
    inj = inj.add(eq_c.matmul(bc.mT())).add(eq_b.matmul(bb.mT()));
  }
  if (d.mosfets > 0) {
    g = g.add(stamp(d.ds_incidence, c.mgm, d.gs_incidence)).add(stamp(d.ds_incidence, c.mgds, d.ds_incidence)).add(stamp(d.ds_incidence, c.mgmb, d.sb_incidence));
    inj = inj.add(c.mid.sub(c.mgm.mul(j.vgs)).sub(c.mgds.mul(j.vds)).sub(c.mgmb.mul(j.vsb)).matmul(d.ds_incidence.mT()));
  }
  return {g, inj};
}

// Thermal voltage and critical voltage of every junction, for the junction-voltage limiter.
struct Limits {
  Series diode_vt, diode_crit, vt, crit;
};

Limits make_limits(const Devices &d, const DeviceParams &p, const SolveOptions &o) {
  pluss::NoGradGuard no_grad;
  Limits l;
  const double root2 = std::sqrt(2.0);
  if (d.diodes > 0) {
    l.diode_vt = p.diode_ideality.mul(o.thermal_voltage).detach();
    l.diode_crit = l.diode_vt.mul(l.diode_vt.div(p.diode_saturation.detach().mul(root2)).log());
  }
  if (d.transistors > 0) {
    const Series sat = p.saturation.detach();
    l.vt = sat.mul(0.0).add(o.thermal_voltage);
    l.crit = l.vt.mul(l.vt.div(sat.mul(root2)).log());
  }
  return l;
}

// SPICE-style junction limiting: past the critical voltage a large step in the junction voltage is compressed to a
// logarithmic one, so the exponential is never evaluated far from where the last iterate linearised it.
Series limit_voltage(const Series &vnew, const Series &vold, const Series &vt, const Series &crit, ScalarType dtype) {
  const Series diff = vnew.sub(vold);
  const Series arg = diff.div(vt).add(1.0);
  const Series rising = arg.gt(0.0).to(dtype);
  const Series from_positive = rising.mul(vold.add(vt.mul(arg.clamp_min(1e-30).log()))).add(rising.neg().add(1.0).mul(crit));
  const Series from_negative = vt.mul(vnew.div(vt).clamp_min(1e-30).log());
  const Series positive = vold.gt(0.0).to(dtype);
  const Series compressed = positive.mul(from_positive).add(positive.neg().add(1.0).mul(from_negative));
  const Series active = vnew.gt(crit).to(dtype).mul(diff.abs().gt(vt.mul(2.0)).to(dtype));
  return vnew.add(active.mul(compressed.sub(vnew)));
}

Junctions limit(const Devices &d, const Limits &l, const Junctions &next, const Junctions &prior, ScalarType dtype) {
  Junctions j;
  if (d.mosfets > 0) {   // a square-law device only needs its steps kept moderate
    j.vgs = prior.vgs.add(next.vgs.sub(prior.vgs).clamp_min(-2.0).clamp_max(2.0));
    j.vds = prior.vds.add(next.vds.sub(prior.vds).clamp_min(-2.0).clamp_max(2.0));
    j.vsb = prior.vsb.add(next.vsb.sub(prior.vsb).clamp_min(-2.0).clamp_max(2.0));
  }
  if (d.diodes > 0) j.vd = limit_voltage(next.vd, prior.vd, l.diode_vt, l.diode_crit, dtype);
  if (d.transistors > 0) {
    j.vbe = limit_voltage(next.vbe, prior.vbe, l.vt, l.crit, dtype);
    j.vbc = limit_voltage(next.vbc, prior.vbc, l.vt, l.crit, dtype);
  }
  return j;
}

double junction_gap(const Devices &d, const Junctions &a, const Junctions &b) {
  double gap = 0.0;
  if (d.mosfets > 0) gap = std::max({gap, max_abs(a.vgs.sub(b.vgs)), max_abs(a.vds.sub(b.vds)), max_abs(a.vsb.sub(b.vsb))});
  if (d.diodes > 0) gap = std::max(gap, max_abs(a.vd.sub(b.vd)));
  if (d.transistors > 0) gap = std::max({gap, max_abs(a.vbe.sub(b.vbe)), max_abs(a.vbc.sub(b.vbc))});
  return gap;
}

void check_solver(const SolveOptions &o, const char *who) {
  PLUSS_CHECK(std::isfinite(o.thermal_voltage) && o.thermal_voltage > 0.0,
              std::string("eng::circuit::") + who + ": solver thermal_voltage must be positive");
  PLUSS_CHECK(std::isfinite(o.tolerance) && o.tolerance > 0.0,
              std::string("eng::circuit::") + who + ": solver tolerance must be positive");
  PLUSS_CHECK(o.max_iterations >= 1, std::string("eng::circuit::") + who + ": solver max_iterations must be at least 1");
  PLUSS_CHECK(std::isfinite(o.step_tolerance) && o.step_tolerance > 0.0 && o.max_refinement >= 0,
              std::string("eng::circuit::") + who + ": solver step_tolerance must be positive and max_refinement not negative");
  PLUSS_CHECK(o.max_current > 0.0, std::string("eng::circuit::") + who + ": solver max_current must be positive");
  PLUSS_CHECK(std::isfinite(o.gmin) && o.gmin >= 0.0,
              std::string("eng::circuit::") + who + ": solver gmin must be non-negative");
}

std::string general(double value) {
  char text[32];
  std::snprintf(text, sizeof text, "%.3g", value);
  return text;
}

struct NewtonOut {
  Series x;
  std::int64_t iterations{};
};

// Solves base x + N(x) = rhs, where N is the device current drawn from the first `reduced` unknowns. The iteration
// runs outside the autodiff tape; one final differentiable step at the converged point carries the sensitivities of
// the solution to the circuit parameters (the implicit-function result).
NewtonOut newton_once(const Devices &d, const DeviceParams &p, const Limits &limits, const Series &base,
                      const Series &rhs, const Series &guess, std::int64_t reduced, const SolveOptions &o,
                      ScalarType dtype, Platform platform, const Shape &lead, const char *who, const char *singular) {
  const std::int64_t size = base.size(-1);
  const double eps = dtype == ScalarType::Float32 ? 1.2e-7 : 2.3e-16;
  const double tolerance = std::max(o.tolerance, 100.0 * eps);
  Series x;
  std::int64_t iterations = 0;
  bool converged = false;
  double last = 0.0;
  {
    pluss::NoGradGuard no_grad;
    const Series base_value = base.detach();
    const Series rhs_value = rhs.detach();
    x = guess.detach();
    Junctions limited = junctions(d, x.slice(-1, 0, reduced));
    for (std::int64_t it = 1; it <= o.max_iterations; ++it) {
      const Companion c = linearize(d, p, limited, o, lead, dtype, platform, reduced);
      const Series step = dense_solve(base_value.add(pad_matrix(c.conductance, size)),
                                      rhs_value.sub(pad_vector(c.injection, size)).unsqueeze(-1), singular, who)
                              .squeeze(-1);
      PLUSS_CHECK(step.isfinite().all().item<bool>(),
                  std::string("eng::circuit::") + who + ": the Newton iteration produced a non-finite value");
      const Junctions raw = junctions(d, step.slice(-1, 0, reduced));
      const Junctions next = limit(d, limits, raw, limited, dtype);
      const double move = max_abs(step.sub(x));
      const double gap = junction_gap(d, raw, next);
      const double scale = 1.0 + max_abs(step);
      x = step;
      limited = next;
      iterations = it;
      last = std::max(move, gap);
      if (move <= tolerance * scale && gap <= tolerance * scale) {
        converged = true;
        break;
      }
    }
  }
  if (!converged)
    PLUSS_FAIL_NO_CONTEXT(std::string("eng::circuit::") + who + ": the Newton iteration did not converge in " +
                          std::to_string(o.max_iterations) + " iterations (last update " + general(last) +
                          "); raise max_iterations, loosen tolerance, or check that the bias is reachable");
  const Companion c = linearize(d, p, junctions(d, x.slice(-1, 0, reduced)), o, lead, dtype, platform, reduced);
  const Series final_step = dense_solve(base.add(pad_matrix(c.conductance, size)),
                                        rhs.sub(pad_vector(c.injection, size)).unsqueeze(-1), singular, who)
                                .squeeze(-1);
  return {final_step, iterations};
}

// Newton with the standard convergence aids: when the plain iteration fails (a singular linearisation or no convergence)
// the solve is repeated with a large junction conductance that is stepped down to `gmin`, and, when `source_stepping` is
// set (operating points), with the sources ramped up from a tenth. The solution must carry no device current above
// `max_current`.
NewtonOut newton(const Devices &d, const DeviceParams &p, const Limits &limits, const Series &base, const Series &rhs,
                 const Series &guess, std::int64_t reduced, const SolveOptions &o, ScalarType dtype, Platform platform,
                 const Shape &lead, const char *who, const char *singular, bool source_stepping = false) {
  NewtonOut result;
  bool solved = false;
  std::string failure;
  try {
    result = newton_once(d, p, limits, base, rhs, guess, reduced, o, dtype, platform, lead, who, singular);
    solved = true;
  } catch (const pluss::Error &first) {
    failure = first.message();
  }
  if (!solved) {
    try {  // gmin stepping
      Series x = guess;
      std::int64_t total = 0;
      for (double g = 1e-3; g > std::max(o.gmin, 1e-18) * 10.0; g *= 0.01) {
        SolveOptions stage = o;
        stage.gmin = g;
        const NewtonOut step = newton_once(d, p, limits, base, rhs, x, reduced, stage, dtype, platform, lead, who, singular);
        x = step.x.detach();
        total += step.iterations;
      }
      result = newton_once(d, p, limits, base, rhs, x, reduced, o, dtype, platform, lead, who, singular);
      result.iterations += total;
      solved = true;
    } catch (const pluss::Error &) {
    }
  }
  if (!solved && source_stepping) {
    try {  // source stepping
      Series x = guess;
      std::int64_t total = 0;
      for (int k = 1; k <= 10; ++k) {
        const NewtonOut step = newton_once(d, p, limits, base, rhs.mul(0.1 * k), x, reduced, o, dtype, platform, lead, who, singular);
        x = step.x.detach();
        total += step.iterations;
        if (k == 10) {
          result = step;
          result.iterations = total;
          solved = true;
        }
      }
    } catch (const pluss::Error &) {
    }
  }
  if (!solved)
    PLUSS_FAIL_NO_CONTEXT(failure + (source_stepping ? " (gmin stepping and source stepping were also tried)" : " (gmin stepping was also tried)"));
  // nothing in the circuit may carry an absurd current
  {
    pluss::NoGradGuard no_grad;
    const Currents c = evaluate(d, p, junctions(d, result.x.detach().slice(-1, 0, reduced)), o);
    double worst = 0.0;
    if (d.diodes > 0) worst = std::max(worst, max_abs(c.id));
    if (d.transistors > 0) worst = std::max({worst, max_abs(c.ic), max_abs(c.ib)});
    if (d.mosfets > 0) worst = std::max(worst, max_abs(c.mid));
    if (!(worst <= o.max_current))
      PLUSS_FAIL_NO_CONTEXT(std::string("eng::circuit::") + who + ": a device carries " + general(worst) +
                            " A, more than max_current (" + general(o.max_current) +
                            " A); a junction driven directly by an ideal source has nothing to limit its current, so add a series "
                            "resistance (or raise max_current)");
  }
  return result;
}

Series int_counts(const std::vector<std::int64_t> &counts, Platform platform) {
  std::vector<double> values(counts.begin(), counts.end());
  return pluss::series::make(values).to(ScalarType::Int64).to(platform);
}

Series zero_counts(std::int64_t length, Platform platform) {
  return pluss::series::zeros({length}, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

} // namespace


// E and F of the linear system E x' + F x = u(t), with x = [node voltages; branch currents].
std::pair<Series, Series> Stamps::dynamics(const Circuit &circuit, const Stamps &st) {
  const ScalarType dtype = circuit.dtype_;
  const Platform platform = circuit.platform();
  const Shape &batch = circuit.batch_shape_;
  const Config cfg = config(dtype, platform);
  const std::int64_t reduced = circuit.nodes_ - 1;
  const std::int64_t nv = circuit.voltage_count_, nl = circuit.inductor_count_, ne = circuit.vcvs_count_,
                     na = circuit.amp_count_;
  const std::int64_t nb = nv + nl + ne + na;
  const Series zero_br = pluss::series::zeros(matrix_shape(batch, nb, nb), cfg);
  const Series f = cat_rows({cat_columns({st.nodal, st.branch_kcl}), cat_columns({st.branch_constraint.mT(), zero_br})});
  const Series cap_nodal = laplace(circuit.capacitor_incidence_, circuit.capacitance_, circuit.nodes_, batch, dtype, platform);
  Series e = cap_nodal;
  if (nb > 0) {
    const Series diag = cat_columns({pluss::series::zeros(vector_shape(batch, nv), cfg), circuit.inductance_.neg(),
                                     pluss::series::zeros(vector_shape(batch, ne + na), cfg)});
    e = cat_rows({cat_columns({cap_nodal, pluss::series::zeros(matrix_shape(batch, reduced, nb), cfg)}),
                  cat_columns({pluss::series::zeros(matrix_shape(batch, nb, reduced), cfg),
                               diagonal_matrix(diag, dtype, platform)})});
  }
  return {f, e};
}

// Result members from states [..., time, unknowns]: node voltages with the ground column, branch currents, controlled
// source currents and device currents. `residual` and `iterations` are left for the caller.
Result Stamps::unpack(const Circuit &circuit, const Stamps &st, const Series &states, const SolveOptions &options) {
  const ScalarType dtype = circuit.dtype_;
  const Platform platform = circuit.platform();
  const Shape &batch = circuit.batch_shape_;
  const Config cfg = config(dtype, platform);
  const std::int64_t steps = states.size(-2);
  const std::int64_t reduced = circuit.nodes_ - 1;
  const std::int64_t nv = circuit.voltage_count_, nl = circuit.inductor_count_, ne = circuit.vcvs_count_,
                     na = circuit.amp_count_;
  const std::int64_t nb = nv + nl + ne + na;
  const std::int64_t n = reduced + nb;
  const Series node_voltage = states.slice(-1, 0, reduced);
  Result out;
  out.voltage = pluss::series::zeros(timed_shape(batch, steps, 1), cfg).unite({node_voltage}, -1);
  const Series currents = states.slice(-1, reduced, n);
  out.source_current = currents.slice(-1, 0, nv);
  out.inductor_current = currents.slice(-1, nv, nv + nl);
  out.vcvs_current = currents.slice(-1, nv + nl, nv + nl + ne);
  out.amp_current = currents.slice(-1, nv + nl + ne, nb);
  out.vccs_current = pluss::series::zeros(timed_shape(batch, steps, circuit.vccs_count_), cfg);
  if (circuit.vccs_count_ > 0)
    out.vccs_current =
        st.vccs_control.mT().matmul(node_voltage.unsqueeze(-1)).squeeze(-1).mul(circuit.vccs_gain_.unsqueeze(-2));
  out.diode_current = pluss::series::zeros(timed_shape(batch, steps, circuit.diode_count_), cfg);
  out.collector_current = pluss::series::zeros(timed_shape(batch, steps, circuit.transistor_count_), cfg);
  out.base_current = out.collector_current;
  out.drain_current = pluss::series::zeros(timed_shape(batch, steps, circuit.mosfet_count_), cfg);
  if (circuit.devices_->any()) {
    const Devices &dev = *circuit.devices_;
    const Currents c = evaluate(dev, device_params(dev, true), junctions(dev, node_voltage), options);
    if (dev.diodes > 0) out.diode_current = c.id;
    if (dev.mosfets > 0) out.drain_current = c.mid.mul(dev.mos_polarity);
    if (dev.transistors > 0) {
      out.collector_current = c.ic.mul(dev.t_polarity);
      out.base_current = c.ib.mul(dev.t_polarity);
    }
  }
  return out;
}

Circuit make(const Net &net) {
  if (net.nodes < 2)
    PLUSS_FAIL("eng::circuit::make: nodes must include ground "
                                "and at least one unknown node");

  const Branches *groups[] = {&net.resistors, &net.capacitors, &net.inductors,
                              &net.current_sources, &net.voltage_sources};
  const char *roles[] = {"resistors", "capacitors", "inductors",
                         "current sources", "voltage sources"};
  for (std::size_t i = 0; i < 5; ++i)
    validate_nodes(*groups[i], net.nodes, roles[i]);
  validate_nodes(net.vccs, net.nodes, "VCCS elements");
  validate_nodes(net.vcvs, net.nodes, "VCVS elements");
  validate_nodes(net.amps, net.nodes);
  validate_endpoints(net.diodes.positive, net.diodes.negative, net.nodes, "diodes", true);
  const auto nd = static_cast<std::int64_t>(net.diodes.positive.size());
  const Transistors &tr = net.transistors;
  const auto nq = static_cast<std::int64_t>(tr.collector.size());
  if (tr.base.size() != tr.collector.size() || tr.emitter.size() != tr.collector.size() ||
      tr.polarity.size() != tr.collector.size())
    PLUSS_FAIL("eng::circuit::make: transistor collector, base, emitter and polarity arrays must have equal length");
  validate_endpoints(tr.collector, tr.emitter, net.nodes, "transistor collector/emitter", true);
  validate_endpoints(tr.base, tr.emitter, net.nodes, "transistor base/emitter", true);
  validate_endpoints(tr.base, tr.collector, net.nodes, "transistor base/collector", true);
  for (auto sign : tr.polarity)
    if (sign != 1 && sign != -1)
      PLUSS_FAIL("eng::circuit::make: transistor polarity must be +1 (NPN) or -1 (PNP)");
  const Mosfets &mo = net.mosfets;
  const auto nm = static_cast<std::int64_t>(mo.drain.size());
  if (mo.gate.size() != mo.drain.size() || mo.source.size() != mo.drain.size() || mo.polarity.size() != mo.drain.size())
    PLUSS_FAIL("eng::circuit::make: MOSFET drain, gate, source and polarity arrays must have equal length");
  validate_endpoints(mo.drain, mo.source, net.nodes, "MOSFET drain/source", true);
  validate_endpoints(mo.gate, mo.source, net.nodes, "MOSFET gate/source", false);
  validate_endpoints(mo.gate, mo.drain, net.nodes, "MOSFET gate/drain", false);
  if (!mo.bulk.empty()) {
    PLUSS_CHECK(mo.bulk.size() == mo.drain.size(), "eng::circuit::make: MOSFET bulk must be empty (body tied to the source) or have one node per MOSFET");
    validate_endpoints(mo.source, mo.bulk, net.nodes, "MOSFET source/bulk", false);
  }
  for (auto sign : mo.polarity)
    if (sign != 1 && sign != -1)
      PLUSS_FAIL("eng::circuit::make: MOSFET polarity must be +1 (NMOS) or -1 (PMOS)");

  ScalarType dtype = ScalarType::Float64;
  Platform platform = Platform::CPU;
  bool have_config = false;
  Shape batch;
  validate_values(net.resistors, roles[0], true, false, dtype, platform,
                  have_config, batch);
  validate_values(net.capacitors, roles[1], false, true, dtype, platform,
                  have_config, batch);
  validate_values(net.inductors, roles[2], true, false, dtype, platform,
                  have_config, batch);
  validate_values(net.current_sources, roles[3], false, false, dtype, platform,
                  have_config, batch);
  validate_values(net.voltage_sources, roles[4], false, false, dtype, platform,
                  have_config, batch);
  validate_values(net.vccs, "VCCS elements", dtype, platform, have_config,
                  batch);
  validate_values(net.vcvs, "VCVS elements", dtype, platform, have_config,
                  batch);
  auto device_values = [&](const std::vector<std::int64_t> &ends, const Series &value, const char *role, bool required) {
    if (!required && !value.defined()) return;
    validate_values(Branches{ends, ends, value}, role, true, false, dtype, platform, have_config, batch);
  };
  if (nd > 0) {
    device_values(net.diodes.positive, net.diodes.saturation, "diode saturation currents", true);
    device_values(net.diodes.positive, net.diodes.ideality, "diode ideality factors", false);
  } else if (net.diodes.saturation.defined() && net.diodes.saturation.numel() != 0) {
    PLUSS_FAIL("eng::circuit::make: diodes has values but no endpoints");
  }
  if (nq > 0) {
    device_values(tr.collector, tr.saturation, "transistor saturation currents", true);
    device_values(tr.collector, tr.forward_gain, "transistor forward gains", true);
    device_values(tr.collector, tr.reverse_gain, "transistor reverse gains", false);
    device_values(tr.collector, tr.early_voltage, "transistor Early voltages", false);
  } else if (tr.saturation.defined() && tr.saturation.numel() != 0) {
    PLUSS_FAIL("eng::circuit::make: transistors has values but no endpoints");
  }
  auto device_nonnegative = [&](const std::vector<std::int64_t> &ends, const Series &value, const char *role) {
    if (!value.defined()) return;
    validate_values(Branches{ends, ends, value}, role, false, true, dtype, platform, have_config, batch);
  };
  if (nm > 0) {
    device_values(mo.drain, mo.transconductance, "MOSFET transconductances", true);
    device_values(mo.drain, mo.threshold, "MOSFET threshold voltages", true);
    device_nonnegative(mo.drain, mo.channel_modulation, "MOSFET channel-length modulation values");
  } else if (mo.transconductance.defined() && mo.transconductance.numel() != 0) {
    PLUSS_FAIL("eng::circuit::make: mosfets has values but no endpoints");
  }
  device_nonnegative(net.diodes.positive, nd > 0 ? net.diodes.junction_capacitance : Series{}, "diode junction capacitances");
  if (nd > 0) {
    if (net.diodes.junction_potential.defined()) device_values(net.diodes.positive, net.diodes.junction_potential, "diode junction potentials", true);
    device_nonnegative(net.diodes.positive, net.diodes.grading, "diode grading coefficients");
    device_nonnegative(net.diodes.positive, net.diodes.transit_time, "diode transit times");
    if (net.diodes.grading.defined())
      PLUSS_CHECK(net.diodes.grading.lt(1.0).all().item<bool>(), "eng::circuit::make: diode grading coefficients must be below one");
  }
  if (nm > 0) {
    device_nonnegative(mo.drain, mo.body_effect, "MOSFET body-effect coefficients");
    if (mo.surface_potential.defined()) device_values(mo.drain, mo.surface_potential, "MOSFET surface potentials", true);
  }
  if (nq > 0) {
    device_nonnegative(tr.collector, tr.base_emitter_capacitance, "transistor base-emitter capacitances");
    device_nonnegative(tr.collector, tr.base_collector_capacitance, "transistor base-collector capacitances");
  }
  if (nm > 0) {
    device_nonnegative(mo.drain, mo.gate_source_capacitance, "MOSFET gate-source capacitances");
    device_nonnegative(mo.drain, mo.gate_drain_capacitance, "MOSFET gate-drain capacitances");
  }
  if (!have_config)
    PLUSS_FAIL(
        "eng::circuit::make: at least one non-empty element group is required");

  // Device capacitances are ordinary capacitors appended to the net's: one place implements charge storage.
  std::vector<std::int64_t> cap_positive = net.capacitors.positive, cap_negative = net.capacitors.negative;
  Series cap_value = values(net.capacitors, batch, dtype, platform);
  auto add_capacitance = [&](const std::vector<std::int64_t> &positive, const std::vector<std::int64_t> &negative, const Series &value) {
    if (!value.defined() || positive.empty()) return;
    const Series expanded = values(Branches{positive, negative, value}, batch, dtype, platform);
    cap_positive.insert(cap_positive.end(), positive.begin(), positive.end());
    cap_negative.insert(cap_negative.end(), negative.begin(), negative.end());
    cap_value = cap_value.size(-1) == 0 ? expanded : cap_value.unite({expanded}, -1);
  };
  if (!net.diodes.junction_potential.defined()) add_capacitance(net.diodes.positive, net.diodes.negative, net.diodes.junction_capacitance);
  add_capacitance(tr.base, tr.emitter, tr.base_emitter_capacitance);
  add_capacitance(tr.base, tr.collector, tr.base_collector_capacitance);
  add_capacitance(mo.gate, mo.source, mo.gate_source_capacitance);
  add_capacitance(mo.gate, mo.drain, mo.gate_drain_capacitance);
  validate_endpoints(cap_positive, cap_negative, net.nodes, "capacitors (including device capacitances)", true);

  auto device_array = [&](const std::vector<std::int64_t> &ends, const Series &value, std::int64_t n, double fallback) {
    if (!value.defined()) return pluss::series::zeros(vector_shape(batch, n), config(dtype, platform)).add(fallback);
    return values(Branches{ends, ends, value}, batch, dtype, platform);
  };
  auto devices = std::make_shared<Devices>();
  devices->diodes = nd;
  devices->transistors = nq;
  devices->mosfets = nm;
  if (nd > 0) {
    devices->diode_incidence = incidence(net.diodes.positive, net.diodes.negative, net.nodes, dtype, platform);
    devices->diode_saturation = device_array(net.diodes.positive, net.diodes.saturation, nd, 0.0);
    devices->diode_ideality = device_array(net.diodes.positive, net.diodes.ideality, nd, 1.0);
    devices->diode_charge = net.diodes.junction_potential.defined() || net.diodes.transit_time.defined();
    if (devices->diode_charge) {
      devices->diode_cj0 = net.diodes.junction_potential.defined() ? device_array(net.diodes.positive, net.diodes.junction_capacitance, nd, 0.0)
                                                                 : pluss::series::zeros(vector_shape(batch, nd), config(dtype, platform));
      devices->diode_vj = device_array(net.diodes.positive, net.diodes.junction_potential, nd, 1.0);
      devices->diode_m = device_array(net.diodes.positive, net.diodes.grading, nd, 0.5);
      devices->diode_tt = device_array(net.diodes.positive, net.diodes.transit_time, nd, 0.0);
    }
  }
  if (nm > 0) {
    std::vector<double> sign(mo.polarity.begin(), mo.polarity.end());
    const Series polarity = pluss::series::make(sign).to(dtype).to(platform);
    devices->mos_polarity = polarity;
    devices->gs_incidence = incidence(mo.gate, mo.source, net.nodes, dtype, platform).mul(polarity);
    devices->ds_incidence = incidence(mo.drain, mo.source, net.nodes, dtype, platform).mul(polarity);
    devices->mos_kp = device_array(mo.drain, mo.transconductance, nm, 0.0);
    devices->mos_vth = device_array(mo.drain, mo.threshold, nm, 0.0);
    devices->mos_lambda = device_array(mo.drain, mo.channel_modulation, nm, 0.0);
    devices->mos_gamma = device_array(mo.drain, mo.body_effect, nm, 0.0);
    devices->mos_phi = device_array(mo.drain, mo.surface_potential, nm, 0.6);
    devices->sb_incidence = mo.bulk.empty() ? pluss::series::zeros({net.nodes - 1, nm}, config(dtype, platform))
                                            : incidence(mo.source, mo.bulk, net.nodes, dtype, platform).mul(polarity);
  }
  if (nq > 0) {
    std::vector<double> sign(tr.polarity.begin(), tr.polarity.end());
    const Series polarity = pluss::series::make(sign).to(dtype).to(platform);
    devices->t_polarity = polarity;
    devices->be_incidence = incidence(tr.base, tr.emitter, net.nodes, dtype, platform).mul(polarity);
    devices->bc_incidence = incidence(tr.base, tr.collector, net.nodes, dtype, platform).mul(polarity);
    devices->ic_incidence = incidence(tr.collector, tr.emitter, net.nodes, dtype, platform).mul(polarity);
    devices->t_saturation = device_array(tr.collector, tr.saturation, nq, 0.0);
    devices->t_forward = device_array(tr.collector, tr.forward_gain, nq, 0.0);
    devices->t_reverse = device_array(tr.collector, tr.reverse_gain, nq, 1.0);
    devices->t_inv_early = tr.early_voltage.defined() ? device_array(tr.collector, tr.early_voltage, nq, 1.0).reciprocal()
                                                      : pluss::series::zeros(vector_shape(batch, nq), config(dtype, platform));
  }

  return Circuit(net.nodes, batch, dtype,
                 incidence(net.resistors, net.nodes, dtype, platform),
                 values(net.resistors, batch, dtype, platform),
                 incidence(cap_positive, cap_negative, net.nodes, dtype, platform),
                 cap_value,
                 incidence(net.inductors, net.nodes, dtype, platform),
                 values(net.inductors, batch, dtype, platform),
                 incidence(net.current_sources, net.nodes, dtype, platform),
                 values(net.current_sources, batch, dtype, platform),
                 incidence(net.voltage_sources, net.nodes, dtype, platform),
                 values(net.voltage_sources, batch, dtype, platform),
                 incidence(net.vccs.positive, net.vccs.negative, net.nodes,
                           dtype, platform),
                 incidence(net.vccs.control_positive, net.vccs.control_negative,
                           net.nodes, dtype, platform),
                 values(net.vccs, batch, dtype, platform),
                 incidence(net.vcvs.positive, net.vcvs.negative, net.nodes,
                           dtype, platform),
                 incidence(net.vcvs.control_positive, net.vcvs.control_negative,
                           net.nodes, dtype, platform),
                 values(net.vcvs, batch, dtype, platform),
                 incidence(net.amps.output, net.amps.reference, net.nodes,
                           dtype, platform),
                 incidence(net.amps.positive_input, net.amps.negative_input,
                           net.nodes, dtype, platform),
                 count(net.voltage_sources), count(net.inductors),
                 count(net.vccs), count(net.vcvs), count(net.amps), devices, nd, nq, nm);
}

Result op(const Circuit &circuit, const SolveOptions &options) {
  check_solver(options, "op");
  const auto dtype = circuit.dtype_;
  const auto platform = circuit.platform();
  const Shape &batch = circuit.batch_shape_;
  const Stamps st = stamps(circuit);
  const auto branch_count = st.branch_kcl.size(-1);

  const Series zero_l = pluss::series::zeros(
      vector_shape(batch, circuit.inductor_count_), config(dtype, platform));
  const Series zero_e = pluss::series::zeros(
      vector_shape(batch, circuit.vcvs_count_), config(dtype, platform));
  const Series zero_a = pluss::series::zeros(
      vector_shape(batch, circuit.amp_count_), config(dtype, platform));
  const Series rhs_branch =
      circuit.voltage_.unite({zero_l, zero_e, zero_a}, -1);
  const Series impedance = pluss::series::zeros(
      matrix_shape(batch, branch_count, branch_count), config(dtype, platform));
  if (circuit.devices_->any()) {
    const Devices &dev = *circuit.devices_;
    const std::int64_t reduced = circuit.nodes_ - 1;
    const auto [f, e] = Stamps::dynamics(circuit, st);
    (void)e;
    const Config cfg = config(dtype, platform);
    const Series u0 = cat_columns({st.rhs_node, rhs_branch});
    const DeviceParams params = device_params(dev, false);
    const Limits limits = make_limits(dev, params, options);
    const Series guess = pluss::series::zeros(vector_shape(batch, f.size(-1)), cfg);
    const NewtonOut solved = newton(dev, params, limits, f, u0, guess, reduced, options, dtype, platform, batch, "op",
                                    "the operating-point equations are singular; check for a node with no path to ground, "
                                    "and for a loop of ideal voltage sources and inductors", true);
    Result result = Stamps::unpack(circuit, st, solved.x.unsqueeze(-2), options);
    for (Series *member : {&result.voltage, &result.source_current, &result.inductor_current, &result.vccs_current,
                           &result.vcvs_current, &result.amp_current, &result.diode_current, &result.collector_current,
                           &result.base_current, &result.drain_current})
      *member = member->squeeze(-2);
    const Series nodes_now = solved.x.slice(-1, 0, reduced);
    const Series drawn = pad_vector(device_injection(dev, evaluate(dev, params, junctions(dev, nodes_now), options), batch,
                                                     dtype, platform, reduced), f.size(-1));
    result.residual = defect(f.matmul(solved.x.unsqueeze(-1)).squeeze(-1).add(drawn).sub(u0));
    result.iterations = int_counts({solved.iterations}, platform);
    return result;
  }
  Result linear = checked_solve("op", [&] { return solve_system(circuit.nodes_, circuit.voltage_count_,
                      circuit.inductor_count_, circuit.vccs_count_,
                      circuit.vcvs_count_, circuit.amp_count_, st.branch_kcl,
                      st.branch_constraint, st.nodal, st.rhs_node, impedance, rhs_branch,
                      st.vccs_control, circuit.vccs_gain_, batch, dtype, platform); });
  linear.iterations = zero_counts(1, platform);
  return linear;
}

Result ac(const Circuit &circuit, const Series &omega) {
  return Stamps::ac(circuit, omega, nullptr, SolveOptions{});
}

Result ac(const Circuit &circuit, const Series &omega, const Result &operating_point, const SolveOptions &options) {
  check_solver(options, "ac");
  return Stamps::ac(circuit, omega, &operating_point, options);
}

Result Stamps::ac(const Circuit &circuit, const Series &omega, const Result *point, const SolveOptions &options) {
  if (!omega.defined() || omega.dim() != 1 || !omega.dtype().is_floating() ||
      omega.numel() == 0 || !omega.isfinite().all().item<bool>() ||
      !omega.ge(0.0).all().item<bool>())
    PLUSS_FAIL("eng::circuit::ac: omega must be a non-empty "
                                "finite non-negative real vector");
  if (omega.platform() != circuit.platform())
    PLUSS_FAIL(
        "eng::circuit::ac: omega and circuit must share a platform");

  const ScalarType complex_dtype = circuit.dtype_ == ScalarType::Float32
                                       ? ScalarType::Complex64
                                       : ScalarType::Complex128;
  const Platform platform = circuit.platform();
  Shape batch = circuit.batch_shape_;
  batch.push_back(omega.numel());
  Shape omega_shape(circuit.batch_shape_.size(), 1);
  omega_shape.push_back(omega.numel());
  omega_shape.push_back(1);
  const Series w = omega.to(circuit.dtype_).reshape(omega_shape);
  const Series jw = pluss::series::complex(w.mul(0.0), w, complex_dtype);

  const Series resistance = circuit.resistance_.unsqueeze(-2).to(complex_dtype);
  const Series capacitance =
      circuit.capacitance_.unsqueeze(-2).to(complex_dtype);
  const Series conductance = resistance.reciprocal();
  Series nodal =
      laplace(circuit.resistor_incidence_.to(complex_dtype), conductance,
              circuit.nodes_, batch, complex_dtype, platform);
  nodal = nodal.add(laplace(circuit.capacitor_incidence_.to(complex_dtype),
                            capacitance.mul(jw), circuit.nodes_, batch,
                            complex_dtype, platform));
  const Series vccs_gain = circuit.vccs_gain_.unsqueeze(-2).to(complex_dtype);
  nodal = nodal.add(controlled_nodal(
      circuit.vccs_output_incidence_.to(complex_dtype),
      circuit.vccs_control_incidence_.to(complex_dtype), vccs_gain,
      circuit.nodes_, batch, complex_dtype, platform));

  const Series current = circuit.current_.unsqueeze(-2).to(complex_dtype);
  const Series rhs_node =
      inject(circuit.current_incidence_.to(complex_dtype), current,
             circuit.nodes_, batch, complex_dtype, platform)
          .neg();
  const Series voltage = circuit.voltage_.unsqueeze(-2).to(complex_dtype);
  Shape source_shape = batch;
  source_shape.push_back(circuit.voltage_count_);
  const Series source = voltage.expand(source_shape, true);
  const Series zero_l =
      pluss::series::zeros(vector_shape(batch, circuit.inductor_count_),
                           config(complex_dtype, platform));
  const Series zero_e =
      pluss::series::zeros(vector_shape(batch, circuit.vcvs_count_),
                           config(complex_dtype, platform));
  const Series zero_a = pluss::series::zeros(
      vector_shape(batch, circuit.amp_count_), config(complex_dtype, platform));
  const Series rhs_branch = source.unite({zero_l, zero_e, zero_a}, -1);

  const auto reduced_nodes = circuit.nodes_ - 1;
  const Series source_kcl =
      batch_matrix(circuit.voltage_incidence_.to(complex_dtype), batch);
  const Series inductor_kcl =
      batch_matrix(circuit.inductor_incidence_.to(complex_dtype), batch);
  const Series vcvs_kcl =
      batch_matrix(circuit.vcvs_output_incidence_.to(complex_dtype), batch);
  const Series amp_kcl =
      batch_matrix(circuit.amp_output_incidence_.to(complex_dtype), batch);
  const Series vcvs_control =
      batch_matrix(circuit.vcvs_control_incidence_.to(complex_dtype), batch);
  const Series vcvs_gain = circuit.vcvs_gain_.unsqueeze(-2).to(complex_dtype);
  const Series vcvs_constraint =
      vcvs_kcl.sub(vcvs_control.mul(vcvs_gain.unsqueeze(-2)));
  const Series amp_constraint =
      batch_matrix(circuit.amp_input_incidence_.to(complex_dtype), batch);
  const Series branch_kcl =
      combine_last({&source_kcl, &inductor_kcl, &vcvs_kcl, &amp_kcl}, batch,
                   reduced_nodes, complex_dtype, platform);
  const Series branch_constraint = combine_last(
      {&source_kcl, &inductor_kcl, &vcvs_constraint, &amp_constraint}, batch,
      reduced_nodes, complex_dtype, platform);

  const Series inductance = circuit.inductance_.unsqueeze(-2).to(complex_dtype);
  const Series z_l = inductance.mul(jw).neg();
  Shape zero_v_shape = batch;
  zero_v_shape.push_back(circuit.voltage_count_);
  const Series zero_v =
      pluss::series::zeros(zero_v_shape, config(complex_dtype, platform));
  const Series diagonal = zero_v.unite({z_l, zero_e, zero_a}, -1);
  const auto branches = circuit.voltage_count_ + circuit.inductor_count_ +
                        circuit.vcvs_count_ + circuit.amp_count_;
  const Series eye =
      pluss::series::eye(branches, config(circuit.dtype_, platform))
          .to(complex_dtype);
  const Series impedance = diagonal.unsqueeze(-1).mul(eye);

  if (circuit.devices_->any()) {
    PLUSS_CHECK(point != nullptr,
                "eng::circuit::ac: a circuit with diodes or transistors needs its operating point; call "
                "ac(circuit, omega, operating_point) with the result of op on the biased circuit");
    const Devices &dev = *circuit.devices_;
    const Series &bias = point->voltage;
    PLUSS_CHECK(bias.defined() && bias.dim() == static_cast<std::int64_t>(circuit.batch_shape_.size()) + 1 &&
                    bias.size(-1) == circuit.nodes_ &&
                    Shape(bias.sizes().begin(), bias.sizes().end() - 1) == circuit.batch_shape_,
                "eng::circuit::ac: operating_point must be the op result of a circuit with the same nodes and parameter batch");
    const Series node = bias.slice(-1, 1, circuit.nodes_).to(circuit.dtype_);
    const Companion small = linearize(dev, device_params(dev, false), junctions(dev, node), options, circuit.batch_shape_,
                                      circuit.dtype_, platform, reduced_nodes);
    nodal = nodal.add(small.conductance.unsqueeze(-3).to(complex_dtype));
    if (dev.diode_charge) {
      const DeviceParams at_rest = device_params(dev, false);
      const Currents here = evaluate(dev, at_rest, junctions(dev, node), options);
      const Series cap = device_capacitance(dev, here, circuit.batch_shape_, circuit.dtype_, platform, reduced_nodes);
      nodal = nodal.add(cap.unsqueeze(-3).to(complex_dtype).mul(jw.unsqueeze(-1)));
    }
  }
  const Series vccs_control =
      batch_matrix(circuit.vccs_control_incidence_.to(complex_dtype), batch);
  Result small_signal = checked_solve("ac", [&] { return solve_system(circuit.nodes_, circuit.voltage_count_,
                      circuit.inductor_count_, circuit.vccs_count_,
                      circuit.vcvs_count_, circuit.amp_count_, branch_kcl,
                      branch_constraint, nodal, rhs_node, impedance, rhs_branch,
                      vccs_control, vccs_gain, batch, complex_dtype, platform); });
  small_signal.iterations = zero_counts(1, platform);
  return small_signal;
}


Result tran(const Circuit &circuit, const Series &time, const Drive &drive, const Start &start,
            const TranOptions &options) {
  const ScalarType dtype = circuit.dtype_;
  const Platform platform = circuit.platform();
  const Shape &batch = circuit.batch_shape_;

  PLUSS_CHECK(time.defined() && time.dim() == 1 && time.numel() >= 2 && time.dtype().is_floating(),
              "eng::circuit::tran: time must be a real floating vector with at least two samples");
  PLUSS_CHECK(time.platform() == platform, "eng::circuit::tran: time and circuit must share a platform");
  const std::vector<double> grid = time.to(Platform::CPU, ScalarType::Float64).items<double>();
  const double h = grid[1] - grid[0];
  PLUSS_CHECK(std::isfinite(h) && h > 0.0, "eng::circuit::tran: time must be finite and strictly increasing");
  // Rounding of the time values themselves (coarse for Float32) is not unevenness.
  const double epsilon = time.scalar_type() == ScalarType::Float32 ? 1.2e-7 : 2.3e-16;
  const double tolerance = std::max(1e-9 * h, 8.0 * epsilon * std::max(std::abs(grid.front()), std::abs(grid.back())));
  for (std::size_t i = 2; i < grid.size(); ++i)
    PLUSS_CHECK(std::isfinite(grid[i]) && std::abs((grid[i] - grid[i - 1]) - h) <= tolerance,
                "eng::circuit::tran: time must be uniformly spaced (one step size for the whole run)");
  const std::int64_t steps = time.numel();
  // The integrators step by exactly h; the discrete simulator checks its grid against that step.
  std::vector<double> exact(grid.size());
  for (std::size_t i = 0; i < exact.size(); ++i) exact[i] = grid.front() + static_cast<double>(i) * h;
  const Series clock = pluss::series::make(exact).to(platform);

  const std::int64_t reduced = circuit.nodes_ - 1;
  const std::int64_t nv = circuit.voltage_count_, nl = circuit.inductor_count_, ne = circuit.vcvs_count_,
                     na = circuit.amp_count_;
  const std::int64_t nb = nv + nl + ne + na;
  const std::int64_t nc = circuit.capacitor_incidence_.size(-1);
  const std::int64_t n = reduced + nb;
  const Config cfg = config(dtype, platform);

  // E x' + F x + N(x) = u(t), with x = [node voltages; branch currents] and N the device currents.
  const Stamps st = Stamps::build(circuit);
  const auto [f, e] = Stamps::dynamics(circuit, st);
  const bool nonlinear = circuit.devices_->any();
  const SolveOptions &solver = options.solver;
  check_solver(solver, "tran");
  const Devices &dev = *circuit.devices_;
  const DeviceParams params = device_params(dev, false);
  const Limits limits = make_limits(dev, params, solver);
  const char *singular = "the step equations are singular; check for a node with no path to ground, and for a loop of "
                         "ideal voltage sources and inductors (or two sources fixing the same voltage)";
  // Current the devices draw from the nodes at state x, padded to the full unknown vector.
  auto drawn = [&](const Series &x) {
    return pad_vector(device_injection(dev, evaluate(dev, params, junctions(dev, x.slice(-1, 0, reduced)), solver), batch,
                                       dtype, platform, reduced),
                      n);
  };

  // Charge stored in the devices at state x (its time derivative is a current), padded like `drawn`.
  auto charged = [&](const Series &x) {
    return pad_vector(device_charge(dev, evaluate(dev, params, junctions(dev, x.slice(-1, 0, reduced)), solver), batch, dtype, platform, reduced), n);
  };
  auto scaled = [&](double weight) {
    DeviceParams copy = params;
    copy.charge_scale = weight;
    return copy;
  };
  auto drawn_at = [&](const Series &x, double weight) {
    return pad_vector(device_injection(dev, evaluate(dev, scaled(weight), junctions(dev, x.slice(-1, 0, reduced)), solver), batch, dtype, platform, reduced), n);
  };

  // Sources over time, [batch, steps, n].
  const Series volts = timed_values(drive.voltage, circuit.voltage_, batch, steps, nv, dtype, platform, "voltage");
  const std::int64_t ni = circuit.current_incidence_.size(-1);
  const Series amps = timed_values(drive.current, circuit.current_, batch, steps, ni, dtype, platform, "current");
  Series rhs_node = pluss::series::zeros(timed_shape(batch, steps, reduced), cfg);
  if (ni > 0) rhs_node = circuit.current_incidence_.matmul(amps.unsqueeze(-1)).squeeze(-1).neg();
  const Series u = cat_columns({rhs_node, volts, pluss::series::zeros(timed_shape(batch, steps, nl + ne + na), cfg)});

  // Consistent initial state: each capacitor is held at its start voltage and each inductor at its start current.
  const Series v0 = lane_values(start.capacitor_voltage, batch, nc, dtype, platform, "start capacitor_voltage");
  const Series i0 = lane_values(start.inductor_current, batch, nl, dtype, platform, "start inductor_current");
  const Series cap_inc = batch_matrix(circuit.capacitor_incidence_.to(dtype), batch);
  const Series pin = cat_columns({pluss::series::zeros(vector_shape(batch, nv), cfg),
                                  pluss::series::zeros(vector_shape(batch, nl), cfg).add(1.0),
                                  pluss::series::zeros(vector_shape(batch, ne + na), cfg)});
  Series constraints = st.branch_constraint.mT();
  if (nb > 0) constraints = constraints.mul(pin.neg().add(1.0).unsqueeze(-1));
  const Series pinned = cat_rows(
      {cat_columns({st.nodal, st.branch_kcl, cap_inc}),
       cat_columns({constraints, diagonal_matrix(pin, dtype, platform), pluss::series::zeros(matrix_shape(batch, nb, nc), cfg)}),
       // the capacitor constraints may be redundant (capacitors in parallel), so their multiplier block is regularised
       // slightly and the agreement of the start voltages is checked afterwards
       cat_columns({cap_inc.mT(), pluss::series::zeros(matrix_shape(batch, nc, nb), cfg),
                    batch_matrix(pluss::series::eye(nc, cfg).mul(-1e-12), batch)})});
  const Series first = cat_columns({row(rhs_node, 0), row(volts, 0), i0,
                                    pluss::series::zeros(vector_shape(batch, ne + na), cfg), v0});
  std::vector<std::int64_t> counts(static_cast<std::size_t>(steps), 0);
  std::vector<std::int64_t> refined_counts;
  Series y0;
  Series start_residual;
  if (nonlinear) {
    const NewtonOut solved = newton(dev, params, limits, pinned, first, pluss::series::zeros(vector_shape(batch, n + nc), cfg),
                                    reduced, solver, dtype, platform, batch, "tran", kStartSingular, true);
    y0 = solved.x;
    counts[0] = solved.iterations;
    start_residual = defect(pinned.matmul(y0.unsqueeze(-1)).squeeze(-1).add(pad_vector(drawn(y0.slice(-1, 0, n)), n + nc)).sub(first));
  } else {
    y0 = dense_solve(pinned, first.unsqueeze(-1), kStartSingular).squeeze(-1);
    start_residual = defect(pinned.matmul(y0.unsqueeze(-1)).squeeze(-1).sub(first));
  }
  const Series x0 = y0.slice(-1, 0, n);
  if (nc > 0) {
    const Series held = cap_inc.mT().matmul(x0.slice(-1, 0, reduced).unsqueeze(-1)).squeeze(-1);
    PLUSS_CHECK(max_abs(held.sub(v0)) <= 1e-6 * (1.0 + max_abs(v0)),
                "eng::circuit::tran: the start voltages of capacitors that share the same nodes disagree (parallel capacitors "
                "must start at the same voltage), or a capacitor directly across ideal sources starts at another voltage");
  }
  Series residual = start_residual.unsqueeze(-1);

  const Series eye = batch_matrix(pluss::series::eye(n, cfg), batch);
  const Series u_tail = u.slice(-2, 1, steps);
  Series states;
  if (nonlinear) {
    // Each step is the same implicit equation as the linear one with the device currents added; Newton solves it.
    const bool trapezoid = options.scheme == Scheme::Trapezoid;
    std::vector<Series> xs{x0};
    std::vector<Series> defects;
    Series previous_current = drawn(x0);
    Series previous_charge = charged(x0);
    std::vector<Series> charges{previous_charge};
    const Series trap_base = f.add(e.mul(2.0 / h));
    const Series trap_back = e.mul(2.0 / h).sub(f);
    const Series be_base = f.add(e.mul(1.0 / h));
    const Series gear_base = f.add(e.mul(1.5 / h));
    const Series gear_now = e.mul(2.0 / h);
    const Series gear_before = e.mul(0.5 / h);
    struct Sub {
      Series x, current, charge;
      std::int64_t iterations{};
      Series defect;
    };
    std::map<int, std::pair<Series, Series>> trap_cache;   // halvings -> (step matrix, history matrix)
    auto trap_matrices = [&](int halvings) -> const std::pair<Series, Series> & {
      auto it = trap_cache.find(halvings);
      if (it == trap_cache.end()) {
        const double hh = h / std::pow(2.0, halvings);
        it = trap_cache.emplace(halvings, std::make_pair(f.add(e.mul(2.0 / hh)), e.mul(2.0 / hh).sub(f))).first;
      }
      return it->second;
    };
    auto trap_sub = [&](const Series &xa, const Series &na, const Series &qa, const Series &ua, const Series &ub, int halvings) -> Sub {
      const auto &m = trap_matrices(halvings);
      const double weight = 2.0 / (h / std::pow(2.0, halvings));
      const Series rhs_sub = ub.add(ua).add(m.second.matmul(xa.unsqueeze(-1)).squeeze(-1)).sub(na).add(qa.mul(weight));
      const NewtonOut s = newton(dev, scaled(weight), limits, m.first, rhs_sub, xa, reduced, solver, dtype, platform, batch, "tran", singular);
      return {s.x, drawn(s.x), charged(s.x), s.iterations,
              defect(m.first.matmul(s.x.unsqueeze(-1)).squeeze(-1).add(drawn_at(s.x, weight)).sub(rhs_sub))};
    };
    // Subdivides an interval whose solution is not resolved by the step: trapezoid sub-steps until a full step and two half
    // steps agree (or max_refinement halvings are reached).
    std::function<Sub(const Series &, const Series &, const Series &, const Series &, const Series &, int, std::int64_t &, std::int64_t &)> refine =
        [&](const Series &xa, const Series &na, const Series &qa, const Series &ua, const Series &ub, int halvings, std::int64_t &used,
            std::int64_t &pieces) -> Sub {
      bool split = false;
      if (halvings < solver.max_refinement) {
        pluss::NoGradGuard no_grad;
        const Series um = ua.add(ub).mul(0.5);
        const Sub whole = trap_sub(xa.detach(), na.detach(), qa.detach(), ua.detach(), ub.detach(), halvings);
        const Sub a = trap_sub(xa.detach(), na.detach(), qa.detach(), ua.detach(), um, halvings + 1);
        const Sub b = trap_sub(a.x, a.current, a.charge, um, ub.detach(), halvings + 1);
        used += whole.iterations + a.iterations + b.iterations;
        split = max_abs(whole.x.sub(b.x)) / (1.0 + max_abs(b.x)) > solver.step_tolerance;
      }
      if (!split) {
        ++pieces;
        Sub accepted = trap_sub(xa, na, qa, ua, ub, halvings);
        used += accepted.iterations;
        return accepted;
      }
      const Series um = ua.add(ub).mul(0.5);
      const Sub first = refine(xa, na, qa, ua, um, halvings + 1, used, pieces);
      Sub second = refine(first.x, first.current, first.charge, um, ub, halvings + 1, used, pieces);
      second.defect = second.defect.maximum(first.defect);
      return second;
    };
    std::vector<std::int64_t> refined(static_cast<std::size_t>(steps), 0);
    for (std::int64_t k = 1; k < steps; ++k) {
      const Series &last = xs.back();
      Series base, rhs;
      double weight;   // the weight of the junction charge in this step's formula
      if (trapezoid) {
        weight = 2.0 / h;
        base = trap_base;
        rhs = row(u, k).add(row(u, k - 1)).add(trap_back.matmul(last.unsqueeze(-1)).squeeze(-1)).sub(previous_current).add(previous_charge.mul(weight));
      } else if (k == 1) {
        weight = 1.0 / h;
        base = be_base;
        rhs = row(u, 1).add(e.matmul(last.unsqueeze(-1)).squeeze(-1).mul(1.0 / h)).add(previous_charge.mul(weight));
      } else {
        weight = 1.5 / h;
        base = gear_base;
        rhs = row(u, k).add(gear_now.matmul(last.unsqueeze(-1)).squeeze(-1))
                  .sub(gear_before.matmul(xs[xs.size() - 2].unsqueeze(-1)).squeeze(-1))
                  .add(previous_charge.mul(2.0 / h)).sub(charges[charges.size() - 2].mul(0.5 / h));
      }
      NewtonOut solved = newton(dev, scaled(weight), limits, base, rhs, last, reduced, solver, dtype, platform, batch, "tran", singular);
      Series step_defect = defect(base.matmul(solved.x.unsqueeze(-1)).squeeze(-1).add(drawn_at(solved.x, weight)).sub(rhs));
      // step-doubling error estimate: the same interval as two half steps of the trapezoid rule
      const Series ua = row(u, k - 1), ub = row(u, k);
      std::int64_t spent = 0;
      bool subdivide = false;
      {
        pluss::NoGradGuard no_grad;
        const Series um = ua.add(ub).mul(0.5);
        const Sub first_half = trap_sub(last.detach(), previous_current.detach(), previous_charge.detach(), ua.detach(), um, 1);
        const Sub second_half = trap_sub(first_half.x, first_half.current, first_half.charge, um, ub.detach(), 1);
        spent += first_half.iterations + second_half.iterations;
        subdivide = max_abs(solved.x.detach().sub(second_half.x)) / (1.0 + max_abs(second_half.x)) > solver.step_tolerance;
      }
      if (subdivide && solver.max_refinement >= 1) {
        std::int64_t used = 0, pieces = 0;
        const Sub fine = refine(last, previous_current, previous_charge, ua, ub, 0, used, pieces);
        solved.x = fine.x;
        step_defect = fine.defect;
        spent += used;
        refined[static_cast<std::size_t>(k)] = pieces;
      }
      previous_current = drawn(solved.x);
      previous_charge = charged(solved.x);
      charges.push_back(previous_charge);
      defects.push_back(step_defect.unsqueeze(-1));
      counts[static_cast<std::size_t>(k)] = solved.iterations + spent;
      xs.push_back(solved.x);
    }
    refined_counts = refined;
    std::vector<Series> tail;
    for (std::size_t k = 1; k < xs.size(); ++k) tail.push_back(xs[k].unsqueeze(-2));
    states = x0.unsqueeze(-2).unite(tail, -2);
    for (const Series &d : defects) residual = residual.unite({d}, -1);
  } else if (options.scheme == Scheme::Trapezoid) {
    const Series m = f.add(e.mul(2.0 / h));
    const Series g = f.sub(e.mul(2.0 / h));
    const Series m_inv = dense_solve(m, eye, kStepSingular);
    const Series w = u.slice(-2, 0, steps - 1).add(u_tail);
    const Series padded = w.unite({pluss::series::zeros(timed_shape(batch, 1, n), cfg)}, -2);
    const auto system = lti::state(m_inv.matmul(g).neg(), m_inv, pluss::series::eye(n, cfg),
                                   pluss::series::zeros({n, n}, cfg), h);
    states = lti::sim(system, padded, clock, x0).values;
    const Series next = states.slice(-2, 1, steps);
    const Series prev = states.slice(-2, 0, steps - 1);
    residual = residual.unite({defect(next.matmul(m.mT()).add(prev.matmul(g.mT())).sub(w))}, -1);
  } else {
    // One backward-Euler step starts the two-step Gear2 recursion.
    const Series m1 = f.add(e.mul(1.0 / h));
    const Series rhs1 = row(u, 1).add(e.matmul(x0.unsqueeze(-1)).squeeze(-1).mul(1.0 / h));
    const Series x1 = dense_solve(m1, rhs1.unsqueeze(-1), kStepSingular).squeeze(-1);
    residual = residual.unite({defect(m1.matmul(x1.unsqueeze(-1)).squeeze(-1).sub(rhs1)).unsqueeze(-1)}, -1);
    states = x0.unsqueeze(-2).unite({x1.unsqueeze(-2)}, -2);
    if (steps > 2) {
      const Series m2 = f.add(e.mul(1.5 / h));
      const Series m2_inv = dense_solve(m2, eye, kStepSingular);
      const Series m2e = m2_inv.matmul(e);
      const Series zero = pluss::series::zeros(matrix_shape(batch, n, n), cfg);
      const Series a = cat_rows({cat_columns({m2e.mul(2.0 / h), m2e.mul(-0.5 / h)}), cat_columns({eye, zero})});
      const Series b = cat_rows({m2_inv, zero});
      const Series c = cat_columns({pluss::series::eye(n, cfg), pluss::series::zeros({n, n}, cfg)});
      const auto system = lti::state(a, b, c, pluss::series::zeros({n, n}, cfg), h);
      const Series drive_tail = u.slice(-2, 2, steps).unite({pluss::series::zeros(timed_shape(batch, 1, n), cfg)}, -2);
      const Series s1 = x1.unite({x0}, -1);
      const Series tail = lti::sim(system, drive_tail, clock.slice(0, 1, steps), s1).values;
      states = x0.unsqueeze(-2).unite({tail}, -2);
      const Series xk = states.slice(-2, 2, steps);
      const Series x1s = states.slice(-2, 1, steps - 1);
      const Series x0s = states.slice(-2, 0, steps - 2);
      const Series expect = u.slice(-2, 2, steps).add(x1s.mul(4.0).sub(x0s).matmul(e.mT()).mul(0.5 / h));
      residual = residual.unite({defect(xk.matmul(m2.mT()).sub(expect))}, -1);
    }
  }

  Result result = Stamps::unpack(circuit, st, states, solver);
  result.residual = residual;
  result.iterations = int_counts(counts, platform);
  result.refinements = int_counts(nonlinear ? refined_counts : std::vector<std::int64_t>(static_cast<std::size_t>(steps), 0), platform);
  return result;
}

} // namespace pluss::subsp::eng::circuit
