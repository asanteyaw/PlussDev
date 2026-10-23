#include "pluss/subsp/eng/power.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/optim.h"

#include <cmath>
#include <complex>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>

namespace pluss::subsp::eng::power {
namespace {

using Shape = std::vector<std::int64_t>;

Config config(ScalarType dtype, Platform platform) {
  return Config{}.with_dtype(dtype).with_platform(platform);
}

Shape leading_shape(const Series &value) {
  Shape shape(value.sizes().begin(), value.sizes().end());
  shape.pop_back();
  return shape;
}

Shape matrix_shape(const Shape &batch, std::int64_t rows, std::int64_t cols) {
  Shape shape = batch;
  shape.push_back(rows);
  shape.push_back(cols);
  return shape;
}

Series static_matrix(const std::vector<double> &values, std::int64_t rows,
                     std::int64_t cols, ScalarType dtype, Platform platform) {
  return pluss::series::make(values, {rows, cols}).to(dtype).to(platform);
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

void validate_base(const Base &base) {
  if (!std::isfinite(base.power) || base.power <= 0.0 ||
      !std::isfinite(base.voltage) || base.voltage <= 0.0)
    throw std::invalid_argument(
        "eng::power: base power and voltage must be finite and positive");
}

void inspect(const Series &value, std::int64_t count, const char *role,
             bool positive, ScalarType &dtype, Platform &platform,
             bool &have_config, Shape &batch) {
  if (!value.defined() || value.dim() < 1 || value.size(-1) != count ||
      !value.dtype().is_floating())
    throw std::invalid_argument(std::string("eng::power::make: ") + role +
                                " must be real floating [..., elements]");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string("eng::power::make: ") + role +
                                " must be finite");
  if (positive && !value.gt(0.0).all().item<bool>())
    throw std::invalid_argument(std::string("eng::power::make: ") + role +
                                " must be positive");

  if (!have_config) {
    dtype = value.scalar_type();
    platform = value.platform();
    have_config = true;
  } else if (dtype != value.scalar_type() || platform != value.platform()) {
    throw std::invalid_argument(
        "eng::power::make: all values must share dtype and platform");
  }

  const Shape current = leading_shape(value);
  if (!current.empty()) {
    if (batch.empty())
      batch = current;
    else if (batch != current)
      throw std::invalid_argument("eng::power::make: non-scalar batches must "
                                  "have equal leading shapes");
  }
}

Series expand(const Series &value, const Shape &batch, std::int64_t count) {
  Shape target = batch;
  target.push_back(count);
  if (leading_shape(value) == batch)
    return value;
  Shape aligned(batch.size(), 1);
  aligned.push_back(count);
  return value.reshape(aligned).expand(target, true);
}

Series grid_value(const Series &value, const Shape &batch, std::int64_t count,
                  ScalarType dtype, Platform platform, const char *operation,
                  const char *role, bool finite = true) {
  if (!value.defined() || value.dim() < 1 || value.size(-1) != count ||
      !value.dtype().is_floating())
    throw std::invalid_argument(std::string("eng::power::") + operation + ": " +
                                role +
                                " must be real floating [..., elements]");
  if (value.scalar_type() != dtype || value.platform() != platform)
    throw std::invalid_argument(
        std::string("eng::power::") + operation +
        ": all values must share Grid dtype and platform");
  const Shape leading = leading_shape(value);
  if (!leading.empty() && leading != batch)
    throw std::invalid_argument(std::string("eng::power::") + operation + ": " +
                                role + " has incompatible scenario axes");
  if (finite && !value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string("eng::power::") + operation + ": " +
                                role + " must be finite");
  return expand(value, batch, count);
}

Series optional(const Series &value, const Shape &batch, std::int64_t count,
                double fill, ScalarType dtype, Platform platform) {
  if (value.defined())
    return expand(value, batch, count);
  Shape shape = batch;
  shape.push_back(count);
  return pluss::series::full(shape, fill, config(dtype, platform));
}

ScalarType complex_dtype(ScalarType dtype) {
  return dtype == ScalarType::Float32 ? ScalarType::Complex64
                                      : ScalarType::Complex128;
}

Series imag_unit(ScalarType dtype, Platform platform) {
  const auto value = dtype == ScalarType::Complex64
                         ? std::complex<double>{0.0, 1.0}
                         : std::complex<double>{0.0, 1.0};
  return pluss::series::make(std::vector<std::complex<double>>{value}, {})
      .to(dtype)
      .to(platform);
}

Series complex(const Series &real, const Series &imag, ScalarType dtype) {
  return real.to(dtype).add(
      imag.to(dtype).mul(imag_unit(dtype, real.platform())));
}

Series phasor(const Series &magnitude, const Series &angle, ScalarType dtype) {
  return complex(magnitude.mul(angle.cos()), magnitude.mul(angle.sin()), dtype);
}

Series matvec(const Series &matrix, const Series &vector) {
  return matrix.matmul(vector.unsqueeze(-1)).squeeze(-1);
}

Series batch_diag(const Series &vector) {
  const auto width = vector.size(-1);
  ScalarType eye_dtype = vector.scalar_type();
  if (eye_dtype == ScalarType::Complex64)
    eye_dtype = ScalarType::Float32;
  else if (eye_dtype == ScalarType::Complex128)
    eye_dtype = ScalarType::Float64;
  const Series identity =
      pluss::series::eye(width, config(eye_dtype, vector.platform()))
          .to(vector.scalar_type());
  return vector.unsqueeze(-1).mul(identity);
}

Series batch_diagonal(const Series &matrix) {
  if (matrix.dim() < 2 || matrix.size(-2) != matrix.size(-1))
    throw std::invalid_argument(
        "eng::power: diagonal extraction requires square trailing axes");
  ScalarType eye_dtype = matrix.scalar_type();
  if (eye_dtype == ScalarType::Complex64)
    eye_dtype = ScalarType::Float32;
  else if (eye_dtype == ScalarType::Complex128)
    eye_dtype = ScalarType::Float64;
  const Series identity =
      pluss::series::eye(matrix.size(-1), config(eye_dtype, matrix.platform()))
          .to(matrix.scalar_type());
  return matrix.mul(identity).sum(-1, false);
}

std::int64_t batch_count(const Shape &batch) {
  return std::accumulate(batch.begin(), batch.end(), std::int64_t{1},
                         std::multiplies<>{});
}

Series block_jacobian(const Series &blocks, std::int64_t batches,
                      std::int64_t rows, std::int64_t columns) {
  if (batches == 1)
    return blocks.reshape({rows, columns});
  const Series identity = pluss::series::eye(
      batches, config(blocks.scalar_type(), blocks.platform()));
  return blocks.reshape({batches, rows, 1, columns})
      .mul(identity.reshape({batches, 1, batches, 1}))
      .reshape({batches * rows, batches * columns});
}

Series one_hot_map(const std::vector<std::int64_t> &indices, std::int64_t rows,
                   std::int64_t cols, ScalarType dtype, Platform platform) {
  std::vector<double> values(static_cast<std::size_t>(rows * cols), 0.0);
  for (std::int64_t column = 0; column < cols; ++column) {
    const auto row = indices[static_cast<std::size_t>(column)];
    values[static_cast<std::size_t>(row * cols + column)] = 1.0;
  }
  return static_matrix(values, rows, cols, dtype, platform);
}

Series endpoint_map(const std::vector<std::int64_t> &nodes, std::int64_t buses,
                    ScalarType dtype, Platform platform) {
  const auto lines = static_cast<std::int64_t>(nodes.size());
  std::vector<double> values(static_cast<std::size_t>(lines * buses), 0.0);
  for (std::int64_t line = 0; line < lines; ++line)
    values[static_cast<std::size_t>(
        line * buses + nodes[static_cast<std::size_t>(line)])] = 1.0;
  return static_matrix(values, lines, buses, dtype, platform);
}

Series voltage(const Series &magnitude, const Series &angle,
               ScalarType complex_type) {
  return phasor(magnitude, angle, complex_type);
}

} // namespace

namespace detail {

struct AcKernel {
  struct BusEval {
    Series voltage;
    Series current;
    Series injection;
    Series d_angle;
    Series d_magnitude;
  };

  struct Eval {
    Series angle;
    Series magnitude;
    Series voltage;
    Series injection;
    Series mismatch;
    Series jacobian;
    Series residual;
  };

  struct Flow {
    Series from;
    Series to;
  };

  struct FlowEval {
    Series from;
    Series to;
    Series d_from_angle;
    Series d_from_magnitude;
    Series d_to_angle;
    Series d_to_magnitude;
  };

  explicit AcKernel(const Grid &value)
      : grid(value), complex_type(complex_dtype(value.dtype_)),
        angle_selector(value.angle_map_.mT()), pq_selector(value.pq_map_.mT()),
        angle_count(value.angle_map_.size(-1)),
        pq_count(value.pq_map_.size(-1)),
        fixed_angle(value.va_.sub(
            matvec(value.angle_map_, matvec(angle_selector, value.va_)))),
        fixed_magnitude(
            pq_count > 0 ? value.vm_.sub(matvec(value.pq_map_,
                                                matvec(pq_selector, value.vm_)))
                         : value.vm_) {}

  [[nodiscard]] Series state(const Series &angle,
                             const Series &magnitude) const {
    const Series angle_vars = matvec(angle_selector, angle);
    if (pq_count == 0)
      return angle_vars;
    return angle_vars.unite({matvec(pq_selector, magnitude)}, -1);
  }

  [[nodiscard]] Series initial_state() const {
    return state(grid.va_, grid.vm_);
  }

