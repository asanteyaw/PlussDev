#include <optional>
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

ScalarType complex_dtype(ScalarType dtype) {
  return dtype == ScalarType::Float32 ? ScalarType::Complex64
                                      : ScalarType::Complex128;
}

bool finite_numeric(const Series &value) {
  return value.dtype().is_complex()
             ? value.real().isfinite().all().item<bool>() &&
                   value.imag().isfinite().all().item<bool>()
             : value.isfinite().all().item<bool>();
}

bool matches_precision(ScalarType model, ScalarType value) {
  return value == model ||
         (model == ScalarType::Float32 && value == ScalarType::Complex64) ||
         (model == ScalarType::Float64 && value == ScalarType::Complex128);
}

Series phasor(const Series &value, ScalarType dtype) {
  if (value.scalar_type() == dtype)
    return value;
  return value.to(dtype);
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

  Series mass_diagonal(const Series &like) const {
    const Series c2 = direction.select(-1, 0).square();
    const Series s2 = direction.select(-1, 1).square();
    const Series scale = density.mul(area).mul(length).div(420.0);
    const Series axial = scale.mul(140.0);
    const Series bend = scale.mul(156.0);
    const Series rotary = scale.mul(length.square()).mul(4.0);
    const Series contribution = lanes({axial.mul(c2).add(bend.mul(s2)),
                                       axial.mul(s2).add(bend.mul(c2)), rotary})
                                    .to(like.platform(), like.scalar_type());
    return pluss::series::zeros_like(like)
        .scatter_add(-2, element_index(from, contribution), contribution)
        .scatter_add(-2, element_index(to, contribution), contribution);
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

struct CorotLinearization {
  Series normal;
  Series tangent;
  Series length;
  Series axial;
  Series moment_i;
  Series moment_j;
  Series axial_stiffness;
  Series bending_stiffness;
  Series from;
  Series to;

  CorotLinearization detached() const {
    return {normal.detach(),
            tangent.detach(),
            length.detach(),
            axial.detach(),
            moment_i.detach(),
            moment_j.detach(),
            axial_stiffness.detach(),
            bending_stiffness.detach(),
            from,
            to};
  }

  Series assemble(const Series &start, const Series &finish,
                  const Series &like) const {
    return pluss::series::zeros_like(like)
        .scatter_add(-2, element_index(from, start), start)
        .scatter_add(-2, element_index(to, finish), finish);
  }

  Series internal(const Series &like) const {
    const Series shear = moment_i.add(moment_j).div(length);
    const Series start_xy =
        normal.mul(axial.neg().unsqueeze(-1))
            .add(tangent.mul(shear.unsqueeze(-1)));
    const Series start = lanes({start_xy.select(-1, 0), start_xy.select(-1, 1),
                                moment_i});
    const Series finish =
        lanes({start_xy.select(-1, 0).neg(), start_xy.select(-1, 1).neg(),
               moment_j});
    return assemble(start, finish, like);
  }

  Series apply(const Series &increment) const {
    const Series start = increment.index_select(-2, from);
    const Series finish = increment.index_select(-2, to);
    const Series delta = finish.slice(-1, 0, 2).sub(start.slice(-1, 0, 2));
    const Series extension = delta.mul(normal).sum(-1, false);
    const Series rotation = delta.mul(tangent).sum(-1, false).div(length);
    const Series turn_i = start.select(-1, 2).sub(rotation);
    const Series turn_j = finish.select(-1, 2).sub(rotation);
    const Series d_axial = axial_stiffness.mul(extension);
    const Series d_moment_i =
        bending_stiffness.mul(turn_i.mul(4.0).add(turn_j.mul(2.0)));
    const Series d_moment_j =
        bending_stiffness.mul(turn_i.mul(2.0).add(turn_j.mul(4.0)));
    const Series d_normal = tangent.mul(rotation.unsqueeze(-1));
    const Series d_gradient =
        normal.mul(rotation.div(length).neg().unsqueeze(-1))
            .sub(tangent.mul(extension.div(length.square()).unsqueeze(-1)));
    const Series moment_sum = moment_i.add(moment_j);
    const Series d_moment_sum = d_moment_i.add(d_moment_j);
    const Series start_xy =
        normal.mul(d_axial.neg().unsqueeze(-1))
            .sub(d_normal.mul(axial.unsqueeze(-1)))
            .add(tangent.mul(d_moment_sum.div(length).unsqueeze(-1)))
            .add(d_gradient.mul(moment_sum.unsqueeze(-1)));
    const Series start_force =
        lanes({start_xy.select(-1, 0), start_xy.select(-1, 1), d_moment_i});
    const Series finish_force =
        lanes({start_xy.select(-1, 0).neg(), start_xy.select(-1, 1).neg(),
               d_moment_j});
    return assemble(start_force, finish_force, increment);
  }

  Series end_force() const {
    const Series shear = moment_i.add(moment_j).div(length);
    return lanes({axial.neg(), shear, moment_i, axial, shear.neg(), moment_j});
  }

  Series energy() const {
    const Series extension = axial.div(axial_stiffness);
    const Series rhs_i = moment_i.mul(2.0).sub(moment_j).div(6.0);
    const Series rhs_j = moment_j.mul(2.0).sub(moment_i).div(6.0);
    const Series turn_i = rhs_i.div(bending_stiffness);
    const Series turn_j = rhs_j.div(bending_stiffness);
    return axial.mul(extension)
        .add(moment_i.mul(turn_i))
        .add(moment_j.mul(turn_j))
        .sum(-1, false)
        .mul(0.5);
  }
};

struct CorotKernel {
  Series position;
  Series direction;
  Series reference_length;
  Series area;
  Series inertia;
  Series modulus;
  Series from;
  Series to;

  CorotLinearization linearize(const Series &displacement) const {
    const Series start_position = position.index_select(-2, from);
    const Series finish_position = position.index_select(-2, to);
    const Series start = displacement.index_select(-2, from);
    const Series finish = displacement.index_select(-2, to);
    // The extension is formed as (L^2 - L0^2) / (L + L0) = (2 c0.d + d.d) / (L + L0) from the reference chord c0 and the relative
    // displacement d, so a member that has hardly moved reports its tiny extension to full relative accuracy instead of the rounding
    // noise of subtracting two nearly equal lengths (which, times a large axial stiffness, would swamp small loads).
    const Series reference_chord = finish_position.sub(start_position);
    const Series relative =
        finish.slice(-1, 0, 2).sub(start.slice(-1, 0, 2));
    const Series chord = reference_chord.add(relative);
    const Series length = chord.square().sum(-1, false).sqrt();
    const Series extension =
        reference_chord.mul(relative).sum(-1, false).mul(2.0)
            .add(relative.square().sum(-1, false))
            .div(length.add(reference_length));
    if (!length.gt(0.0).all().item<bool>())
      throw std::runtime_error(
          "eng::structure: a deformed frame member collapsed to zero length");
    const Series normal = chord.div(length.unsqueeze(-1));
    const Series tangent =
        lanes({normal.select(-1, 1).neg(), normal.select(-1, 0)});
    const Series angle = chord.select(-1, 1).atan2(chord.select(-1, 0));
    const Series reference_angle =
        direction.select(-1, 1).atan2(direction.select(-1, 0));
    const Series rigid_rotation = angle.sub(reference_angle);
    const Series turn_i = start.select(-1, 2).sub(rigid_rotation);
    const Series turn_j = finish.select(-1, 2).sub(rigid_rotation);
    const Series axial_stiffness = modulus.mul(area).div(reference_length);
    const Series bending_stiffness = modulus.mul(inertia).div(reference_length);
    const Series axial = axial_stiffness.mul(extension);
    const Series moment_i =
        bending_stiffness.mul(turn_i.mul(4.0).add(turn_j.mul(2.0)));
    const Series moment_j =
        bending_stiffness.mul(turn_i.mul(2.0).add(turn_j.mul(4.0)));
    return {normal, tangent, length, axial, moment_i, moment_j,
            axial_stiffness, bending_stiffness, from, to};
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
                          std::int64_t dofs_per_node, const char *operation,
                          bool allow_complex = false) {
  if (!load.defined() || load.dim() < 2 ||
      (!load.dtype().is_floating() &&
       !(allow_complex && load.dtype().is_complex())) ||
      load.size(-2) != nodes || load.size(-1) != dofs_per_node)
    throw std::invalid_argument(std::string(operation) +
                                (allow_complex
                                     ? ": load must be floating or complex "
                                     : ": load must be real floating ") +
                                "[..., node, dof]");
  if (!matches_precision(model.scalar_type(), load.scalar_type()) ||
      load.platform() != model.platform())
    throw std::invalid_argument(std::string(operation) +
                                ": load must share the structure precision "
                                "and platform");
  if (!finite_numeric(load))
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
    if (fix.value.dim() < 1 ||
        (!fix.value.dtype().is_floating() &&
         !(allow_complex && fix.value.dtype().is_complex())) ||
        fix.value.size(-1) != constrained ||
        fix.value.scalar_type() != load.scalar_type() ||
        fix.value.platform() != load.platform() ||
        !finite_numeric(fix.value))
      throw std::invalid_argument(
          std::string(operation) +
          (allow_complex
               ? ": constraint values must be finite floating or complex "
               : ": constraint values must be finite real floating ") +
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
      !solved.converged.all().template item<bool>()) {
    if (solved.breakdown.any().template item<bool>())
      throw std::runtime_error(
          std::string(operation) +
          ": the structure has a direction with no stiffness under this "
          "load, so it is a mechanism; check the supports and the bar "
          "connectivity");
    throw std::runtime_error(
        std::string(operation) +
        ": the stiffness solve stopped after " +
        std::to_string(solved.iterations) +
        " iterations without reaching its tolerance; raise cg.max_iterations "
        "or loosen cg.relative_tolerance (long or slender structures are "
        "poorly conditioned)");
  }
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
    throw std::invalid_argument(std::string(operation) +
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
      dofs - static_cast<std::int64_t>(layout.fixed_mask.reshape({cases, dofs})
                                           .select(0, 0)
                                           .sum()
                                           .item<double>());
  if (options.count > free_dofs)
    throw std::invalid_argument(std::string(operation) +
                                ": count exceeds the free degree count");
  const Config cfg = config(layout.load.scalar_type(), layout.load.platform());
  const Series lane =
      pluss::series::arange(1, dofs + 1, cfg).reshape({1, dofs});
  const Series mode = pluss::series::arange(1, options.count + 1, cfg)
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

Series stack_time(const std::vector<Series> &values) {
  Series result = values.front().unsqueeze(-3);
  if (values.size() == 1)
    return result;
  std::vector<Series> rest;
  rest.reserve(values.size() - 1);
  for (std::size_t index = 1; index < values.size(); ++index)
    rest.push_back(values[index].unsqueeze(-3));
  return result.unite(rest, -3, true);
}

Series stack_samples(const std::vector<Series> &values) {
  Series result = values.front().unsqueeze(-1);
  if (values.size() == 1)
    return result;
  std::vector<Series> rest;
  rest.reserve(values.size() - 1);
  for (std::size_t index = 1; index < values.size(); ++index)
    rest.push_back(values[index].unsqueeze(-1));
  return result.unite(rest, -1, true);
}

Series inspect_state(const Series &value, const Series &model,
                     const Shape &batch, std::int64_t nodes, const char *role,
                     const char *operation) {
  if (!value.defined()) {
    Shape shape = batch;
    shape.push_back(nodes);
    shape.push_back(3);
    return pluss::series::zeros(shape,
                                config(model.scalar_type(), model.platform()));
  }
  if (value.dim() < 2 || !value.dtype().is_floating() ||
      value.size(-2) != nodes || value.size(-1) != 3 ||
      value.scalar_type() != model.scalar_type() ||
      value.platform() != model.platform() ||
      !value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) + ": initial " + role +
                                " must be finite real floating "
                                "[..., node, 3] with the frame dtype and "
                                "platform");
  return expand_tail(value, batch, 2, operation);
}

Series state_norm(const Series &flat) {
  return flat.square().sum(-1, false).sqrt();
}

void validate_nonlinear_controls(std::int64_t max_iterations,
                                 std::int64_t max_line_search,
                                 double relative_tolerance,
                                 double absolute_tolerance,
                                 double line_search_shrink, double armijo,
                                 const char *operation) {
  if (max_iterations <= 0 || max_line_search <= 0)
    throw std::invalid_argument(
        std::string(operation) +
        ": iteration and line-search limits must be positive");
  if (!(relative_tolerance >= 0.0) ||
      !std::isfinite(relative_tolerance) ||
      !(absolute_tolerance >= 0.0) ||
      !std::isfinite(absolute_tolerance))
    throw std::invalid_argument(std::string(operation) +
                                ": tolerances must be finite and non-negative");
  if (!(line_search_shrink > 0.0 && line_search_shrink < 1.0) ||
      !std::isfinite(line_search_shrink) || !(armijo > 0.0 && armijo < 1.0) ||
      !std::isfinite(armijo))
    throw std::invalid_argument(
        std::string(operation) +
        ": line_search_shrink and armijo must be finite and in (0, 1)");
}

struct NewtonStep {
  Series correction;
  Series converged;
  std::int64_t iterations{};
};

template <class Apply>
NewtonStep solve_newton_step(Apply &&apply, const Series &rhs,
                             const Series &free_mask,
                             const pluss::subsp::math::krylov::CgOptions
                                 &options) {
  const Series zero_lane = state_norm(rhs).eq(0.0);
  const bool shift_zero = rhs.tracks_slope() && zero_lane.any().item<bool>();
  const Series probe =
      shift_zero
          ? zero_lane.to(rhs.scalar_type()).unsqueeze(-1).mul(free_mask)
          : pluss::series::zeros_like(rhs);
  const auto primary = pluss::subsp::math::krylov::cg(
      apply, rhs.add(probe), {-1}, options);
  if (!shift_zero)
    return {primary.solution,
            primary.converged.logical_and(primary.breakdown.logical_not())
                .squeeze(-1),
            primary.iterations};
  const auto offset = pluss::subsp::math::krylov::cg(
      apply, probe, {-1}, options);
  return {primary.solution.sub(offset.solution),
          primary.converged.logical_and(offset.converged)
              .logical_and(primary.breakdown.logical_not())
              .logical_and(offset.breakdown.logical_not())
              .squeeze(-1),
          std::max(primary.iterations, offset.iterations)};
}

struct EquilibriumResult {
  Series displacement;
  Series residual_norm;
  Series converged;
  std::int64_t iterations{};
};

// The line search measures progress in the residual scaled by the reference stiffness diagonal, so the very stiff axial and the soft
// bending degrees of a slender member weigh as the displacements they produce rather than as raw forces: a full Newton step on a slender
// member changes the axial force by far more than the bending forces it fixes, and the unscaled norm would reject it.
EquilibriumResult equilibrate(const CorotKernel &kernel,
                              const SolveLayout &layout,
                              const Series &inverse_scale,
                              const Series &external,
                              Series displacement,
                              const Series &prescribed,
                              std::int64_t max_iterations,
                              std::int64_t max_line_search,
                              double relative_tolerance,
                              double absolute_tolerance,
                              double line_search_shrink, double armijo,
                              const pluss::subsp::math::krylov::CgOptions
                                  &solve_options) {
  displacement = displacement.reshape(layout.flat_shape)
                     .mul(layout.free_mask)
                     .add(prescribed.reshape(layout.flat_shape)
                              .mul(layout.fixed_mask))
                     .reshape(layout.node_shape);
  const Series external_flat =
      external.reshape(layout.flat_shape).mul(layout.free_mask);
  const Series scale = state_norm(external_flat);
  // Internal forces cannot be evaluated more accurately than rounding allows, about eps times the stiffness times the size of the
  // structure, so a tolerance below that floor could never be met however many iterations are spent.
  const Series rounding_floor =
      scale.mul(0.0).add(
          inverse_scale.reciprocal().max().mul(
              displacement.abs().max().maximum(kernel.reference_length.max())
                  .mul(100.0 * std::numeric_limits<double>::epsilon())));
  const Series tolerance = scale.mul(relative_tolerance)
                               .maximum(absolute_tolerance)
                               .maximum(rounding_floor);
  Series residual_norm = scale.mul(0.0);
  Series converged = residual_norm.eq(-1.0);
  std::int64_t iterations = 0;

  for (; iterations < max_iterations; ++iterations) {
    const CorotLinearization state = kernel.linearize(displacement);
    const Series residual =
        external.sub(state.internal(displacement))
            .reshape(layout.flat_shape)
            .mul(layout.free_mask);
    residual_norm = state_norm(residual);
    converged = residual_norm.le(tolerance);
    const bool force_linearization =
        iterations == 0 && residual.tracks_slope() &&
        residual_norm.eq(0.0).any().item<bool>();
    if (converged.all().item<bool>() && !force_linearization)
      break;

    const CorotLinearization tangent_state = state.detached();
    const auto tangent = [&](const Series &flat) {
      const Series value =
          flat.mul(layout.free_mask).reshape(layout.node_shape);
      return tangent_state.apply(value)
          .reshape(layout.flat_shape)
          .mul(layout.free_mask)
          .add(flat.mul(layout.fixed_mask));
    };
    const NewtonStep solved =
        solve_newton_step(tangent, residual, layout.free_mask, solve_options);
    if (!solved.converged.all().item<bool>())
      return {displacement, residual_norm, solved.converged, iterations + 1};

    const Series correction =
        solved.correction.mul(layout.free_mask).reshape(layout.node_shape);
    const Series old_merit = state_norm(residual.mul(inverse_scale)).square();
    bool accepted = false;
    double alpha = 1.0;
    for (std::int64_t search = 0; search < max_line_search; ++search) {
      const Series trial = displacement.add(correction.mul(alpha));
      const Series trial_residual =
          external.sub(kernel.linearize(trial).internal(trial))
              .reshape(layout.flat_shape)
              .mul(layout.free_mask);
      const Series trial_merit =
          state_norm(trial_residual.mul(inverse_scale)).square();
      if (trial_merit
              .le(old_merit.mul(1.0 - armijo * alpha)
                      .add(state_norm(inverse_scale.mul(tolerance.unsqueeze(-1)))
                               .square()))
              .all()
              .item<bool>()) {
        displacement = trial;
        accepted = true;
        break;
      }
      alpha *= line_search_shrink;
    }
    if (!accepted)
      return {displacement, residual_norm, converged, iterations + 1};
  }

  const Series residual =
      external.sub(kernel.linearize(displacement).internal(displacement))
          .reshape(layout.flat_shape)
          .mul(layout.free_mask);
  residual_norm = state_norm(residual);
  converged = residual_norm.le(tolerance);
  return {displacement, residual_norm, converged, iterations};
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
  return solve(frame, load, MemberLoad{}, fix, options);
}

FrameResult solve(const Frame &frame, const Series &load,
                  const MemberLoad &member_load, const Fix &fix,
                  const SolveOptions &options) {
  constexpr const char *operation = "eng::structure::solve";
  // Leading axes of the member loads (a set of load intensities) join those of the nodal load.
  Series nodal = load;
  if (load.defined() && (member_load.transverse.defined() || member_load.axial.defined())) {
    Shape batch = leading_shape(load, 2);
    if (member_load.transverse.defined() && member_load.transverse.dim() >= 1)
      batch = broadcast_shape(batch, leading_shape(member_load.transverse, 1), operation);
    if (member_load.axial.defined() && member_load.axial.dim() >= 1)
      batch = broadcast_shape(batch, leading_shape(member_load.axial, 1), operation);
    Shape full = batch;
    full.push_back(load.size(-2));
    full.push_back(load.size(-1));
    nodal = load.add(pluss::series::zeros(full, config(load.scalar_type(), load.platform())));
  }
  SolveLayout layout =
      prepare_solve(frame.position_, nodal, fix, frame.nodes_, 3, operation);
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

  // A uniform load q (transverse) and p (axial) on a member of length L is carried by the consistent nodal loads
  // (p L / 2, q L / 2, q L^2 / 12) at the first node and (p L / 2, q L / 2, -q L^2 / 12) at the second. The member's
  // own end actions are those of the nodal solution plus the fixed-end actions, which are the negatives of these.
  Series equivalent;
  const bool has_transverse = member_load.transverse.defined();
  const bool has_axial = member_load.axial.defined();
  if (has_transverse || has_axial) {
    const auto dtype = frame.position_.scalar_type();
    const auto platform = frame.position_.platform();
    const Series zero = kernel.length.mul(0.0);
    Series q = zero, p = zero;
    if (has_transverse) {
      inspect_values(member_load.transverse, 1, operation, "transverse load",
                     dtype, platform);
      if (member_load.transverse.size(-1) != frame.members_)
        throw std::invalid_argument(
            "eng::structure::solve: the transverse member load must end in "
            "the member count");
      q = expand_tail(member_load.transverse, layout.batch, 1, operation);
    }
    if (has_axial) {
      inspect_values(member_load.axial, 1, operation, "axial load", dtype,
                     platform);
      if (member_load.axial.size(-1) != frame.members_)
        throw std::invalid_argument(
            "eng::structure::solve: the axial member load must end in the "
            "member count");
      p = expand_tail(member_load.axial, layout.batch, 1, operation);
    }
    const Series length = kernel.length;
    const Series force_p = p.mul(length).mul(0.5);
    const Series force_q = q.mul(length).mul(0.5);
    const Series moment = q.mul(length.square()).div(12.0);
    equivalent = lanes({force_p, force_q, moment, force_p, force_q, moment.neg()});
    layout.load = layout.load.add(kernel.assemble(equivalent, layout.load));
  }

  const CoreResult core = solve_system(kernel, layout, options, operation);
  Series end_force = kernel.local_force(core.displacement);
  if (equivalent.defined()) end_force = end_force.sub(equivalent);
  return {core.displacement,
          core.reaction,
          end_force,
          core.displacement.mul(core.internal).sum({-2, -1}, false).mul(0.5),
          core.residual_norm,
          core.converged,
          core.breakdown,
          core.iterations};
}

NonlinearResult solve(const Frame &frame, const Series &load, const Fix &fix,
                      const NonlinearOptions &options) {
  constexpr const char *operation = "eng::structure::solve";
  validate_nonlinear_controls(
      options.max_iterations, options.max_line_search,
      options.relative_tolerance, options.absolute_tolerance,
      options.line_search_shrink, options.armijo, operation);
  if (options.load_steps <= 0 || options.max_cutbacks < 0 ||
      !(options.min_load_step > 0.0 && options.min_load_step <= 1.0) ||
      !std::isfinite(options.min_load_step))
    throw std::invalid_argument(
        "eng::structure::solve: load_steps must be positive, max_cutbacks "
        "must be non-negative, and min_load_step must be in (0, 1]");

  const SolveLayout layout =
      prepare_solve(frame.position_, load, fix, frame.nodes_, 3, operation);
  const CorotKernel kernel{
      expand_tail(frame.position_, layout.batch, 2, operation),
      expand_tail(frame.direction_, layout.batch, 2, operation),
      expand_tail(frame.length_, layout.batch, 1, operation),
      expand_tail(frame.area_, layout.batch, 1, operation),
      expand_tail(frame.inertia_, layout.batch, 1, operation),
      expand_tail(frame.modulus_, layout.batch, 1, operation),
      frame.from_index_,
      frame.to_index_};

  const FrameKernel linear_kernel{
      expand_tail(frame.direction_, layout.batch, 2, operation),
      expand_tail(frame.length_, layout.batch, 1, operation),
      expand_tail(frame.area_, layout.batch, 1, operation),
      expand_tail(frame.inertia_, layout.batch, 1, operation),
      expand_tail(frame.modulus_, layout.batch, 1, operation),
      Series{},
      frame.from_index_,
      frame.to_index_};
  const Series inverse_scale =
      linear_kernel.diagonal(pluss::series::zeros_like(layout.load))
          .abs()
          .reshape(layout.flat_shape)
          .mul(layout.free_mask)
          .add(layout.fixed_mask)
          .reciprocal();

  Series displacement = pluss::series::zeros_like(layout.load);
  double load_factor = 0.0;
  double increment = 1.0 / static_cast<double>(options.load_steps);
  std::int64_t steps = 0;
  std::int64_t cutbacks = 0;
  std::int64_t consecutive_cutbacks = 0;
  std::int64_t iterations = 0;
  Series residual_norm =
      state_norm(layout.load.reshape(layout.flat_shape).mul(0.0));
  Series converged = residual_norm.eq(0.0);

  while (load_factor < 1.0) {
    const double target = std::min(1.0, load_factor + increment);
    const Series external = layout.load.mul(target);
    const Series prescribed =
        layout.prescribed_flat.mul(target).reshape(layout.node_shape);
    const EquilibriumResult attempt = equilibrate(
        kernel, layout, inverse_scale, external, displacement, prescribed,
        options.max_iterations, options.max_line_search,
        options.relative_tolerance, options.absolute_tolerance,
        options.line_search_shrink, options.armijo, options.solve);
    iterations += attempt.iterations;
    if (attempt.converged.all().item<bool>()) {
      displacement = attempt.displacement;
      residual_norm = attempt.residual_norm;
      converged = attempt.converged;
      load_factor = target;
      ++steps;
      consecutive_cutbacks = 0;
      increment = std::min(1.0 / static_cast<double>(options.load_steps),
                           increment * 2.0);
      continue;
    }
    ++cutbacks;
    ++consecutive_cutbacks;
    increment *= 0.5;
    converged = attempt.converged;
    residual_norm = attempt.residual_norm;
    if (consecutive_cutbacks > options.max_cutbacks ||
        increment < options.min_load_step)
      break;
  }

  if (load_factor < 1.0 && options.fail_on_nonconvergence)
    throw std::runtime_error(
        "eng::structure::solve: nonlinear equilibrium did not converge "
        "before the load-step cutback limit");
  const CorotLinearization state = kernel.linearize(displacement);
  const Series applied = layout.load.mul(load_factor);
  const Series internal = state.internal(displacement);
  residual_norm = state_norm(
      applied.sub(internal).reshape(layout.flat_shape).mul(layout.free_mask));
  const Series final_scale =
      state_norm(applied.reshape(layout.flat_shape).mul(layout.free_mask));
  const Series final_floor =
      final_scale.mul(0.0).add(
          inverse_scale.reciprocal().max().mul(
              displacement.abs().max().maximum(kernel.reference_length.max())
                  .mul(100.0 * std::numeric_limits<double>::epsilon())));
  const Series final_tolerance =
      final_scale.mul(options.relative_tolerance)
          .maximum(options.absolute_tolerance)
          .maximum(final_floor);
  converged = residual_norm.le(final_tolerance);
  if (load_factor < 1.0)
    converged = converged.logical_and(converged.logical_not());
  const Series reaction = internal.sub(applied)
                              .reshape(layout.flat_shape)
                              .mul(layout.fixed_mask)
                              .reshape(layout.node_shape);
  return {displacement,
          reaction,
          state.end_force(),
          state.energy(),
          residual_norm,
          converged,
          load_factor,
          iterations,
          steps,
          cutbacks};
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
  const Series zero_load =
      pluss::series::zeros(load_shape, config(frame.position_.scalar_type(),
                                              frame.position_.platform()));
  const SolveLayout layout = prepare_solve(frame.position_, zero_load, fix,
                                           frame.nodes_, 3, operation);
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
      kernel
          .diagonal(pluss::series::zeros(node_shape,
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
  auto spectral = spectral_seed(layout, dofs, options, operation)
                      .linearalg.gen_eigh(apply_stiffness, apply_mass,
                                          solve_stiffness, eig_options);
  const Series mass_norm =
      spectral.vectors.mul(apply_mass(spectral.vectors)).sum(-1, false).sqrt();
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
  const SolveLayout layout = prepare_solve(frame.position_, reference_load, fix,
                                           frame.nodes_, 3, operation);
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
  // Buckling needs a member in compression: a zero, tensile or purely transverse reference load leaves no destabilising axial force, and
  // the eigenproblem would fail with a message about its internals.
  if (!compression.amax(-1)
           .gt(end_force.abs().amax(-1).amax(-1).mul(1e-9))
           .all()
           .item<bool>())
    throw std::invalid_argument(
        "eng::structure::buckling: the reference load puts no member in "
        "compression, so there is no buckling factor; apply a compressive "
        "axial reference load (a zero, tensile or purely transverse load "
        "never buckles a frame in a linearised analysis)");

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
      kernel
          .diagonal(pluss::series::zeros(node_shape,
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
  const auto seed = spectral_seed(layout, dofs, options, operation);
  const auto spectral = [&] {
    try {
      return seed.linearalg.gen_eigh(apply_stiffness, apply_geometric,
                                     solve_stiffness, eig_options);
    } catch (const std::exception &error) {
      throw std::runtime_error(
          std::string(operation) + ": could not extract " +
          std::to_string(options.count) +
          " positive buckling factors (" + error.what() +
          "); ask for fewer modes, or check that the reference load "
          "compresses enough of the structure");
    }
  }();
  const Series norm = spectral.vectors.abs().amax(-1).unsqueeze(-1);
  const Series vectors = spectral.vectors.div(norm);
  return {spectral.values, vectors.reshape(node_shape),
          spectral.residual.div(norm.squeeze(-1)), spectral.converged,
          spectral.iterations};
}

MotionResult motion(const Frame &frame, const Series &load, const Series &time,
                    const Fix &fix, const MotionState &initial,
                    const MotionOptions &options) {
  constexpr const char *operation = "eng::structure::motion";
  if (!frame.density_.defined())
    throw std::invalid_argument(
        "eng::structure::motion: frame sections do not define density");
  if (!time.defined() || time.dim() != 1 || !time.dtype().is_floating() ||
      time.numel() < 2 || time.scalar_type() != frame.position_.scalar_type() ||
      time.platform() != frame.position_.platform() ||
      !time.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: time must be a finite real floating vector "
        "with at least two samples and the frame dtype and platform");
  const Series step = time.slice(0, 1).sub(time.slice(0, 0, -1));
  if (!step.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: time samples must be strictly increasing");
  if (!load.defined() || load.dim() < 3 || !load.dtype().is_floating() ||
      load.size(-3) != time.numel() || load.size(-2) != frame.nodes_ ||
      load.size(-1) != 3 ||
      load.scalar_type() != frame.position_.scalar_type() ||
      load.platform() != frame.position_.platform() ||
      !load.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: load must be finite real floating "
        "[..., time, node, 3] with the frame dtype and platform");
  if (fix.value.defined() && !fix.value.eq(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: transient supports must be homogeneous");
  if (!(options.spectral_radius >= 0.0 && options.spectral_radius <= 1.0) ||
      !std::isfinite(options.spectral_radius))
    throw std::invalid_argument(
        "eng::structure::motion: spectral_radius must be finite and in "
        "[0, 1]");
  if (!(options.mass_damping >= 0.0) || !std::isfinite(options.mass_damping) ||
      !(options.stiffness_damping >= 0.0) ||
      !std::isfinite(options.stiffness_damping))
    throw std::invalid_argument(
        "eng::structure::motion: damping coefficients must be finite and "
        "non-negative");

  Shape batch = broadcast_shape(leading_shape(frame.position_, 2),
                                leading_shape(load, 3), operation);
  if (initial.displacement.defined())
    batch = broadcast_shape(batch, leading_shape(initial.displacement, 2),
                            operation);
  if (initial.velocity.defined())
    batch =
        broadcast_shape(batch, leading_shape(initial.velocity, 2), operation);
  if (fix.value.defined())
    batch = broadcast_shape(batch, leading_shape(fix.value, 1), operation);
  const Series force = expand_tail(load, batch, 3, operation);
  std::vector<Series> force_sample;
  force_sample.reserve(static_cast<std::size_t>(time.numel()));
  for (std::int64_t index = 0; index < time.numel(); ++index)
    force_sample.push_back(force.select(-3, index));
  const SolveLayout layout = prepare_solve(
      frame.position_, force_sample.front(), fix, frame.nodes_, 3, operation);
  const FrameKernel kernel{expand_tail(frame.direction_, batch, 2, operation),
                           expand_tail(frame.length_, batch, 1, operation),
                           expand_tail(frame.area_, batch, 1, operation),
                           expand_tail(frame.inertia_, batch, 1, operation),
                           expand_tail(frame.modulus_, batch, 1, operation),
                           expand_tail(frame.density_, batch, 1, operation),
                           frame.from_index_,
                           frame.to_index_};
  Series displacement =
      inspect_state(initial.displacement, frame.position_, batch, frame.nodes_,
                    "displacement", operation);
  Series velocity = inspect_state(initial.velocity, frame.position_, batch,
                                  frame.nodes_, "velocity", operation);
  if (!displacement.reshape(layout.flat_shape)
           .mul(layout.fixed_mask)
           .eq(0.0)
           .all()
           .item<bool>() ||
      !velocity.reshape(layout.flat_shape)
           .mul(layout.fixed_mask)
           .eq(0.0)
           .all()
           .item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: initial support displacement and velocity "
        "must be zero");

  const auto damp = [&](const Series &value) {
    return kernel.apply_mass(value)
        .mul(options.mass_damping)
        .add(kernel.apply(value).mul(options.stiffness_damping));
  };
  const Series mass_diagonal =
      kernel.mass_diagonal(pluss::series::zeros_like(displacement))
          .reshape(layout.flat_shape);
  const Series stiffness_diagonal =
      kernel.diagonal(pluss::series::zeros_like(displacement))
          .reshape(layout.flat_shape);
  const Series mass_preconditioner =
      mass_diagonal.mul(layout.free_mask).add(layout.fixed_mask);
  if (!mass_preconditioner.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: a free degree of freedom has no mass");

  const auto mass_apply = [&](const Series &flat) {
    return kernel
        .apply_mass(flat.mul(layout.free_mask).reshape(layout.node_shape))
        .reshape(layout.flat_shape)
        .mul(layout.free_mask)
        .add(flat.mul(layout.fixed_mask));
  };
  const Series initial_rhs = layout.load.sub(damp(velocity))
                                 .sub(kernel.apply(displacement))
                                 .reshape(layout.flat_shape)
                                 .mul(layout.free_mask);
  const Series zero_lane = initial_rhs.square().sum(-1, true).eq(0.0);
  const bool shift_zero =
      initial_rhs.tracks_slope() && zero_lane.any().item<bool>();
  const Series probe =
      shift_zero ? zero_lane.to(initial_rhs.scalar_type()).mul(layout.free_mask)
                 : pluss::series::zeros_like(initial_rhs);
  const Series shifted_rhs = initial_rhs.add(probe);
  const auto initial_primary = pluss::subsp::math::krylov::pcg(
      mass_apply,
      [&](const Series &residual) { return residual.div(mass_preconditioner); },
      shifted_rhs, {-1}, options.solve);
  const auto initial_offset =
      shift_zero
          ? pluss::subsp::math::krylov::pcg(
                mass_apply,
                [&](const Series &residual) {
                  return residual.div(mass_preconditioner);
                },
                probe, {-1}, options.solve)
          : pluss::subsp::math::krylov::CgResult{
                pluss::series::zeros_like(initial_primary.solution),
                pluss::series::zeros_like(initial_primary.residual_norm),
                pluss::series::zeros_like(initial_primary.relative_residual),
                pluss::series::ones_like(initial_primary.converged),
                pluss::series::zeros_like(initial_primary.breakdown),
                0};
  const Series initial_solution =
      initial_primary.solution.sub(initial_offset.solution);
  const Series initial_residual = initial_rhs.sub(mass_apply(initial_solution));
  const Series initial_residual_norm =
      initial_residual.square().sum(-1, false).sqrt();
  const Series initial_converged =
      initial_primary.converged.logical_and(initial_offset.converged)
          .squeeze(-1);
  const Series initial_breakdown =
      initial_primary.breakdown.logical_or(initial_offset.breakdown)
          .squeeze(-1);
  if (options.fail_on_nonconvergence && !initial_converged.all().item<bool>())
    throw std::runtime_error(
        "eng::structure::motion: initial acceleration solve did not "
        "converge");
  Series acceleration =
      initial_solution.mul(layout.free_mask).reshape(layout.node_shape);

  std::vector<Series> displacements{displacement};
  std::vector<Series> velocities{velocity};
  std::vector<Series> accelerations{acceleration};
  std::vector<Series> reactions;
  std::vector<Series> kinetic;
  std::vector<Series> strain;
  std::vector<Series> residuals{initial_residual_norm};
  std::vector<Series> converged{initial_converged};
  std::vector<Series> breakdown{initial_breakdown};
  std::vector<std::int64_t> iterations{
      std::max(initial_primary.iterations, initial_offset.iterations)};
  const auto sample_count = static_cast<std::size_t>(time.numel());
  displacements.reserve(sample_count);
  velocities.reserve(sample_count);
  accelerations.reserve(sample_count);
  reactions.reserve(sample_count);
  kinetic.reserve(sample_count);
  strain.reserve(sample_count);
  residuals.reserve(sample_count);
  converged.reserve(sample_count);
  breakdown.reserve(sample_count);
  iterations.reserve(sample_count);
  const auto record = [&](const Series &u, const Series &v, const Series &a,
                          const Series &applied) {
    const Series inertia = kernel.apply_mass(a);
    const Series elastic = kernel.apply(u);
    const Series velocity_mass = kernel.apply_mass(v);
    const Series damping =
        velocity_mass.mul(options.mass_damping)
            .add(kernel.apply(v).mul(options.stiffness_damping));
    reactions.push_back(inertia.add(damping)
                            .add(elastic)
                            .sub(applied)
                            .reshape(layout.flat_shape)
                            .mul(layout.fixed_mask)
                            .reshape(layout.node_shape));
    kinetic.push_back(v.mul(velocity_mass).sum({-2, -1}, false).mul(0.5));
    strain.push_back(u.mul(elastic).sum({-2, -1}, false).mul(0.5));
  };
  record(displacement, velocity, acceleration, force_sample.front());

  const double radius = options.spectral_radius;
  const double alpha_m = (2.0 - radius) / (1.0 + radius);
  const double alpha_f = 1.0 / (1.0 + radius);
  const double beta =
      0.25 * (1.0 + alpha_m - alpha_f) * (1.0 + alpha_m - alpha_f);
  const double gamma = 0.5 + alpha_m - alpha_f;

  for (std::int64_t index = 0; index + 1 < time.numel(); ++index) {
    const Series &current_displacement = displacements.back();
    const Series &current_velocity = velocities.back();
    const Series &current_acceleration = accelerations.back();
    const Series dt = step.select(0, index);
    const Series cu = dt.square().mul(beta * alpha_f / alpha_m);
    const Series cv = dt.mul(gamma * alpha_f / alpha_m);
    const Series pred_u =
        current_displacement.add(current_velocity.mul(dt.mul(alpha_f)))
            .add(current_acceleration.mul(
                dt.square().mul(alpha_f * (0.5 - beta / alpha_m))));
    const Series pred_v = current_velocity.add(
        current_acceleration.mul(dt.mul(alpha_f * (1.0 - gamma / alpha_m))));
    const Series applied =
        force_sample[static_cast<std::size_t>(index)]
            .mul(1.0 - alpha_f)
            .add(
                force_sample[static_cast<std::size_t>(index + 1)].mul(alpha_f));
    const Series rhs = applied.sub(damp(pred_v))
                           .sub(kernel.apply(pred_u))
                           .reshape(layout.flat_shape)
                           .mul(layout.free_mask);
    const Series mass_scale = cv.mul(options.mass_damping).add(1.0);
    const Series stiffness_scale = cu.add(cv.mul(options.stiffness_damping));
    const auto effective = [&](const Series &flat) {
      const Series value =
          flat.mul(layout.free_mask).reshape(layout.node_shape);
      return kernel.apply_mass(value)
          .mul(mass_scale)
          .add(kernel.apply(value).mul(stiffness_scale))
          .reshape(layout.flat_shape)
          .mul(layout.free_mask)
          .add(flat.mul(layout.fixed_mask));
    };
    const Series diagonal = mass_diagonal.mul(mass_scale)
                                .add(stiffness_diagonal.mul(stiffness_scale))
                                .mul(layout.free_mask)
                                .add(layout.fixed_mask);
    const auto step_solved = pluss::subsp::math::krylov::pcg(
        effective,
        [&](const Series &residual) { return residual.div(diagonal); }, rhs,
        {-1}, options.solve);
    if (options.fail_on_nonconvergence &&
        !step_solved.converged.all().item<bool>())
      throw std::runtime_error(
          "eng::structure::motion: effective step solve did not converge");
    const Series alpha_acceleration =
        step_solved.solution.mul(layout.free_mask).reshape(layout.node_shape);
    const Series next_displacement =
        pred_u.add(alpha_acceleration.mul(cu))
            .sub(current_displacement.mul(1.0 - alpha_f))
            .div(alpha_f);
    const Series next_velocity = pred_v.add(alpha_acceleration.mul(cv))
                                     .sub(current_velocity.mul(1.0 - alpha_f))
                                     .div(alpha_f);
    const Series next_acceleration =
        alpha_acceleration.sub(current_acceleration.mul(1.0 - alpha_m))
            .div(alpha_m);
    displacements.push_back(next_displacement);
    velocities.push_back(next_velocity);
    accelerations.push_back(next_acceleration);
    residuals.push_back(step_solved.residual_norm.squeeze(-1));
    converged.push_back(step_solved.converged.squeeze(-1));
    breakdown.push_back(step_solved.breakdown.squeeze(-1));
    iterations.push_back(step_solved.iterations);
    record(displacements.back(), velocities.back(), accelerations.back(),
           force_sample[static_cast<std::size_t>(index + 1)]);
  }

  return {time,
          stack_time(displacements),
          stack_time(velocities),
          stack_time(accelerations),
          stack_time(reactions),
          stack_samples(kinetic),
          stack_samples(strain),
          stack_samples(residuals),
          stack_samples(converged),
          stack_samples(breakdown),
          pluss::series::make(iterations,
                              {static_cast<std::int64_t>(iterations.size())})
              .to(time.platform())};
}

NonlinearMotionResult
motion(const Frame &frame, const Series &load, const Series &time,
       const Fix &fix, const MotionState &initial,
       const NonlinearMotionOptions &options) {
  constexpr const char *operation = "eng::structure::motion";
  if (!frame.density_.defined())
    throw std::invalid_argument(
        "eng::structure::motion: frame sections do not define density");
  if (!time.defined() || time.dim() != 1 || time.numel() < 2 ||
      !time.dtype().is_floating() ||
      time.scalar_type() != frame.position_.scalar_type() ||
      time.platform() != frame.position_.platform() ||
      !time.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: time must be a finite real floating vector "
        "with at least two samples and the frame dtype and platform");
  const Series step = time.slice(0, 1).sub(time.slice(0, 0, -1));
  if (!step.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: time samples must be strictly increasing");
  if (!load.defined() || load.dim() < 3 || !load.dtype().is_floating() ||
      load.size(-3) != time.numel() || load.size(-2) != frame.nodes_ ||
      load.size(-1) != 3 ||
      load.scalar_type() != frame.position_.scalar_type() ||
      load.platform() != frame.position_.platform() ||
      !load.isfinite().all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: load must be finite real floating "
        "[..., time, node, 3] with the frame dtype and platform");
  if (fix.value.defined() && !fix.value.eq(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: nonlinear transient supports must be "
        "homogeneous");
  if (!(options.spectral_radius >= 0.0 && options.spectral_radius <= 1.0) ||
      !std::isfinite(options.spectral_radius))
    throw std::invalid_argument(
        "eng::structure::motion: spectral_radius must be finite and in "
        "[0, 1]");
  if (!(options.mass_damping >= 0.0) || !std::isfinite(options.mass_damping) ||
      !(options.stiffness_damping >= 0.0) ||
      !std::isfinite(options.stiffness_damping))
    throw std::invalid_argument(
        "eng::structure::motion: damping coefficients must be finite and "
        "non-negative");
  validate_nonlinear_controls(
      options.max_iterations, options.max_line_search,
      options.relative_tolerance, options.absolute_tolerance,
      options.line_search_shrink, options.armijo, operation);

  Shape batch = broadcast_shape(leading_shape(frame.position_, 2),
                                leading_shape(load, 3), operation);
  if (initial.displacement.defined())
    batch = broadcast_shape(batch, leading_shape(initial.displacement, 2),
                            operation);
  if (initial.velocity.defined())
    batch =
        broadcast_shape(batch, leading_shape(initial.velocity, 2), operation);
  if (fix.value.defined())
    batch = broadcast_shape(batch, leading_shape(fix.value, 1), operation);
  const Series force = expand_tail(load, batch, 3, operation);
  std::vector<Series> force_sample;
  force_sample.reserve(static_cast<std::size_t>(time.numel()));
  for (std::int64_t index = 0; index < time.numel(); ++index)
    force_sample.push_back(force.select(-3, index));
  const SolveLayout layout = prepare_solve(
      frame.position_, force_sample.front(), fix, frame.nodes_, 3, operation);
  const FrameKernel linear_kernel{
      expand_tail(frame.direction_, batch, 2, operation),
      expand_tail(frame.length_, batch, 1, operation),
      expand_tail(frame.area_, batch, 1, operation),
      expand_tail(frame.inertia_, batch, 1, operation),
      expand_tail(frame.modulus_, batch, 1, operation),
      expand_tail(frame.density_, batch, 1, operation),
      frame.from_index_,
      frame.to_index_};
  const CorotKernel nonlinear_kernel{
      expand_tail(frame.position_, batch, 2, operation),
      expand_tail(frame.direction_, batch, 2, operation),
      expand_tail(frame.length_, batch, 1, operation),
      expand_tail(frame.area_, batch, 1, operation),
      expand_tail(frame.inertia_, batch, 1, operation),
      expand_tail(frame.modulus_, batch, 1, operation),
      frame.from_index_,
      frame.to_index_};
  Series displacement =
      inspect_state(initial.displacement, frame.position_, batch, frame.nodes_,
                    "displacement", operation);
  Series velocity = inspect_state(initial.velocity, frame.position_, batch,
                                  frame.nodes_, "velocity", operation);
  if (!displacement.reshape(layout.flat_shape)
           .mul(layout.fixed_mask)
           .eq(0.0)
           .all()
           .item<bool>() ||
      !velocity.reshape(layout.flat_shape)
           .mul(layout.fixed_mask)
           .eq(0.0)
           .all()
           .item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: initial support displacement and velocity "
        "must be zero");

  const auto damp = [&](const Series &value) {
    return linear_kernel.apply_mass(value)
        .mul(options.mass_damping)
        .add(linear_kernel.apply(value).mul(options.stiffness_damping));
  };
  const Series mass_diagonal =
      linear_kernel.mass_diagonal(pluss::series::zeros_like(displacement))
          .reshape(layout.flat_shape);
  if (!mass_diagonal.mul(layout.free_mask)
           .add(layout.fixed_mask)
           .gt(0.0)
           .all()
           .item<bool>())
    throw std::invalid_argument(
        "eng::structure::motion: a free degree of freedom has no mass");
  const auto mass_apply = [&](const Series &flat) {
    return linear_kernel
        .apply_mass(flat.mul(layout.free_mask).reshape(layout.node_shape))
        .reshape(layout.flat_shape)
        .mul(layout.free_mask)
        .add(flat.mul(layout.fixed_mask));
  };
  const CorotLinearization initial_state =
      nonlinear_kernel.linearize(displacement);
  const Series initial_rhs =
      force_sample.front()
          .sub(damp(velocity))
          .sub(initial_state.internal(displacement))
          .reshape(layout.flat_shape)
          .mul(layout.free_mask);
  const Series stiffness_diagonal =
      linear_kernel.diagonal(pluss::series::zeros_like(displacement))
          .abs()
          .reshape(layout.flat_shape);
  const Series mass_preconditioner =
      mass_diagonal.mul(layout.free_mask).add(layout.fixed_mask);
  pluss::subsp::math::krylov::CgOptions mass_options;
  mass_options.max_iterations = options.solve.max_iterations;
  mass_options.relative_tolerance = options.solve.relative_tolerance;
  mass_options.absolute_tolerance = options.solve.absolute_tolerance;
  mass_options.check_interval = 1;
  const Series zero_lane = state_norm(initial_rhs).eq(0.0);
  const bool shift_zero =
      initial_rhs.tracks_slope() && zero_lane.any().item<bool>();
  const Series probe =
      shift_zero
          ? zero_lane.to(initial_rhs.scalar_type())
                .unsqueeze(-1)
                .mul(layout.free_mask)
          : pluss::series::zeros_like(initial_rhs);
  const auto initial_primary = pluss::subsp::math::krylov::pcg(
      mass_apply,
      [&](const Series &residual) {
        return residual.div(mass_preconditioner);
      },
      initial_rhs.add(probe), {-1}, mass_options);
  const auto initial_offset =
      shift_zero
          ? pluss::subsp::math::krylov::pcg(
                mass_apply,
                [&](const Series &residual) {
                  return residual.div(mass_preconditioner);
                },
                probe, {-1}, mass_options)
          : pluss::subsp::math::krylov::CgResult{
                pluss::series::zeros_like(initial_primary.solution),
                pluss::series::zeros_like(initial_primary.residual_norm),
                pluss::series::zeros_like(initial_primary.relative_residual),
                pluss::series::ones_like(initial_primary.converged),
                pluss::series::zeros_like(initial_primary.breakdown),
                0};
  const Series initial_solution =
      initial_primary.solution.sub(initial_offset.solution);
  const Series initial_converged =
      initial_primary.converged.logical_and(initial_offset.converged).squeeze(-1);
  if (options.fail_on_nonconvergence &&
      !initial_converged.all().item<bool>())
    throw std::runtime_error(
        "eng::structure::motion: initial acceleration solve did not "
        "converge");
  Series acceleration =
      initial_solution.mul(layout.free_mask).reshape(layout.node_shape);
  Series initial_residual =
      initial_rhs.sub(mass_apply(acceleration.reshape(layout.flat_shape)));

  std::vector<Series> displacements{displacement};
  std::vector<Series> velocities{velocity};
  std::vector<Series> accelerations{acceleration};
  std::vector<Series> reactions;
  std::vector<Series> kinetic;
  std::vector<Series> strain;
  std::vector<Series> residuals{state_norm(initial_residual)};
  std::vector<Series> converged{initial_converged};
  std::vector<std::int64_t> iterations{0};
  const auto sample_count = static_cast<std::size_t>(time.numel());
  displacements.reserve(sample_count);
  velocities.reserve(sample_count);
  accelerations.reserve(sample_count);
  reactions.reserve(sample_count);
  kinetic.reserve(sample_count);
  strain.reserve(sample_count);
  residuals.reserve(sample_count);
  converged.reserve(sample_count);
  iterations.reserve(sample_count);
  const auto record = [&](const Series &u, const Series &v, const Series &a,
                          const Series &applied) {
    const Series inertia = linear_kernel.apply_mass(a);
    const Series velocity_mass = linear_kernel.apply_mass(v);
    const CorotLinearization state = nonlinear_kernel.linearize(u);
    const Series internal = state.internal(u);
    const Series damping =
        velocity_mass.mul(options.mass_damping)
            .add(linear_kernel.apply(v).mul(options.stiffness_damping));
    reactions.push_back(inertia.add(damping)
                            .add(internal)
                            .sub(applied)
                            .reshape(layout.flat_shape)
                            .mul(layout.fixed_mask)
                            .reshape(layout.node_shape));
    kinetic.push_back(v.mul(velocity_mass).sum({-2, -1}, false).mul(0.5));
    strain.push_back(state.energy());
  };
  record(displacement, velocity, acceleration, force_sample.front());

  const double radius = options.spectral_radius;
  const double alpha_m = (2.0 - radius) / (1.0 + radius);
  const double alpha_f = 1.0 / (1.0 + radius);
  const double beta =
      0.25 * (1.0 + alpha_m - alpha_f) * (1.0 + alpha_m - alpha_f);
  const double gamma = 0.5 + alpha_m - alpha_f;

  for (std::int64_t index = 0; index + 1 < time.numel(); ++index) {
    const Series &current_displacement = displacements.back();
    const Series &current_velocity = velocities.back();
    const Series &current_acceleration = accelerations.back();
    const Series dt = step.select(0, index);
    const Series cu = dt.square().mul(beta * alpha_f / alpha_m);
    const Series cv = dt.mul(gamma * alpha_f / alpha_m);
    const Series pred_u =
        current_displacement.add(current_velocity.mul(dt.mul(alpha_f)))
            .add(current_acceleration.mul(
                dt.square().mul(alpha_f * (0.5 - beta / alpha_m))));
    const Series pred_v = current_velocity.add(
        current_acceleration.mul(dt.mul(alpha_f * (1.0 - gamma / alpha_m))));
    const Series applied =
        force_sample[static_cast<std::size_t>(index)]
            .mul(1.0 - alpha_f)
            .add(
                force_sample[static_cast<std::size_t>(index + 1)].mul(alpha_f));
    Series alpha_acceleration = current_acceleration;
    // progress is measured in the residual scaled by the effective stiffness diagonal (see equilibrate)
    const Series inverse_scale =
        mass_diagonal.add(stiffness_diagonal.mul(cu))
            .mul(layout.free_mask)
            .add(layout.fixed_mask)
            .reciprocal();
    const Series load_scale =
        state_norm(applied.reshape(layout.flat_shape).mul(layout.free_mask));
    // The internal forces of a stiff member cannot be evaluated more accurately than rounding allows, about eps times its stiffness
    // times its size, so a tolerance below that floor can never be met (a rigidly rotated, unloaded rod is the extreme case).
    const Series rounding_floor =
        load_scale.mul(0.0).add(
            stiffness_diagonal.max().mul(
                pred_u.abs().max().maximum(nonlinear_kernel.reference_length.max())
                    .mul(100.0 * std::numeric_limits<double>::epsilon())));
    const Series tolerance =
        load_scale.mul(options.relative_tolerance)
            .maximum(options.absolute_tolerance)
            .maximum(rounding_floor);
    Series residual_norm = load_scale.mul(0.0);
    Series step_converged = residual_norm.eq(-1.0);
    std::int64_t step_iterations = 0;

    for (; step_iterations < options.max_iterations; ++step_iterations) {
      const Series trial_u = pred_u.add(alpha_acceleration.mul(cu));
      const Series trial_v = pred_v.add(alpha_acceleration.mul(cv));
      const CorotLinearization state = nonlinear_kernel.linearize(trial_u);
      const Series residual =
          applied.sub(linear_kernel.apply_mass(alpha_acceleration))
              .sub(damp(trial_v))
              .sub(state.internal(trial_u))
              .reshape(layout.flat_shape)
              .mul(layout.free_mask);
      residual_norm = state_norm(residual);
      step_converged = residual_norm.le(tolerance);
      const bool force_linearization =
          step_iterations == 0 && residual.tracks_slope() &&
          residual_norm.eq(0.0).any().item<bool>();
      if (step_converged.all().item<bool>() && !force_linearization)
        break;
      const CorotLinearization tangent_state = state.detached();
      const auto effective = [&](const Series &flat) {
        const Series value =
            flat.mul(layout.free_mask).reshape(layout.node_shape);
        return linear_kernel.apply_mass(value)
            .add(linear_kernel.apply_mass(value)
                     .mul(cv.mul(options.mass_damping)))
            .add(linear_kernel.apply(value)
                     .mul(cv.mul(options.stiffness_damping)))
            .add(tangent_state.apply(value).mul(cu))
            .reshape(layout.flat_shape)
            .mul(layout.free_mask)
            .add(flat.mul(layout.fixed_mask));
      };
      const NewtonStep solved = solve_newton_step(
          effective, residual, layout.free_mask, options.solve);
      if (!solved.converged.all().item<bool>()) {
        step_converged = solved.converged;
        break;
      }
      const Series correction = solved.correction.mul(layout.free_mask)
                                    .reshape(layout.node_shape);
      const Series old_merit = state_norm(residual.mul(inverse_scale)).square();
      bool accepted = false;
      double scale = 1.0;
      for (std::int64_t search = 0; search < options.max_line_search;
           ++search) {
        const Series candidate = alpha_acceleration.add(correction.mul(scale));
        const Series candidate_u = pred_u.add(candidate.mul(cu));
        const Series candidate_v = pred_v.add(candidate.mul(cv));
        const Series candidate_residual =
            applied.sub(linear_kernel.apply_mass(candidate))
                .sub(damp(candidate_v))
                .sub(nonlinear_kernel.linearize(candidate_u)
                         .internal(candidate_u))
                .reshape(layout.flat_shape)
                .mul(layout.free_mask);
        const Series candidate_merit =
            state_norm(candidate_residual.mul(inverse_scale)).square();
        if (candidate_merit
                .le(old_merit.mul(1.0 - options.armijo * scale)
                        .add(state_norm(inverse_scale.mul(
                                            tolerance.unsqueeze(-1)))
                                 .square()))
                .all()
                .item<bool>()) {
          alpha_acceleration = candidate;
          accepted = true;
          break;
        }
        scale *= options.line_search_shrink;
      }
      if (!accepted)
        break;
    }

    const Series alpha_u = pred_u.add(alpha_acceleration.mul(cu));
    const Series alpha_v = pred_v.add(alpha_acceleration.mul(cv));
    const Series final_residual =
        applied.sub(linear_kernel.apply_mass(alpha_acceleration))
            .sub(damp(alpha_v))
            .sub(nonlinear_kernel.linearize(alpha_u).internal(alpha_u))
            .reshape(layout.flat_shape)
            .mul(layout.free_mask);
    residual_norm = state_norm(final_residual);
    step_converged = residual_norm.le(tolerance);
    if (options.fail_on_nonconvergence &&
        !step_converged.all().item<bool>())
      throw std::runtime_error(
          "eng::structure::motion: nonlinear effective step did not "
          "converge");
    const Series next_displacement =
        alpha_u.sub(current_displacement.mul(1.0 - alpha_f)).div(alpha_f);
    const Series next_velocity =
        alpha_v.sub(current_velocity.mul(1.0 - alpha_f)).div(alpha_f);
    const Series next_acceleration =
        alpha_acceleration.sub(current_acceleration.mul(1.0 - alpha_m))
            .div(alpha_m);
    displacements.push_back(next_displacement);
    velocities.push_back(next_velocity);
    accelerations.push_back(next_acceleration);
    residuals.push_back(residual_norm);
    converged.push_back(step_converged);
    iterations.push_back(step_iterations);
    record(next_displacement, next_velocity, next_acceleration,
           force_sample[static_cast<std::size_t>(index + 1)]);
  }

  return {time,
          stack_time(displacements),
          stack_time(velocities),
          stack_time(accelerations),
          stack_time(reactions),
          stack_samples(kinetic),
          stack_samples(strain),
          stack_samples(residuals),
          stack_samples(converged),
          pluss::series::make(iterations,
                              {static_cast<std::int64_t>(iterations.size())})
              .to(time.platform())};
}

HarmonicResult harmonic(const Frame &frame, const Series &load,
                        const Series &omega, const Fix &fix,
                        const HarmonicOptions &options) {
  constexpr const char *operation = "eng::structure::harmonic";
  if (!frame.density_.defined())
    throw std::invalid_argument(
        "eng::structure::harmonic: frame sections do not define density");
  if (!omega.defined() || omega.dim() != 1 || omega.numel() == 0 ||
      !omega.dtype().is_floating() ||
      omega.scalar_type() != frame.position_.scalar_type() ||
      omega.platform() != frame.position_.platform() ||
      !omega.isfinite().all().item<bool>() || !omega.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::harmonic: omega must be a non-empty, finite, "
        "non-negative real vector with the frame dtype and platform");
  if (!load.defined() || load.dim() < 3 ||
      (!load.dtype().is_floating() && !load.dtype().is_complex()) ||
      load.size(-3) != omega.numel() || load.size(-2) != frame.nodes_ ||
      load.size(-1) != 3 ||
      !matches_precision(frame.position_.scalar_type(), load.scalar_type()) ||
      load.platform() != frame.position_.platform() || !finite_numeric(load))
    throw std::invalid_argument(
        "eng::structure::harmonic: load must be finite [..., frequency, "
        "node, 3] with the frame precision and platform");
  if (!(options.mass_damping >= 0.0) ||
      !std::isfinite(options.mass_damping) ||
      !(options.stiffness_damping >= 0.0) ||
      !std::isfinite(options.stiffness_damping))
    throw std::invalid_argument(
        "eng::structure::harmonic: damping coefficients must be finite and "
        "non-negative");

  const ScalarType dtype = complex_dtype(frame.position_.scalar_type());
  const Series force = phasor(load, dtype);
  Fix harmonic_fix{fix.node, fix.axis, {}};
  if (fix.value.defined()) {
    if (fix.value.dim() < 1 ||
        (!fix.value.dtype().is_floating() &&
         !fix.value.dtype().is_complex()) ||
        fix.value.size(-1) != static_cast<std::int64_t>(fix.node.size()) ||
        !matches_precision(frame.position_.scalar_type(),
                           fix.value.scalar_type()) ||
        fix.value.platform() != frame.position_.platform() ||
        !finite_numeric(fix.value))
      throw std::invalid_argument(
          "eng::structure::harmonic: support amplitudes must be finite "
          "[..., constrained dof] with the frame precision and platform");
    if (!fix.value.abs().eq(0.0).all().item<bool>())
      throw std::invalid_argument(
          "eng::structure::harmonic: support amplitudes must be zero");
    harmonic_fix.value = phasor(fix.value.unsqueeze(-2), dtype);
  }

  // The explicit singleton keeps frame-parameter batches independent from
  // the frequency axis carried by the force amplitudes.
  const Series model = frame.position_.unsqueeze(-3);
  const SolveLayout layout =
      prepare_solve(model, force, harmonic_fix, frame.nodes_, 3, operation,
                    true);
  const FrameKernel kernel{
      expand_tail(frame.direction_.unsqueeze(-3), layout.batch, 2, operation),
      expand_tail(frame.length_.unsqueeze(-2), layout.batch, 1, operation),
      expand_tail(frame.area_.unsqueeze(-2), layout.batch, 1, operation),
      expand_tail(frame.inertia_.unsqueeze(-2), layout.batch, 1, operation),
      expand_tail(frame.modulus_.unsqueeze(-2), layout.batch, 1, operation),
      expand_tail(frame.density_.unsqueeze(-2), layout.batch, 1, operation),
      frame.from_index_,
      frame.to_index_};

  Shape omega_shape(layout.batch.size(), 1);
  omega_shape.back() = omega.numel();
  const Series omega_batch = omega.reshape(omega_shape).expand(layout.batch, true);
  const Series omega_node = omega_batch.unsqueeze(-1).unsqueeze(-1);
  const Series iomega = pluss::series::complex(
      pluss::series::zeros_like(omega_node), omega_node, dtype);
  const Series omega2 = omega_node.square().to(dtype);
  const auto dynamic = [&](const Series &value) {
    const Series stiffness = kernel.apply(value);
    const Series inertia = kernel.apply_mass(value);
    const Series damping =
        inertia.mul(options.mass_damping)
            .add(stiffness.mul(options.stiffness_damping));
    return stiffness.add(damping.mul(iomega)).sub(inertia.mul(omega2));
  };
  const auto apply_flat = [&](const Series &flat) {
    const Series value =
        flat.mul(layout.free_mask).reshape(layout.node_shape);
    return dynamic(value)
        .reshape(layout.flat_shape)
        .mul(layout.free_mask)
        .add(flat.mul(layout.fixed_mask));
  };

  const Series stiffness_diagonal =
      kernel.diagonal(pluss::series::zeros_like(layout.load))
          .abs()
          .reshape(layout.flat_shape);
  const Series free = layout.free_mask.real();
  const Series fixed = layout.fixed_mask.real();
  if (!stiffness_diagonal.mul(free).add(fixed).gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "eng::structure::harmonic: a free degree of freedom has no "
        "stiffness; add a support or load-bearing element");

  const Series rhs = layout.load.reshape(layout.flat_shape).mul(layout.free_mask);
  const auto solved = pluss::subsp::math::krylov::gmres(
      apply_flat, rhs, {-1}, options.solve);
  if (options.fail_on_nonconvergence &&
      !solved.converged.all().item<bool>())
    throw std::runtime_error(
        "eng::structure::harmonic: dynamic stiffness solve did not "
        "converge; a sampled frequency may be singular or poorly "
        "conditioned");

  const Series displacement =
      solved.solution.mul(layout.free_mask).reshape(layout.node_shape);
  const Series velocity = displacement.mul(iomega);
  const Series acceleration = displacement.mul(omega2).neg();
  const Series internal = dynamic(displacement);
  const Series reaction = internal.sub(layout.load)
                              .reshape(layout.flat_shape)
                              .mul(layout.fixed_mask)
                              .reshape(layout.node_shape);
  return {omega,
          displacement,
          velocity,
          acceleration,
          reaction,
          solved.residual_norm.squeeze(-1),
          solved.relative_residual.squeeze(-1),
          solved.converged.squeeze(-1),
          solved.iterations,
          solved.operator_evaluations,
          solved.restarts};
}

} // namespace pluss::subsp::eng::structure
