#include "pluss/subsp/eng/structure.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace pluss::subsp::eng::structure {
namespace {

using Shape = std::vector<std::int64_t>;

Config config(ScalarType dtype, Platform platform) {
  return Config{}.with_dtype(dtype).with_platform(platform);
}

Shape shape_of(const Series &value) {
  return Shape(value.sizes().begin(), value.sizes().end());
}

Shape leading_shape(const Series &value, std::int64_t trailing) {
  Shape shape = shape_of(value);
  shape.resize(shape.size() - static_cast<std::size_t>(trailing));
  return shape;
}

Shape broadcast_shape(const Shape &lhs, const Shape &rhs,
                      const char *operation) {
  const std::size_t rank = std::max(lhs.size(), rhs.size());
  Shape result(rank, 1);
  for (std::size_t offset = 0; offset < rank; ++offset) {
    const auto left = offset < lhs.size() ? lhs[lhs.size() - 1 - offset] : 1;
    const auto right = offset < rhs.size() ? rhs[rhs.size() - 1 - offset] : 1;
    if (left != right && left != 1 && right != 1)
      throw std::invalid_argument(std::string(operation) +
                                  ": scenario axes are not broadcastable");
    result[rank - 1 - offset] = std::max(left, right);
  }
  return result;
}

Series expand_tail(const Series &value, const Shape &batch,
                   std::int64_t trailing, const char *operation) {
  const Shape current = leading_shape(value, trailing);
  if (current == batch)
    return value;
  if (current.size() > batch.size())
    throw std::invalid_argument(std::string(operation) +
                                ": scenario rank cannot broadcast");
  const std::size_t offset = batch.size() - current.size();
  for (std::size_t axis = 0; axis < current.size(); ++axis)
    if (current[axis] != 1 && current[axis] != batch[offset + axis])
      throw std::invalid_argument(std::string(operation) +
                                  ": scenario axes are not broadcastable");
  Shape aligned(offset, 1);
  const Shape full = shape_of(value);
  aligned.insert(aligned.end(), full.begin(), full.end());
  Shape target = batch;
  target.insert(target.end(), full.end() - trailing, full.end());
  return value.reshape(aligned).expand(target, true);
}

void inspect_values(const Series &value, std::int64_t trailing,
                    const char *operation, const char *role, ScalarType dtype,
                    Platform platform) {
  if (!value.defined() || value.dim() < trailing ||
      !value.dtype().is_floating())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be a real floating Series");
  if (value.scalar_type() != dtype || value.platform() != platform)
    throw std::invalid_argument(
        std::string(operation) +
        ": geometry and section values must share dtype and platform");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be finite");
}

Series element_index(const Series &index, const Series &element_values) {
  Shape aligned(static_cast<std::size_t>(element_values.dim()), 1);
  aligned[aligned.size() - 2] = index.numel();
  return index.reshape(aligned).expand(shape_of(element_values), true);
}

Series lane_index(const Series &index, const Shape &batch) {
  Shape aligned(batch.size(), 1);
  aligned.push_back(index.numel());
  Shape target = batch;
  target.push_back(index.numel());
  return index.reshape(aligned).expand(target, true);
}

Series lanes(std::initializer_list<Series> values) {
  auto it = values.begin();
  Series result = it->unsqueeze(-1);
  const auto dtype = result.scalar_type();
  const auto platform = result.platform();
  for (++it; it != values.end(); ++it)
    result = result.unite({it->to(platform, dtype).unsqueeze(-1)}, -1, true);
  return result;
}

struct MeshData {
  std::int64_t nodes{};
  std::int64_t elements{};
  std::int64_t dim{};
  ScalarType dtype{};
  Platform platform{};
  Series from{};
  Series to{};
};

MeshData inspect_mesh(const Mesh &mesh, bool planar, const char *operation) {
  if (!mesh.position.defined() || mesh.position.dim() < 2 ||
      !mesh.position.dtype().is_floating())
    throw std::invalid_argument(std::string(operation) +
                                ": position must be real floating "
                                "[..., node, axis]");
  if (!mesh.position.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) +
                                ": position must be finite");
  const std::int64_t nodes = mesh.position.size(-2);
  const std::int64_t dim = mesh.position.size(-1);
  if (nodes < 2 || (planar ? dim != 2 : (dim != 2 && dim != 3)))
    throw std::invalid_argument(
        std::string(operation) +
        (planar ? ": a frame needs at least two nodes and exactly two spatial "
                  "axes"
                : ": position needs at least two nodes and two or three "
                  "spatial axes"));
  if (mesh.from.empty() || mesh.from.size() != mesh.to.size())
    throw std::invalid_argument(std::string(operation) +
                                ": from and to must be equally sized, "
                                "non-empty connectivity arrays");
  const auto count = static_cast<std::int64_t>(mesh.from.size());
  for (std::int64_t element = 0; element < count; ++element) {
    const auto from = mesh.from[static_cast<std::size_t>(element)];
    const auto to = mesh.to[static_cast<std::size_t>(element)];
    if (from < 0 || from >= nodes || to < 0 || to >= nodes)
      throw std::out_of_range(std::string(operation) +
                              ": connectivity contains an invalid node");
    if (from == to)
      throw std::invalid_argument(std::string(operation) +
                                  ": an element cannot connect a node to "
                                  "itself");
  }
  const auto platform = mesh.position.platform();
  return {nodes,
          count,
          dim,
          mesh.position.scalar_type(),
          platform,
          pluss::series::make(mesh.from, {count}).to(platform),
          pluss::series::make(mesh.to, {count}).to(platform)};
}

