#include "pluss/subsp/eng/circuit.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
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
    throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
                                " endpoint arrays must have equal length");
  for (std::size_t i = 0; i < positive.size(); ++i) {
    const auto p = positive[i];
    const auto n = negative[i];
    if (p < 0 || p >= nodes || n < 0 || n >= nodes)
      throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
                                  " contains a node outside [0, nodes)");
    if (distinct && p == n)
      throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
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
    throw std::invalid_argument(
        std::string("eng::circuit::make: ") + role +
        " output and control arrays must have equal length");
  validate_endpoints(controls.control_positive, controls.control_negative,
                     nodes, std::string(role) + " control", false);
}

void validate_nodes(const Amps &amps, std::int64_t nodes) {
  validate_endpoints(amps.output, amps.reference, nodes, "op amps", true);
  if (amps.positive_input.size() != amps.output.size() ||
      amps.negative_input.size() != amps.output.size())
    throw std::invalid_argument("eng::circuit::make: op-amp output and input "
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
      throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
                                  " has values but no endpoints");
    return;
  }
  const Series &value = branches.value;
  if (!value.defined() || value.dim() < 1 || value.size(-1) != branches_count ||
      !value.dtype().is_floating())
    throw std::invalid_argument(
        std::string("eng::circuit::make: ") + role +
        " values must be real floating [..., elements]");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
                                " values must be finite");
  if (positive && !value.gt(0.0).all().item<bool>())
    throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
                                " values must be positive");
  if (nonnegative && !value.ge(0.0).all().item<bool>())
    throw std::invalid_argument(std::string("eng::circuit::make: ") + role +
                                " values must be non-negative");

  if (!have_config) {
    dtype = value.scalar_type();
    platform = value.platform();
    have_config = true;
  } else if (dtype != value.scalar_type() || platform != value.platform()) {
    throw std::invalid_argument(
        "eng::circuit::make: every value group must share dtype and platform");
  }

  const Shape current = leading_shape(value);
  if (!current.empty()) {
    if (batch.empty())
      batch = current;
    else if (batch != current)
      throw std::invalid_argument("eng::circuit::make: non-scalar parameter "
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
          defect};
}

} // namespace

Circuit make(const Net &net) {
  if (net.nodes < 2)
    throw std::invalid_argument("eng::circuit::make: nodes must include ground "
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
  if (!have_config)
    throw std::invalid_argument(
        "eng::circuit::make: at least one non-empty element group is required");

  return Circuit(net.nodes, batch, dtype,
                 incidence(net.resistors, net.nodes, dtype, platform),
                 values(net.resistors, batch, dtype, platform),
                 incidence(net.capacitors, net.nodes, dtype, platform),
                 values(net.capacitors, batch, dtype, platform),
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
                 count(net.vccs), count(net.vcvs), count(net.amps));
}

Result op(const Circuit &circuit) {
  const auto dtype = circuit.dtype_;
  const auto platform = circuit.platform();
  const Shape &batch = circuit.batch_shape_;
  const auto reduced_nodes = circuit.nodes_ - 1;
  const auto branch_count = circuit.voltage_count_ + circuit.inductor_count_ +
                            circuit.vcvs_count_ + circuit.amp_count_;

  const Series conductance = circuit.resistance_.reciprocal();
  Series nodal = laplace(circuit.resistor_incidence_, conductance,
                         circuit.nodes_, batch, dtype, platform);
  nodal = nodal.add(controlled_nodal(
      circuit.vccs_output_incidence_, circuit.vccs_control_incidence_,
      circuit.vccs_gain_, circuit.nodes_, batch, dtype, platform));
  const Series rhs_node = inject(circuit.current_incidence_, circuit.current_,
                                 circuit.nodes_, batch, dtype, platform)
                              .neg();

  const Series source_kcl =
      batch_matrix(circuit.voltage_incidence_.to(dtype), batch);
  const Series inductor_kcl =
      batch_matrix(circuit.inductor_incidence_.to(dtype), batch);
  const Series vcvs_kcl =
      batch_matrix(circuit.vcvs_output_incidence_.to(dtype), batch);
  const Series amp_kcl =
      batch_matrix(circuit.amp_output_incidence_.to(dtype), batch);
  const Series vcvs_control =
      batch_matrix(circuit.vcvs_control_incidence_.to(dtype), batch);
  const Series vcvs_constraint =
      vcvs_kcl.sub(vcvs_control.mul(circuit.vcvs_gain_.unsqueeze(-2)));
  const Series amp_constraint =
      batch_matrix(circuit.amp_input_incidence_.to(dtype), batch);
  const Series branch_kcl =
      combine_last({&source_kcl, &inductor_kcl, &vcvs_kcl, &amp_kcl}, batch,
                   reduced_nodes, dtype, platform);
  const Series branch_constraint = combine_last(
      {&source_kcl, &inductor_kcl, &vcvs_constraint, &amp_constraint}, batch,
      reduced_nodes, dtype, platform);

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
  const Series vccs_control =
      batch_matrix(circuit.vccs_control_incidence_.to(dtype), batch);
  return solve_system(circuit.nodes_, circuit.voltage_count_,
                      circuit.inductor_count_, circuit.vccs_count_,
                      circuit.vcvs_count_, circuit.amp_count_, branch_kcl,
                      branch_constraint, nodal, rhs_node, impedance, rhs_branch,
                      vccs_control, circuit.vccs_gain_, batch, dtype, platform);
}

Result ac(const Circuit &circuit, const Series &omega) {
  if (!omega.defined() || omega.dim() != 1 || !omega.dtype().is_floating() ||
      omega.numel() == 0 || !omega.isfinite().all().item<bool>() ||
      !omega.ge(0.0).all().item<bool>())
    throw std::invalid_argument("eng::circuit::ac: omega must be a non-empty "
                                "finite non-negative real vector");
  if (omega.platform() != circuit.platform())
    throw std::invalid_argument(
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

  const Series vccs_control =
      batch_matrix(circuit.vccs_control_incidence_.to(complex_dtype), batch);
  return solve_system(circuit.nodes_, circuit.voltage_count_,
                      circuit.inductor_count_, circuit.vccs_count_,
                      circuit.vcvs_count_, circuit.amp_count_, branch_kcl,
                      branch_constraint, nodal, rhs_node, impedance, rhs_branch,
                      vccs_control, vccs_gain, batch, complex_dtype, platform);
}

} // namespace pluss::subsp::eng::circuit
