#include "pluss/subsp/eng/fluid.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::fluid {
namespace {

using Shape = std::vector<std::int64_t>;

constexpr double pi = 3.141592653589793238462643383279502884;

Config config(ScalarType dtype, Platform platform) {
  return Config{}.with_dtype(dtype).with_platform(platform);
}

Shape leading_shape(const Series &value) {
  Shape shape(value.sizes().begin(), value.sizes().end());
  shape.pop_back();
  return shape;
}

Shape vector_shape(const Shape &batch, std::int64_t width) {
  Shape shape = batch;
  shape.push_back(width);
  return shape;
}

Shape matrix_shape(const Shape &batch, std::int64_t rows,
                   std::int64_t columns) {
  Shape shape = batch;
  shape.push_back(rows);
  shape.push_back(columns);
  return shape;
}

Series static_matrix(const std::vector<double> &values, std::int64_t rows,
                     std::int64_t columns, ScalarType dtype,
                     Platform platform) {
  return pluss::series::make(values, {rows, columns}).to(dtype).to(platform);
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

Series matvec(const Series &matrix, const Series &vector) {
  if (matrix.scalar_type() != vector.scalar_type())
    throw std::logic_error(
        "eng::fluid: internal matrix/vector dtype mismatch for [" +
        std::to_string(matrix.size(-2)) + ", " +
        std::to_string(matrix.size(-1)) + "] x [" +
        std::to_string(vector.size(-1)) +
        "]: " + std::string(dtype_of(matrix.scalar_type()).name) + " versus " +
        std::string(dtype_of(vector.scalar_type()).name));
  return matrix.matmul(vector.unsqueeze(-1)).squeeze(-1);
}

Series batch_diag(const Series &vector) {
  const Series identity = pluss::series::eye(
      vector.size(-1), config(vector.scalar_type(), vector.platform()));
  return vector.unsqueeze(-1).mul(identity);
}

void inspect(const Series &value, std::int64_t width, const char *role,
             bool positive, ScalarType &dtype, Platform &platform,
             bool &have_config, Shape &batch) {
  if (!value.defined() || value.dim() < 1 || value.size(-1) != width ||
      !value.dtype().is_floating())
    throw std::invalid_argument(std::string("eng::fluid::network: ") + role +
                                " must be real floating [..., elements]");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string("eng::fluid::network: ") + role +
                                " must be finite");
  if (positive && !value.gt(0.0).all().item<bool>())
    throw std::invalid_argument(std::string("eng::fluid::network: ") + role +
                                " must be positive");

  if (!have_config) {
    dtype = value.scalar_type();
    platform = value.platform();
    have_config = true;
  } else if (dtype != value.scalar_type() || platform != value.platform()) {
    throw std::invalid_argument(
        "eng::fluid::network: all physical values must share dtype and "
        "platform");
  }

  const Shape current = leading_shape(value);
  if (!current.empty()) {
    if (batch.empty())
      batch = current;
    else if (batch != current)
      throw std::invalid_argument(
          "eng::fluid::network: non-scalar batches must have equal leading "
          "shapes");
  }
}

Series expand(const Series &value, const Shape &batch, std::int64_t width) {
  if (leading_shape(value) == batch)
    return value;
  Shape aligned(batch.size(), 1);
  aligned.push_back(width);
  return value.reshape(aligned).expand(vector_shape(batch, width), true);
}

Series optional(const Series &value, const Shape &batch, std::int64_t width,
                double fill, ScalarType dtype, Platform platform) {
  if (value.defined())
    return expand(value, batch, width);
  return pluss::series::full(vector_shape(batch, width), fill,
                             config(dtype, platform));
}

Series join_last(const std::vector<Series> &parts) {
  if (parts.empty())
    throw std::logic_error("eng::fluid: cannot join an empty link family");
  Series out = parts.front();
  for (std::size_t index = 1; index < parts.size(); ++index)
    out = out.unite({parts[index]}, -1);
  return out;
}

void validate_topology(const std::vector<std::int64_t> &from,
                       const std::vector<std::int64_t> &to,
                       const std::vector<NodeKind> &kind, std::int64_t nodes) {
  const auto links = static_cast<std::int64_t>(from.size());
  if (nodes < 2)
    throw std::invalid_argument(
        "eng::fluid::network: at least two nodes are required");
  if (links < 1)
    throw std::invalid_argument(
        "eng::fluid::network: at least one link is required");
  if (to.size() != from.size())
    throw std::invalid_argument(
        "eng::fluid::network: link endpoint arrays must have equal length");

  std::vector<std::vector<std::int64_t>> adjacency(
      static_cast<std::size_t>(nodes));
  for (std::int64_t link = 0; link < links; ++link) {
    const auto first = from[static_cast<std::size_t>(link)];
    const auto second = to[static_cast<std::size_t>(link)];
    if (first < 0 || first >= nodes || second < 0 || second >= nodes)
      throw std::invalid_argument(
          "eng::fluid::network: link endpoint is outside the node range");
    if (first == second)
      throw std::invalid_argument(
          "eng::fluid::network: a link cannot join a node to itself");
    adjacency[static_cast<std::size_t>(first)].push_back(second);
    adjacency[static_cast<std::size_t>(second)].push_back(first);
  }

  std::vector<bool> visited(static_cast<std::size_t>(nodes), false);
  for (std::int64_t seed = 0; seed < nodes; ++seed) {
    if (visited[static_cast<std::size_t>(seed)])
      continue;
    bool has_fixed_grade = false;
    std::queue<std::int64_t> work;
    work.push(seed);
    visited[static_cast<std::size_t>(seed)] = true;
    while (!work.empty()) {
      const auto node = work.front();
      work.pop();
      has_fixed_grade =
          has_fixed_grade ||
          kind[static_cast<std::size_t>(node)] != NodeKind::Junction;
      for (const auto next : adjacency[static_cast<std::size_t>(node)]) {
        if (!visited[static_cast<std::size_t>(next)]) {
          visited[static_cast<std::size_t>(next)] = true;
          work.push(next);
        }
      }
    }
    if (!has_fixed_grade)
      throw std::invalid_argument(
          "eng::fluid::network: every connected component needs a reservoir "
          "or tank");
  }
}

struct LossEval {
  Series loss;
  Series gradient;
  Series friction;
};

LossEval darcy_loss(const Series &length, const Series &diameter,
                    const Series &roughness, const Series &minor_loss,
                    const Liquid &liquid, ScalarType dtype, const Series &flow,
                    double min_gradient) {
  // EPANET's Darcy-Weisbach partition uses w = |Q|/(nu D), with
  // Re = 4 w / pi.  The cubic 2000--4000 transition avoids a discontinuous
  // Newton derivative while retaining Swamee--Jain above the transition.
  constexpr double a1 = 1000.0 * pi;
  constexpr double a2 = 500.0 * pi;
  constexpr double a8 = 4.6184131985906665869;
  constexpr double a9 = -0.8685889638065036553;
  constexpr double ab = 0.0032889547634539906;
  constexpr double ac = -0.00514214965799059;

  const Series abs_flow = flow.abs();
  const double tiny = dtype == ScalarType::Float32 ? 1e-20 : 1e-30;
  const Series safe_flow = abs_flow.clamp_min(tiny);
  const Series w = safe_flow.div(liquid.kinematic_viscosity).div(diameter);
  const Series reynolds = w.mul(4.0 / pi);

  const Series base =
      length.mul(8.0 / (pi * pi * liquid.gravity)).div(diameter.pow(5.0));
  const Series minor =
      minor_loss.mul(8.0 / (pi * pi * liquid.gravity)).div(diameter.pow(4.0));

  const Series laminar_resistance =
      length.mul(128.0 * liquid.kinematic_viscosity / (pi * liquid.gravity))
          .div(diameter.pow(4.0));
  const Series laminar_loss =
      flow.mul(laminar_resistance.add(minor.mul(abs_flow)));
  const Series laminar_gradient =
      laminar_resistance.add(minor.mul(abs_flow).mul(2.0));
  const Series laminar_friction = abs_flow.gt(0.0).where(
      reynolds.reciprocal().mul(64.0), pluss::series::zeros_like(flow));

  const Series relative = roughness.div(diameter.mul(3.7));
  const Series y1 = w.pow(-0.9).mul(a8);
  const Series y2 = relative.add(y1);
  const Series y3 = y2.log().mul(a9);
  const Series turbulent_friction = y3.square().reciprocal();
  const Series turbulent_df =
      turbulent_friction.mul(y1).mul(1.8 * a9).div(y2.mul(y3).mul(safe_flow));

  const Series transition_y2 = relative.add(ab);
  const Series transition_y3 = transition_y2.log().mul(a9);
  const Series fa = transition_y3.square().reciprocal();
  const Series fb =
      transition_y2.mul(transition_y3).reciprocal().mul(ac).add(2.0).mul(fa);
  const Series ratio = w.div(a2);
  const Series x1 = fa.mul(7.0).sub(fb);
  const Series x2 = fa.mul(-17.0).add(fb.mul(2.5)).add(0.128);
  const Series x3 = fa.mul(13.0).sub(fb.mul(2.0)).sub(0.128);
  const Series x4 = fa.mul(-3.0).add(fb.mul(0.5)).add(0.032);
  const Series transition_friction =
      x1.add(ratio.mul(x2.add(ratio.mul(x3.add(ratio.mul(x4))))));
  const Series transition_df =
      x2.add(ratio.mul(x3.mul(2.0).add(ratio.mul(x4).mul(3.0))))
          .div(liquid.kinematic_viscosity * a2)
          .div(diameter);

  const Series upper = w.ge(a1);
  const Series friction = upper.where(turbulent_friction, transition_friction);
  const Series df = upper.where(turbulent_df, transition_df);
  const Series coefficient = friction.mul(base).add(minor);
  const Series nonlaminar_loss = flow.mul(abs_flow).mul(coefficient);
  const Series nonlaminar_gradient = coefficient.mul(abs_flow).mul(2.0).add(
      df.mul(base).mul(abs_flow.square()));

  const Series laminar = w.le(a2);
  return {
      laminar.where(laminar_loss, nonlaminar_loss).to(dtype),
      laminar.where(laminar_gradient, nonlaminar_gradient)
          .clamp_min(min_gradient)
          .to(dtype),
      laminar.where(laminar_friction, friction).to(dtype),
  };
}

LossEval hazen_loss(const Series &length, const Series &diameter,
                    const Series &roughness, const Series &minor_loss,
                    const Liquid &liquid, ScalarType dtype, const Series &flow,
                    double min_gradient) {
  const Series abs_flow = flow.abs();
  const double tiny = dtype == ScalarType::Float32 ? 1e-20 : 1e-30;
  const Series powered = abs_flow.clamp_min(tiny).pow(0.852);
  const Series resistance =
      length.mul(10.67).div(roughness.pow(1.852)).div(diameter.pow(4.8704));
  const Series minor =
      minor_loss.mul(8.0 / (pi * pi * liquid.gravity)).div(diameter.pow(4.0));
  const Series loss =
      flow.mul(resistance.mul(powered).add(minor.mul(abs_flow)));
  const Series gradient = resistance.mul(powered)
                              .mul(1.852)
                              .add(minor.mul(abs_flow).mul(2.0))
                              .clamp_min(min_gradient);
  return {loss.to(dtype), gradient.to(dtype), pluss::series::zeros_like(flow)};
}

LossEval loss(const Series &length, const Series &diameter,
              const Series &roughness, const Series &minor,
              const Liquid &liquid, ScalarType dtype, const Series &flow,
              const Options &options) {
  return options.loss == LossModel::DarcyWeisbach
             ? darcy_loss(length, diameter, roughness, minor, liquid, dtype,
                          flow, options.min_gradient)
             : hazen_loss(length, diameter, roughness, minor, liquid, dtype,
                          flow, options.min_gradient);
}

struct DemandEval {
  Series demand;
  Series emitter;
  Series gradient;
};

DemandEval demand(const Series &base, const Series &coefficient,
                  const Series &head, const Series &elevation,
                  const Options &options) {
  const Series pressure = head.sub(elevation);
  const Series zero = pluss::series::zeros_like(base);
  Series delivered = base;
  Series demand_gradient = zero;
  if (options.demand_mode == DemandMode::Pressure) {
    const double span = options.full_pressure - options.min_pressure;
    const Series positive = base.gt(0.0);
    const Series interior =
        pressure.gt(options.min_pressure)
            .logical_and(pressure.lt(options.full_pressure));
    const Series ratio =
        pressure.sub(options.min_pressure).div(span).clamp(1e-30, 1.0);
    const Series partial = base.mul(ratio.pow(options.demand_exponent));
    const Series pressure_demand =
        pressure.ge(options.full_pressure)
            .where(base, interior.where(partial, zero));
    delivered = positive.where(pressure_demand, base);
    demand_gradient = positive.logical_and(interior).where(
        base.mul(options.demand_exponent / span)
            .mul(ratio.pow(options.demand_exponent - 1.0)),
        zero);
  }

  const Series positive_pressure = pressure.gt(0.0);
  const Series safe_pressure = pressure.clamp_min(1e-30);
  const Series emitter = positive_pressure.where(
      coefficient.mul(safe_pressure.pow(options.emitter_exponent)), zero);
  const Series emitter_gradient = positive_pressure.where(
      coefficient.mul(options.emitter_exponent)
          .mul(safe_pressure.pow(options.emitter_exponent - 1.0)),
      zero);
  return {delivered.to(base.scalar_type()), emitter.to(base.scalar_type()),
          demand_gradient.add(emitter_gradient).to(base.scalar_type())};
}

Series stack0(const std::vector<Series> &values) {
  if (values.empty())
    throw std::logic_error("eng::fluid: cannot stack an empty trajectory");
  Series out = values.front().unsqueeze(0);
  for (std::size_t i = 1; i < values.size(); ++i)
    out = out.unite({values[i].unsqueeze(0)}, 0);
  return out;
}

Series local_mask(std::int64_t width, std::int64_t index, Platform platform) {
  std::vector<std::uint8_t> values(static_cast<std::size_t>(width), 0);
  values[static_cast<std::size_t>(index)] = 1;
  return pluss::series::make(values).to(ScalarType::Bool).to(platform);
}

Series pattern(const Series &values, std::int64_t width, double time,
               double step, ScalarType dtype, Platform platform,
               const Series &fallback) {
  if (!values.defined())
    return fallback;
  const auto periods = values.size(0);
  const auto period =
      static_cast<std::int64_t>(std::floor(time / step)) % periods;
  return values.select(0, period).to(dtype).to(platform);
}

void inspect_pattern(const Series &values, std::int64_t width, const char *role,
                     bool nonnegative) {
  if (!values.defined())
    return;
  if (values.dim() != 2 || values.size(0) < 1 || values.size(1) != width ||
      !values.dtype().is_floating())
    throw std::invalid_argument(std::string("eng::fluid::run: ") + role +
                                " must be real floating [period, element]");
  if (!values.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string("eng::fluid::run: ") + role +
                                " must be finite");
  if (nonnegative && !values.ge(0.0).all().item<bool>())
    throw std::invalid_argument(std::string("eng::fluid::run: ") + role +
                                " must be nonnegative");
}

} // namespace