struct TrussKernel {
  Series direction;
  Series stiffness;
  Series from;
  Series to;

  Series apply(const Series &displacement) const {
    const Series start = displacement.index_select(-2, from);
    const Series finish = displacement.index_select(-2, to);
    const Series extension = finish.sub(start).mul(direction).sum(-1, false);
    const Series axial = stiffness.mul(extension);
    const Series element_force = axial.unsqueeze(-1).mul(direction);
    return pluss::series::zeros_like(displacement)
        .scatter_add(-2, element_index(from, element_force),
                     element_force.neg())
        .scatter_add(-2, element_index(to, element_force), element_force);
  }

  Series diagonal(const Series &like) const {
    const Series contribution = stiffness.unsqueeze(-1).mul(direction.square());
    return pluss::series::zeros_like(like)
        .scatter_add(-2, element_index(from, contribution), contribution)
        .scatter_add(-2, element_index(to, contribution), contribution);
  }
};

struct FrameKernel {
  Series direction;
  Series length;
  Series area;
  Series inertia;
  Series modulus;
  Series density;
  Series from;
  Series to;

  Series local_values(const Series &nodal) const {
    const Series start = nodal.index_select(-2, from);
    const Series finish = nodal.index_select(-2, to);
    const Series c = direction.select(-1, 0);
    const Series s = direction.select(-1, 1);
    const Series ui =
        start.select(-1, 0).mul(c).add(start.select(-1, 1).mul(s));
    const Series vi =
        start.select(-1, 1).mul(c).sub(start.select(-1, 0).mul(s));
    const Series uj =
        finish.select(-1, 0).mul(c).add(finish.select(-1, 1).mul(s));
    const Series vj =
        finish.select(-1, 1).mul(c).sub(finish.select(-1, 0).mul(s));
    return lanes({ui, vi, start.select(-1, 2), uj, vj, finish.select(-1, 2)});
  }

  Series local_force(const Series &displacement) const {
    const Series u = local_values(displacement);
    const Series ka = modulus.mul(area).div(length);
    const Series ei = modulus.mul(inertia);
    const Series kb = ei.mul(12.0).div(length.pow(3.0));
    const Series kc = ei.mul(6.0).div(length.square());
    const Series kd = ei.mul(4.0).div(length);
    const Series ke = ei.mul(2.0).div(length);
    const Series ui = u.select(-1, 0);
    const Series vi = u.select(-1, 1);
    const Series ti = u.select(-1, 2);
    const Series uj = u.select(-1, 3);
    const Series vj = u.select(-1, 4);
    const Series tj = u.select(-1, 5);
    const Series ni = ka.mul(ui.sub(uj));
    const Series qi = kb.mul(vi.sub(vj)).add(kc.mul(ti.add(tj)));
    const Series mi = kc.mul(vi.sub(vj)).add(kd.mul(ti)).add(ke.mul(tj));
    const Series mj = kc.mul(vi.sub(vj)).add(ke.mul(ti)).add(kd.mul(tj));
    return lanes({ni, qi, mi, ni.neg(), qi.neg(), mj})
        .to(displacement.platform(), displacement.scalar_type());
  }

  Series assemble(const Series &local, const Series &like) const {
    const Series c = direction.select(-1, 0);
    const Series s = direction.select(-1, 1);
    const Series ni = local.select(-1, 0);
    const Series vi = local.select(-1, 1);
    const Series nj = local.select(-1, 3);
    const Series vj = local.select(-1, 4);
    const Series start = lanes({c.mul(ni).sub(s.mul(vi)),
                                s.mul(ni).add(c.mul(vi)), local.select(-1, 2)});
    const Series finish =
        lanes({c.mul(nj).sub(s.mul(vj)), s.mul(nj).add(c.mul(vj)),
               local.select(-1, 5)});
    return pluss::series::zeros_like(like)
        .scatter_add(-2, element_index(from, start), start)
        .scatter_add(-2, element_index(to, finish), finish);
  }

  Series apply(const Series &displacement) const {
    return assemble(local_force(displacement), displacement);
  }