  [[nodiscard]] std::pair<Series, Series>
  unpack(const Series &state_value) const {
    const Series angle_vars = state_value.slice(-1, 0, angle_count);
    const Series angle = fixed_angle.add(matvec(grid.angle_map_, angle_vars));
    if (pq_count == 0)
      return {angle, fixed_magnitude};
    const Series magnitude_vars =
        state_value.slice(-1, angle_count, angle_count + pq_count);
    return {angle, fixed_magnitude.add(matvec(grid.pq_map_, magnitude_vars))};
  }

  [[nodiscard]] BusEval bus(const Series &magnitude,
                            const Series &angle) const {
    const Series volts = voltage(magnitude, angle, complex_type);
    const Series current = matvec(grid.ybus_, volts);
    const Series diag_v = batch_diag(volts);
    const Series diag_i = batch_diag(current);
    const Series diag_vnorm = batch_diag(volts.div(magnitude.to(complex_type)));
    const Series d_angle =
        diag_v.matmul(diag_i.sub(grid.ybus_.matmul(diag_v)).conj())
            .mul(imag_unit(complex_type, grid.platform()));
    const Series d_magnitude =
        diag_v.matmul(grid.ybus_.matmul(diag_vnorm).conj())
            .add(diag_i.conj().matmul(diag_vnorm));
    return {volts, current, volts.mul(current.conj()), d_angle, d_magnitude};
  }

  [[nodiscard]] Eval evaluate(const Series &state_value, const Series &p,
                              const Series &q) const {
    const auto [angle, magnitude] = unpack(state_value);
    const BusEval values = bus(magnitude, angle);
    const Series raw = values.injection.sub(complex(p, q, complex_type));
    const Series p_mismatch = matvec(angle_selector, raw.real());
    const Series mismatch =
        pq_count > 0 ? p_mismatch.unite({matvec(pq_selector, raw.imag())}, -1)
                     : p_mismatch;

    const Series j11 =
        angle_selector.matmul(values.d_angle.real()).matmul(grid.angle_map_);
    Series jacobian = j11;
    if (pq_count > 0) {
      const Series j12 =
          angle_selector.matmul(values.d_magnitude.real()).matmul(grid.pq_map_);
      const Series j21 =
          pq_selector.matmul(values.d_angle.imag()).matmul(grid.angle_map_);
      const Series j22 =
          pq_selector.matmul(values.d_magnitude.imag()).matmul(grid.pq_map_);
      jacobian = j11.unite({j12}, -1).unite({j21.unite({j22}, -1)}, -2);
    }
    return {angle,
            magnitude,
            values.voltage,
            values.injection,
            mismatch,
            jacobian,
            mismatch.abs().amax(-1, false)};
  }

  [[nodiscard]] Flow flow(const Series &volts) const {
    const Series from_voltage = matvec(grid.from_map_.to(complex_type), volts);
    const Series to_voltage = matvec(grid.to_map_.to(complex_type), volts);
    return {from_voltage.mul(matvec(grid.yfrom_, volts).conj()),
            to_voltage.mul(matvec(grid.yto_, volts).conj())};
  }

  [[nodiscard]] FlowEval flow_eval(const Series &volts,
                                   const Series &magnitude) const {
    const Series cf = grid.from_map_.to(complex_type);
    const Series ct = grid.to_map_.to(complex_type);
    const Series diag_v = batch_diag(volts);
    const Series diag_vnorm = batch_diag(volts.div(magnitude.to(complex_type)));
    const Series from_voltage = matvec(cf, volts);
    const Series to_voltage = matvec(ct, volts);
    const Series from_current = matvec(grid.yfrom_, volts);
    const Series to_current = matvec(grid.yto_, volts);
    const Series from_power = from_voltage.mul(from_current.conj());
    const Series to_power = to_voltage.mul(to_current.conj());
    const Series diag_from_voltage = batch_diag(from_voltage);
    const Series diag_to_voltage = batch_diag(to_voltage);
    const Series diag_from_current = batch_diag(from_current);
    const Series diag_to_current = batch_diag(to_current);
    const Series d_from_angle =
        diag_from_voltage
            .matmul(diag_from_current.matmul(cf)
                        .sub(grid.yfrom_.matmul(diag_v))
                        .conj())
            .mul(imag_unit(complex_type, grid.platform()));
    const Series d_to_angle =
        diag_to_voltage
            .matmul(
                diag_to_current.matmul(ct).sub(grid.yto_.matmul(diag_v)).conj())
            .mul(imag_unit(complex_type, grid.platform()));
    const Series d_from_magnitude =
        diag_from_voltage.matmul(grid.yfrom_.matmul(diag_vnorm).conj())
            .add(diag_from_current.conj().matmul(cf.matmul(diag_vnorm)));
    const Series d_to_magnitude =
        diag_to_voltage.matmul(grid.yto_.matmul(diag_vnorm).conj())
            .add(diag_to_current.conj().matmul(ct.matmul(diag_vnorm)));
    return {from_power,       to_power,   d_from_angle,
            d_from_magnitude, d_to_angle, d_to_magnitude};
  }

