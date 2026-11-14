#include "pluss/subsp/eng/fluid.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::fluid {
namespace {

using Shape = std::vector<std::int64_t>;

constexpr double pi = 3.141592653589793238462643383279502884;

Config config(ScalarType dtype, Platform platform) {
  return Config{}.with_dtype(dtype).with_platform(platform);
}

Shape vector_shape(const Shape &batch, std::int64_t width) {
  Shape shape = batch;
  shape.push_back(width);
  return shape;
}

Shape field_shape(const Shape &batch, std::int64_t width,
                  std::int64_t cells) {
  Shape shape = batch;
  shape.push_back(width);
  shape.push_back(cells);
  return shape;
}

Shape leading_shape(const Series &value) {
  Shape shape(value.sizes().begin(), value.sizes().end());
  shape.pop_back();
  return shape;
}

Series matvec(const Series &matrix, const Series &vector) {
  if (matrix.scalar_type() != vector.scalar_type()) {
    std::string matrix_shape{"["};
    for (const auto size : matrix.sizes())
      matrix_shape += std::to_string(size) + ",";
    matrix_shape += "]";
    std::string vector_shape_text{"["};
    for (const auto size : vector.sizes())
      vector_shape_text += std::to_string(size) + ",";
    vector_shape_text += "]";
    throw std::logic_error(
        "eng::fluid route: internal matrix/vector dtype mismatch: " +
        std::string(dtype_of(matrix.scalar_type()).name) + " matrix versus " +
        std::string(dtype_of(vector.scalar_type()).name) + " vector; shapes " +
        matrix_shape + " x " + vector_shape_text);
  }
  return matrix.matmul(vector.unsqueeze(-1)).squeeze(-1);
}

Series matmul(const Series &left, const Series &right, const char *role) {
  if (left.scalar_type() != right.scalar_type())
    throw std::logic_error(std::string("eng::fluid route: ") + role +
                           " dtype mismatch");
  return left.matmul(right);
}

Series batch_diag(const Series &vector) {
  const Series identity = pluss::series::eye(
      vector.size(-1), config(vector.scalar_type(), vector.platform()));
  return vector.unsqueeze(-1).mul(identity);
}

Series stack0(const std::vector<Series> &values) {
  if (values.empty())
    throw std::logic_error("eng::fluid route: empty trajectory");
  Series out = values.front().unsqueeze(0);
  for (std::size_t index = 1; index < values.size(); ++index)
    out = out.unite({values[index].unsqueeze(0)}, 0);
  return out;
}

Series expand_field(const Series &value, const Shape &batch,
                    std::int64_t width, ScalarType dtype, Platform platform,
                    const char *operation, const char *role, bool required,
                    double fallback) {
  if (!value.defined()) {
    if (required)
      throw std::invalid_argument(std::string(operation) + ": " +
                                  role + " is required");
    return pluss::series::full(vector_shape(batch, width), fallback,
                               config(dtype, platform));
  }
  if (value.dim() < 1 || value.size(-1) != width ||
      !value.dtype().is_floating())
    throw std::invalid_argument(std::string(operation) + ": " +
                                role + " must be real floating [..., " +
                                std::to_string(width) + "]");
  if (value.scalar_type() != dtype || value.platform() != platform)
    throw std::invalid_argument(std::string(operation) + ": " +
                                role +
                                " must share the network dtype and platform");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) + ": " +
                                role + " must be finite");

  const Shape leading = leading_shape(value);
  if (leading == batch)
    return value;
  if (!leading.empty())
    throw std::invalid_argument(std::string(operation) + ": " +
                                role +
                                " must be unbatched or match the network batch");
  Shape aligned(batch.size(), 1);
  aligned.push_back(width);
  return value.reshape(aligned).expand(vector_shape(batch, width), true);
}

Series repeat_cells(const Series &value, std::int64_t cells) {
  Shape shape(value.sizes().begin(), value.sizes().end());
  shape.push_back(cells);
  return value.unsqueeze(-1).expand(shape, true);
}

Series inventory(const Series &field, const Series &cell_volume,
                 const Series &tank_quality, const Series &tank_volume) {
  Series total =
      field.mul(cell_volume.unsqueeze(-1)).sum(-1, false).sum(-1, false);
  if (tank_quality.size(-1) > 0)
    total = total.add(tank_quality.mul(tank_volume).sum(-1, false));
  return total;
}