  Series diagonal(const Series &like) const {
    const Series c2 = direction.select(-1, 0).square();
    const Series s2 = direction.select(-1, 1).square();
    const Series ka = modulus.mul(area).div(length);
    const Series kb = modulus.mul(inertia).mul(12.0).div(length.pow(3.0));
    const Series kd = modulus.mul(inertia).mul(4.0).div(length);
    const Series contribution =
        lanes({ka.mul(c2).add(kb.mul(s2)), ka.mul(s2).add(kb.mul(c2)), kd})
            .to(like.platform(), like.scalar_type());
    return pluss::series::zeros_like(like)
        .scatter_add(-2, element_index(from, contribution), contribution)
        .scatter_add(-2, element_index(to, contribution), contribution);
  }

  Series apply_mass(const Series &acceleration) const {
    const Series a = local_values(acceleration);
    const Series l2 = length.square();
    const Series scale = density.mul(area).mul(length).div(420.0);
    const Series u1 = a.select(-1, 0);
    const Series v1 = a.select(-1, 1);
    const Series t1 = a.select(-1, 2);
    const Series u2 = a.select(-1, 3);
    const Series v2 = a.select(-1, 4);
    const Series t2 = a.select(-1, 5);
    const Series f0 = u1.mul(140.0).add(u2.mul(70.0));
    const Series f1 = v1.mul(156.0)
                          .add(t1.mul(length).mul(22.0))
                          .add(v2.mul(54.0))
                          .sub(t2.mul(length).mul(13.0));
    const Series f2 = v1.mul(length)
                          .mul(22.0)
                          .add(t1.mul(l2).mul(4.0))
                          .add(v2.mul(length).mul(13.0))
                          .sub(t2.mul(l2).mul(3.0));
    const Series f3 = u1.mul(70.0).add(u2.mul(140.0));
    const Series f4 = v1.mul(54.0)
                          .add(t1.mul(length).mul(13.0))
                          .add(v2.mul(156.0))
                          .sub(t2.mul(length).mul(22.0));
    const Series f5 = v1.mul(length)
                          .mul(-13.0)
                          .sub(t1.mul(l2).mul(3.0))
                          .sub(v2.mul(length).mul(22.0))
                          .add(t2.mul(l2).mul(4.0));
    return assemble(lanes({f0, f1, f2, f3, f4, f5}).mul(scale.unsqueeze(-1)),
                    acceleration);
  }

  Series apply_geometric(const Series &displacement,
                         const Series &compression) const {
    const Series u = local_values(displacement);
    const Series vi = u.select(-1, 1);
    const Series ti = u.select(-1, 2);
    const Series vj = u.select(-1, 4);
    const Series tj = u.select(-1, 5);
    const Series l2 = length.square();
    const Series scale = compression.div(length.mul(30.0));
    const Series q1 = vi.mul(36.0)
                          .add(ti.mul(length).mul(3.0))
                          .sub(vj.mul(36.0))
                          .add(tj.mul(length).mul(3.0));
    const Series m1 = vi.mul(length)
                          .mul(3.0)
                          .add(ti.mul(l2).mul(4.0))
                          .sub(vj.mul(length).mul(3.0))
                          .sub(tj.mul(l2));
    const Series q2 = vi.mul(-36.0)
                          .sub(ti.mul(length).mul(3.0))
                          .add(vj.mul(36.0))
                          .sub(tj.mul(length).mul(3.0));
    const Series m2 = vi.mul(length)
                          .mul(3.0)
                          .sub(ti.mul(l2))
                          .sub(vj.mul(length).mul(3.0))
                          .add(tj.mul(l2).mul(4.0));
    const Series zero = vi.mul(0.0);
    return assemble(
        lanes({zero, q1, m1, zero, q2, m2}).mul(scale.unsqueeze(-1)),
        displacement);
  }
};

struct SolveLayout {
  Shape batch;
  Shape node_shape;
  Shape flat_shape;
  Series load;
  Series fixed_mask;
  Series free_mask;
  Series prescribed_flat;
};