  const Grid &grid;
  ScalarType complex_type;
  Series angle_selector;
  Series pq_selector;
  std::int64_t angle_count;
  std::int64_t pq_count;
  Series fixed_angle;
  Series fixed_magnitude;
};

} // namespace detail

double zbase(const Base &base) {
  validate_base(base);
  return base.voltage * base.voltage / base.power;
}

double ybase(const Base &base) { return 1.0 / zbase(base); }

double ibase(const Base &base) {
  validate_base(base);
  return base.power / (std::sqrt(3.0) * base.voltage);
}

Series pu(const Series &value, double base) {
  if (!std::isfinite(base) || base <= 0.0)
    throw std::invalid_argument(
        "eng::power::pu: base must be finite and positive");
  return value.div(base);
}

Series si(const Series &value, double base) {
  if (!std::isfinite(base) || base <= 0.0)
    throw std::invalid_argument(
        "eng::power::si: base must be finite and positive");
  return value.mul(base);
}

DcResult Grid::solve_dc(const Series &specified) const {
  const Series selector = angle_map_.mT();
  const Series initial = matvec(selector, va_);
  const Series fixed = va_.sub(matvec(angle_map_, initial));
  const Series reduced = selector.matmul(bbus_).matmul(angle_map_);
  const Series rhs =
      matvec(selector, specified.sub(bus_shift_).sub(matvec(bbus_, fixed)));
  const Series unknown =
      reduced.linearalg.lu_solve(rhs.unsqueeze(-1)).squeeze(-1);
  const Series angle = fixed.add(matvec(angle_map_, unknown));
  const Series injection = matvec(bbus_, angle).add(bus_shift_);
  const Series from_power = matvec(bfrom_, angle).add(from_shift_);
  const Series residual =
      matvec(selector, injection.sub(specified)).abs().amax(-1, false);
  return {angle, injection, from_power, from_power.neg(), residual};
}

Grid make(const Net &net) {
  validate_base(net.base);
  const auto buses = static_cast<std::int64_t>(net.buses.kind.size());
  const auto gens = static_cast<std::int64_t>(net.gens.bus.size());
  const auto lines = static_cast<std::int64_t>(net.lines.from.size());
  if (buses < 2)
    throw std::invalid_argument(
        "eng::power::make: at least two buses are required");
  if (lines < 1 || net.lines.to.size() != net.lines.from.size())
    throw std::invalid_argument("eng::power::make: line endpoint arrays must "
                                "have equal non-zero length");
  if (!net.lines.active.empty() &&
      net.lines.active.size() != net.lines.from.size())
    throw std::invalid_argument(
        "eng::power::make: line status must be empty or match the line count");
  if (!net.gens.active.empty() && net.gens.active.size() != net.gens.bus.size())
    throw std::invalid_argument("eng::power::make: generator status must be "
                                "empty or match the generator count");
  for (const auto bus : net.gens.bus) {
    if (bus < 0 || bus >= buses)
      throw std::invalid_argument(
          "eng::power::make: generator bus lies outside the network");
  }

  std::int64_t slack = 0;
  std::int64_t slack_bus = -1;
  std::vector<std::int64_t> angle_nodes;
  std::vector<std::int64_t> pq_nodes;
  for (std::int64_t bus = 0; bus < buses; ++bus) {
    const auto kind = net.buses.kind[static_cast<std::size_t>(bus)];
    if (kind == BusKind::Slack) {
      ++slack;
      slack_bus = bus;
    } else
      angle_nodes.push_back(bus);
    if (kind == BusKind::PQ)
      pq_nodes.push_back(bus);
  }
  if (slack != 1)
    throw std::invalid_argument(
        "eng::power::make: exactly one slack bus is required");
  for (std::int64_t line = 0; line < lines; ++line) {
    const auto from = net.lines.from[static_cast<std::size_t>(line)];
    const auto to = net.lines.to[static_cast<std::size_t>(line)];
    if (from < 0 || from >= buses || to < 0 || to >= buses || from == to)
      throw std::invalid_argument(
          "eng::power::make: every line must join two distinct valid buses");
  }
  std::vector<std::vector<std::int64_t>> adjacency(
      static_cast<std::size_t>(buses));
  for (std::int64_t line = 0; line < lines; ++line) {
    if (!net.lines.active.empty() &&
        !net.lines.active[static_cast<std::size_t>(line)])
      continue;
    const auto from = net.lines.from[static_cast<std::size_t>(line)];
    const auto to = net.lines.to[static_cast<std::size_t>(line)];
    adjacency[static_cast<std::size_t>(from)].push_back(to);
    adjacency[static_cast<std::size_t>(to)].push_back(from);
  }
  std::vector<bool> reached(static_cast<std::size_t>(buses), false);
  std::queue<std::int64_t> pending;
  reached[static_cast<std::size_t>(slack_bus)] = true;
  pending.push(slack_bus);
  while (!pending.empty()) {
    const auto bus = pending.front();
    pending.pop();
    for (const auto next : adjacency[static_cast<std::size_t>(bus)]) {
      if (!reached[static_cast<std::size_t>(next)]) {
        reached[static_cast<std::size_t>(next)] = true;
        pending.push(next);
      }
    }
  }
  for (const bool connected : reached) {
    if (!connected)
      throw std::invalid_argument(
          "eng::power::make: every bus must connect to the slack island");
  }

  ScalarType dtype = ScalarType::Float64;
  Platform platform = Platform::CPU;
  bool have_config = false;
  Shape batch;
  inspect(net.buses.p, buses, "bus p", false, dtype, platform, have_config,
          batch);
  inspect(net.buses.q, buses, "bus q", false, dtype, platform, have_config,
          batch);
  inspect(net.buses.vm, buses, "bus vm", true, dtype, platform, have_config,
          batch);
  inspect(net.buses.va, buses, "bus va", false, dtype, platform, have_config,
          batch);
  if (gens > 0) {
    inspect(net.gens.p, gens, "generator p", false, dtype, platform,
            have_config, batch);
    inspect(net.gens.q, gens, "generator q", false, dtype, platform,
            have_config, batch);
  } else if ((net.gens.p.defined() && net.gens.p.numel() != 0) ||
             (net.gens.q.defined() && net.gens.q.numel() != 0)) {
    throw std::invalid_argument(
        "eng::power::make: generator values require generator buses");
  }
  inspect(net.lines.r, lines, "line r", false, dtype, platform, have_config,
          batch);
  inspect(net.lines.x, lines, "line x", false, dtype, platform, have_config,
          batch);
  const Series *optional_values[] = {&net.buses.g, &net.buses.b,
                                     &net.lines.charge, &net.lines.tap,
                                     &net.lines.shift};
  const char *optional_roles[] = {"bus g", "bus b", "line charge", "line tap",
                                  "line shift"};
  const std::int64_t optional_counts[] = {buses, buses, lines, lines, lines};
  for (std::size_t i = 0; i < 5; ++i) {
    if (optional_values[i]->defined())
      inspect(*optional_values[i], optional_counts[i], optional_roles[i],
              i == 3, dtype, platform, have_config, batch);
  }

  Series p = expand(net.buses.p, batch, buses);
  const Series fixed_p = p;
  Series q = expand(net.buses.q, batch, buses);
  const Series fixed_q = q;
  const Series vm = expand(net.buses.vm, batch, buses);
  const Series va = expand(net.buses.va, batch, buses);
  const Series g = optional(net.buses.g, batch, buses, 0.0, dtype, platform);
  const Series b = optional(net.buses.b, batch, buses, 0.0, dtype, platform);
  const Series r = expand(net.lines.r, batch, lines);
  const Series x = expand(net.lines.x, batch, lines);
  if (!r.square().add(x.square()).gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::make: line series impedance cannot be zero");
  const Series charge =
      optional(net.lines.charge, batch, lines, 0.0, dtype, platform);
  const Series tap =
      optional(net.lines.tap, batch, lines, 1.0, dtype, platform);
  const Series shift =
      optional(net.lines.shift, batch, lines, 0.0, dtype, platform);
  Series gen_map = pluss::series::undefined();
  Series gen_active = pluss::series::undefined();
  Series gen_p = pluss::series::undefined();
  Series gen_q = pluss::series::undefined();
  if (gens > 0) {
    std::vector<double> gen_status(static_cast<std::size_t>(gens), 1.0);
    if (!net.gens.active.empty()) {
      for (std::int64_t i = 0; i < gens; ++i)
        gen_status[static_cast<std::size_t>(i)] =
            net.gens.active[static_cast<std::size_t>(i)] ? 1.0 : 0.0;
    }
    gen_active = expand(pluss::series::make(gen_status).to(dtype).to(platform),
                        batch, gens);
    gen_map =
        batch_matrix(endpoint_map(net.gens.bus, buses, dtype, platform), batch);
    gen_p = expand(net.gens.p, batch, gens).mul(gen_active);
    gen_q = expand(net.gens.q, batch, gens).mul(gen_active);
    p = p.add(matvec(gen_map.mT(), gen_p));
    q = q.add(matvec(gen_map.mT(), gen_q));
  }

  std::vector<double> active_values(static_cast<std::size_t>(lines), 1.0);
  if (!net.lines.active.empty()) {
    for (std::int64_t i = 0; i < lines; ++i)
      active_values[static_cast<std::size_t>(i)] =
          net.lines.active[static_cast<std::size_t>(i)] ? 1.0 : 0.0;
  }
  const Series active =
      pluss::series::make(active_values).to(dtype).to(platform);
  const Series line_active = expand(active, batch, lines);
  const Series from_map =
      batch_matrix(endpoint_map(net.lines.from, buses, dtype, platform), batch);
  const Series to_map =
      batch_matrix(endpoint_map(net.lines.to, buses, dtype, platform), batch);

  const auto complex_type = complex_dtype(dtype);
  const Series series_y =
      active.to(complex_type).div(complex(r, x, complex_type));
  const Series charging =
      complex(charge.mul(0.0), charge.mul(0.5), complex_type)
          .mul(active.to(complex_type));
  const Series tap_phase = phasor(tap, shift, complex_type);
  const Series ytt = series_y.add(charging);
  const Series yff = ytt.div(tap.square().to(complex_type));
  const Series yft = series_y.neg().div(tap_phase.conj());
  const Series ytf = series_y.neg().div(tap_phase);
  const Series cf = from_map.to(complex_type);
  const Series ct = to_map.to(complex_type);
  const Series yfrom = cf.mul(yff.unsqueeze(-1)).add(ct.mul(yft.unsqueeze(-1)));
  const Series yto = cf.mul(ytf.unsqueeze(-1)).add(ct.mul(ytt.unsqueeze(-1)));
  const Series shunt = batch_diag(complex(g, b, complex_type));
  const Series from_bus = from_map.mT().to(complex_type).matmul(yfrom);
  const Series to_bus = to_map.mT().to(complex_type).matmul(yto);
  const Series ybus = from_bus.add(to_bus).add(shunt);

  const Series cft = from_map.sub(to_map);
  const Series dc_weight = active.div(x.mul(tap));
  const Series bfrom = cft.mul(dc_weight.unsqueeze(-1));
  const Series bbus = cft.mT().matmul(bfrom);
  const Series from_shift = dc_weight.mul(shift).neg();
  const Series bus_shift = matvec(cft.mT(), from_shift);

  const Series angle_map =
      batch_matrix(one_hot_map(angle_nodes, buses,
                               static_cast<std::int64_t>(angle_nodes.size()),
                               dtype, platform),
                   batch);
  const Series pq_map = batch_matrix(
      one_hot_map(pq_nodes, buses, static_cast<std::int64_t>(pq_nodes.size()),
                  dtype, platform),
      batch);
  return Grid(net.base, buses, gens, lines, batch, dtype, p, q, vm, va, ybus,
              yfrom, yto, from_map, to_map, angle_map, pq_map, fixed_p, fixed_q,
              gen_map, gen_active, gen_p, gen_q, line_active, bbus, bfrom,
              bus_shift, from_shift);
}

DcResult dc(const Grid &grid) { return grid.solve_dc(grid.p_); }

AcResult ac(const Grid &grid, const Options &options) {
  if (!std::isfinite(options.tolerance) || options.tolerance <= 0.0 ||
      options.max_iterations < 1)
    throw std::invalid_argument(
        "eng::power::ac: tolerance and maximum iterations must be positive");

  const detail::AcKernel kernel(grid);
  Series state = kernel.initial_state();
  detail::AcKernel::Eval current = kernel.evaluate(state, grid.p_, grid.q_);
  Series converged = current.residual.le(options.tolerance);
  std::int64_t iterations = 0;
  while (!converged.all().item<bool>() && iterations < options.max_iterations) {
    const Series step = current.jacobian.linearalg
                            .lu_solve(current.mismatch.neg().unsqueeze(-1))
                            .squeeze(-1)
                            .to(grid.dtype_);
    state = state.add(step);
    ++iterations;
    current = kernel.evaluate(state, grid.p_, grid.q_);
    converged = current.residual.le(options.tolerance);
  }

  const detail::AcKernel::Flow flows = kernel.flow(current.voltage);
  return {current.voltage,   current.magnitude, current.angle,
          current.injection, flows.from,        flows.to,
          current.residual,  converged,         iterations};
}

Series ptdf(const Grid &grid) {
  const Series selector = grid.angle_map_.mT();
  const Series reduced = selector.matmul(grid.bbus_).matmul(grid.angle_map_);
  const Series response = reduced.linearalg.lu_solve(selector);
  return grid.bfrom_.matmul(grid.angle_map_).matmul(response);
}

LodfResult lodf(const Grid &grid, double tolerance) {
  if (!std::isfinite(tolerance) || tolerance <= 0.0)
    throw std::invalid_argument(
        "eng::power::lodf: tolerance must be finite and positive");

  const Series transfer = grid.from_map_.sub(grid.to_map_).mT();
  const Series response = ptdf(grid).matmul(transfer);
  const Series denominator = batch_diagonal(response).neg().add(1.0);
  const Series valid =
      denominator.abs().gt(tolerance).logical_and(grid.line_active_.gt(0.0));
  const Series safe_denominator =
      valid.where(denominator, pluss::series::ones_like(denominator));
  const Series raw = response.div(safe_denominator.unsqueeze(-2));
  const Series raw_diagonal = batch_diagonal(raw);
  const Series factor =
      raw.sub(batch_diag(raw_diagonal))
          .sub(batch_diag(pluss::series::ones_like(raw_diagonal)));
  const Series absent = pluss::series::full_like(
      factor, std::numeric_limits<double>::quiet_NaN());
  return {valid.unsqueeze(-2).where(factor, absent), valid};
}

OutageResult outages(const Grid &grid, double tolerance) {
  const DcResult base = dc(grid);
  const LodfResult factors = lodf(grid, tolerance);
  const Series flow = base.from_power.unsqueeze(-2).add(
      factors.factor.mT().mul(base.from_power.unsqueeze(-1)));
  const Series absent =
      pluss::series::full_like(flow, std::numeric_limits<double>::quiet_NaN());
  return {base.from_power, factors.valid.unsqueeze(-1).where(flow, absent),
          factors.valid};
}

DispatchResult dispatch(const Grid &grid, const GenCost &cost,
                        const Series &line_limit,
                        const DispatchOptions &options) {
  namespace qp = pluss::subsp::math::qp;
  if (grid.gens_ < 1)
    throw std::invalid_argument(
        "eng::power::dispatch: at least one generator is required");
  if (!std::isfinite(options.outage_tolerance) ||
      options.outage_tolerance <= 0.0)
    throw std::invalid_argument(
        "eng::power::dispatch: outage tolerance must be finite and positive");

  const Series lower =
      grid_value(cost.lower, grid.batch_shape_, grid.gens_, grid.dtype_,
                 grid.platform(), "dispatch", "generator lower bounds", false);
  const Series upper =
      grid_value(cost.upper, grid.batch_shape_, grid.gens_, grid.dtype_,
                 grid.platform(), "dispatch", "generator upper bounds", false);
  const Series linear =
      grid_value(cost.linear, grid.batch_shape_, grid.gens_, grid.dtype_,
                 grid.platform(), "dispatch", "linear costs");
  const Series quadratic =
      grid_value(cost.quadratic, grid.batch_shape_, grid.gens_, grid.dtype_,
                 grid.platform(), "dispatch", "quadratic costs");
  const Series limit =
      grid_value(line_limit, grid.batch_shape_, grid.lines_, grid.dtype_,
                 grid.platform(), "dispatch", "line limits", false);
  if (lower.isnan().any().item<bool>() || upper.isnan().any().item<bool>())
    throw std::invalid_argument(
        "eng::power::dispatch: generator bounds must not be NaN");
  if (!lower.le(upper).all().item<bool>())
    throw std::invalid_argument("eng::power::dispatch: generator lower bounds "
                                "must not exceed upper bounds");
  if (!quadratic.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::dispatch: quadratic costs must be non-negative");
  if (limit.isnan().any().item<bool>() || !limit.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::dispatch: line limits must be positive and not NaN");
  if (!grid.gen_active_.gt(0.0).any().item<bool>())
    throw std::invalid_argument(
        "eng::power::dispatch: at least one generator must be active");

  const Series zero = pluss::series::zeros_like(lower);
  const Series active = grid.gen_active_.gt(0.0);
  const Series bounded_lower = active.where(lower, zero);
  const Series bounded_upper = active.where(upper, zero);
  const Series identity = batch_matrix(
      pluss::series::eye(grid.gens_, config(grid.dtype_, grid.platform())),
      grid.batch_shape_);
  const Series balance = pluss::series::ones_like(lower).unsqueeze(-2);
  const Series balance_value = grid.fixed_p_.sum(-1, false).neg().unsqueeze(-1);

  const Series transfer = ptdf(grid);
  const Series flow_matrix = transfer.matmul(grid.gen_map_.mT());
  const Series fixed_flow = grid.solve_dc(grid.fixed_p_).from_power;
  const Series flow_lower = limit.neg().sub(fixed_flow);
  const Series flow_upper = limit.sub(fixed_flow);

  Series constraint = balance.unite({identity, flow_matrix}, -2);
  Series constraint_lower =
      balance_value.unite({bounded_lower, flow_lower}, -1);
  Series constraint_upper =
      balance_value.unite({bounded_upper, flow_upper}, -1);

  LodfResult factors{pluss::series::undefined(), pluss::series::undefined()};
  if (options.secure) {
    factors = lodf(grid, options.outage_tolerance);
    const Series transposed = factors.factor.mT();
    Series post_matrix = flow_matrix.unsqueeze(-3).add(
        transposed.unsqueeze(-1).mul(flow_matrix.unsqueeze(-2)));
    Series post_fixed =
        fixed_flow.unsqueeze(-2).add(transposed.mul(fixed_flow.unsqueeze(-1)));
    const Series valid = factors.valid.unsqueeze(-1);
    post_matrix = valid.unsqueeze(-1).where(
        post_matrix, pluss::series::zeros_like(post_matrix));
    const Series neg_inf = pluss::series::full_like(
        post_fixed, -std::numeric_limits<double>::infinity());
    const Series pos_inf = pluss::series::full_like(
        post_fixed, std::numeric_limits<double>::infinity());
    Series post_lower =
        valid.where(limit.neg().unsqueeze(-2).sub(post_fixed), neg_inf);
    Series post_upper =
        valid.where(limit.unsqueeze(-2).sub(post_fixed), pos_inf);
    Shape matrix_shape = grid.batch_shape_;
    matrix_shape.push_back(grid.lines_ * grid.lines_);
    matrix_shape.push_back(grid.gens_);
    Shape vector_shape = grid.batch_shape_;
    vector_shape.push_back(grid.lines_ * grid.lines_);
    constraint = constraint.unite({post_matrix.reshape(matrix_shape)}, -2);
    constraint_lower =
        constraint_lower.unite({post_lower.reshape(vector_shape)}, -1);
    constraint_upper =
        constraint_upper.unite({post_upper.reshape(vector_shape)}, -1);
  }

  const Series hessian = batch_diag(quadratic.mul(2.0));
  const qp::Result solved =
      qp::solve(hessian, linear, constraint, constraint_lower, constraint_upper,
                options.qp);
  const Series generation = solved.x;
  const Series injection =
      grid.fixed_p_.add(matvec(grid.gen_map_.mT(), generation));
  const DcResult state = grid.solve_dc(injection);
  const Series objective = quadratic.mul(generation.square())
                               .add(linear.mul(generation))
                               .sum(-1, false);

  Series post_flow = pluss::series::undefined();
  if (options.secure) {
    const Series candidate = state.from_power.unsqueeze(-2).add(
        factors.factor.mT().mul(state.from_power.unsqueeze(-1)));
    post_flow = factors.valid.unsqueeze(-1).where(
        candidate, pluss::series::full_like(
                       candidate, std::numeric_limits<double>::quiet_NaN()));
  }
  return {generation,          state.angle,
          state.injection,     state.from_power,
          post_flow,           factors.valid,
          objective,           solved.converged,
          solved.iterations,   solved.primal_residual,
          solved.dual_residual};
}

OpfResult opf(const Grid &grid, const GenCost &cost, const Limits &limits,
              const OpfOptions &options) {
  namespace optim = pluss::subsp::math::optim;
  if (grid.gens_ < 1 || !grid.gen_active_.gt(0.0).any().item<bool>())
    throw std::invalid_argument(
        "eng::power::opf: at least one generator must be active");
  if (options.max_iterations < 1 ||
      !std::isfinite(options.constraint_tolerance) ||
      options.constraint_tolerance <= 0.0 ||
      !std::isfinite(options.optimality_tolerance) ||
      options.optimality_tolerance <= 0.0)
    throw std::invalid_argument(
        "eng::power::opf: iteration limit and tolerances must be positive");

  const auto gen_value = [&](const Series &value, const char *role,
                             bool finite = true) {
    return grid_value(value, grid.batch_shape_, grid.gens_, grid.dtype_,
                      grid.platform(), "opf", role, finite);
  };
  const auto bus_value = [&](const Series &value, const char *role,
                             bool finite = true) {
    return grid_value(value, grid.batch_shape_, grid.buses_, grid.dtype_,
                      grid.platform(), "opf", role, finite);
  };
  const Series p_lower = gen_value(cost.lower, "active lower bounds", false);
  const Series p_upper = gen_value(cost.upper, "active upper bounds", false);
  const Series q_lower =
      gen_value(limits.q_lower, "reactive lower bounds", false);
  const Series q_upper =
      gen_value(limits.q_upper, "reactive upper bounds", false);
  const Series vm_lower =
      bus_value(limits.vm_lower, "voltage lower bounds", false);
  const Series vm_upper =
      bus_value(limits.vm_upper, "voltage upper bounds", false);
  const Series linear = gen_value(cost.linear, "active linear costs");
  const Series quadratic = gen_value(cost.quadratic, "active quadratic costs");
  const auto optional_cost = [&](const Series &value, const char *role) {
    return value.defined() ? gen_value(value, role)
                           : pluss::series::zeros_like(linear);
  };
  const Series constant = optional_cost(cost.constant, "active constant costs");
  const Series q_linear = optional_cost(cost.q_linear, "reactive linear costs");
  const Series q_quadratic =
      optional_cost(cost.q_quadratic, "reactive quadratic costs");
  const Series q_constant =
      optional_cost(cost.q_constant, "reactive constant costs");

  const bool constrain_flow = limits.flow.defined();
  const Series flow_limit =
      constrain_flow
          ? grid_value(limits.flow, grid.batch_shape_, grid.lines_, grid.dtype_,
                       grid.platform(), "opf", "branch ratings")
          : pluss::series::ones_like(grid.line_active_);

  const auto has_nan = [](const Series &value) {
    return value.isnan().any().item<bool>();
  };
  if (has_nan(p_lower) || has_nan(p_upper) || has_nan(q_lower) ||
      has_nan(q_upper) || has_nan(vm_lower) || has_nan(vm_upper) ||
      has_nan(flow_limit))
    throw std::invalid_argument("eng::power::opf: bounds must not be NaN");
  if (!p_lower.le(p_upper).all().item<bool>() ||
      !q_lower.le(q_upper).all().item<bool>() ||
      !vm_lower.le(vm_upper).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::opf: every lower bound must not exceed its upper bound");
  if (!vm_lower.gt(0.0).all().item<bool>() ||
      !flow_limit.gt(0.0).all().item<bool>())
    throw std::invalid_argument("eng::power::opf: voltage lower bounds and "
                                "branch ratings must be positive");
  if (!quadratic.ge(0.0).all().item<bool>() ||
      !q_quadratic.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::opf: quadratic costs must be non-negative");

  const detail::AcKernel kernel(grid);
  const std::int64_t batches = batch_count(grid.batch_shape_);
  const std::int64_t angle_count = grid.buses_ - 1;
  const std::int64_t width = angle_count + grid.buses_ + 2 * grid.gens_;
  const std::int64_t equality_rows = 2 * grid.buses_;
  const std::int64_t inequality_rows = constrain_flow ? 2 * grid.lines_ : 0;
  const Series active_gen = grid.gen_active_.gt(0.0);
  const Series zero_gen = pluss::series::zeros_like(grid.gen_active_);
  const Series bounded_p_lower = active_gen.where(p_lower, zero_gen);
  const Series bounded_p_upper = active_gen.where(p_upper, zero_gen);
  const Series bounded_q_lower = active_gen.where(q_lower, zero_gen);
  const Series bounded_q_upper = active_gen.where(q_upper, zero_gen);
  const Series angle_vars = matvec(grid.angle_map_.mT(), grid.va_);
  const Series angle_lower = pluss::series::full_like(
      angle_vars, -std::numeric_limits<double>::infinity());
  const Series angle_upper = pluss::series::full_like(
      angle_vars, std::numeric_limits<double>::infinity());
  const Series initial =
      angle_vars.unite({grid.vm_, grid.gen_p_, grid.gen_q_}, -1)
          .reshape({batches * width});
  const Series lower =
      angle_lower.unite({vm_lower, bounded_p_lower, bounded_q_lower}, -1)
          .reshape({batches * width});
  const Series upper =
      angle_upper.unite({vm_upper, bounded_p_upper, bounded_q_upper}, -1)
          .reshape({batches * width});

  const auto shaped = [&](std::int64_t trailing) {
    Shape shape = grid.batch_shape_;
    shape.push_back(trailing);
    return shape;
  };
  const auto unpack = [&](const Series &flat) {
    const Series state = flat.to(grid.dtype_).reshape({batches, width});
    const Series va =
        state.slice(-1, 0, angle_count).reshape(shaped(angle_count));
    const Series vm = state.slice(-1, angle_count, angle_count + grid.buses_)
                          .reshape(shaped(grid.buses_));
    const Series pg = state
                          .slice(-1, angle_count + grid.buses_,
                                 angle_count + grid.buses_ + grid.gens_)
                          .reshape(shaped(grid.gens_));
    const Series qg =
        state.slice(-1, angle_count + grid.buses_ + grid.gens_, width)
            .reshape(shaped(grid.gens_));
    return std::array<Series, 4>{va, vm, pg, qg};
  };
  const auto restore_batch = [&](const Series &value, std::int64_t trailing) {
    Shape shape = grid.batch_shape_;
    shape.push_back(trailing);
    return value.reshape(shape);
  };

  optim::ConstrainedProblem problem;
  problem.objective.analytic = [&, batches, width](
                                   const optim::ParameterPack<Series> &params) {
    const auto [va, vm, pg, qg] = unpack(params.at("x"));
    (void)va;
    (void)vm;
    const Series scenario =
        quadratic.reshape({batches, grid.gens_})
            .mul(pg.square())
            .add(linear.reshape({batches, grid.gens_}).mul(pg))
            .add(constant.reshape({batches, grid.gens_}))
            .add(q_quadratic.reshape({batches, grid.gens_}).mul(qg.square()))
            .add(q_linear.reshape({batches, grid.gens_}).mul(qg))
            .add(q_constant.reshape({batches, grid.gens_}))
            .sum(-1, false);
    const Series zero_state =
        pluss::series::zeros({batches, angle_count + grid.buses_}, grid.dtype_)
            .to(grid.platform());
    const Series gradient =
        zero_state
            .unite({quadratic.reshape({batches, grid.gens_})
                        .mul(pg)
                        .mul(2.0)
                        .add(linear.reshape({batches, grid.gens_})),
                    q_quadratic.reshape({batches, grid.gens_})
                        .mul(qg)
                        .mul(2.0)
                        .add(q_linear.reshape({batches, grid.gens_}))},
                   -1)
            .reshape({batches * width});
    return std::pair{scenario.sum(), optim::pack_series(gradient, "x")};
  };

  problem.nonlinear.analytic = [&, batches, width, equality_rows,
                                inequality_rows](
                                   const optim::ParameterPack<Series> &params) {
    const auto [va, vm, pg, qg] = unpack(params.at("x"));
    const Series angle = kernel.fixed_angle.add(matvec(grid.angle_map_, va));
    const detail::AcKernel::BusEval bus = kernel.bus(vm, angle);
    const detail::AcKernel::FlowEval flow = kernel.flow_eval(bus.voltage, vm);
    const Series gen_bus =
        grid.gen_map_.mT().reshape({batches, grid.buses_, grid.gens_});
    const Series specified_p =
        grid.fixed_p_.reshape({batches, grid.buses_})
            .add(matvec(gen_bus, pg.reshape({batches, grid.gens_})));
    const Series specified_q =
        grid.fixed_q_.reshape({batches, grid.buses_})
            .add(matvec(gen_bus, qg.reshape({batches, grid.gens_})));
    const Series equality = bus.injection.real()
                                .reshape({batches, grid.buses_})
                                .sub(specified_p)
                                .unite({bus.injection.imag()
                                            .reshape({batches, grid.buses_})
                                            .sub(specified_q)},
                                       -1);

    const Series angle_map =
        grid.angle_map_.reshape({batches, grid.buses_, angle_count});
    const Series bus_da =
        bus.d_angle.reshape({batches, grid.buses_, grid.buses_});
    const Series bus_dm =
        bus.d_magnitude.reshape({batches, grid.buses_, grid.buses_});
    const Series dpa = bus_da.real().matmul(angle_map);
    const Series dqa = bus_da.imag().matmul(angle_map);
    const Series zero_bg = gen_bus.mul(0.0);
    const Series p_jac = dpa.unite({bus_dm.real(), gen_bus.neg(), zero_bg}, -1);
    const Series q_jac = dqa.unite({bus_dm.imag(), zero_bg, gen_bus.neg()}, -1);
    const Series equality_jac = p_jac.unite({q_jac}, -2);

    const Series flat_limit = flow_limit.reshape({batches, grid.lines_});
    const Series rated =
        grid.line_active_.reshape({batches, grid.lines_}).gt(0.0);
    const Series from_power = flow.from.reshape({batches, grid.lines_});
    const Series to_power = flow.to.reshape({batches, grid.lines_});
    Series inequality =
        pluss::series::empty({0}, grid.dtype_).to(grid.platform());
    Series inequality_jac =
        pluss::series::empty({0, batches * width}, grid.dtype_)
            .to(grid.platform());
    const auto flow_jacobian = [&](const Series &power, const Series &d_angle,
                                   const Series &d_magnitude) {
      const Series da =
          power.conj()
              .unsqueeze(-1)
              .mul(d_angle.matmul(angle_map.to(d_angle.scalar_type())))
              .real()
              .mul(2.0);
      const Series dm =
          power.conj().unsqueeze(-1).mul(d_magnitude).real().mul(2.0);
      const Series zero =
          pluss::series::zeros({batches, grid.lines_, grid.gens_}, grid.dtype_)
              .to(grid.platform());
      return da.unite({dm, zero, zero}, -1).mul(rated.unsqueeze(-1));
    };
    if (inequality_rows > 0) {
      const Series from_residual =
          rated.where(from_power.abs().square().sub(flat_limit.square()),
                      pluss::series::full_like(flat_limit, -1.0));
      const Series to_residual =
          rated.where(to_power.abs().square().sub(flat_limit.square()),
                      pluss::series::full_like(flat_limit, -1.0));
      inequality = from_residual.unite({to_residual}, -1);
      const Series from_jac = flow_jacobian(
          from_power,
          flow.d_from_angle.reshape({batches, grid.lines_, grid.buses_}),
          flow.d_from_magnitude.reshape({batches, grid.lines_, grid.buses_}));
      const Series to_jac = flow_jacobian(
          to_power,
          flow.d_to_angle.reshape({batches, grid.lines_, grid.buses_}),
          flow.d_to_magnitude.reshape({batches, grid.lines_, grid.buses_}));
      inequality_jac = block_jacobian(from_jac.unite({to_jac}, -2), batches,
                                      inequality_rows, width);
    }

    optim::NonlinearConstraintEvaluation out;
    out.equality = equality.reshape({batches * equality_rows});
    out.inequality = inequality.reshape({batches * inequality_rows});
    out.equality_jacobian = optim::ParameterPack<Series>(
        std::vector<optim::ParameterPack<Series>::Block>{
            {"x",
             block_jacobian(equality_jac, batches, equality_rows, width)}});
    out.inequality_jacobian = optim::ParameterPack<Series>(
        std::vector<optim::ParameterPack<Series>::Block>{
            {"x", inequality_jac}});
    return out;
  };
  problem.bounds.lower = optim::pack_series(lower, "x");
  problem.bounds.upper = optim::pack_series(upper, "x");

  optim::OptimaOptions solver = optim::presets::sequential_qp();
  solver.sequential_qp.max_iterations = options.max_iterations;
  solver.sequential_qp.constraint_tolerance = options.constraint_tolerance;
  solver.sequential_qp.optimality_tolerance = options.optimality_tolerance;
  const optim::OptimaState solved =
      optim::optima(problem, optim::pack_series(initial, "x"), solver);
  const auto [va, vm, pg, qg] = unpack(solved.params.at("x"));
  const Series angle = kernel.fixed_angle.add(matvec(grid.angle_map_, va));
  const detail::AcKernel::BusEval bus = kernel.bus(vm, angle);
  const detail::AcKernel::FlowEval flow = kernel.flow_eval(bus.voltage, vm);
  const Series scenario_objective =
      quadratic.reshape({batches, grid.gens_})
          .mul(pg.square())
          .add(linear.reshape({batches, grid.gens_}).mul(pg))
          .add(constant.reshape({batches, grid.gens_}))
          .add(q_quadratic.reshape({batches, grid.gens_}).mul(qg.square()))
          .add(q_linear.reshape({batches, grid.gens_}).mul(qg))
          .add(q_constant.reshape({batches, grid.gens_}))
          .sum(-1, false);

  const auto shadow_or_zero = [&](const Series &value, std::int64_t rows) {
    if (value.defined() && value.numel() == batches * rows)
      return value.reshape({batches, rows});
    return pluss::series::zeros({batches, rows}, ScalarType::Float64)
        .to(grid.platform());
  };
  const Series equality_shadow =
      shadow_or_zero(solved.multipliers.nonlinear_equalities, equality_rows);
  const Series inequality_shadow = shadow_or_zero(
      solved.multipliers.nonlinear_inequalities, inequality_rows);
  const Series from_shadow =
      inequality_rows > 0
          ? inequality_shadow.slice(-1, 0, grid.lines_)
          : pluss::series::zeros({batches, grid.lines_}, ScalarType::Float64)
                .to(grid.platform());
  const Series to_shadow =
      inequality_rows > 0
          ? inequality_shadow.slice(-1, grid.lines_, inequality_rows)
          : pluss::series::zeros({batches, grid.lines_}, ScalarType::Float64)
                .to(grid.platform());
  return {restore_batch(pg.to(grid.dtype_), grid.gens_),
          restore_batch(qg.to(grid.dtype_), grid.gens_),
          restore_batch(bus.voltage, grid.buses_),
          restore_batch(vm.to(grid.dtype_), grid.buses_),
          restore_batch(angle.to(grid.dtype_), grid.buses_),
          restore_batch(bus.injection, grid.buses_),
          restore_batch(flow.from, grid.lines_),
          restore_batch(flow.to, grid.lines_),
          scenario_objective.reshape(grid.batch_shape_),
          restore_batch(equality_shadow.slice(-1, 0, grid.buses_), grid.buses_),
          restore_batch(equality_shadow.slice(-1, grid.buses_, equality_rows),
                        grid.buses_),
          restore_batch(from_shadow, grid.lines_),
          restore_batch(to_shadow, grid.lines_),
          solved.converged,
          solved.iteration,
          solved.evaluation.max_constraint_violation,
          solved.output.first_order_optimality,
          solved.message};
}

EstimateResult estimate(const Grid &grid, const Measurements &measurements,
                        const EstimateOptions &options) {
  if (!std::isfinite(options.tolerance) || options.tolerance <= 0.0 ||
      !std::isfinite(options.observability_tolerance) ||
      options.observability_tolerance <= 0.0 || options.max_iterations < 1)
    throw std::invalid_argument("eng::power::estimate: tolerances and maximum "
                                "iterations must be positive");
  const auto count = static_cast<std::int64_t>(measurements.kind.size());
  if (count < 1 || measurements.index.size() != measurements.kind.size())
    throw std::invalid_argument(
        "eng::power::estimate: kind and index must have equal non-zero length");
  const auto angle_count = grid.buses_ - 1;
  const auto state_count = angle_count + grid.buses_;
  if (count < state_count)
    throw std::invalid_argument("eng::power::estimate: measurement count is "
                                "smaller than the state dimension");

  const Series observed =
      grid_value(measurements.value, grid.batch_shape_, count, grid.dtype_,
                 grid.platform(), "estimate", "measurement values");
  const Series stddev = grid_value(
      measurements.stddev, grid.batch_shape_, count, grid.dtype_,
      grid.platform(), "estimate", "measurement standard deviations");
  if (!stddev.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::estimate: standard deviations must be positive");

  std::vector<std::int64_t> rows;
  std::vector<double> angle_flags;
  rows.reserve(static_cast<std::size_t>(count));
  angle_flags.reserve(static_cast<std::size_t>(count));
  const auto line = grid.lines_;
  const auto bus = grid.buses_;
  for (std::int64_t measurement = 0; measurement < count; ++measurement) {
    const auto kind = measurements.kind[static_cast<std::size_t>(measurement)];
    const auto index =
        measurements.index[static_cast<std::size_t>(measurement)];
    const bool bus_measure =
        kind == MeasureKind::BusP || kind == MeasureKind::Angle ||
        kind == MeasureKind::BusQ || kind == MeasureKind::Magnitude;
    const auto width = bus_measure ? bus : line;
    if (index < 0 || index >= width)
      throw std::invalid_argument("eng::power::estimate: measurement index "
                                  "lies outside its element family");
    std::int64_t offset = 0;
    switch (kind) {
    case MeasureKind::FromP:
      offset = 0;
      break;
    case MeasureKind::ToP:
      offset = line;
      break;
    case MeasureKind::BusP:
      offset = 2 * line;
      break;
    case MeasureKind::Angle:
      offset = 2 * line + bus;
      break;
    case MeasureKind::FromQ:
      offset = 2 * line + 2 * bus;
      break;
    case MeasureKind::ToQ:
      offset = 3 * line + 2 * bus;
      break;
    case MeasureKind::BusQ:
      offset = 4 * line + 2 * bus;
      break;
    case MeasureKind::Magnitude:
      offset = 4 * line + 3 * bus;
      break;
    }
    rows.push_back(offset + index);
    angle_flags.push_back(kind == MeasureKind::Angle ? 1.0 : 0.0);
  }
  const Series row_index = pluss::series::make(rows).to(grid.platform());
  const Series angle_mask =
      pluss::series::make(angle_flags).gt(0.0).to(grid.platform());

  const detail::AcKernel kernel(grid);
  const auto complex_type = kernel.complex_type;
  const Series angle_map_complex = grid.angle_map_.to(complex_type);
  const Series identity = batch_matrix(
      pluss::series::eye(bus, config(grid.dtype_, grid.platform())),
      grid.batch_shape_);
  const Series zero_square = identity.mul(0.0);
  const Series angle_jacobian = grid.angle_map_.unite({zero_square}, -1);
  const Series magnitude_jacobian =
      grid.angle_map_.mul(0.0).unite({identity}, -1);

  struct Evaluation {
    Series voltage;
    Series injection;
    Series from_power;
    Series to_power;
    Series predicted;
    Series jacobian;
  };

  const auto evaluate = [&](const Series &magnitude,
                            const Series &angle) -> Evaluation {
    const detail::AcKernel::BusEval values = kernel.bus(magnitude, angle);
    const Series volts = values.voltage;
    const detail::AcKernel::FlowEval flow = kernel.flow_eval(volts, magnitude);

    const auto state_jacobian = [&](const Series &d_angle,
                                    const Series &d_magnitude) {
      return d_angle.matmul(angle_map_complex).unite({d_magnitude}, -1);
    };
    const Series from_jacobian =
        state_jacobian(flow.d_from_angle, flow.d_from_magnitude);
    const Series to_jacobian =
        state_jacobian(flow.d_to_angle, flow.d_to_magnitude);
    const Series bus_jacobian =
        state_jacobian(values.d_angle, values.d_magnitude);
    const Series canonical = flow.from.real().unite(
        {flow.to.real(), values.injection.real(), angle, flow.from.imag(),
         flow.to.imag(), values.injection.imag(), magnitude},
        -1);
    const Series canonical_jacobian = from_jacobian.real().unite(
        {to_jacobian.real(), bus_jacobian.real(), angle_jacobian,
         from_jacobian.imag(), to_jacobian.imag(), bus_jacobian.imag(),
         magnitude_jacobian},
        -2);
    return {volts,
            values.injection,
            flow.from,
            flow.to,
            canonical.index_select(-1, row_index),
            canonical_jacobian.index_select(-2, row_index)};
  };

  const Series angle_selector = grid.angle_map_.mT();
  Series angle_vars = matvec(angle_selector, grid.va_);
  const Series fixed_angle = grid.va_.sub(matvec(grid.angle_map_, angle_vars));
  Series magnitude = grid.vm_;
  Series angle = fixed_angle.add(matvec(grid.angle_map_, angle_vars));
  Series converged = observed.sum(-1, false).mul(0.0).gt(1.0);
  std::int64_t iterations = 0;

  while (!converged.all().item<bool>() && iterations < options.max_iterations) {
    const Evaluation current = evaluate(magnitude, angle);
    Series residual = observed.sub(current.predicted);
    const Series wrapped = residual.sin().atan2(residual.cos());
    residual = angle_mask.where(wrapped, residual);
    const Series scaled_jacobian = current.jacobian.div(stddev.unsqueeze(-1));
    const Series scaled_residual = residual.div(stddev);
    const Series gain = scaled_jacobian.mT().matmul(scaled_jacobian);
    const Series spectrum = gain.linearalg.eigvalsh();
    const double eigen_floor = spectrum.amin().item<double>();
    const double eigen_scale = std::max(1.0, spectrum.amax().item<double>());
    if (!std::isfinite(eigen_floor) ||
        eigen_floor <= options.observability_tolerance * eigen_scale)
      throw std::invalid_argument(
          "eng::power::estimate: measurement Jacobian is not observable");
    const Series rhs =
        scaled_jacobian.mT().matmul(scaled_residual.unsqueeze(-1));
    const Series raw_step = gain.linearalg.cholesky(false)
                                .linearalg.cholesky_solve(rhs, false)
                                .squeeze(-1);
    const Series magnitude_step =
        raw_step.slice(-1, angle_count, angle_count + grid.buses_);
    const Series step_limit = magnitude_step.lt(0.0).where(
        magnitude.div(magnitude_step.neg()).mul(0.95),
        pluss::series::full_like(magnitude_step,
                                 std::numeric_limits<double>::infinity()));
    const Series safe_step =
        raw_step.mul(step_limit.amin(-1, false).clamp_max(1.0).unsqueeze(-1));
    const Series step = converged.unsqueeze(-1).where(
        pluss::series::zeros_like(safe_step), safe_step);
    angle_vars = angle_vars.add(step.slice(-1, 0, angle_count));
    magnitude =
        magnitude.add(step.slice(-1, angle_count, angle_count + grid.buses_));
    angle = fixed_angle.add(matvec(grid.angle_map_, angle_vars));
    converged = converged.logical_or(
        safe_step.abs().amax(-1, false).le(options.tolerance));
    ++iterations;
  }

  const Evaluation final = evaluate(magnitude, angle);
  Series residual = observed.sub(final.predicted);
  residual = angle_mask.where(residual.sin().atan2(residual.cos()), residual);
  const Series scaled_jacobian = final.jacobian.div(stddev.unsqueeze(-1));
  const Series gain = scaled_jacobian.mT().matmul(scaled_jacobian);
  const Series state_identity = batch_matrix(
      pluss::series::eye(state_count, config(grid.dtype_, grid.platform())),
      grid.batch_shape_);
  const Series covariance =
      gain.linearalg.cholesky(false).linearalg.cholesky_solve(state_identity,
                                                              false);
  const Series leverage =
      final.jacobian.matmul(covariance).mul(final.jacobian).sum(-1, false);
  const Series residual_variance = stddev.square().sub(leverage);
  const Series reliable = residual_variance.gt(
      stddev.square().mul(options.observability_tolerance));
  const Series normalized =
      reliable.where(residual.div(residual_variance.clamp_min(0.0).sqrt()),
                     pluss::series::zeros_like(residual));
  const Series objective = residual.div(stddev).square().sum(-1, false);
  return {final.voltage,    magnitude,      angle,           final.injection,
          final.from_power, final.to_power, final.predicted, residual,
          normalized,       objective,      covariance,      converged,
          iterations};
}

TraceResult trace(const Grid &grid, const LoadPath &path,
                  const TraceOptions &options) {
  if (!std::isfinite(options.step) || options.step <= 0.0 ||
      !std::isfinite(options.step_min) || options.step_min <= 0.0 ||
      !std::isfinite(options.step_max) || options.step_max < options.step ||
      options.step_min > options.step ||
      !std::isfinite(options.adapt_tolerance) ||
      options.adapt_tolerance <= 0.0 || !std::isfinite(options.adapt_damping) ||
      options.adapt_damping < 0.0 || options.adapt_damping > 1.0 ||
      !std::isfinite(options.tolerance) || options.tolerance <= 0.0 ||
      options.max_steps < 1 || options.max_iterations < 1)
    throw std::invalid_argument(
        "eng::power::trace: step bounds, tolerance, and iteration limits "
        "must be positive and ordered");
  if (options.stop == TraceStop::Target &&
      (!std::isfinite(options.target) || options.target == 0.0))
    throw std::invalid_argument(
        "eng::power::trace: target loading factor must be finite and non-zero");

  const Series target_p =
      grid_value(path.p, grid.batch_shape_, grid.buses_, grid.dtype_,
                 grid.platform(), "trace", "target active injections");
  const Series target_q =
      grid_value(path.q, grid.batch_shape_, grid.buses_, grid.dtype_,
                 grid.platform(), "trace", "target reactive injections");
  const Series delta_p = target_p.sub(grid.p_);
  const Series delta_q = target_q.sub(grid.q_);
  const detail::AcKernel kernel(grid);
  Series lambda_derivative = matvec(kernel.angle_selector, delta_p);
  if (kernel.pq_count > 0)
    lambda_derivative =
        lambda_derivative.unite({matvec(kernel.pq_selector, delta_q)}, -1);
  lambda_derivative = lambda_derivative.neg();
  if (!lambda_derivative.abs().amax(-1, false).gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::power::trace: every scenario needs a non-zero controllable "
        "transfer direction");

  const Options base_options{.tolerance = options.tolerance,
                             .max_iterations = options.max_iterations};
  const AcResult base = ac(grid, base_options);
  if (!base.converged.all().item<bool>())
    throw std::runtime_error(
        "eng::power::trace: base power flow did not converge");

  const std::int64_t state_count = kernel.angle_count + kernel.pq_count;
  const std::int64_t bordered_count = state_count + 1;
  const Series bordered_identity = batch_matrix(
      pluss::series::eye(bordered_count, config(grid.dtype_, grid.platform())),
      grid.batch_shape_);
  const Series state_identity = batch_matrix(
      pluss::series::eye(state_count, config(grid.dtype_, grid.platform())),
      grid.batch_shape_);
  const Series zero_lane = grid.p_.sum(-1, false).mul(0.0);
  const Series all_lanes = zero_lane.eq(0.0);

  const auto schedule = [&](const Series &factor) {
    return std::pair<Series, Series>{
        grid.p_.add(delta_p.mul(factor.unsqueeze(-1))),
        grid.q_.add(delta_q.mul(factor.unsqueeze(-1)))};
  };
  const auto split = [&](const Series &z) {
    return std::pair<Series, Series>{z.slice(-1, 0, state_count),
                                     z.select(-1, state_count)};
  };
  const auto bordered = [&](const Series &jacobian, const Series &border) {
    return jacobian.unite({lambda_derivative.unsqueeze(-1)}, -1)
        .unite({border.unsqueeze(-2)}, -2);
  };
  const auto safe_solve = [&](const Series &matrix, const Series &rhs,
                              const Series &work) {
    const Series &identity =
        matrix.size(-1) == bordered_count ? bordered_identity : state_identity;
    const Series safe_matrix =
        work.unsqueeze(-1).unsqueeze(-1).where(matrix, identity);
    const Series safe_rhs =
        work.unsqueeze(-1).where(rhs, pluss::series::zeros_like(rhs));
    return safe_matrix.linearalg.lu_solve(safe_rhs.unsqueeze(-1))
        .squeeze(-1)
        .to(grid.dtype_);
  };
  const auto limit_step = [&](const Series &z, const Series &raw_step) {
    if (kernel.pq_count == 0)
      return raw_step;
    const Series magnitude =
        z.slice(-1, kernel.angle_count, kernel.angle_count + kernel.pq_count);
    const Series magnitude_step = raw_step.slice(
        -1, kernel.angle_count, kernel.angle_count + kernel.pq_count);
    const Series bound = magnitude_step.lt(0.0).where(
        magnitude.div(magnitude_step.neg()).mul(0.95),
        pluss::series::full_like(magnitude_step,
                                 std::numeric_limits<double>::infinity()));
    return raw_step.mul(bound.amin(-1, false).clamp_max(1.0).unsqueeze(-1));
  };

  const auto tangent = [&](const Series &z, const Series &reference,
                           const Series &work) {
    const auto [state, factor] = split(z);
    const auto [p, q] = schedule(factor);
    const auto values = kernel.evaluate(state, p, q);
    const Series rhs = pluss::series::zeros_like(state).unite(
        {pluss::series::ones_like(factor).unsqueeze(-1)}, -1);
    Series next = safe_solve(bordered(values.jacobian, reference), rhs, work);
    next = next.div(next.square().sum(-1, false).sqrt().unsqueeze(-1));
    const Series reverse = next.mul(reference).sum(-1, false).lt(0.0);
    next = reverse.unsqueeze(-1).where(next.neg(), next);
    return work.unsqueeze(-1).where(next, reference);
  };

  struct Corrected {
    Series z;
    Series tangent;
    Series converged;
    std::int64_t iterations{};
  };
  const auto correct = [&](const Series &prediction, const Series &border,
                           const Series &work) {
    Corrected result{prediction, border, work.logical_not(), 0};
    for (std::int64_t iteration = 0; iteration < options.max_iterations;
         ++iteration) {
      const auto [state, factor] = split(result.z);
      const auto [p, q] = schedule(factor);
      const auto values = kernel.evaluate(state, p, q);
      const Series constraint =
          border.mul(result.z.sub(prediction)).sum(-1, false);
      const Series done =
          values.residual.le(options.tolerance)
              .logical_and(constraint.abs().le(options.tolerance));
      result.converged = result.converged.logical_or(work.logical_and(done));
      result.iterations = iteration;
      if (result.converged.all().item<bool>())
        break;
      const Series active = work.logical_and(result.converged.logical_not());
      const Series rhs =
          values.mismatch.unite({constraint.unsqueeze(-1)}, -1).neg();
      const Series raw =
          safe_solve(bordered(values.jacobian, border), rhs, active);
      const Series step = limit_step(result.z, raw);
      result.z = result.z.add(
          active.unsqueeze(-1).where(step, pluss::series::zeros_like(step)));
    }
    const auto [state, factor] = split(result.z);
    const auto [p, q] = schedule(factor);
    const auto values = kernel.evaluate(state, p, q);
    const Series constraint =
        border.mul(result.z.sub(prediction)).sum(-1, false);
    result.converged = result.converged.logical_or(
        work.logical_and(values.residual.le(options.tolerance))
            .logical_and(constraint.abs().le(options.tolerance)));
    result.tangent = tangent(result.z, border, work);
    return result;
  };

  const auto fix_factor = [&](const Series &seed, const Series &factor,
                              const Series &work) {
    Series state = seed;
    Series converged = work.logical_not();
    for (std::int64_t iteration = 0; iteration < options.max_iterations;
         ++iteration) {
      const auto [p, q] = schedule(factor);
      const auto values = kernel.evaluate(state, p, q);
      converged = converged.logical_or(
          work.logical_and(values.residual.le(options.tolerance)));
      if (converged.all().item<bool>())
        break;
      const Series active = work.logical_and(converged.logical_not());
      const Series raw =
          safe_solve(values.jacobian, values.mismatch.neg(), active);
      const Series state_z = state.unite({factor.unsqueeze(-1)}, -1);
      const Series padded =
          raw.unite({pluss::series::zeros_like(factor).unsqueeze(-1)}, -1);
      const Series step = limit_step(state_z, padded).slice(-1, 0, state_count);
      state = state.add(
          active.unsqueeze(-1).where(step, pluss::series::zeros_like(step)));
    }
    const auto [p, q] = schedule(factor);
    const auto final = kernel.evaluate(state, p, q);
    converged = converged.logical_or(
        work.logical_and(final.residual.le(options.tolerance)));
    return std::pair<Series, Series>{state, converged};
  };

  Series z = kernel.state(base.angle, base.magnitude)
                 .unite({zero_lane.unsqueeze(-1)}, -1);
  const double direction =
      options.stop == TraceStop::Target && options.target < 0.0 ? -1.0 : 1.0;
  Series reference = pluss::series::zeros_like(z);
  reference =
      reference.slice(-1, 0, state_count)
          .unite({pluss::series::ones_like(zero_lane).mul(direction).unsqueeze(
                     -1)},
                 -1);
  Series branch_tangent = tangent(z, reference, all_lanes);
  Series active = all_lanes;
  Series reached = all_lanes.logical_not();
  double step = options.step;

  std::vector<Series> voltage_history;
  std::vector<Series> magnitude_history;
  std::vector<Series> angle_history;
  std::vector<Series> injection_history;
  std::vector<Series> from_history;
  std::vector<Series> to_history;
  std::vector<Series> factor_history;
  std::vector<Series> tangent_history;
  std::vector<Series> residual_history;
  std::vector<Series> valid_history;

  const auto record = [&](const Series &point, const Series &point_tangent,
                          const Series &valid) {
    const auto [state, factor] = split(point);
    const auto [p, q] = schedule(factor);
    const auto values = kernel.evaluate(state, p, q);
    const auto flows = kernel.flow(values.voltage);
    voltage_history.push_back(values.voltage);
    magnitude_history.push_back(values.magnitude);
    angle_history.push_back(values.angle);
    injection_history.push_back(values.injection);
    from_history.push_back(flows.from);
    to_history.push_back(flows.to);
    factor_history.push_back(factor);
    tangent_history.push_back(point_tangent);
    residual_history.push_back(values.residual);
    valid_history.push_back(valid);
  };
  record(z, branch_tangent, all_lanes);

  for (std::int64_t point = 0;
       point < options.max_steps && active.any().item<bool>(); ++point) {
    Corrected next{z, branch_tangent, active.logical_not(), 0};
    bool accepted = false;
    double trial_step = step;
    Series accepted_prediction = z;
    while (trial_step >= options.step_min) {
      const Series prediction = z.add(branch_tangent.mul(trial_step));
      next = correct(prediction, branch_tangent, active);
      if (next.converged.all().item<bool>()) {
        accepted_prediction = prediction;
        accepted = true;
        break;
      }
      trial_step *= 0.5;
    }
    if (!accepted)
      break;

    const Series current_rate =
        branch_tangent.select(-1, state_count).mul(direction);
    const Series next_rate =
        next.tangent.select(-1, state_count).mul(direction);
    const Series fold =
        active.logical_and(current_rate.gt(0.0)).logical_and(next_rate.le(0.0));
    Series target_hit = all_lanes.logical_not();
    if (options.stop == TraceStop::Target) {
      target_hit = active.logical_and(next.z.select(-1, state_count)
                                          .sub(options.target)
                                          .mul(direction)
                                          .ge(0.0));
    }
    const Series nose_hit = options.stop == TraceStop::Nose
                                ? fold
                                : fold.logical_and(target_hit.logical_not());

    Series final_z = next.z;
    Series final_tangent = next.tangent;
    if (target_hit.any().item<bool>()) {
      const Series target_factor =
          pluss::series::ones_like(zero_lane).mul(options.target);
      const auto fixed = fix_factor(next.z.slice(-1, 0, state_count),
                                    target_factor, target_hit);
      if (!fixed.second.all().item<bool>())
        throw std::runtime_error(
            "eng::power::trace: target correction did not converge");
      const Series target_z =
          fixed.first.unite({target_factor.unsqueeze(-1)}, -1);
      final_z = target_hit.unsqueeze(-1).where(target_z, final_z);
      const Series target_tangent = tangent(target_z, next.tangent, target_hit);
      final_tangent =
          target_hit.unsqueeze(-1).where(target_tangent, final_tangent);
    }

    if (nose_hit.any().item<bool>()) {
      Series lower = pluss::series::zeros_like(zero_lane);
      Series upper = pluss::series::ones_like(zero_lane);
      Series fold_z = next.z;
      Series fold_tangent = next.tangent;
      for (int refinement = 0; refinement < 44; ++refinement) {
        const Series middle = lower.add(upper).mul(0.5);
        const Series prediction =
            z.add(branch_tangent.mul(middle.mul(trial_step).unsqueeze(-1)));
        const Corrected trial = correct(prediction, branch_tangent, nose_hit);
        const Series right =
            nose_hit.logical_and(trial.converged.logical_not().logical_or(
                trial.tangent.select(-1, state_count).mul(direction).le(0.0)));
        const Series usable_right = right.logical_and(trial.converged);
        fold_z = usable_right.unsqueeze(-1).where(trial.z, fold_z);
        fold_tangent =
            usable_right.unsqueeze(-1).where(trial.tangent, fold_tangent);
        upper = right.where(middle, upper);
        lower = nose_hit.logical_and(right.logical_not()).where(middle, lower);
      }
      final_z = nose_hit.unsqueeze(-1).where(fold_z, final_z);
      final_tangent = nose_hit.unsqueeze(-1).where(fold_tangent, final_tangent);
    }

    const Series stopped = target_hit.logical_or(nose_hit);
    record(final_z, final_tangent, active);
    if (options.stop == TraceStop::Target)
      reached = reached.logical_or(target_hit);
    else
      reached = reached.logical_or(nose_hit);
    z = stopped.unsqueeze(-1).where(final_z, next.z);
    branch_tangent = stopped.unsqueeze(-1).where(final_tangent, next.tangent);
    active = active.logical_and(stopped.logical_not());
    if (options.adaptive) {
      const double error =
          next.z.sub(accepted_prediction).abs().amax().item<double>();
      const double scale =
          error == 0.0
              ? 2.0
              : std::min(2.0,
                         1.0 + options.adapt_damping *
                                   (options.adapt_tolerance / error - 1.0));
      step = std::clamp(trial_step * scale, options.step_min, options.step_max);
    } else {
      step = trial_step;
    }
  }

  const auto stack_vector = [](const std::vector<Series> &values) {
    std::vector<Series> tail;
    tail.reserve(values.size() - 1);
    for (std::size_t i = 1; i < values.size(); ++i)
      tail.push_back(values[i].unsqueeze(-2));
    return values.front().unsqueeze(-2).unite(tail, -2);
  };
  const auto stack_scalar = [](const std::vector<Series> &values) {
    std::vector<Series> tail;
    tail.reserve(values.size() - 1);
    for (std::size_t i = 1; i < values.size(); ++i)
      tail.push_back(values[i].unsqueeze(-1));
    return values.front().unsqueeze(-1).unite(tail, -1);
  };

  return {stack_vector(voltage_history),
          stack_vector(magnitude_history),
          stack_vector(angle_history),
          stack_vector(injection_history),
          stack_vector(from_history),
          stack_vector(to_history),
          stack_scalar(factor_history),
          stack_vector(tangent_history),
          stack_scalar(residual_history),
          stack_scalar(valid_history),
          reached,
          static_cast<std::int64_t>(factor_history.size())};
}

} // namespace pluss::subsp::eng::power