struct MixState {
  Series node;
  Series link_out;
  Series upstream;
  Series inflow_rate;
  Series outflow_rate;
  Series inflow_mass;
};

} // namespace

namespace detail {

enum class RouteKind { Constituent, Age, Heat };

struct RouteSpec {
  const char *operation{"eng::fluid::transport"};
  RouteKind kind{RouteKind::Constituent};
  Series initial;
  Series source;
  Series pipe_rate;
  Series tank_rate;
  Series pipe_ambient;
  Series pipe_ua;
  Series tank_ambient;
  Series tank_ua;
  double cp{4180.0};
};

struct TransportEngine {
  static TransportResult run(const Network &network, const Schedule &schedule,
                             const RouteSpec &route,
                             const RouteOptions &route_options,
                             const Options &hydraulic_options) {
    if (!std::isfinite(route_options.step) || route_options.step <= 0.0 ||
        !std::isfinite(route_options.courant) ||
        route_options.courant <= 0.0 || route_options.courant > 1.0 ||
        !std::isfinite(route_options.flow_tolerance) ||
        route_options.flow_tolerance < 0.0 || route_options.cells <= 0)
      throw std::invalid_argument(
          std::string(route.operation) +
          ": route step, Courant number, flow tolerance, and cell count are "
          "invalid");

    const auto dtype = network.dtype_;
    const auto platform = network.head_.platform();
    const auto &batch = network.batch_shape_;
    const auto nodes = network.nodes_;
    const auto links = network.links();
    const auto pipes = network.pipes_;
    const auto tanks = network.tanks_;
    const auto cells = route_options.cells;
    const auto cfg = config(dtype, platform);

    const bool age = route.kind == RouteKind::Age;
    const bool thermal = route.kind == RouteKind::Heat;
    if (thermal && (!std::isfinite(route.cp) || route.cp <= 0.0))
      throw std::invalid_argument(
          std::string(route.operation) + ": cp must be finite and positive");

    Series node = expand_field(route.initial, batch, nodes, dtype, platform,
                               route.operation,
                               thermal ? "initial temperature"
                                       : "initial quality",
                               !age, 0.0);
    if (!node.ge(0.0).all().item<bool>())
      throw std::invalid_argument(
          std::string(route.operation) +
          (thermal ? ": initial temperature must be nonnegative"
                   : ": initial quality must be nonnegative"));

    Series source = age
                        ? pluss::series::zeros(vector_shape(batch, nodes), cfg)
                        : expand_field(route.source, batch, nodes, dtype,
                                       platform, route.operation,
                                       thermal ? "source temperature"
                                               : "source quality",
                                       false, 0.0);
    if (!age && !route.source.defined())
      source = node;
    if (!source.ge(0.0).all().item<bool>())
      throw std::invalid_argument(
          std::string(route.operation) +
          (thermal ? ": source temperature must be nonnegative"
                   : ": source quality must be nonnegative"));
    if (age && (route.source.defined() || route.pipe_rate.defined() ||
                route.tank_rate.defined()))
      throw std::invalid_argument(
          std::string(route.operation) +
          ": Age supplies its zero boundary and unit growth law; source and "
          "reaction fields are not accepted");

    std::vector<double> junction_values(static_cast<std::size_t>(nodes), 0.0);
    std::vector<double> reservoir_values(static_cast<std::size_t>(nodes), 0.0);
    for (std::int64_t index = 0; index < nodes; ++index) {
      const auto kind = network.node_kind_[static_cast<std::size_t>(index)];
      junction_values[static_cast<std::size_t>(index)] =
          kind == NodeKind::Junction ? 1.0 : 0.0;
      reservoir_values[static_cast<std::size_t>(index)] =
          kind == NodeKind::Reservoir ? 1.0 : 0.0;
    }
    const Series junction_mask =
        pluss::series::make(junction_values).to(dtype).to(platform);
    const Series reservoir_mask =
        pluss::series::make(reservoir_values).to(dtype).to(platform);
    const Series pipe_mask =
        pluss::series::ones({links}, cfg)
            .sub(network.pump_mask_)
            .sub(network.valve_mask_);
    const Series device_mask = pluss::series::ones_like(pipe_mask).sub(pipe_mask);
    const Series from_map = network.energy_.eq(1.0).to(dtype);
    const Series to_map = network.energy_.eq(-1.0).to(dtype);
    const Series from_put = from_map.mT();
    const Series to_put = to_map.mT();
    const Series identity = pluss::series::eye(nodes, cfg);

    const Series link_volume =
        network.length_.mul(network.diameter_.square()).mul(pi / 4.0).mul(pipe_mask);
    const Series cell_volume = link_volume.div(static_cast<double>(cells));

    Series pipe_rate;
    Series pipe_equilibrium;
    Series tank_rate;
    Series tank_equilibrium;
    Series tank_ua = pluss::series::zeros(vector_shape(batch, tanks), cfg);
    if (thermal) {
      Series physical_ua = expand_field(
          route.pipe_ua, batch, pipes, dtype, platform, route.operation,
          "pipe ua", false, 0.0);
      Series physical_ambient = expand_field(
          route.pipe_ambient, batch, pipes, dtype, platform, route.operation,
          "pipe ambient temperature",
          pipes > 0 && physical_ua.gt(0.0).any().item<bool>(), 0.0);
      tank_ua = expand_field(route.tank_ua, batch, tanks, dtype, platform,
                             route.operation, "tank ua", false, 0.0);
      tank_equilibrium = expand_field(
          route.tank_ambient, batch, tanks, dtype, platform, route.operation,
          "tank ambient temperature",
          tanks > 0 && tank_ua.gt(0.0).any().item<bool>(), 0.0);
      if (!physical_ua.ge(0.0).all().item<bool>() ||
          !tank_ua.ge(0.0).all().item<bool>())
        throw std::invalid_argument(std::string(route.operation) +
                                    ": ua values must be nonnegative");
      if (!physical_ambient.ge(0.0).all().item<bool>() ||
          !tank_equilibrium.ge(0.0).all().item<bool>())
        throw std::invalid_argument(
            std::string(route.operation) +
            ": ambient temperatures must be nonnegative");

      const Series physical_volume = link_volume.slice(-1, 0, pipes);
      pipe_rate = physical_ua.neg().div(
          physical_volume.mul(network.liquid_.density * route.cp));
      pipe_equilibrium = physical_ambient;
      tank_rate = pluss::series::zeros(vector_shape(batch, tanks), cfg);
    } else {
      pipe_rate = expand_field(route.pipe_rate, batch, pipes, dtype, platform,
                               route.operation, "pipe reaction rate", false,
                               0.0);
      pipe_equilibrium = pluss::series::zeros(vector_shape(batch, pipes), cfg);
      tank_rate = expand_field(route.tank_rate, batch, tanks, dtype, platform,
                               route.operation, "tank reaction rate", false,
                               0.0);
      tank_equilibrium =
          pluss::series::zeros(vector_shape(batch, tanks), cfg);
    }
    if (links > pipes) {
      const Series device_values = pluss::series::zeros(
          vector_shape(batch, links - pipes), cfg);
      pipe_rate = pipe_rate.unite({device_values}, -1);
      pipe_equilibrium = pipe_equilibrium.unite({device_values}, -1);
    }

    const Series initial_downstream = matvec(to_map, node);
    Series field = repeat_cells(initial_downstream, cells);
    Series tank_quality = matvec(network.tank_pick_, node);
    Series tank_volume =
        network.tank_area_.mul(matvec(network.tank_pick_,
                                      network.head_.sub(network.elevation_)));

    RunResult hydraulic = fluid::run(network, schedule, hydraulic_options);
    const auto times = hydraulic.time.items<double>();
    if (times.empty())
      throw std::logic_error(
          std::string(route.operation) +
          ": hydraulic run returned no samples");

    auto put_tanks = [&](const Series &base, const Series &values) {
      if (tanks == 0)
        return base;
      const Series previous = matvec(network.tank_pick_, base);
      return base.add(matvec(network.tank_put_, values.sub(previous)));
    };

    auto fixed_nodes = [&](const Series &base) {
      Series value = reservoir_mask.eq(1.0).where(source, base);
      value = put_tanks(value, tank_quality);
      return value;
    };

    auto mix = [&](const Series &flow, const Series &demand,
                   const Series &current_node, const Series &current_field) {
      const Series positive = flow.clamp_min(0.0);
      const Series negative = flow.neg().clamp_min(0.0);
      const Series abs_flow = positive.add(negative);
      const Series downstream = flow.ge(0.0).where(
          current_field.select(-1, cells - 1), current_field.select(-1, 0));
      const Series inflow_rate =
          matvec(to_put, positive).add(matvec(from_put, negative));
      const Series outflow_rate =
          matvec(from_put, positive).add(matvec(to_put, negative));
      const Series pipe_inflow_mass =
          matvec(to_put, positive.mul(downstream).mul(pipe_mask))
              .add(matvec(from_put,
                          negative.mul(downstream).mul(pipe_mask)));
      const Series injection = demand.neg().clamp_min(0.0);
      const Series total = inflow_rate.add(injection);

      const Series forward = matmul(
          to_put,
          matmul(batch_diag(positive.mul(device_mask)), from_map,
                 "forward device inner product"),
          "forward device transfer");
      const Series reverse = matmul(
          from_put,
          matmul(batch_diag(negative.mul(device_mask)), to_map,
                 "reverse device inner product"),
          "reverse device transfer");
      const Series matrix = batch_diag(total).sub(forward.add(reverse));
      const Series active = junction_mask.eq(1.0).logical_and(
          total.gt(route_options.flow_tolerance));
      const Series system = active.unsqueeze(-1).where(matrix, identity);
      const Series mixed_rhs = pipe_inflow_mass.mul(junction_mask)
                                   .add(injection.mul(source));
      const Series fixed = fixed_nodes(current_node);
      const Series rhs = active.where(mixed_rhs, fixed);
      if (system.scalar_type() != rhs.scalar_type())
        throw std::logic_error(
            std::string(route.operation) +
            ": junction system dtype mismatch");
      const Series mixed =
          system.linearalg.lu_solve(rhs.unsqueeze(-1)).squeeze(-1).to(dtype);
      const Series solved_node = fixed_nodes(mixed);
      const Series solved_upstream = flow.ge(0.0).where(
          matvec(from_map, solved_node), matvec(to_map, solved_node));
      const Series link_out =
          pipe_mask.eq(1.0).where(downstream, solved_upstream);
      const Series inflow_mass =
          matvec(to_put, positive.mul(link_out))
              .add(matvec(from_put, negative.mul(link_out)));
      return MixState{solved_node, link_out, solved_upstream, inflow_rate,
                      outflow_rate, inflow_mass};
    };

    auto link_quality = [&](const Series &flow, const Series &current_node,
                            const Series &current_field) {
      const Series average = current_field.mean(-1, false);
      const Series upstream = flow.ge(0.0).where(matvec(from_map, current_node),
                                                matvec(to_map, current_node));
      return pipe_mask.eq(1.0).where(average, upstream);
    };

    std::vector<Series> node_values;
    std::vector<Series> link_values;
    std::vector<Series> tank_values_out;
    std::vector<Series> mass_values;
    std::vector<Series> error_values;
    node_values.reserve(times.size());
    link_values.reserve(times.size());
    tank_values_out.reserve(times.size());
    mass_values.reserve(times.size());
    error_values.reserve(times.size());

    Series initial_mass = inventory(field, cell_volume, tank_quality, tank_volume);
    Series cumulative_in = initial_mass.mul(0.0);
    Series cumulative_out = initial_mass.mul(0.0);
    Series cumulative_reaction = initial_mass.mul(0.0);

    for (std::size_t sample = 0; sample < times.size(); ++sample) {
      const Series sample_flow = hydraulic.flow.select(0, sample);
      node = fixed_nodes(node);
      const MixState reported = mix(sample_flow, hydraulic.demand.select(0, sample),
                                    node, field);
      node = reported.node;
      const Series stored = inventory(field, cell_volume, tank_quality, tank_volume);
      node_values.push_back(node);
      link_values.push_back(link_quality(sample_flow, node, field));
      tank_values_out.push_back(tank_quality);
      mass_values.push_back(stored);
      error_values.push_back(stored.sub(initial_mass)
                                 .sub(cumulative_reaction)
                                 .sub(cumulative_in)
                                 .add(cumulative_out));
      if (sample + 1 == times.size())
        break;

      double time = times[sample];
      const double end = times[sample + 1];
      const Series demand = hydraulic.demand.select(0, sample);
      const Series emitter = hydraulic.emitter.select(0, sample);
      const Series spill = hydraulic.spill.select(0, sample);
      while (time < end) {
        const Series positive = sample_flow.clamp_min(0.0);
        const Series negative = sample_flow.neg().clamp_min(0.0);
        const Series abs_flow = positive.add(negative);
        const Series inf = pluss::series::full_like(
            abs_flow, std::numeric_limits<double>::infinity());
        const Series active_pipe =
            pipe_mask.eq(1.0).logical_and(
                abs_flow.gt(route_options.flow_tolerance));
        const double pipe_limit =
            active_pipe.where(cell_volume.div(abs_flow.clamp_min(1e-30)), inf)
                .amin()
                .item<double>();

        const Series inflow_rate =
            matvec(to_put, positive).add(matvec(from_put, negative));
        const Series outflow_rate =
            matvec(from_put, positive).add(matvec(to_put, negative));
        double tank_limit = std::numeric_limits<double>::infinity();
        const Series routed_tank_quality = tank_quality;
        if (tanks > 0) {
          const Series tank_out =
              matvec(network.tank_pick_, outflow_rate).add(spill);
          const Series tank_inf = pluss::series::full_like(
              tank_out, std::numeric_limits<double>::infinity());
          tank_limit =
              tank_out.gt(route_options.flow_tolerance)
                  .where(tank_volume.div(tank_out.clamp_min(1e-30)), tank_inf)
                  .amin()
                  .item<double>();
        }

        double dt = std::min(route_options.step, end - time);
        dt = std::min(dt, route_options.courant * pipe_limit);
        dt = std::min(dt, route_options.courant * tank_limit);
        if (!std::isfinite(dt) || dt <= 0.0)
          dt = std::min(route_options.step, end - time);
        if (!std::isfinite(dt) || dt <= 0.0)
          throw std::runtime_error(
              std::string(route.operation) +
              ": route clock failed to advance");

        const Series before =
            inventory(field, cell_volume, tank_quality, tank_volume);
        Series pipe_step_factor = pluss::series::ones_like(pipe_rate);
        if (age) {
          field = field.add(pipe_mask.unsqueeze(-1).mul(dt));
          if (tanks > 0)
            tank_quality = tank_quality.add(dt);
        } else {
          pipe_step_factor = pipe_rate.mul(dt).exp();
          const Series pipe_decay = pipe_step_factor.unsqueeze(-1);
          field = pipe_equilibrium.unsqueeze(-1).add(
              field.sub(pipe_equilibrium.unsqueeze(-1)).mul(pipe_decay));
          if (tanks > 0) {
            const Series effective_tank_rate =
                thermal
                    ? tank_ua.neg().div(tank_volume.clamp_min(1e-30).mul(
                          network.liquid_.density * route.cp))
                    : tank_rate;
            const Series tank_decay = effective_tank_rate.mul(dt).exp();
            tank_quality = tank_equilibrium.add(
                tank_quality.sub(tank_equilibrium).mul(tank_decay));
          }
        }
        node = fixed_nodes(node);
        const Series reacted =
            inventory(field, cell_volume, tank_quality, tank_volume);
        cumulative_reaction = cumulative_reaction.add(reacted.sub(before));

        const MixState state = mix(sample_flow, demand, node, field);
        node = state.node;
        const Series boundary = sample_flow.ge(0.0).where(
            matvec(from_map, node), matvec(to_map, node));
        const Series routed_boundary =
            age ? boundary.add(pipe_mask.mul(dt))
                : pipe_equilibrium.add(
                      boundary.sub(pipe_equilibrium).mul(pipe_step_factor));
        cumulative_reaction = cumulative_reaction.add(
            routed_boundary.sub(boundary).mul(abs_flow).mul(dt).sum(-1, false));
        const Series safe_cell = pipe_mask.eq(1.0).where(
            cell_volume, pluss::series::ones_like(cell_volume));
        const Series cfl = abs_flow.mul(dt).div(safe_cell).mul(pipe_mask);
        const Series left =
            cells == 1
                ? routed_boundary.unsqueeze(-1)
                : routed_boundary.unsqueeze(-1).unite(
                      {field.slice(-1, 0, cells - 1)}, -1);
        const Series right =
            cells == 1
                ? routed_boundary.unsqueeze(-1)
                : field.slice(-1, 1, cells).unite(
                      {routed_boundary.unsqueeze(-1)}, -1);
        const Series forward = field.add(
            left.sub(field).mul(cfl.unsqueeze(-1)));
        const Series reverse = field.add(
            right.sub(field).mul(cfl.unsqueeze(-1)));
        const Series routed = sample_flow.ge(0.0).unsqueeze(-1).where(
            forward, reverse);
        field = pipe_mask.eq(1.0).unsqueeze(-1).where(routed, field);

        if (tanks > 0) {
          const Series tank_in =
              matvec(network.tank_pick_, state.inflow_rate);
          const Series tank_out =
              matvec(network.tank_pick_, state.outflow_rate).add(spill);
          const Series tank_mass_in =
              matvec(network.tank_pick_, state.inflow_mass);
          const Series remaining = tank_volume.sub(tank_out.mul(dt)).clamp_min(0.0);
          const Series next_mass =
              tank_quality.mul(remaining).add(tank_mass_in.mul(dt));
          tank_volume = remaining.add(tank_in.mul(dt));
          tank_quality = next_mass.div(tank_volume.clamp_min(1e-30));
          node = put_tanks(node, tank_quality);
        }

        const Series reservoir_in =
            state.outflow_rate.mul(reservoir_mask).mul(source).sum(-1, false);
        const Series injection_in =
            demand.neg().clamp_min(0.0).mul(source).sum(-1, false);
        const Series reservoir_out =
            state.inflow_mass.mul(reservoir_mask).sum(-1, false);
        const Series use_out = demand.clamp_min(0.0)
                                   .add(emitter)
                                   .mul(node)
                                   .sum(-1, false);
        Series spill_out = reservoir_out.mul(0.0);
        if (tanks > 0)
          spill_out = spill.mul(routed_tank_quality).sum(-1, false);
        cumulative_in = cumulative_in.add(reservoir_in.add(injection_in).mul(dt));
        cumulative_out =
            cumulative_out.add(reservoir_out.add(use_out).add(spill_out).mul(dt));

        time = std::min(end, time + dt);
      }

      node = fixed_nodes(node);
      node = mix(hydraulic.flow.select(0, sample + 1),
                 hydraulic.demand.select(0, sample + 1), node, field)
                 .node;
    }

    return {std::move(hydraulic), stack0(node_values), stack0(link_values),
            stack0(tank_values_out), stack0(mass_values), stack0(error_values)};
  }
};

} // namespace detail