SolveLayout prepare_solve(const Series &model, const Series &load,
                          const Fix &fix, std::int64_t nodes,
                          std::int64_t dofs_per_node, const char *operation) {
  if (!load.defined() || load.dim() < 2 || !load.dtype().is_floating() ||
      load.size(-2) != nodes || load.size(-1) != dofs_per_node)
    throw std::invalid_argument(std::string(operation) +
                                ": load must be real floating "
                                "[..., node, dof]");
  if (load.scalar_type() != model.scalar_type() ||
      load.platform() != model.platform())
    throw std::invalid_argument(std::string(operation) +
                                ": load must share the structure dtype and "
                                "platform");
  if (!load.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) +
                                ": load must be finite");
  if (fix.node.empty() || fix.node.size() != fix.axis.size())
    throw std::invalid_argument(std::string(operation) +
                                ": node and axis constraints must be equally "
                                "sized and non-empty");
  const auto constrained = static_cast<std::int64_t>(fix.node.size());
  const std::int64_t dofs = nodes * dofs_per_node;
  std::vector<std::int64_t> fixed_dofs;
  fixed_dofs.reserve(fix.node.size());
  for (std::size_t lane = 0; lane < fix.node.size(); ++lane) {
    if (fix.node[lane] < 0 || fix.node[lane] >= nodes || fix.axis[lane] < 0 ||
        fix.axis[lane] >= dofs_per_node)
      throw std::out_of_range(std::string(operation) +
                              ": a constrained degree of freedom is out of "
                              "range");
    fixed_dofs.push_back(fix.node[lane] * dofs_per_node + fix.axis[lane]);
  }
  auto sorted = fixed_dofs;
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
    throw std::invalid_argument(std::string(operation) +
                                ": constrained degrees of freedom must be "
                                "unique");
  Shape batch = broadcast_shape(leading_shape(model, 2), leading_shape(load, 2),
                                operation);
  if (fix.value.defined()) {
    if (fix.value.dim() < 1 || !fix.value.dtype().is_floating() ||
        fix.value.size(-1) != constrained ||
        fix.value.scalar_type() != load.scalar_type() ||
        fix.value.platform() != load.platform() ||
        !fix.value.isfinite().all().item<bool>())
      throw std::invalid_argument(
          std::string(operation) +
          ": constraint values must be finite real floating "
          "[..., constrained dof] with the structure dtype and platform");
    batch = broadcast_shape(batch, leading_shape(fix.value, 1), operation);
  }
  const auto dtype = load.scalar_type();
  const auto platform = load.platform();
  std::vector<double> mask_values(static_cast<std::size_t>(dofs), 0.0);
  for (const auto dof : fixed_dofs)
    mask_values[static_cast<std::size_t>(dof)] = 1.0;
  Series fixed_mask =
      pluss::series::make(mask_values, {dofs}).to(dtype).to(platform);
  fixed_mask = expand_tail(fixed_mask, batch, 1, operation);
  const Series free_mask = pluss::series::ones_like(fixed_mask).sub(fixed_mask);
  const Series fixed_index =
      pluss::series::make(fixed_dofs, {constrained}).to(platform);
  const Series fixed_value = fix.value.defined()
                                 ? expand_tail(fix.value, batch, 1, operation)
                                 : pluss::series::zeros(
                                       [&] {
                                         Shape shape = batch;
                                         shape.push_back(constrained);
                                         return shape;
                                       }(),
                                       config(dtype, platform));
  Shape flat_shape = batch;
  flat_shape.push_back(dofs);
  const Series prescribed_flat =
      pluss::series::zeros(flat_shape, config(dtype, platform))
          .scatter(-1, lane_index(fixed_index, batch), fixed_value);
  Shape node_shape = batch;
  node_shape.push_back(nodes);
  node_shape.push_back(dofs_per_node);
  return {batch,          node_shape,
          flat_shape,     expand_tail(load, batch, 2, operation),
          fixed_mask,     free_mask,
          prescribed_flat};
}

struct CoreResult {
  Series displacement;
  Series reaction;
  Series internal;
  Series residual_norm;
  Series converged;
  Series breakdown;
  std::int64_t iterations{};
};

template <class Kernel>
CoreResult solve_system(const Kernel &kernel, const SolveLayout &layout,
                        const SolveOptions &options, const char *operation) {
  const auto apply_flat = [&](const Series &flat) {
    return kernel.apply(flat.mul(layout.free_mask).reshape(layout.node_shape))
        .reshape(layout.flat_shape)
        .mul(layout.free_mask)
        .add(flat.mul(layout.fixed_mask));
  };
  const Series prescribed = layout.prescribed_flat.reshape(layout.node_shape);
  const Series rhs = layout.load.sub(kernel.apply(prescribed))
                         .reshape(layout.flat_shape)
                         .mul(layout.free_mask);
  const Series diagonal =
      kernel.diagonal(pluss::series::zeros_like(layout.load))
          .to(layout.load.platform(), layout.load.scalar_type())
          .reshape(layout.flat_shape)
          .mul(layout.free_mask)
          .add(layout.fixed_mask);
  if (!diagonal.gt(0.0).all().item<bool>())
    throw std::invalid_argument(std::string(operation) +
                                ": a free degree of freedom has no stiffness; "
                                "add a support or load-bearing element");
  const auto solved = pluss::subsp::math::krylov::pcg(
      apply_flat,
      [&](const Series &residual) { return residual.div(diagonal); }, rhs, {-1},
      options.cg);
  if (options.fail_on_nonconvergence &&
      !solved.converged.all().template item<bool>())
    throw std::runtime_error(std::string(operation) +
                             ": constrained stiffness solve did not converge; "
                             "the structure may contain a mechanism");
  const Series displacement =
      layout.prescribed_flat.add(solved.solution.mul(layout.free_mask))
          .reshape(layout.node_shape);
  const Series internal = kernel.apply(displacement);
  const Series reaction = internal.sub(layout.load)
                              .reshape(layout.flat_shape)
                              .mul(layout.fixed_mask)
                              .reshape(layout.node_shape);
  return {displacement,
          reaction,
          internal,
          solved.residual_norm.squeeze(-1),
          solved.converged.squeeze(-1),
          solved.breakdown.squeeze(-1),
          solved.iterations};
}

