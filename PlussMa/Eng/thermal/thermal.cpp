#include "pluss/subsp/eng/thermal.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/interp.h"
#include "pluss/subsp/math/roots.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>

namespace pluss::subsp::eng::thermal {

namespace {

using Shape = std::vector<std::int64_t>;

constexpr double kPi = 3.14159265358979323846;

void need_real(const Series &value, const char *who, const char *name) {
  PLUSS_CHECK(value.defined() && value.dtype().is_floating(),
              std::string(who) + ": " + name + " must be a defined real floating array");
  PLUSS_CHECK(value.isfinite().all().item<bool>(), std::string(who) + ": " + name + " must be finite");
}

void need_positive(const Series &value, const char *who, const char *name) {
  need_real(value, who, name);
  PLUSS_CHECK(value.gt(0.0).all().item<bool>(), std::string(who) + ": " + name + " must be positive");
}

void need_nonneg(const Series &value, const char *who, const char *name) {
  need_real(value, who, name);
  PLUSS_CHECK(value.ge(0.0).all().item<bool>(), std::string(who) + ": " + name + " must not be negative");
}

std::string shortest(double value) {
  char buffer[32];
  std::snprintf(buffer, sizeof buffer, "%g", value);
  return buffer;
}

Series zeros_like_batch(const Series &like, std::int64_t tail) {
  Shape shape(like.sizes().begin(), like.sizes().end() - 1);
  shape.push_back(tail);
  return pluss::series::zeros(shape, Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
}

// Column vector of constants in the dtype and platform of `like`.
Series constants(const std::vector<double> &values, const Series &like) {
  return pluss::series::make(values).to(like.scalar_type()).to(like.platform());
}

// ---------------------------------------------------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------------------------------------------------

void check_nodes(const std::vector<std::int64_t> &nodes, std::int64_t count, const char *who, const char *group) {
  for (const std::int64_t node : nodes)
    PLUSS_CHECK(node >= 0 && node < count, std::string(who) + ": a " + group + " node index " + std::to_string(node) +
                                              " is outside 0.." + std::to_string(count - 1));
}

void check_group(const Series &value, std::size_t count, const char *who, const char *group, const char *name) {
  if (count == 0) {
    PLUSS_CHECK(!value.defined(), std::string(who) + ": " + group + " has values but no nodes");
    return;
  }
  need_real(value, who, name);
  PLUSS_CHECK(value.dim() >= 1 && value.size(-1) == static_cast<std::int64_t>(count),
              std::string(who) + ": " + name + " must have one value per " + group + " (last axis " + std::to_string(count) + ")");
}

// Every node must reach a fixed temperature through links.
void check_paths(const Net &net, const char *who) {
  PLUSS_CHECK(!net.fixed.node.empty(), std::string(who) + ": needs at least one fixed temperature");
  std::vector<std::int64_t> parent(static_cast<std::size_t>(net.nodes));
  std::iota(parent.begin(), parent.end(), 0);
  const auto find = [&](std::int64_t x) {
    while (parent[static_cast<std::size_t>(x)] != x) x = parent[static_cast<std::size_t>(x)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
    return x;
  };
  for (std::size_t k = 0; k < net.links.a.size(); ++k) parent[static_cast<std::size_t>(find(net.links.a[k]))] = find(net.links.b[k]);
  std::vector<bool> anchored(static_cast<std::size_t>(net.nodes), false);
  for (const std::int64_t node : net.fixed.node) anchored[static_cast<std::size_t>(find(node))] = true;
  for (std::int64_t node = 0; node < net.nodes; ++node)
    PLUSS_CHECK(anchored[static_cast<std::size_t>(find(node))],
                std::string(who) + ": node " + std::to_string(node) + " has no conduction path to a fixed temperature");
}

circuit::Net compile(const Net &net, bool with_stores, const char *who) {
  PLUSS_CHECK(net.nodes >= 1, std::string(who) + ": the network needs at least one node");
  check_nodes(net.links.a, net.nodes, who, "link");
  check_nodes(net.links.b, net.nodes, who, "link");
  PLUSS_CHECK(net.links.a.size() == net.links.b.size(), std::string(who) + ": links.a and links.b must have the same length");
  for (std::size_t k = 0; k < net.links.a.size(); ++k)
    PLUSS_CHECK(net.links.a[k] != net.links.b[k], std::string(who) + ": a link joins node " + std::to_string(net.links.a[k]) + " to itself");
  check_nodes(net.stores.node, net.nodes, who, "store");
  check_nodes(net.fixed.node, net.nodes, who, "fixed");
  check_nodes(net.heats.node, net.nodes, who, "heat");
  check_group(net.links.conductance, net.links.a.size(), who, "link", "links.conductance");
  check_group(net.stores.capacity, net.stores.node.size(), who, "store", "stores.capacity");
  check_group(net.fixed.temperature, net.fixed.node.size(), who, "fixed node", "fixed.temperature");
  check_group(net.heats.power, net.heats.node.size(), who, "heat node", "heats.power");
  if (net.links.conductance.defined()) need_positive(net.links.conductance, who, "links.conductance");
  if (net.stores.capacity.defined()) need_positive(net.stores.capacity, who, "stores.capacity");

  const auto shift = [](const std::vector<std::int64_t> &nodes) {
    std::vector<std::int64_t> out(nodes.size());
    for (std::size_t k = 0; k < nodes.size(); ++k) out[k] = nodes[k] + 1;
    return out;
  };
  circuit::Net c;
  c.nodes = net.nodes + 1;
  if (!net.links.a.empty()) c.resistors = {shift(net.links.a), shift(net.links.b), net.links.conductance.reciprocal()};
  if (with_stores && !net.stores.node.empty())
    c.capacitors = {shift(net.stores.node), std::vector<std::int64_t>(net.stores.node.size(), 0), net.stores.capacity};
  if (!net.fixed.node.empty())
    c.voltage_sources = {shift(net.fixed.node), std::vector<std::int64_t>(net.fixed.node.size(), 0), net.fixed.temperature};
  if (!net.heats.node.empty())
    c.current_sources = {std::vector<std::int64_t>(net.heats.node.size(), 0), shift(net.heats.node), net.heats.power};
  return c;
}

// Heat difference matrix [nodes, links]: temperature @ matrix is T_a - T_b.
Series link_matrix(const Net &net, const Series &like) {
  const auto nodes = static_cast<std::size_t>(net.nodes), links = net.links.a.size();
  std::vector<double> data(nodes * std::max<std::size_t>(links, 1), 0.0);
  for (std::size_t k = 0; k < links; ++k) {
    data[static_cast<std::size_t>(net.links.a[k]) * links + k] += 1.0;
    data[static_cast<std::size_t>(net.links.b[k]) * links + k] -= 1.0;
  }
  if (links == 0) return Series{};
  return pluss::series::make(data).reshape({static_cast<std::int64_t>(nodes), static_cast<std::int64_t>(links)}).to(like.scalar_type()).to(like.platform());
}

Result finish(const Net &net, const circuit::Result &run) {
  Result out;
  const Series node_temperature = run.voltage.slice(-1, 1, net.nodes + 1);
  out.temperature = node_temperature;
  if (!net.links.a.empty()) {
    out.link_flow = node_temperature.matmul(link_matrix(net, node_temperature)).mul(net.links.conductance);
  } else {
    out.link_flow = zeros_like_batch(node_temperature, 0);
  }
  // The circuit's source current runs through the source from its positive node (the network node) to the reference, so it
  // is the heat the boundary absorbs from the network when positive.
  out.fixed_flow = net.fixed.node.empty() ? zeros_like_batch(node_temperature, 0) : run.source_current.neg();
  out.residual = run.residual;
  return out;
}

}  // namespace

Result steady(const Net &net, const circuit::SolveOptions &options) {
  const circuit::Net c = compile(net, false, "eng::thermal::steady");
  check_paths(net, "eng::thermal::steady");
  return finish(net, circuit::op(circuit::make(c), options));
}

Result tran(const Net &net, const Series &time, const Drive &drive, const Start &start, circuit::Scheme scheme) {
  const circuit::Net c = compile(net, true, "eng::thermal::tran");
  PLUSS_CHECK(time.defined() && time.dim() == 1 && time.size(0) >= 2, "eng::thermal::tran: time must be a one-dimensional array of at least two samples");
  const auto check_drive = [&](const Series &value, std::size_t count, const char *name, const char *needs, const char *per) {
    if (!value.defined()) return;
    PLUSS_CHECK(count > 0, std::string("eng::thermal::tran: ") + name + " needs " + needs + " in the network");
    need_real(value, "eng::thermal::tran", name);
    PLUSS_CHECK(value.dim() >= 2 && value.size(-1) == static_cast<std::int64_t>(count),
                std::string("eng::thermal::tran: ") + name + " needs one value per " + per + " (" + std::to_string(count) + ") on its last axis");
    PLUSS_CHECK(value.size(-2) == time.size(0), std::string("eng::thermal::tran: ") + name + " needs one row per time sample (" +
                                                   std::to_string(time.size(0)) + "), got " + std::to_string(value.size(-2)));
  };
  check_drive(drive.temperature, net.fixed.node.size(), "drive.temperature", "fixed temperatures", "fixed node");
  check_drive(drive.power, net.heats.node.size(), "drive.power", "heats", "heat node");
  if (start.temperature.defined()) {
    PLUSS_CHECK(!net.stores.node.empty(), "eng::thermal::tran: start.temperature needs stores in the network");
    need_real(start.temperature, "eng::thermal::tran", "start.temperature");
    PLUSS_CHECK(start.temperature.size(-1) == static_cast<std::int64_t>(net.stores.node.size()),
                "eng::thermal::tran: start.temperature needs one value per store (" + std::to_string(net.stores.node.size()) + ") on its last axis");
  }
  circuit::Drive cd;
  cd.voltage = drive.temperature;
  cd.current = drive.power;
  circuit::Start cs;
  cs.capacitor_voltage = start.temperature;
  circuit::TranOptions co;
  co.scheme = scheme;
  return finish(net, circuit::tran(circuit::make(c), time, cd, cs, co));
}

// ---------------------------------------------------------------------------------------------------------------------
// Resistances and fins
// ---------------------------------------------------------------------------------------------------------------------

Series wall(const Series &conductivity, const Series &thickness, const Series &area) {
  need_positive(conductivity, "eng::thermal::wall", "conductivity");
  need_positive(thickness, "eng::thermal::wall", "thickness");
  need_positive(area, "eng::thermal::wall", "area");
  return thickness.div(conductivity.mul(area));
}

Series tube(const Series &conductivity, const Series &r_in, const Series &r_out, const Series &length) {
  need_positive(conductivity, "eng::thermal::tube", "conductivity");
  need_positive(r_in, "eng::thermal::tube", "r_in");
  need_positive(r_out, "eng::thermal::tube", "r_out");
  need_positive(length, "eng::thermal::tube", "length");
  PLUSS_CHECK(r_out.gt(r_in).all().item<bool>(), "eng::thermal::tube: r_out must exceed r_in");
  return r_out.div(r_in).log().div(conductivity.mul(length).mul(2.0 * kPi));
}

Series shell(const Series &conductivity, const Series &r_in, const Series &r_out) {
  need_positive(conductivity, "eng::thermal::shell", "conductivity");
  need_positive(r_in, "eng::thermal::shell", "r_in");
  need_positive(r_out, "eng::thermal::shell", "r_out");
  PLUSS_CHECK(r_out.gt(r_in).all().item<bool>(), "eng::thermal::shell: r_out must exceed r_in");
  return r_in.reciprocal().sub(r_out.reciprocal()).div(conductivity.mul(4.0 * kPi));
}

Series film(const Series &coefficient, const Series &area) {
  need_positive(coefficient, "eng::thermal::film", "coefficient");
  need_positive(area, "eng::thermal::film", "area");
  return coefficient.mul(area).reciprocal();
}

Series biot(const Series &coefficient, const Series &conductivity, const Series &length) {
  need_positive(coefficient, "eng::thermal::biot", "coefficient");
  need_positive(conductivity, "eng::thermal::biot", "conductivity");
  need_positive(length, "eng::thermal::biot", "length");
  return coefficient.mul(length).div(conductivity);
}

FinResult fin(const Series &coefficient, const Series &conductivity, const Series &perimeter, const Series &area,
              const Series &length, const Series &base, const Series &ambient, Tip tip) {
  const char *who = "eng::thermal::fin";
  need_positive(coefficient, who, "coefficient");
  need_positive(conductivity, who, "conductivity");
  need_positive(perimeter, who, "perimeter");
  need_positive(area, who, "area");
  need_positive(length, who, "length");
  need_real(base, who, "base");
  need_real(ambient, who, "ambient");
  const Series excess = base.sub(ambient);
  PLUSS_CHECK(excess.ne(0.0).all().item<bool>(), std::string(who) + ": base and ambient temperatures must differ");

  const Series m = coefficient.mul(perimeter).div(conductivity.mul(area)).sqrt();
  const Series ml = m.mul(length);
  const Series strength = coefficient.mul(perimeter).mul(conductivity).mul(area).sqrt().mul(excess);
  const Series cosh_ml = ml.cosh(), sinh_ml = ml.sinh();
  FinResult out;
  Series wetted = perimeter.mul(length);
  if (tip == Tip::Adiabatic) {
    out.heat = strength.mul(ml.tanh());
    out.tip = ambient.add(excess.div(cosh_ml));
  } else if (tip == Tip::Convective) {
    const Series ratio = coefficient.div(m.mul(conductivity));
    out.heat = strength.mul(sinh_ml.add(ratio.mul(cosh_ml))).div(cosh_ml.add(ratio.mul(sinh_ml)));
    out.tip = ambient.add(excess.div(cosh_ml.add(ratio.mul(sinh_ml))));
    wetted = wetted.add(area);
  } else {
    out.heat = strength;
    out.tip = ambient.add(excess.mul(0.0));
  }
  out.efficiency = out.heat.div(coefficient.mul(wetted).mul(excess));
  return out;
}

Slab mesh(const Layers &layers, const Series &area) {
  const char *who = "eng::thermal::mesh";
  need_positive(layers.thickness, who, "thickness");
  need_positive(layers.conductivity, who, "conductivity");
  need_positive(layers.density, who, "density");
  need_positive(layers.specific_heat, who, "specific_heat");
  need_positive(area, who, "area");
  const std::int64_t count = layers.thickness.size(-1);
  PLUSS_CHECK(layers.thickness.dim() >= 1 && layers.conductivity.size(-1) == count && layers.density.size(-1) == count &&
                  layers.specific_heat.size(-1) == count && static_cast<std::int64_t>(layers.cells.size()) == count,
              std::string(who) + ": thickness, conductivity, density, specific_heat and cells must have one entry per layer");
  std::int64_t cells = 0;
  for (const std::int64_t c : layers.cells) {
    PLUSS_CHECK(c >= 1, std::string(who) + ": every layer needs at least one cell");
    cells += c;
  }

  const Series like = layers.thickness;
  const Series a = area.unsqueeze(-1);
  const auto per_cell = [&](const Series &layer_values) {
    std::vector<Series> pieces;
    for (std::int64_t l = 0; l < count; ++l)
      for (std::int64_t c = 0; c < layers.cells[static_cast<std::size_t>(l)]; ++c) pieces.push_back(layer_values.slice(-1, l, l + 1));
    Series first = pieces.front();
    pieces.erase(pieces.begin());
    return pieces.empty() ? first : first.unite(pieces, -1);
  };
  const Series cell_count = constants(std::vector<double>(layers.cells.begin(), layers.cells.end()), like);
  const Series dx = per_cell(layers.thickness.div(cell_count));
  const Series k = per_cell(layers.conductivity);
  const Series rho_cp = per_cell(layers.density.mul(layers.specific_heat));
  const Series half = dx.div(k.mul(a).mul(2.0));
  const Series zero = zeros_like_batch(half, 1);
  const Series resistance = zero.unite({half}, -1).add(half.unite({zero}, -1));

  Slab out;
  out.cells = cells;
  out.inner = 0;
  out.outer = cells + 1;
  out.net.nodes = cells + 2;
  for (std::int64_t j = 0; j <= cells; ++j) {
    out.net.links.a.push_back(j);
    out.net.links.b.push_back(j + 1);
  }
  out.net.links.conductance = resistance.reciprocal();
  for (std::int64_t j = 1; j <= cells; ++j) out.net.stores.node.push_back(j);
  out.net.stores.capacity = rho_cp.mul(dx).mul(a);
  return out;
}

// ---------------------------------------------------------------------------------------------------------------------
// Exchangers
// ---------------------------------------------------------------------------------------------------------------------

namespace {

void check_ratio(const Series &cr, const char *who) {
  need_real(cr, who, "cr");
  PLUSS_CHECK(cr.ge(0.0).logical_and(cr.le(1.0)).all().item<bool>(), std::string(who) + ": cr must lie in [0, 1]");
}

Series eff_of(const Series &ntu_value, const Series &cr, Flow flow) {
  switch (flow) {
  case Flow::Parallel: return ntu_value.mul(cr.add(1.0)).neg().expm1().neg().div(cr.add(1.0));
  case Flow::Counter: {
    const Series x = cr.neg().add(1.0).maximum(1e-8);
    const Series a = ntu_value.mul(x).neg().expm1().neg();
    const Series e = ntu_value.mul(x).neg().exp();
    return a.div(a.add(x.mul(e)));
  }
  case Flow::Cross: {
    const Series safe = cr.maximum(1e-8);
    const Series inner = safe.mul(ntu_value.pow(0.78)).neg().expm1().mul(ntu_value.pow(0.22)).div(safe);
    return inner.expm1().neg();
  }
  case Flow::Shell: {
    const Series s = cr.mul(cr).add(1.0).sqrt();
    const Series e = ntu_value.mul(s).neg().exp();
    const Series ratio = e.add(1.0).div(ntu_value.mul(s).neg().expm1().neg());
    return s.mul(ratio).add(cr).add(1.0).reciprocal().mul(2.0);
  }
  case Flow::Boil: return ntu_value.neg().expm1().neg();
  }
  PLUSS_FAIL("eng::thermal::eff: unknown flow arrangement");
}

}  // namespace

Series eff(const Series &ntu_value, const Series &cr, Flow flow) {
  need_nonneg(ntu_value, "eng::thermal::eff", "ntu");
  check_ratio(cr, "eng::thermal::eff");
  return eff_of(ntu_value, cr, flow);
}

Series ntu(const Series &effectiveness, const Series &cr, Flow flow) {
  const char *who = "eng::thermal::ntu";
  need_real(effectiveness, who, "effectiveness");
  check_ratio(cr, who);
  PLUSS_CHECK(effectiveness.ge(0.0).all().item<bool>(), std::string(who) + ": effectiveness must not be negative");
  const Series one = cr.mul(0.0).add(1.0);
  switch (flow) {
  case Flow::Parallel: {
    const Series c = cr.add(1.0);
    PLUSS_CHECK(effectiveness.mul(c).lt(1.0).all().item<bool>(),
                std::string(who) + ": a parallel-flow exchanger cannot exceed effectiveness 1 / (1 + cr)");
    return effectiveness.mul(c).neg().add(1.0).log().neg().div(c);
  }
  case Flow::Counter: {
    PLUSS_CHECK(effectiveness.lt(1.0).all().item<bool>(), std::string(who) + ": a counter-flow exchanger cannot reach effectiveness 1");
    const Series x = cr.neg().add(1.0).maximum(1e-8);
    return effectiveness.mul(cr).neg().add(1.0).div(effectiveness.neg().add(1.0)).log().div(x);
  }
  case Flow::Boil: {
    PLUSS_CHECK(effectiveness.lt(1.0).all().item<bool>(), std::string(who) + ": effectiveness must be below 1");
    return effectiveness.neg().add(1.0).log().neg();
  }
  case Flow::Shell: {
    const Series s = cr.mul(cr).add(1.0).sqrt();
    const Series limit = s.add(cr).add(1.0).reciprocal().mul(2.0);
    PLUSS_CHECK(effectiveness.lt(limit).all().item<bool>(),
                std::string(who) + ": effectiveness is at or above the limit 2 / (1 + cr + sqrt(1 + cr^2)) of a shell-and-tube exchanger");
    const Series big = effectiveness.reciprocal().mul(2.0).sub(cr.add(1.0)).div(s);
    return big.sub(1.0).div(big.add(1.0)).log().neg().div(s);
  }
  case Flow::Cross: {
    PLUSS_CHECK(effectiveness.lt(1.0).all().item<bool>(), std::string(who) + ": effectiveness must be below 1");
    const auto f = [&](const Series &x) { return eff_of(x, cr, Flow::Cross).sub(effectiveness); };
    const Series hi = one.mul(1.0e4);
    PLUSS_CHECK(f(hi).ge(0.0).all().item<bool>(), std::string(who) + ": effectiveness is above the limit of a cross-flow exchanger with this cr");
    math::roots::Options options;
    options.max_iterations = 200;
    options.x_tolerance = 1e-12;
    return math::roots::illinois(f, one.mul(0.0), hi, options).root;
  }
  }
  PLUSS_FAIL("eng::thermal::ntu: unknown flow arrangement");
}

Exchange rate(const Series &hot_c, const Series &cold_c, const Series &hot_in, const Series &cold_in, const Series &ua, Flow flow) {
  const char *who = "eng::thermal::rate";
  need_positive(hot_c, who, "hot_c");
  need_positive(cold_c, who, "cold_c");
  need_real(hot_in, who, "hot_in");
  need_real(cold_in, who, "cold_in");
  need_nonneg(ua, who, "ua");
  const Series c_min = hot_c.minimum(cold_c), c_max = hot_c.maximum(cold_c);
  const Series cr = c_min.div(c_max);
  Exchange out;
  out.ntu = ua.div(c_min);
  out.effectiveness = eff_of(out.ntu, cr, flow);
  out.duty = out.effectiveness.mul(c_min).mul(hot_in.sub(cold_in));
  out.hot_out = hot_in.sub(out.duty.div(hot_c));
  out.cold_out = cold_in.add(out.duty.div(cold_c));
  return out;
}

Series lmtd(const Series &hot_in, const Series &hot_out, const Series &cold_in, const Series &cold_out, Flow flow) {
  const char *who = "eng::thermal::lmtd";
  PLUSS_CHECK(flow == Flow::Parallel || flow == Flow::Counter,
              std::string(who) + ": only parallel and counter flow have a plain log-mean difference; use rate for other arrangements");
  need_real(hot_in, who, "hot_in");
  need_real(hot_out, who, "hot_out");
  need_real(cold_in, who, "cold_in");
  need_real(cold_out, who, "cold_out");
  const Series d1 = flow == Flow::Counter ? hot_in.sub(cold_out) : hot_in.sub(cold_in);
  const Series d2 = flow == Flow::Counter ? hot_out.sub(cold_in) : hot_out.sub(cold_out);
  PLUSS_CHECK(d1.gt(0.0).logical_and(d2.gt(0.0)).all().item<bool>(),
              std::string(who) + ": the hot stream must stay warmer than the cold stream at both ends (a temperature cross)");
  const Series diff = d1.sub(d2);
  const Series equal = diff.abs().le(d1.add(d2).mul(1e-9)).to(d1.scalar_type());
  const Series stable = d1.div(d2).log().add(equal);
  return diff.div(stable).mul(equal.neg().add(1.0)).add(d1.add(d2).mul(0.5).mul(equal));
}

// ---------------------------------------------------------------------------------------------------------------------
// Moist air
// ---------------------------------------------------------------------------------------------------------------------

namespace {

constexpr double kWaterRatio = 0.621945;
constexpr double kCpAir = 1006.0, kCpVapour = 1860.0, kLatent0 = 2501000.0;

void need_range(const Series &temperature, double lo, double hi, const char *who, const char *name) {
  need_real(temperature, who, name);
  PLUSS_CHECK(temperature.ge(lo).logical_and(temperature.le(hi)).all().item<bool>(),
              std::string(who) + ": " + name + " must lie between " + std::to_string(static_cast<int>(lo)) + " and " +
                  std::to_string(static_cast<int>(hi)) + " deg C");
}

Series psat_of(const Series &t) {
  const Series k = t.add(273.15);
  const Series lnk = k.log();
  const Series water = k.reciprocal().mul(-5.8002206e3).add(1.3914993).add(k.mul(-4.8640239e-2)).add(k.mul(k).mul(4.1764768e-5))
                           .add(k.mul(k).mul(k).mul(-1.4452093e-8)).add(lnk.mul(6.5459673));
  const Series ice = k.reciprocal().mul(-5.6745359e3).add(6.3925247).add(k.mul(-9.677843e-3)).add(k.mul(k).mul(6.2215701e-7))
                         .add(k.mul(k).mul(k).mul(2.0747825e-9)).add(k.mul(k).mul(k).mul(k).mul(-9.484024e-13)).add(lnk.mul(4.1635019));
  const Series wet_side = t.ge(0.0).to(t.scalar_type());
  return water.mul(wet_side).add(ice.mul(wet_side.neg().add(1.0))).exp();
}

Series pw_of_ratio(const Series &w, const Series &pressure) { return pressure.mul(w).div(w.add(kWaterRatio)); }

Series w_of(const Series &pw, const Series &pressure) { return pw.mul(kWaterRatio).div(pressure.sub(pw)); }

}  // namespace

Series psat(const Series &temperature) {
  need_range(temperature, -60.0, 150.0, "eng::thermal::psat", "temperature");
  return psat_of(temperature);
}

Series ratio(const Series &temperature, const Series &relative, const Series &pressure) {
  const char *who = "eng::thermal::ratio";
  need_range(temperature, -60.0, 150.0, who, "temperature");
  need_real(relative, who, "relative");
  need_positive(pressure, who, "pressure");
  PLUSS_CHECK(relative.ge(0.0).logical_and(relative.le(1.0)).all().item<bool>(), std::string(who) + ": relative humidity must lie in [0, 1]");
  const Series pw = psat_of(temperature).mul(relative);
  PLUSS_CHECK(pw.lt(pressure).all().item<bool>(), std::string(who) + ": the vapour pressure reaches the total pressure (the air boils)");
  return w_of(pw, pressure);
}

Series rh(const Series &temperature, const Series &w, const Series &pressure) {
  const char *who = "eng::thermal::rh";
  need_range(temperature, -60.0, 150.0, who, "temperature");
  need_nonneg(w, who, "w");
  need_positive(pressure, who, "pressure");
  return pw_of_ratio(w, pressure).div(psat_of(temperature));
}

Series enth(const Series &temperature, const Series &w) {
  need_range(temperature, -60.0, 150.0, "eng::thermal::enth", "temperature");
  need_nonneg(w, "eng::thermal::enth", "w");
  return temperature.mul(kCpAir).add(w.mul(temperature.mul(kCpVapour).add(kLatent0)));
}

Series vol(const Series &temperature, const Series &w, const Series &pressure) {
  need_range(temperature, -60.0, 150.0, "eng::thermal::vol", "temperature");
  need_nonneg(w, "eng::thermal::vol", "w");
  need_positive(pressure, "eng::thermal::vol", "pressure");
  return temperature.add(273.15).mul(w.mul(1.607858).add(1.0)).mul(287.042).div(pressure);
}

Series dew(const Series &w, const Series &pressure) {
  const char *who = "eng::thermal::dew";
  need_positive(w, who, "w");
  need_positive(pressure, who, "pressure");
  const Series pw = pw_of_ratio(w, pressure);
  const Series lo = pw.mul(0.0).add(-60.0), hi = pw.mul(0.0).add(150.0);
  const auto f = [&](const Series &x) { return psat_of(x).sub(pw); };
  PLUSS_CHECK(f(lo).le(0.0).logical_and(f(hi).ge(0.0)).all().item<bool>(),
              std::string(who) + ": the dew point lies outside -60 to 150 deg C");
  math::roots::Options options;
  options.max_iterations = 200;
  options.x_tolerance = 1e-10;
  return math::roots::illinois(f, lo, hi, options).root;
}

Series wet(const Series &temperature, const Series &w, const Series &pressure) {
  const char *who = "eng::thermal::wet";
  need_range(temperature, -50.0, 100.0, who, "temperature");
  need_positive(w, who, "w");
  need_positive(pressure, who, "pressure");
  const Series td = dew(w, pressure);
  PLUSS_CHECK(td.le(temperature.add(1e-9)).all().item<bool>(), std::string(who) + ": the air is supersaturated (dew point above the temperature)");
  const auto balance = [&](const Series &x) {
    const Series ws = w_of(psat_of(x), pressure);
    const Series above = x.ge(0.0).to(x.scalar_type());
    const Series liquid = ws.mul(x.mul(-2326.0).add(kLatent0)).sub(temperature.sub(x).mul(kCpAir))
                              .div(temperature.mul(kCpVapour).add(kLatent0).sub(x.mul(4186.0)));
    const Series solid = ws.mul(x.mul(-240.0).add(2830000.0)).sub(temperature.sub(x).mul(kCpAir))
                             .div(temperature.mul(kCpVapour).add(2830000.0).sub(x.mul(2100.0)));
    return liquid.mul(above).add(solid.mul(above.neg().add(1.0))).sub(w);
  };
  const Series lo = td.sub(1.0).maximum(-60.0);
  math::roots::Options options;
  options.max_iterations = 200;
  options.x_tolerance = 1e-10;
  return math::roots::illinois(balance, lo, temperature, options).root;
}

MixState mix(const Series &t1, const Series &w1, const Series &m1, const Series &t2, const Series &w2, const Series &m2) {
  const char *who = "eng::thermal::mix";
  need_nonneg(w1, who, "w1");
  need_nonneg(w2, who, "w2");
  need_positive(m1, who, "m1");
  need_positive(m2, who, "m2");
  const Series h1 = enth(t1, w1), h2 = enth(t2, w2);
  const Series total = m1.add(m2);
  MixState out;
  out.w = w1.mul(m1).add(w2.mul(m2)).div(total);
  const Series h = h1.mul(m1).add(h2.mul(m2)).div(total);
  out.temperature = h.sub(out.w.mul(kLatent0)).div(out.w.mul(kCpVapour).add(kCpAir));
  return out;
}

// ---------------------------------------------------------------------------------------------------------------------
// Property provider
// ---------------------------------------------------------------------------------------------------------------------

namespace {

struct Table {
  std::vector<double> t, cp, k, mu;
};

// Incropera, DeWitt, Bergman and Lavine, Fundamentals of Heat and Mass Transfer, Table A.6 (saturated water).
const Table &water_table() {
  static const Table table{
      {273.15, 275, 280, 285, 290, 295, 300, 305, 310, 315, 320, 325, 330, 335, 340, 345, 350, 355, 360, 365, 370, 373.15},
      {4217, 4211, 4198, 4189, 4184, 4181, 4179, 4178, 4178, 4179, 4180, 4182, 4184, 4186, 4188, 4191, 4195, 4199, 4203, 4209, 4214, 4217},
      {0.569, 0.574, 0.582, 0.590, 0.598, 0.606, 0.613, 0.620, 0.628, 0.634, 0.640, 0.645, 0.650, 0.656, 0.660, 0.668, 0.668, 0.671, 0.674, 0.677, 0.679, 0.680},
      {1750e-6, 1652e-6, 1422e-6, 1225e-6, 1080e-6, 959e-6, 855e-6, 769e-6, 695e-6, 631e-6, 577e-6, 528e-6, 489e-6, 453e-6, 420e-6, 389e-6,
       365e-6, 343e-6, 324e-6, 306e-6, 289e-6, 279e-6}};
  return table;
}

// Same book, Table A.4 (air at one atmosphere).
const Table &air_table() {
  static const Table table{{250, 300, 350, 400, 450, 500, 550, 600},
                           {1006, 1007, 1009, 1014, 1021, 1030, 1040, 1051},
                           {22.3e-3, 26.3e-3, 30.0e-3, 33.8e-3, 37.3e-3, 40.7e-3, 43.9e-3, 46.9e-3},
                           {159.6e-7, 184.6e-7, 208.2e-7, 230.1e-7, 250.7e-7, 270.1e-7, 288.4e-7, 305.8e-7}};
  return table;
}

Series lookup(const Table &table, const std::vector<double> &column, const Series &temperature) {
  const Series flat = temperature.reshape(Shape{temperature.numel()});
  const Series out = math::interp::linear(constants(table.t, temperature), constants(column, temperature), flat);
  return out.reshape(temperature.sizes());
}

}  // namespace

Properties props(Fluid fluid, const Series &temperature, const Series &pressure) {
  const char *who = "eng::thermal::props";
  need_real(temperature, who, "temperature");
  const Origin info = source(fluid);
  PLUSS_CHECK(temperature.ge(info.t_min).logical_and(temperature.le(info.t_max)).all().item<bool>(),
              std::string(who) + ": temperature must lie between " + shortest(info.t_min) + " and " + shortest(info.t_max) +
                  " K for " + info.name + " (nothing is extrapolated)");
  Properties out;
  if (fluid == Fluid::Water) {
    const Table &table = water_table();
    const Series c = temperature.sub(273.15);
    const Series numerator = c.mul(-280.54253e-12).add(105.56302e-9).mul(c).add(-46.170461e-6).mul(c).add(-7.9870401e-3).mul(c)
                                 .add(16.945176).mul(c).add(999.83952);
    out.density = numerator.div(c.mul(16.879850e-3).add(1.0));
    out.specific_heat = lookup(table, table.cp, temperature);
    out.conductivity = lookup(table, table.k, temperature);
    out.viscosity = lookup(table, table.mu, temperature);
  } else {
    const Table &table = air_table();
    const Series p = pressure.defined() ? pressure : temperature.mul(0.0).add(101325.0);
    need_positive(p, who, "pressure");
    out.density = p.div(temperature.mul(287.05));
    out.specific_heat = lookup(table, table.cp, temperature);
    out.conductivity = lookup(table, table.k, temperature);
    out.viscosity = lookup(table, table.mu, temperature);
  }
  out.prandtl = out.viscosity.mul(out.specific_heat).div(out.conductivity);
  return out;
}

Origin source(Fluid fluid) {
  Origin info;
  if (fluid == Fluid::Water) {
    info.name = "water";
    info.reference =
        "density: Kell (1975) polynomial; specific heat, conductivity and viscosity: Incropera, DeWitt, Bergman and Lavine, "
        "Fundamentals of Heat and Mass Transfer, Table A.6, linearly interpolated";
    info.state = "saturated liquid; pressure does not enter; no enthalpy reference is carried";
    info.t_min = 273.15;
    info.t_max = 373.15;
  } else {
    info.name = "air";
    info.reference =
        "density: ideal gas with R = 287.05 J/(kg K); specific heat, conductivity and viscosity: Incropera, DeWitt, Bergman and Lavine, "
        "Fundamentals of Heat and Mass Transfer, Table A.4 (one atmosphere), linearly interpolated";
    info.state = "dry air as an ideal gas at the given pressure (default 101325 Pa); transport properties do not depend on pressure";
    info.t_min = 250.0;
    info.t_max = 600.0;
  }
  return info;
}

}  // namespace pluss::subsp::eng::thermal