TransportResult transport(const Network &network, const Schedule &schedule,
                          const Quality &quality,
                          const RouteOptions &route_options,
                          const Options &hydraulic_options) {
  detail::RouteSpec route;
  route.kind = quality.mode == QualityMode::Age
                   ? detail::RouteKind::Age
                   : detail::RouteKind::Constituent;
  route.initial = quality.initial;
  route.source = quality.source;
  route.pipe_rate = quality.pipe_rate;
  route.tank_rate = quality.tank_rate;
  return detail::TransportEngine::run(network, schedule, route, route_options,
                                      hydraulic_options);
}

HeatResult heat(const Network &network, const Schedule &schedule,
                const Heat &thermal, const RouteOptions &route_options,
                const Options &hydraulic_options) {
  detail::RouteSpec route;
  route.operation = "eng::fluid::heat";
  route.kind = detail::RouteKind::Heat;
  route.initial = thermal.initial;
  route.source = thermal.source;
  route.pipe_ambient = thermal.ambient;
  route.pipe_ua = thermal.ua;
  route.tank_ambient = thermal.tank_ambient;
  route.tank_ua = thermal.tank_ua;
  route.cp = thermal.cp;
  TransportResult routed = detail::TransportEngine::run(
      network, schedule, route, route_options, hydraulic_options);
  const double energy_scale = network.liquid().density * thermal.cp;
  return {std::move(routed.hydraulic), std::move(routed.node),
          std::move(routed.link), std::move(routed.tank),
          routed.mass.mul(energy_scale),
          routed.mass_error.mul(energy_scale)};
}

} // namespace pluss::subsp::eng::fluid