void validate_eig_options(const EigOptions &options, const char *operation) {
  if (options.count <= 0 || options.max_iterations <= 0)
    throw std::invalid_argument(
        std::string(operation) +
        ": count and max_iterations must both be positive");
  if (!(options.relative_tolerance >= 0.0) ||
      !std::isfinite(options.relative_tolerance) ||
      !(options.absolute_tolerance >= 0.0) ||
      !std::isfinite(options.absolute_tolerance))
    throw std::invalid_argument(
        std::string(operation) +
        ": tolerances must be finite and non-negative");
}

Series insert_mode(const Series &value, const Shape &batch,
                   std::int64_t trailing, std::int64_t count) {
  Shape target = batch;
  target.push_back(count);
  const Shape source = shape_of(value);
  target.insert(target.end(), source.end() - trailing, source.end());
  return value.unsqueeze(-(trailing + 1)).expand(target, true);
}

Series spectral_seed(const SolveLayout &layout, std::int64_t dofs,
                     const EigOptions &options, const char *operation) {
  std::int64_t cases = 1;
  for (const std::int64_t extent : layout.batch)
    cases *= extent;
  const std::int64_t free_dofs =
      dofs - static_cast<std::int64_t>(
                 layout.fixed_mask.reshape({cases, dofs})
                     .select(0, 0)
                     .sum()
                     .item<double>());
  if (options.count > free_dofs)
    throw std::invalid_argument(std::string(operation) +
                                ": count exceeds the free degree count");
  const Config cfg =
      config(layout.load.scalar_type(), layout.load.platform());
  const Series lane =
      pluss::series::arange(1, dofs + 1, cfg).reshape({1, dofs});
  const Series mode =
      pluss::series::arange(1, options.count + 1, cfg)
          .reshape({options.count, 1});
  Series basis = lane.mul(mode)
                     .mul(3.141592653589793238462643383279502884 /
                          static_cast<double>(dofs + 1))
                     .sin();
  Shape aligned(layout.batch.size(), 1);
  aligned.push_back(options.count);
  aligned.push_back(dofs);
  Shape target = layout.batch;
  target.push_back(options.count);
  target.push_back(dofs);
  return basis.reshape(aligned)
      .expand(target, true)
      .mul(insert_mode(layout.free_mask, layout.batch, 1, options.count));
}

} // namespace