Network network(const Net &net, const Liquid &liquid) {
  const auto nodes = static_cast<std::int64_t>(net.nodes.kind.size());
  const auto pipes = static_cast<std::int64_t>(net.pipes.from.size());
  const auto pumps = static_cast<std::int64_t>(net.pumps.from.size());
  const auto valves = static_cast<std::int64_t>(net.valves.from.size());
  const auto links = pipes + pumps + valves;
  if (net.pipes.to.size() != net.pipes.from.size() ||
      net.pumps.to.size() != net.pumps.from.size() ||
      net.valves.to.size() != net.valves.from.size() ||
      net.valves.kind.size() != net.valves.from.size())
    throw std::invalid_argument(
        "eng::fluid::network: each link family needs matching metadata");
  std::vector<std::int64_t> from = net.pipes.from;
  std::vector<std::int64_t> to = net.pipes.to;
  from.insert(from.end(), net.pumps.from.begin(), net.pumps.from.end());
  from.insert(from.end(), net.valves.from.begin(), net.valves.from.end());
  to.insert(to.end(), net.pumps.to.begin(), net.pumps.to.end());
  to.insert(to.end(), net.valves.to.begin(), net.valves.to.end());
  if (!std::isfinite(liquid.density) || liquid.density <= 0.0 ||
      !std::isfinite(liquid.kinematic_viscosity) ||
      liquid.kinematic_viscosity <= 0.0 || !std::isfinite(liquid.gravity) ||
      liquid.gravity <= 0.0)
    throw std::invalid_argument(
        "eng::fluid::network: liquid properties must be finite and positive");
  validate_topology(from, to, net.nodes.kind, nodes);

  std::int64_t junctions = 0;
  std::int64_t reservoirs = 0;
  std::int64_t tanks = 0;
  for (const auto kind : net.nodes.kind) {
    if (kind == NodeKind::Junction)
      ++junctions;
    else if (kind == NodeKind::Reservoir)
      ++reservoirs;
    else if (kind == NodeKind::Tank)
      ++tanks;
    else
      throw std::invalid_argument(
          "eng::fluid::network: node kind is not recognized");
  }
  if (junctions == 0 || reservoirs + tanks == 0)
    throw std::invalid_argument(
        "eng::fluid::network: at least one junction and one reservoir or "
        "tank are required");
  if (tanks != static_cast<std::int64_t>(net.tanks.overflow.size()))
    throw std::invalid_argument(
        "eng::fluid::network: tank overflow metadata must match tank nodes");

  ScalarType dtype = ScalarType::Float64;
  Platform platform = Platform::CPU;
  bool have_config = false;
  Shape batch;
  inspect(net.nodes.elevation, nodes, "node elevation", false, dtype, platform,
          have_config, batch);
  inspect(net.nodes.demand, nodes, "node demand", false, dtype, platform,
          have_config, batch);
  inspect(net.nodes.head, nodes, "node head", false, dtype, platform,
          have_config, batch);
  if (net.nodes.emitter.defined())
    inspect(net.nodes.emitter, nodes, "node emitter coefficient", false, dtype,
            platform, have_config, batch);
  if (tanks > 0) {
    inspect(net.tanks.area, tanks, "tank area", true, dtype, platform,
            have_config, batch);
    inspect(net.tanks.min_level, tanks, "tank minimum level", false, dtype,
            platform, have_config, batch);
    inspect(net.tanks.max_level, tanks, "tank maximum level", false, dtype,
            platform, have_config, batch);
  }
  if (pipes > 0) {
    inspect(net.pipes.length, pipes, "pipe length", true, dtype, platform,
            have_config, batch);
    inspect(net.pipes.diameter, pipes, "pipe diameter", true, dtype, platform,
            have_config, batch);
    inspect(net.pipes.roughness, pipes, "pipe roughness", false, dtype,
            platform, have_config, batch);
    if (net.pipes.minor.defined())
      inspect(net.pipes.minor, pipes, "pipe minor-loss coefficient", false,
              dtype, platform, have_config, batch);
  }
  if (pumps > 0) {
    inspect(net.pumps.shutoff_head, pumps, "pump shutoff head", true, dtype,
            platform, have_config, batch);
    inspect(net.pumps.resistance, pumps, "pump curve resistance", true, dtype,
            platform, have_config, batch);
    inspect(net.pumps.exponent, pumps, "pump curve exponent", true, dtype,
            platform, have_config, batch);
    if (net.pumps.speed.defined())
      inspect(net.pumps.speed, pumps, "pump relative speed", true, dtype,
              platform, have_config, batch);
  }
  if (valves > 0) {
    inspect(net.valves.diameter, valves, "valve diameter", true, dtype,
            platform, have_config, batch);
    inspect(net.valves.minor, valves, "valve minor-loss coefficient", false,
            dtype, platform, have_config, batch);
    inspect(net.valves.setting, valves, "valve setting", false, dtype, platform,
            have_config, batch);
  }

  Series elevation = expand(net.nodes.elevation, batch, nodes);
  Series demand = expand(net.nodes.demand, batch, nodes);
  Series head = expand(net.nodes.head, batch, nodes);
  Series emitter =
      optional(net.nodes.emitter, batch, nodes, 0.0, dtype, platform);
  Series tank_area = tanks > 0 ? expand(net.tanks.area, batch, tanks)
                               : pluss::series::zeros(vector_shape(batch, 0),
                                                      config(dtype, platform));
  Series tank_min = tanks > 0 ? expand(net.tanks.min_level, batch, tanks)
                              : pluss::series::zeros(vector_shape(batch, 0),
                                                     config(dtype, platform));
  Series tank_max = tanks > 0 ? expand(net.tanks.max_level, batch, tanks)
                              : pluss::series::zeros(vector_shape(batch, 0),
                                                     config(dtype, platform));
  if (!emitter.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::fluid::network: emitter coefficients must be nonnegative");
  if (tanks > 0 && !tank_max.gt(tank_min).all().item<bool>())
    throw std::invalid_argument(
        "eng::fluid::network: tank maximum level must exceed minimum level");
  const auto fill = [&](std::int64_t count, double value) {
    return pluss::series::full(vector_shape(batch, count), value,
                               config(dtype, platform));
  };
  std::vector<Series> length_parts, diameter_parts, roughness_parts,
      minor_parts;
  std::vector<Series> pump_head_parts, pump_r_parts, pump_n_parts,
      pump_speed_parts, valve_diameter_parts, valve_minor_parts,
      valve_setting_parts;
  if (pipes > 0) {
    length_parts.push_back(expand(net.pipes.length, batch, pipes));
    diameter_parts.push_back(expand(net.pipes.diameter, batch, pipes));
    roughness_parts.push_back(expand(net.pipes.roughness, batch, pipes));
    minor_parts.push_back(
        optional(net.pipes.minor, batch, pipes, 0.0, dtype, platform));
    pump_head_parts.push_back(fill(pipes, 0.0));
    pump_r_parts.push_back(fill(pipes, 0.0));
    pump_n_parts.push_back(fill(pipes, 1.0));
    pump_speed_parts.push_back(fill(pipes, 1.0));
    valve_diameter_parts.push_back(fill(pipes, 1.0));
    valve_minor_parts.push_back(fill(pipes, 0.0));
    valve_setting_parts.push_back(fill(pipes, 0.0));
  }
  if (pumps > 0) {
    length_parts.push_back(fill(pumps, 1.0));
    diameter_parts.push_back(fill(pumps, 1.0));
    roughness_parts.push_back(fill(pumps, 0.0));
    minor_parts.push_back(fill(pumps, 0.0));
    pump_head_parts.push_back(expand(net.pumps.shutoff_head, batch, pumps));
    pump_r_parts.push_back(expand(net.pumps.resistance, batch, pumps));
    pump_n_parts.push_back(expand(net.pumps.exponent, batch, pumps));
    pump_speed_parts.push_back(
        optional(net.pumps.speed, batch, pumps, 1.0, dtype, platform));
    valve_diameter_parts.push_back(fill(pumps, 1.0));
    valve_minor_parts.push_back(fill(pumps, 0.0));
    valve_setting_parts.push_back(fill(pumps, 0.0));
  }
  if (valves > 0) {
    length_parts.push_back(fill(valves, 1.0));
    diameter_parts.push_back(expand(net.valves.diameter, batch, valves));
    roughness_parts.push_back(fill(valves, 0.0));
    minor_parts.push_back(fill(valves, 0.0));
    pump_head_parts.push_back(fill(valves, 0.0));
    pump_r_parts.push_back(fill(valves, 0.0));
    pump_n_parts.push_back(fill(valves, 1.0));
    pump_speed_parts.push_back(fill(valves, 1.0));
    valve_diameter_parts.push_back(expand(net.valves.diameter, batch, valves));
    valve_minor_parts.push_back(expand(net.valves.minor, batch, valves));
    valve_setting_parts.push_back(expand(net.valves.setting, batch, valves));
  }
  Series length = join_last(length_parts);
  Series diameter = join_last(diameter_parts);
  Series roughness = join_last(roughness_parts);
  Series minor = join_last(minor_parts);
  Series pump_head = join_last(pump_head_parts);
  Series pump_resistance = join_last(pump_r_parts);
  Series pump_exponent = join_last(pump_n_parts);
  Series pump_speed = join_last(pump_speed_parts);
  Series valve_diameter = join_last(valve_diameter_parts);
  Series valve_minor = join_last(valve_minor_parts);
  Series valve_setting = join_last(valve_setting_parts);
  if (!minor.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::fluid::network: minor-loss coefficients must be nonnegative");
  if (!roughness.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::fluid::network: roughness must be nonnegative");
  if (!valve_minor.ge(0.0).all().item<bool>() ||
      !valve_setting.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::fluid::network: valve loss and settings must be nonnegative");

  std::vector<double> incidence_values(static_cast<std::size_t>(nodes * links),
                                       0.0);
  std::vector<double> energy_values(static_cast<std::size_t>(links * nodes),
                                    0.0);
  std::vector<double> normal_values(energy_values.size(), 0.0);
  std::vector<double> control_values(energy_values.size(), 0.0);
  std::vector<double> pump_mask_values(static_cast<std::size_t>(links), 0.0);
  std::vector<double> valve_mask_values(static_cast<std::size_t>(links), 0.0);
  std::vector<double> flow_mask_values(static_cast<std::size_t>(links), 0.0);
  std::vector<double> pressure_mask_values(static_cast<std::size_t>(links),
                                           0.0);
  for (std::int64_t link = 0; link < links; ++link) {
    const auto first = from[static_cast<std::size_t>(link)];
    const auto second = to[static_cast<std::size_t>(link)];
    incidence_values[static_cast<std::size_t>(first * links + link)] = -1.0;
    incidence_values[static_cast<std::size_t>(second * links + link)] = 1.0;
    energy_values[static_cast<std::size_t>(link * nodes + first)] = 1.0;
    energy_values[static_cast<std::size_t>(link * nodes + second)] = -1.0;
    bool controlled = false;
    if (link >= pipes + pumps) {
      const auto valve = link - pipes - pumps;
      const auto kind = net.valves.kind[static_cast<std::size_t>(valve)];
      valve_mask_values[static_cast<std::size_t>(link)] = 1.0;
      if (kind == ValveKind::Flow) {
        flow_mask_values[static_cast<std::size_t>(link)] = 1.0;
        controlled = true;
      } else if (kind == ValveKind::Reduce || kind == ValveKind::Sustain) {
        const auto node = kind == ValveKind::Reduce ? second : first;
        if (net.nodes.kind[static_cast<std::size_t>(node)] !=
            NodeKind::Junction)
          throw std::invalid_argument(
              "eng::fluid::network: a pressure-control valve must regulate "
              "a junction node");
        control_values[static_cast<std::size_t>(link * nodes + node)] = 1.0;
        pressure_mask_values[static_cast<std::size_t>(link)] = 1.0;
        controlled = true;
      } else if (kind != ValveKind::Throttle) {
        throw std::invalid_argument(
            "eng::fluid::network: valve kind is not recognized");
      }
    } else if (link >= pipes) {
      pump_mask_values[static_cast<std::size_t>(link)] = 1.0;
    }
    if (!controlled) {
      normal_values[static_cast<std::size_t>(link * nodes + first)] = 1.0;
      normal_values[static_cast<std::size_t>(link * nodes + second)] = -1.0;
    }
  }

  std::vector<double> pick_values(static_cast<std::size_t>(junctions * nodes),
                                  0.0);
  std::vector<double> put_values(static_cast<std::size_t>(nodes * junctions),
                                 0.0);
  std::vector<double> fixed_values(static_cast<std::size_t>(nodes * nodes),
                                   0.0);
  std::vector<double> tank_pick_values(static_cast<std::size_t>(tanks * nodes),
                                       0.0);
  std::vector<double> tank_put_values(static_cast<std::size_t>(nodes * tanks),
                                      0.0);
  std::int64_t junction = 0;
  std::int64_t tank = 0;
  std::vector<std::int64_t> tank_of_node(static_cast<std::size_t>(nodes), -1);
  for (std::int64_t node = 0; node < nodes; ++node) {
    if (net.nodes.kind[static_cast<std::size_t>(node)] == NodeKind::Junction) {
      pick_values[static_cast<std::size_t>(junction * nodes + node)] = 1.0;
      put_values[static_cast<std::size_t>(node * junctions + junction)] = 1.0;
      ++junction;
    } else {
      fixed_values[static_cast<std::size_t>(node * nodes + node)] = 1.0;
      if (net.nodes.kind[static_cast<std::size_t>(node)] == NodeKind::Tank) {
        tank_of_node[static_cast<std::size_t>(node)] = tank;
        tank_pick_values[static_cast<std::size_t>(tank * nodes + node)] = 1.0;
        tank_put_values[static_cast<std::size_t>(node * tanks + tank)] = 1.0;
        ++tank;
      }
    }
  }

  const Series incidence =
      static_matrix(incidence_values, nodes, links, dtype, platform);
  const Series energy =
      static_matrix(energy_values, links, nodes, dtype, platform);
  const Series junction_pick =
      static_matrix(pick_values, junctions, nodes, dtype, platform);
  const Series junction_put =
      static_matrix(put_values, nodes, junctions, dtype, platform);
  const Series fixed_put =
      static_matrix(fixed_values, nodes, nodes, dtype, platform);
  const Series tank_pick =
      static_matrix(tank_pick_values, tanks, nodes, dtype, platform);
  const Series tank_put =
      static_matrix(tank_put_values, nodes, tanks, dtype, platform);

  const Series fixed_demand = matvec(fixed_put, demand);
  const Series fixed_emitter = matvec(fixed_put, emitter);
  if (!fixed_demand.eq(0.0).all().item<bool>() ||
      !fixed_emitter.eq(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::fluid::network: reservoir and tank demand/emitter lanes must "
        "be zero");
  if (tanks > 0) {
    const Series tank_level = matvec(tank_pick, head.sub(elevation));
    if (!tank_level.ge(tank_min)
             .logical_and(tank_level.le(tank_max))
             .all()
             .item<bool>())
      throw std::invalid_argument(
          "eng::fluid::network: initial tank level must be within bounds");
  }

  std::vector<double> tank_overflow_values(static_cast<std::size_t>(tanks),
                                           0.0);
  for (std::int64_t i = 0; i < tanks; ++i)
    tank_overflow_values[static_cast<std::size_t>(i)] =
        net.tanks.overflow[static_cast<std::size_t>(i)] ? 1.0 : 0.0;

  return Network(
      nodes, pipes, pumps, valves, junctions, reservoirs, tanks, net.nodes.kind,
      std::move(tank_of_node), net.valves.kind, std::move(batch), dtype, liquid,
      std::move(elevation), std::move(demand), std::move(head),
      std::move(emitter), std::move(tank_area), std::move(tank_min),
      std::move(tank_max),
      pluss::series::make(tank_overflow_values).to(dtype).to(platform),
      tank_pick, tank_put, std::move(length), std::move(diameter),
      std::move(roughness), std::move(minor), incidence, energy, junction_pick,
      junction_put, fixed_put,
      pluss::series::make(pump_mask_values).to(dtype).to(platform),
      pluss::series::make(valve_mask_values).to(dtype).to(platform),
      std::move(pump_head), std::move(pump_resistance),
      std::move(pump_exponent), std::move(pump_speed),
      std::move(valve_diameter), std::move(valve_minor),
      std::move(valve_setting),
      static_matrix(normal_values, links, nodes, dtype, platform),
      static_matrix(control_values, links, nodes, dtype, platform),
      pluss::series::make(flow_mask_values).to(dtype).to(platform),
      pluss::series::make(pressure_mask_values).to(dtype).to(platform),
      pluss::series::zeros(vector_shape(batch, links),
                           config(dtype, platform)));
}

SteadyResult steady(const Network &network, const Options &options) {
  if (!std::isfinite(options.head_tolerance) || options.head_tolerance <= 0.0 ||
      !std::isfinite(options.flow_tolerance) || options.flow_tolerance <= 0.0 ||
      !std::isfinite(options.min_gradient) || options.min_gradient <= 0.0 ||
      !std::isfinite(options.min_pressure) ||
      !std::isfinite(options.full_pressure) ||
      options.full_pressure <= options.min_pressure ||
      !std::isfinite(options.demand_exponent) ||
      options.demand_exponent <= 0.0 ||
      !std::isfinite(options.emitter_exponent) ||
      options.emitter_exponent <= 0.0 || options.max_iterations <= 0)
    throw std::invalid_argument(
        "eng::fluid::steady: pressure bounds, exponents, tolerances, gradient "
        "floor, and iteration limit are invalid");
  const Series one = pluss::series::ones_like(network.pump_mask_);
  const Series pipe_mask = one.sub(network.pump_mask_).sub(network.valve_mask_);
  const Series normal_mask =
      one.sub(network.flow_valve_mask_).sub(network.pressure_valve_mask_);
  const Series closed = network.closed_mask_.gt(0.0);
  const Series open = closed.logical_not().to(network.dtype_);
  const Series active_normal = normal_mask.mul(open);
  const Series active_flow = network.flow_valve_mask_.mul(open);
  const Series active_pressure = network.pressure_valve_mask_.mul(open);
  if (options.loss == LossModel::HazenWilliams &&
      !pipe_mask.eq(0.0)
           .logical_or(network.roughness_.gt(0.0))
           .all()
           .item<bool>())
    throw std::invalid_argument(
        "eng::fluid::steady: Hazen-Williams coefficients must be positive");

  const Shape &batch = network.batch_shape_;
  const Series junction_incidence =
      network.junction_pick_.matmul(network.incidence_);
  const Series head_map =
      network.normal_head_map_.add(network.control_head_map_);
  const Series junction_energy = head_map.matmul(network.junction_put_);
  Series junction_head = matvec(network.junction_pick_, network.head_);
  const Series fixed_head = matvec(network.fixed_put_, network.head_);
  const auto evaluate_demand = [&](const Series &head) {
    const auto raw = demand(network.demand_, network.emitter_, head,
                            network.elevation_, options);
    const auto project = [&](const Series &value) {
      return matvec(network.junction_put_,
                    matvec(network.junction_pick_, value));
    };
    return DemandEval{project(raw.demand), project(raw.emitter),
                      project(raw.gradient)};
  };

  const Series initial_head =
      matvec(network.junction_put_, junction_head).add(fixed_head);
  const auto initial_demand = evaluate_demand(initial_head);
  const Series junction_demand =
      matvec(network.junction_pick_,
             initial_demand.demand.add(initial_demand.emitter));

  const Series gram = junction_incidence.matmul(junction_incidence.mT());
  Series flow =
      network.flow_guess_.defined()
          ? network.flow_guess_
          : matvec(junction_incidence.mT(),
                   gram.linearalg.lu_solve(junction_demand.unsqueeze(-1))
                       .squeeze(-1)
                       .to(network.dtype_));

  Series converged =
      pluss::series::zeros(batch, config(ScalarType::Bool, network.platform()));
  std::int64_t iterations = 0;
  const auto evaluate = [&](const Series &current_head,
                            const Series &current_flow) {
    const auto pipe = loss(network.length_, network.diameter_,
                           network.roughness_, network.minor_, network.liquid_,
                           network.dtype_, current_flow, options);
    const double tiny = network.dtype_ == ScalarType::Float32 ? 1e-20 : 1e-30;
    const Series abs_flow = current_flow.abs().clamp_min(tiny);
    const Series speed_power = network.pump_speed_.clamp_min(tiny)
                                   .log()
                                   .mul(network.pump_exponent_.neg().add(2.0))
                                   .exp();
    const Series flow_power =
        abs_flow.log().mul(network.pump_exponent_.sub(1.0)).exp();
    const Series pump_curve = network.pump_resistance_.mul(speed_power)
                                  .mul(current_flow)
                                  .mul(flow_power);
    const Series pump_loss =
        pump_curve.sub(network.pump_head_.mul(network.pump_speed_.square()));
    const Series pump_gradient = network.pump_resistance_.mul(speed_power)
                                     .mul(network.pump_exponent_)
                                     .mul(flow_power)
                                     .clamp_min(options.min_gradient);
    const Series valve_coeff =
        network.valve_minor_.mul(8.0 / (pi * pi * network.liquid_.gravity))
            .div(network.valve_diameter_.pow(4.0));
    const Series valve_loss =
        current_flow.mul(current_flow.abs()).mul(valve_coeff);
    const Series valve_gradient = current_flow.abs()
                                      .mul(valve_coeff)
                                      .mul(2.0)
                                      .clamp_min(options.min_gradient);
    const Series constitutive_loss =
        pipe.loss.mul(pipe_mask)
            .add(pump_loss.mul(network.pump_mask_))
            .add(valve_loss.mul(network.valve_mask_));
    const Series constitutive_gradient =
        pipe.gradient.mul(pipe_mask)
            .add(pump_gradient.mul(network.pump_mask_))
            .add(valve_gradient.mul(network.valve_mask_));
    const Series pressure_target =
        matvec(network.control_head_map_, network.elevation_)
            .add(network.valve_setting_);
    const Series residual =
        matvec(network.normal_head_map_, current_head)
            .sub(constitutive_loss)
            .mul(active_normal)
            .add(matvec(network.control_head_map_, current_head)
                     .sub(pressure_target)
                     .mul(active_pressure))
            .add(current_flow.sub(network.valve_setting_).mul(active_flow))
            .add(current_flow.mul(closed.to(network.dtype_)));
    const Series q_jacobian = constitutive_gradient.neg()
                                  .mul(active_normal)
                                  .add(active_flow)
                                  .add(closed.to(network.dtype_));
    return std::tuple<Series, Series, Series>{
        residual.to(network.dtype_), q_jacobian.to(network.dtype_),
        pipe.friction.mul(pipe_mask).to(network.dtype_)};
  };

  for (; iterations < options.max_iterations; ++iterations) {
    const Series head =
        matvec(network.junction_put_, junction_head).add(fixed_head);
    const auto [energy_residual, q_jacobian, friction] = evaluate(head, flow);
    const auto demand_eval = evaluate_demand(head);
    const Series junction_use = matvec(
        network.junction_pick_, demand_eval.demand.add(demand_eval.emitter));
    const Series balance = matvec(junction_incidence, flow).sub(junction_use);
    const Series residual = energy_residual.unite({balance}, -1);

    const Series head_ok =
        energy_residual.abs().amax(-1, false).le(options.head_tolerance);
    const Series flow_ok =
        balance.abs().amax(-1, false).le(options.flow_tolerance);
    converged = head_ok.logical_and(flow_ok);
    if (converged.all().item<bool>())
      break;

    const Series active_junction_energy =
        batch_matrix(junction_energy, batch).mul(open.unsqueeze(-1));
    const Series top =
        active_junction_energy.unite({batch_diag(q_jacobian)}, -1);
    const Series junction_gradient =
        matvec(network.junction_pick_, demand_eval.gradient);
    const Series bottom =
        batch_diag(junction_gradient)
            .neg()
            .unite({batch_matrix(junction_incidence, batch)}, -1);
    const Series jacobian = top.unite({bottom}, -2);
    const Series delta =
        jacobian.linearalg.lu_solve(residual.neg().unsqueeze(-1))
            .squeeze(-1)
            .to(network.dtype_);
    junction_head = junction_head.add(delta.slice(-1, 0, network.junctions_));
    flow = flow.add(delta.slice(-1, network.junctions_,
                                network.junctions_ + network.links()));
  }

  const Series head =
      matvec(network.junction_put_, junction_head).add(fixed_head);
  const auto [energy_residual, q_jacobian, friction] = evaluate(head, flow);
  const auto demand_eval = evaluate_demand(head);
  const Series total_use = demand_eval.demand.add(demand_eval.emitter);
  const Series balance = matvec(network.incidence_, flow).sub(total_use);
  const Series final_junction_demand =
      matvec(network.junction_pick_, total_use);
  converged = energy_residual.abs()
                  .amax(-1, false)
                  .le(options.head_tolerance)
                  .logical_and(matvec(junction_incidence, flow)
                                   .sub(final_junction_demand)
                                   .abs()
                                   .amax(-1, false)
                                   .le(options.flow_tolerance));
  if (options.fail_on_nonconvergence && !converged.all().item<bool>())
    throw std::runtime_error(
        "eng::fluid::steady: coupled hydraulic Newton solve did not converge");
  if (flow.lt(-options.flow_tolerance)
          .logical_and(network.pump_mask_.gt(0.0))
          .logical_and(closed.logical_not())
          .any()
          .item<bool>())
    throw std::runtime_error(
        "eng::fluid::steady: an active pump requires reverse flow");
  if (flow.lt(-options.flow_tolerance)
          .logical_and(
              network.flow_valve_mask_.add(network.pressure_valve_mask_)
                  .gt(0.0))
          .logical_and(closed.logical_not())
          .any()
          .item<bool>())
    throw std::runtime_error(
        "eng::fluid::steady: an active control valve requires reverse flow");

  const Series pressure =
      head.sub(network.elevation_)
          .mul(network.liquid_.density * network.liquid_.gravity)
          .to(network.dtype_);
  const Series velocity = flow.mul(4.0 / pi)
                              .div(network.diameter_.square())
                              .mul(one.sub(network.pump_mask_))
                              .to(network.dtype_);
  const Series head_loss = matvec(network.energy_, head);
  return {head,
          pressure,
          flow,
          velocity,
          head_loss,
          friction,
          balance,
          energy_residual,
          converged,
          demand_eval.demand,
          demand_eval.emitter,
          iterations};
}

RunResult run(const Network &network, const Schedule &schedule,
              const Options &options) {
  if (!std::isfinite(schedule.duration) || schedule.duration < 0.0 ||
      !std::isfinite(schedule.step) || schedule.step <= 0.0 ||
      !std::isfinite(schedule.pattern_step) || schedule.pattern_step <= 0.0)
    throw std::invalid_argument(
        "eng::fluid::run: duration must be nonnegative and time steps must "
        "be finite and positive");
  inspect_pattern(schedule.demand, network.nodes_, "demand pattern", true);
  inspect_pattern(schedule.pump, network.pumps_, "pump pattern", true);
  inspect_pattern(schedule.valve, network.valves_, "valve pattern", true);

  const auto validate_action = [&](const Action &action, const char *role) {
    const auto count =
        action.device == Device::Pump ? network.pumps_ : network.valves_;
    if (action.index < 0 || action.index >= count)
      throw std::invalid_argument(std::string("eng::fluid::run: ") + role +
                                  " device index is outside its family");
    if (action.act == Act::Set &&
        (!std::isfinite(action.value) || action.value < 0.0 ||
         (action.device == Device::Pump && action.value == 0.0)))
      throw std::invalid_argument(std::string("eng::fluid::run: ") + role +
                                  " setting is invalid");
  };
  for (const auto &control : schedule.time) {
    if (!std::isfinite(control.at) || control.at < 0.0 ||
        control.at > schedule.duration)
      throw std::invalid_argument(
          "eng::fluid::run: timed control lies outside the run interval");
    validate_action(control.action, "timed control");
  }
  for (const auto &control : schedule.state) {
    if (control.node < 0 || control.node >= network.nodes_ ||
        !std::isfinite(control.threshold))
      throw std::invalid_argument(
          "eng::fluid::run: state control node or threshold is invalid");
    validate_action(control.action, "state control");
  }

  const Shape &batch = network.batch_shape_;
  const auto bool_config = config(ScalarType::Bool, network.platform());
  const Series true_lanes =
      pluss::series::zeros(batch, bool_config).logical_not();
  Series control_open =
      pluss::series::zeros(vector_shape(batch, network.links()), bool_config)
          .logical_not();
  Series pump_latch =
      pluss::series::zeros(vector_shape(batch, network.pumps_), bool_config);
  Series valve_latch =
      pluss::series::zeros(vector_shape(batch, network.valves_), bool_config);
  Series pump_value =
      network.pumps_ > 0
          ? network.pump_speed_.slice(-1, network.pipes_,
                                      network.pipes_ + network.pumps_)
          : pluss::series::zeros(vector_shape(batch, 0),
                                 config(network.dtype_, network.platform()));
  const Series base_valve_minor =
      network.valves_ > 0
          ? network.valve_minor_.slice(-1, network.pipes_ + network.pumps_,
                                       network.links())
          : pluss::series::zeros(vector_shape(batch, 0),
                                 config(network.dtype_, network.platform()));
  const Series base_valve_setting =
      network.valves_ > 0
          ? network.valve_setting_.slice(-1, network.pipes_ + network.pumps_,
                                         network.links())
          : pluss::series::zeros(vector_shape(batch, 0),
                                 config(network.dtype_, network.platform()));
  std::vector<std::uint8_t> throttle_values(
      static_cast<std::size_t>(network.valves_), 0);
  for (std::int64_t i = 0; i < network.valves_; ++i)
    throttle_values[static_cast<std::size_t>(i)] =
        network.valve_kind_[static_cast<std::size_t>(i)] == ValveKind::Throttle
            ? 1
            : 0;
  const Series throttle_mask = pluss::series::make(throttle_values)
                                   .to(ScalarType::Bool)
                                   .to(network.platform());
  Series valve_value =
      throttle_mask.where(base_valve_minor, base_valve_setting);
  const Series base_pump = pump_value;
  const Series base_valve = valve_value;

  const auto replace = [&](const Series &whole, const Series &part,
                           std::int64_t begin, std::int64_t end) {
    std::vector<Series> pieces;
    if (begin > 0)
      pieces.push_back(whole.slice(-1, 0, begin));
    if (end > begin)
      pieces.push_back(part);
    if (end < network.links())
      pieces.push_back(whole.slice(-1, end, network.links()));
    return join_last(pieces);
  };

  const auto apply = [&](const Action &action, const Series &condition) {
    const auto family_width =
        action.device == Device::Pump ? network.pumps_ : network.valves_;
    const auto offset = action.device == Device::Pump
                            ? network.pipes_
                            : network.pipes_ + network.pumps_;
    const Series family = condition.unsqueeze(-1).logical_and(
        local_mask(family_width, action.index, network.platform()));
    const Series link = condition.unsqueeze(-1).logical_and(
        local_mask(network.links(), offset + action.index, network.platform()));

    const Series before_open = control_open;
    const Series before_pump_latch = pump_latch;
    const Series before_valve_latch = valve_latch;
    const Series before_pump_value = pump_value;
    const Series before_valve_value = valve_value;
    if (action.act == Act::Open) {
      control_open = control_open.logical_or(link);
      if (action.device == Device::Pump)
        pump_latch = pump_latch.logical_and(family.logical_not());
      else
        valve_latch = valve_latch.logical_and(family.logical_not());
    } else if (action.act == Act::Close) {
      control_open = control_open.logical_and(link.logical_not());
    } else {
      control_open = control_open.logical_or(link);
      if (action.device == Device::Pump) {
        pump_latch = pump_latch.logical_or(family);
        pump_value = family.where(
            pluss::series::full_like(pump_value, action.value), pump_value);
      } else {
        valve_latch = valve_latch.logical_or(family);
        valve_value = family.where(
            pluss::series::full_like(valve_value, action.value), valve_value);
      }
    }
    return !before_open.equal(control_open) ||
           !before_pump_latch.equal(pump_latch) ||
           !before_valve_latch.equal(valve_latch) ||
           !before_pump_value.equal(pump_value) ||
           !before_valve_value.equal(valve_value);
  };

  Series tank_level =
      matvec(network.tank_pick_, network.head_.sub(network.elevation_));
  const Series tank_incidence = network.tank_pick_.matmul(network.incidence_);
  Series warm_head = network.head_;
  Series warm_flow;
  std::vector<bool> fired(schedule.time.size(), false);

  std::vector<double> times;
  std::vector<std::int64_t> iteration_values;
  std::vector<Series> heads, pressures, flows, velocities, losses, frictions,
      balances, residuals, demands, emitters, levels, spills, opens,
      converged_values;
  double time = 0.0;
  constexpr double time_tol = 1e-9;
  const auto append = [&](const SteadyResult &result, const Series &level,
                          const Series &spill, const Series &open) {
    times.push_back(time);
    iteration_values.push_back(result.iterations);
    heads.push_back(result.head);
    pressures.push_back(result.pressure);
    flows.push_back(result.flow);
    velocities.push_back(result.velocity);
    losses.push_back(result.head_loss);
    frictions.push_back(result.friction_factor);
    balances.push_back(result.balance);
    residuals.push_back(result.energy_residual);
    demands.push_back(result.demand);
    emitters.push_back(result.emitter);
    levels.push_back(level);
    spills.push_back(spill);
    opens.push_back(open);
    converged_values.push_back(result.converged);
  };

  const auto base_steps = static_cast<std::int64_t>(std::ceil(
      schedule.duration / std::min(schedule.step, schedule.pattern_step)));
  const std::int64_t max_steps =
      (base_steps + static_cast<std::int64_t>(schedule.time.size()) + 1) *
          (static_cast<std::int64_t>(schedule.state.size()) +
           2 * network.tanks_ + 2) +
      32;
  for (std::int64_t step_count = 0; step_count < max_steps; ++step_count) {
    for (std::size_t i = 0; i < schedule.time.size(); ++i) {
      if (!fired[i] && schedule.time[i].at <= time + time_tol) {
        apply(schedule.time[i].action, true_lanes);
        fired[i] = true;
      }
    }

    const Series demand_scale = pattern(
        schedule.demand, network.nodes_, time, schedule.pattern_step,
        network.dtype_, network.platform(),
        pluss::series::ones({network.nodes_},
                            config(network.dtype_, network.platform())));
    const Series pump_scale = pattern(
        schedule.pump, network.pumps_, time, schedule.pattern_step,
        network.dtype_, network.platform(),
        pluss::series::ones({network.pumps_},
                            config(network.dtype_, network.platform())));
    const Series valve_base =
        pattern(schedule.valve, network.valves_, time, schedule.pattern_step,
                network.dtype_, network.platform(), base_valve);
    const Series patterned_pump = base_pump.mul(pump_scale);
    const Series effective_pump = pump_latch.where(pump_value, patterned_pump);
    const Series effective_valve = valve_latch.where(valve_value, valve_base);

    Series bound_closed =
        pluss::series::zeros_like(control_open).to(ScalarType::Bool);
    SteadyResult result;
    Network snapshot = network;
    Series effective_open = control_open;
    bool settled = false;
    for (std::int64_t pass = 0; pass < 16; ++pass) {
      Series pump_positive =
          pluss::series::zeros(vector_shape(batch, network.links()),
                               bool_config)
              .logical_not();
      if (network.pumps_ > 0)
        pump_positive =
            replace(pump_positive, effective_pump.gt(0.0), network.pipes_,
                    network.pipes_ + network.pumps_);
      effective_open = control_open.logical_and(pump_positive)
                           .logical_and(bound_closed.logical_not());
      snapshot.demand_ = network.demand_.mul(demand_scale);
      snapshot.head_ = warm_head;
      if (network.tanks_ > 0) {
        const Series tank_head =
            matvec(network.tank_pick_, network.elevation_).add(tank_level);
        const Series old_tank_head = matvec(network.tank_pick_, warm_head);
        snapshot.head_ = warm_head.add(
            matvec(network.tank_put_, tank_head.sub(old_tank_head)));
      }
      if (network.pumps_ > 0)
        snapshot.pump_speed_ =
            replace(network.pump_speed_, effective_pump, network.pipes_,
                    network.pipes_ + network.pumps_);
      if (network.valves_ > 0) {
        const Series active_minor =
            throttle_mask.where(effective_valve, base_valve_minor);
        const Series active_setting = throttle_mask.logical_not().where(
            effective_valve, base_valve_setting);
        snapshot.valve_minor_ =
            replace(network.valve_minor_, active_minor,
                    network.pipes_ + network.pumps_, network.links());
        snapshot.valve_setting_ =
            replace(network.valve_setting_, active_setting,
                    network.pipes_ + network.pumps_, network.links());
      }
      snapshot.closed_mask_ = effective_open.logical_not().to(network.dtype_);
      snapshot.flow_guess_ = warm_flow;
      result = steady(snapshot, options);
      warm_head = result.head.detach();
      warm_flow = result.flow.detach();

      bool changed = false;
      for (const auto &control : schedule.state) {
        Series observed;
        const auto tank_index =
            network.tank_of_node_[static_cast<std::size_t>(control.node)];
        if (tank_index >= 0)
          observed = tank_level.select(-1, tank_index);
        else
          observed = result.head.select(-1, control.node)
                         .sub(network.elevation_.select(-1, control.node));
        const Series condition = control.sense == Sense::Above
                                     ? observed.ge(control.threshold)
                                     : observed.le(control.threshold);
        changed = apply(control.action, condition) || changed;
      }

      if (network.tanks_ > 0) {
        const Series contribution =
            tank_incidence.mul(result.flow.unsqueeze(-2));
        const Series at_min = tank_level.le(network.tank_min_.add(1e-10));
        const Series at_max = tank_level.ge(network.tank_max_.sub(1e-10))
                                  .logical_and(network.tank_overflow_.eq(0.0));
        const Series bad =
            at_min.unsqueeze(-1)
                .logical_and(contribution.lt(-options.flow_tolerance))
                .logical_or(at_max.unsqueeze(-1).logical_and(
                    contribution.gt(options.flow_tolerance)))
                .any(-2, false);
        const Series next_closed = bound_closed.logical_or(bad);
        changed = !next_closed.equal(bound_closed) || changed;
        bound_closed = next_closed;
      }
      if (!changed) {
        settled = true;
        break;
      }
    }
    if (!settled)
      throw std::runtime_error(
          "eng::fluid::run: controls did not settle at a hydraulic event");

    Series tank_flow = matvec(network.tank_pick_, result.balance);
    const Series at_max = tank_level.ge(network.tank_max_.sub(1e-10));
    const Series spill =
        at_max.logical_and(network.tank_overflow_.gt(0.0))
            .logical_and(tank_flow.gt(0.0))
            .where(tank_flow, pluss::series::zeros_like(tank_flow));
    const Series tank_rate = tank_flow.sub(spill).div(network.tank_area_);
    append(result, tank_level, spill, effective_open);
    if (time >= schedule.duration - time_tol)
      return {pluss::series::make(times),
              stack0(heads),
              stack0(pressures),
              stack0(flows),
              stack0(velocities),
              stack0(losses),
              stack0(frictions),
              stack0(balances),
              stack0(residuals),
              stack0(demands),
              stack0(emitters),
              stack0(levels),
              stack0(spills),
              stack0(opens),
              stack0(converged_values),
              pluss::series::make(iteration_values)};

    double dt = std::min(schedule.step, schedule.duration - time);
    const double next_pattern =
        (std::floor(time / schedule.pattern_step) + 1.0) *
        schedule.pattern_step;
    if (next_pattern > time + time_tol)
      dt = std::min(dt, next_pattern - time);
    for (std::size_t i = 0; i < schedule.time.size(); ++i)
      if (!fired[i] && schedule.time[i].at > time + time_tol)
        dt = std::min(dt, schedule.time[i].at - time);

    if (network.tanks_ > 0) {
      const Series inf = pluss::series::full_like(
          tank_rate, std::numeric_limits<double>::infinity());
      const Series to_max = tank_rate.gt(0.0).where(
          network.tank_max_.sub(tank_level).div(tank_rate.clamp_min(1e-30)),
          inf);
      const Series to_min = tank_rate.lt(0.0).where(
          network.tank_min_.sub(tank_level).div(tank_rate.clamp_max(-1e-30)),
          inf);
      const double tank_event =
          to_max.gt(time_tol).where(to_max, inf).amin().item<double>();
      const double tank_min_event =
          to_min.gt(time_tol).where(to_min, inf).amin().item<double>();
      dt = std::min(dt, std::min(tank_event, tank_min_event));
      for (const auto &control : schedule.state) {
        const auto tank_index =
            network.tank_of_node_[static_cast<std::size_t>(control.node)];
        if (tank_index < 0)
          continue;
        const Series value = tank_level.select(-1, tank_index);
        const Series rate = tank_rate.select(-1, tank_index);
        const Series candidate =
            control.sense == Sense::Above
                ? rate.gt(0.0)
                      .logical_and(value.lt(control.threshold))
                      .where(
                          pluss::series::full_like(value, control.threshold)
                              .sub(value)
                              .div(rate.clamp_min(1e-30)),
                          pluss::series::full_like(
                              value, std::numeric_limits<double>::infinity()))
                : rate.lt(0.0)
                      .logical_and(value.gt(control.threshold))
                      .where(
                          pluss::series::full_like(value, control.threshold)
                              .sub(value)
                              .div(rate.clamp_max(-1e-30)),
                          pluss::series::full_like(
                              value, std::numeric_limits<double>::infinity()));
        const double state_event =
            candidate.gt(time_tol)
                .where(candidate,
                       pluss::series::full_like(
                           candidate, std::numeric_limits<double>::infinity()))
                .amin()
                .item<double>();
        dt = std::min(dt, state_event);
      }
    }
    if (!std::isfinite(dt) || dt <= time_tol)
      throw std::runtime_error(
          "eng::fluid::run: event clock failed to make forward progress");
    if (network.tanks_ > 0)
      tank_level = tank_level.add(tank_rate.mul(dt))
                       .clamp_min(network.tank_min_)
                       .clamp_max(network.tank_max_);
    time = std::min(schedule.duration, time + dt);
  }
  throw std::runtime_error(
      "eng::fluid::run: event count exceeded the deterministic safety bound");
}

} // namespace pluss::subsp::eng::fluid