Truss truss(const Mesh &mesh, const Bars &bars) {
  constexpr const char *operation = "eng::structure::truss";
  const MeshData info = inspect_mesh(mesh, false, operation);
  inspect_values(bars.area, 1, operation, "area", info.dtype, info.platform);
  inspect_values(bars.modulus, 1, operation, "modulus", info.dtype,
                 info.platform);
  if (bars.area.size(-1) != info.elements ||
      bars.modulus.size(-1) != info.elements)
    throw std::invalid_argument(
        "eng::structure::truss: area and modulus must end in the bar count");
  if (!bars.area.gt(0.0).all().item<bool>() ||
      !bars.modulus.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::truss: area and modulus must be positive");
  Shape batch = leading_shape(mesh.position, 2);
  batch = broadcast_shape(batch, leading_shape(bars.area, 1), operation);
  batch = broadcast_shape(batch, leading_shape(bars.modulus, 1), operation);
  const Series position = expand_tail(mesh.position, batch, 2, operation);
  const Series area = expand_tail(bars.area, batch, 1, operation);
  const Series modulus = expand_tail(bars.modulus, batch, 1, operation);
  const Series delta = position.index_select(-2, info.to)
                           .sub(position.index_select(-2, info.from));
  const Series length = delta.square().sum(-1, false).sqrt();
  if (!length.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::truss: every bar must have positive length");
  return Truss(position, delta.div(length.unsqueeze(-1)), length, area, modulus,
               area.mul(modulus).div(length), info.from, info.to, info.nodes,
               info.elements, info.dim);
}

Frame frame(const Mesh &mesh, const Sections &sections) {
  constexpr const char *operation = "eng::structure::frame";
  const MeshData info = inspect_mesh(mesh, true, operation);
  inspect_values(sections.area, 1, operation, "area", info.dtype,
                 info.platform);
  inspect_values(sections.inertia, 1, operation, "inertia", info.dtype,
                 info.platform);
  inspect_values(sections.modulus, 1, operation, "modulus", info.dtype,
                 info.platform);
  if (sections.area.size(-1) != info.elements ||
      sections.inertia.size(-1) != info.elements ||
      sections.modulus.size(-1) != info.elements)
    throw std::invalid_argument(
        "eng::structure::frame: area, inertia, and modulus must end in the "
        "member count");
  if (!sections.area.gt(0.0).all().item<bool>() ||
      !sections.inertia.gt(0.0).all().item<bool>() ||
      !sections.modulus.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::frame: area, inertia, and modulus must be positive");
  Shape batch = leading_shape(mesh.position, 2);
  batch = broadcast_shape(batch, leading_shape(sections.area, 1), operation);
  batch = broadcast_shape(batch, leading_shape(sections.inertia, 1), operation);
  batch = broadcast_shape(batch, leading_shape(sections.modulus, 1), operation);
  if (sections.density.defined()) {
    inspect_values(sections.density, 1, operation, "density", info.dtype,
                   info.platform);
    if (sections.density.size(-1) != info.elements)
      throw std::invalid_argument(
          "eng::structure::frame: density must end in the member count");
    if (!sections.density.gt(0.0).all().item<bool>())
      throw std::invalid_argument(
          "eng::structure::frame: density must be positive");
    batch =
        broadcast_shape(batch, leading_shape(sections.density, 1), operation);
  }
  const Series position = expand_tail(mesh.position, batch, 2, operation);
  const Series area = expand_tail(sections.area, batch, 1, operation);
  const Series inertia = expand_tail(sections.inertia, batch, 1, operation);
  const Series modulus = expand_tail(sections.modulus, batch, 1, operation);
  const Series density =
      sections.density.defined()
          ? expand_tail(sections.density, batch, 1, operation)
          : Series{};
  const Series delta = position.index_select(-2, info.to)
                           .sub(position.index_select(-2, info.from));
  const Series length = delta.square().sum(-1, false).sqrt();
  if (!length.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::frame: every frame member must have positive length");
  return Frame(position, delta.div(length.unsqueeze(-1)), length, area, inertia,
               modulus, density, info.from, info.to, info.nodes, info.elements);
}

StaticResult solve(const Truss &truss, const Series &load, const Fix &fix,
                   const SolveOptions &options) {
  constexpr const char *operation = "eng::structure::solve";
  const SolveLayout layout = prepare_solve(truss.position_, load, fix,
                                           truss.nodes_, truss.dim_, operation);
  const Series direction =
      expand_tail(truss.direction_, layout.batch, 2, operation);
  const Series stiffness =
      expand_tail(truss.stiffness_, layout.batch, 1, operation);
  const TrussKernel kernel{direction, stiffness, truss.from_index_,
                           truss.to_index_};
  const CoreResult core = solve_system(kernel, layout, options, operation);
  const Series start = core.displacement.index_select(-2, truss.from_index_);
  const Series finish = core.displacement.index_select(-2, truss.to_index_);
  const Series extension = finish.sub(start).mul(direction).sum(-1, false);
  const Series length = expand_tail(truss.length_, layout.batch, 1, operation);
  const Series area = expand_tail(truss.area_, layout.batch, 1, operation);
  const Series modulus =
      expand_tail(truss.modulus_, layout.batch, 1, operation);
  const Series strain = extension.div(length);
  const Series stress = modulus.mul(strain);
  return {core.displacement,
          core.reaction,
          strain,
          stress,
          area.mul(stress),
          core.displacement.mul(core.internal).sum({-2, -1}, false).mul(0.5),
          core.residual_norm,
          core.converged,
          core.breakdown,
          core.iterations};
}

FrameResult solve(const Frame &frame, const Series &load, const Fix &fix,
                  const SolveOptions &options) {
  constexpr const char *operation = "eng::structure::solve";
  const SolveLayout layout =
      prepare_solve(frame.position_, load, fix, frame.nodes_, 3, operation);
  const FrameKernel kernel{
      expand_tail(frame.direction_, layout.batch, 2, operation),
      expand_tail(frame.length_, layout.batch, 1, operation),
      expand_tail(frame.area_, layout.batch, 1, operation),
      expand_tail(frame.inertia_, layout.batch, 1, operation),
      expand_tail(frame.modulus_, layout.batch, 1, operation),
      frame.density_.defined()
          ? expand_tail(frame.density_, layout.batch, 1, operation)
          : Series{},
      frame.from_index_,
      frame.to_index_};
  const CoreResult core = solve_system(kernel, layout, options, operation);
  return {core.displacement,
          core.reaction,
          kernel.local_force(core.displacement),
          core.displacement.mul(core.internal).sum({-2, -1}, false).mul(0.5),
          core.residual_norm,
          core.converged,
          core.breakdown,
          core.iterations};
}

Series mass(const Frame &frame, const Series &acceleration) {
  constexpr const char *operation = "eng::structure::mass";
  if (!frame.density_.defined())
    throw std::invalid_argument(
        "eng::structure::mass: frame sections do not define density");
  if (!acceleration.defined() || acceleration.dim() < 2 ||
      !acceleration.dtype().is_floating() ||
      acceleration.size(-2) != frame.nodes_ || acceleration.size(-1) != 3)
    throw std::invalid_argument(
        "eng::structure::mass: acceleration must be real floating "
        "[..., node, 3]");
  if (acceleration.scalar_type() != frame.position_.scalar_type() ||
      acceleration.platform() != frame.position_.platform())
    throw std::invalid_argument(
        "eng::structure::mass: acceleration must share the frame dtype and "
        "platform");
  if (!acceleration.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::mass: acceleration must be finite");
  Shape batch = broadcast_shape(leading_shape(frame.position_, 2),
                                leading_shape(acceleration, 2), operation);
  const Series value = expand_tail(acceleration, batch, 2, operation);
  const FrameKernel kernel{expand_tail(frame.direction_, batch, 2, operation),
                           expand_tail(frame.length_, batch, 1, operation),
                           expand_tail(frame.area_, batch, 1, operation),
                           expand_tail(frame.inertia_, batch, 1, operation),
                           expand_tail(frame.modulus_, batch, 1, operation),
                           expand_tail(frame.density_, batch, 1, operation),
                           frame.from_index_,
                           frame.to_index_};
  return kernel.apply_mass(value);
}

ModeResult modes(const Frame &frame, const Fix &fix,
                 const EigOptions &options) {
  constexpr const char *operation = "eng::structure::modes";
  validate_eig_options(options, operation);
  if (!frame.density_.defined())
    throw std::invalid_argument(
        "eng::structure::modes: frame sections do not define density");
  if (fix.value.defined() && !fix.value.eq(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::modes: modal supports must be homogeneous");

  Shape load_shape = shape_of(frame.position_);
  load_shape.back() = 3;
  const Series zero_load = pluss::series::zeros(
      load_shape,
      config(frame.position_.scalar_type(), frame.position_.platform()));
  const SolveLayout layout =
      prepare_solve(frame.position_, zero_load, fix, frame.nodes_, 3,
                    operation);
  const std::int64_t dofs = frame.nodes_ * 3;
  Shape node_shape = layout.batch;
  node_shape.push_back(options.count);
  node_shape.push_back(frame.nodes_);
  node_shape.push_back(3);
  Shape flat_shape = layout.batch;
  flat_shape.push_back(options.count);
  flat_shape.push_back(dofs);
  const Series free =
      insert_mode(layout.free_mask, layout.batch, 1, options.count);
  const Series fixed =
      insert_mode(layout.fixed_mask, layout.batch, 1, options.count);
  const FrameKernel kernel{
      insert_mode(expand_tail(frame.direction_, layout.batch, 2, operation),
                  layout.batch, 2, options.count),
      insert_mode(expand_tail(frame.length_, layout.batch, 1, operation),
                  layout.batch, 1, options.count),
      insert_mode(expand_tail(frame.area_, layout.batch, 1, operation),
                  layout.batch, 1, options.count),
      insert_mode(expand_tail(frame.inertia_, layout.batch, 1, operation),
                  layout.batch, 1, options.count),
      insert_mode(expand_tail(frame.modulus_, layout.batch, 1, operation),
                  layout.batch, 1, options.count),
      insert_mode(expand_tail(frame.density_, layout.batch, 1, operation),
                  layout.batch, 1, options.count),
      frame.from_index_,
      frame.to_index_};
  const auto apply_stiffness = [&](const Series &flat) {
    return kernel.apply(flat.mul(free).reshape(node_shape))
        .reshape(flat_shape)
        .mul(free)
        .add(flat.mul(fixed));
  };
  const auto apply_mass = [&](const Series &flat) {
    return kernel.apply_mass(flat.mul(free).reshape(node_shape))
        .reshape(flat_shape)
        .mul(free);
  };
  const Series diagonal =
      kernel.diagonal(pluss::series::zeros(node_shape,
                                           config(frame.position_.scalar_type(),
                                                  frame.position_.platform())))
          .reshape(flat_shape)
          .mul(free)
          .add(fixed);
  if (!diagonal.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::modes: a free degree of freedom has no stiffness");
  const auto solve_stiffness = [&](const Series &rhs) {
    const auto solved = pluss::subsp::math::krylov::pcg(
        apply_stiffness,
        [&](const Series &residual) { return residual.div(diagonal); }, rhs,
        {-1}, options.solve);
    if (!solved.converged.all().item<bool>())
      throw std::runtime_error(
          "eng::structure::modes: an inverse stiffness solve did not "
          "converge");
    return solved.solution.mul(free);
  };
  const pluss::subsp::linearalg::GenEigOptions eig_options{
      options.max_iterations, options.relative_tolerance,
      options.absolute_tolerance};
  auto spectral =
      spectral_seed(layout, dofs, options, operation)
          .linearalg.gen_eigh(apply_stiffness, apply_mass, solve_stiffness,
                              eig_options);
  const Series mass_norm =
      spectral.vectors.mul(apply_mass(spectral.vectors))
          .sum(-1, false)
          .sqrt();
  const Series vectors = spectral.vectors.div(mass_norm.unsqueeze(-1));
  const Series omega = spectral.values.sqrt();
  return {omega,
          omega.div(2.0 * 3.141592653589793238462643383279502884),
          vectors.reshape(node_shape),
          spectral.residual.div(mass_norm),
          spectral.converged,
          spectral.iterations};
}

BuckResult buckling(const Frame &frame, const Series &reference_load,
                    const Fix &fix, const EigOptions &options) {
  constexpr const char *operation = "eng::structure::buckling";
  validate_eig_options(options, operation);
  const SolveLayout layout = prepare_solve(
      frame.position_, reference_load, fix, frame.nodes_, 3, operation);
  const FrameKernel static_kernel{
      expand_tail(frame.direction_, layout.batch, 2, operation),
      expand_tail(frame.length_, layout.batch, 1, operation),
      expand_tail(frame.area_, layout.batch, 1, operation),
      expand_tail(frame.inertia_, layout.batch, 1, operation),
      expand_tail(frame.modulus_, layout.batch, 1, operation),
      frame.density_.defined()
          ? expand_tail(frame.density_, layout.batch, 1, operation)
          : Series{},
      frame.from_index_,
      frame.to_index_};
  const CoreResult prestress = solve_system(
      static_kernel, layout, SolveOptions{options.solve, true}, operation);
  const Series end_force = static_kernel.local_force(prestress.displacement);
  const Series compression =
      end_force.select(-1, 0).sub(end_force.select(-1, 3)).mul(0.5);

  const std::int64_t dofs = frame.nodes_ * 3;
  Shape node_shape = layout.batch;
  node_shape.push_back(options.count);
  node_shape.push_back(frame.nodes_);
  node_shape.push_back(3);
  Shape flat_shape = layout.batch;
  flat_shape.push_back(options.count);
  flat_shape.push_back(dofs);
  const Series free =
      insert_mode(layout.free_mask, layout.batch, 1, options.count);
  const Series fixed =
      insert_mode(layout.fixed_mask, layout.batch, 1, options.count);
  const FrameKernel kernel{
      insert_mode(static_kernel.direction, layout.batch, 2, options.count),
      insert_mode(static_kernel.length, layout.batch, 1, options.count),
      insert_mode(static_kernel.area, layout.batch, 1, options.count),
      insert_mode(static_kernel.inertia, layout.batch, 1, options.count),
      insert_mode(static_kernel.modulus, layout.batch, 1, options.count),
      frame.density_.defined()
          ? insert_mode(static_kernel.density, layout.batch, 1, options.count)
          : Series{},
      frame.from_index_,
      frame.to_index_};
  const Series mode_compression =
      insert_mode(compression, layout.batch, 1, options.count);
  const auto apply_stiffness = [&](const Series &flat) {
    return kernel.apply(flat.mul(free).reshape(node_shape))
        .reshape(flat_shape)
        .mul(free)
        .add(flat.mul(fixed));
  };
  const auto apply_geometric = [&](const Series &flat) {
    return kernel
        .apply_geometric(flat.mul(free).reshape(node_shape), mode_compression)
        .reshape(flat_shape)
        .mul(free);
  };
  const Series diagonal =
      kernel.diagonal(pluss::series::zeros(node_shape,
                                           config(frame.position_.scalar_type(),
                                                  frame.position_.platform())))
          .reshape(flat_shape)
          .mul(free)
          .add(fixed);
  if (!diagonal.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::buckling: a free degree of freedom has no "
        "stiffness");
  const auto solve_stiffness = [&](const Series &rhs) {
    const auto solved = pluss::subsp::math::krylov::pcg(
        apply_stiffness,
        [&](const Series &residual) { return residual.div(diagonal); }, rhs,
        {-1}, options.solve);
    if (!solved.converged.all().item<bool>())
      throw std::runtime_error(
          "eng::structure::buckling: an inverse stiffness solve did not "
          "converge");
    return solved.solution.mul(free);
  };
  const pluss::subsp::linearalg::GenEigOptions eig_options{
      options.max_iterations, options.relative_tolerance,
      options.absolute_tolerance};
  auto spectral =
      spectral_seed(layout, dofs, options, operation)
          .linearalg.gen_eigh(apply_stiffness, apply_geometric,
                              solve_stiffness, eig_options);
  const Series norm = spectral.vectors.abs().amax(-1).unsqueeze(-1);
  const Series vectors = spectral.vectors.div(norm);
  return {spectral.values,
          vectors.reshape(node_shape),
          spectral.residual.div(norm.squeeze(-1)),
          spectral.converged,
          spectral.iterations};
}

} // namespace pluss::subsp::eng::structure
