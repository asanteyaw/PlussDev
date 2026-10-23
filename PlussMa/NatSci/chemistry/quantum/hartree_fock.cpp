#include "pluss/subsp/natsci/chemistry/quantum.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::chemistry::quantum {
namespace {

constexpr const char *operation = "natsci::chemistry::quantum::hf";

[[noreturn]] void fail(const std::string &message) {
  throw std::invalid_argument(std::string(operation) + ": " + message);
}

std::vector<std::int64_t> batch_shape(const Series &value,
                                      std::int64_t trailing) {
  const auto &shape = value.sizes();
  return {shape.begin(), shape.end() - trailing};
}

std::vector<std::int64_t> shape_of(const Series &value) {
  const auto shape = value.sizes();
  return {shape.begin(), shape.end()};
}

std::vector<std::int64_t>
broadcast_shape(const std::vector<std::int64_t> &left,
                const std::vector<std::int64_t> &right) {
  const std::size_t rank = std::max(left.size(), right.size());
  std::vector<std::int64_t> result(rank, 1);
  for (std::size_t offset = 0; offset < rank; ++offset) {
    const std::int64_t l =
        offset < left.size() ? left[left.size() - 1 - offset] : 1;
    const std::int64_t r =
        offset < right.size() ? right[right.size() - 1 - offset] : 1;
    if (l != r && l != 1 && r != 1)
      fail("leading batch dimensions are not broadcastable");
    result[rank - 1 - offset] = std::max(l, r);
  }
  return result;
}

std::vector<std::int64_t> with_tail(const std::vector<std::int64_t> &batch,
                                    std::initializer_list<std::int64_t> tail) {
  std::vector<std::int64_t> shape = batch;
  shape.insert(shape.end(), tail.begin(), tail.end());
  return shape;
}

Series expand_to(const Series &value, const std::vector<std::int64_t> &shape) {
  return value.sizes() == shape ? value : value.expand(shape, true);
}

std::vector<std::int64_t> identity_order(std::int64_t rank) {
  std::vector<std::int64_t> order(static_cast<std::size_t>(rank));
  for (std::int64_t axis = 0; axis < rank; ++axis)
    order[static_cast<std::size_t>(axis)] = axis;
  return order;
}

Series swap_axes(const Series &value, std::int64_t first, std::int64_t second) {
  auto order = identity_order(value.dim());
  const auto normalize = [rank = value.dim()](std::int64_t axis) {
    return axis < 0 ? axis + rank : axis;
  };
  std::swap(order[static_cast<std::size_t>(normalize(first))],
            order[static_cast<std::size_t>(normalize(second))]);
  return value.permute(order);
}

Series exchange_order(const Series &eri) {
  auto order = identity_order(eri.dim());
  const std::int64_t rank = eri.dim();
  // (mu, nu | lambda, sigma) -> (mu, lambda | nu, sigma)
  std::swap(order[static_cast<std::size_t>(rank - 3)],
            order[static_cast<std::size_t>(rank - 2)]);
  return eri.permute(order);
}

Series pair_order(const Series &eri) {
  auto order = identity_order(eri.dim());
  const std::int64_t rank = eri.dim();
  order[static_cast<std::size_t>(rank - 4)] = rank - 2;
  order[static_cast<std::size_t>(rank - 3)] = rank - 1;
  order[static_cast<std::size_t>(rank - 2)] = rank - 4;
  order[static_cast<std::size_t>(rank - 1)] = rank - 3;
  return eri.permute(order);
}

void require_finite(const Series &value, const char *role) {
  if (!value.isfinite().all().item<bool>())
    fail(std::string(role) + " must contain only finite values");
}

struct Prepared final {
  Series overlap;
  Series core;
  Series eri;
  Series exchange_eri;
  Series nuclear;
  Series orthogonalizer;
  std::vector<std::int64_t> batch;
  std::int64_t nao{};
};

Prepared prepare(const AoIntegrals &input, const HfOptions &options) {
  if (!input.overlap.defined() || !input.core.defined() || !input.eri.defined())
    fail("overlap, core, and eri must be defined");
  if (input.overlap.dim() < 2 || input.core.dim() < 2 || input.eri.dim() < 4)
    fail("overlap/core need two matrix axes and eri needs four AO axes");
  if (!input.overlap.dtype().is_floating() ||
      !input.core.dtype().is_floating() || !input.eri.dtype().is_floating())
    fail("AO integrals must use a real floating dtype");
  if (input.overlap.scalar_type() != input.core.scalar_type() ||
      input.overlap.scalar_type() != input.eri.scalar_type())
    fail("AO integrals must share one dtype");
  if (input.overlap.platform() != input.core.platform() ||
      input.overlap.platform() != input.eri.platform())
    fail("AO integrals must share one platform");
  if (options.max_iterations <= 0 || !(options.energy_tolerance > 0.0) ||
      !std::isfinite(options.energy_tolerance) ||
      !(options.gradient_tolerance > 0.0) ||
      !std::isfinite(options.gradient_tolerance) || options.damping < 0.0 ||
      options.damping >= 1.0 || !std::isfinite(options.damping) ||
      !(options.overlap_cutoff > 0.0) || !std::isfinite(options.overlap_cutoff))
    fail("invalid iteration, tolerance, damping, or overlap option");
  if (options.diis_start < 0 || options.diis_space < 0 ||
      options.diis_space == 1 || !(options.diis_rcond > 0.0) ||
      !std::isfinite(options.diis_rcond))
    fail("DIIS start/space/rcond options are invalid");

  const std::int64_t nao = input.overlap.size(-1);
  if (nao <= 0 || input.overlap.size(-2) != nao || input.core.size(-2) != nao ||
      input.core.size(-1) != nao)
    fail("overlap and core must be non-empty square AO matrices");
  for (std::int64_t axis = 1; axis <= 4; ++axis)
    if (input.eri.size(-axis) != nao)
      fail("eri's four trailing AO dimensions must match the matrices");
  auto batch = broadcast_shape(batch_shape(input.overlap, 2),
                               batch_shape(input.core, 2));
  batch = broadcast_shape(batch, batch_shape(input.eri, 4));
  if (input.nuclear_energy.defined()) {
    if (!input.nuclear_energy.dtype().is_floating())
      fail("nuclear energy must use a real floating dtype");
    if (input.nuclear_energy.platform() != input.overlap.platform())
      fail("nuclear energy must share the AO-integral platform");
    batch = broadcast_shape(batch, shape_of(input.nuclear_energy));
  }

  Prepared out;
  out.batch = batch;
  out.nao = nao;
  out.overlap = expand_to(input.overlap, with_tail(batch, {nao, nao}));
  out.core = expand_to(input.core, with_tail(batch, {nao, nao}));
  out.eri = expand_to(input.eri, with_tail(batch, {nao, nao, nao, nao}));
  out.exchange_eri = exchange_order(out.eri);
  if (input.nuclear_energy.defined()) {
    out.nuclear = input.nuclear_energy.scalar_type() == out.core.scalar_type()
                      ? input.nuclear_energy
                      : input.nuclear_energy.to(out.core.scalar_type());
    out.nuclear = expand_to(out.nuclear, batch);
  } else {
    out.nuclear = out.core.sum(-1, false).sum(-1, false).mul(0.0);
  }

  require_finite(out.overlap, "overlap");
  require_finite(out.core, "core");
  require_finite(out.eri, "eri");
  require_finite(out.nuclear, "nuclear energy");
  const double symmetry_tolerance =
      out.core.scalar_type() == ScalarType::Float32 ? 2e-5 : 1e-10;
  if (!out.overlap.allclose(out.overlap.transpose(-2, -1), symmetry_tolerance,
                            symmetry_tolerance) ||
      !out.core.allclose(out.core.transpose(-2, -1), symmetry_tolerance,
                         symmetry_tolerance))
    fail("overlap and core must be symmetric");
  if (options.verify_symmetry &&
      (!out.eri.allclose(swap_axes(out.eri, -4, -3), symmetry_tolerance,
                         symmetry_tolerance) ||
       !out.eri.allclose(swap_axes(out.eri, -2, -1), symmetry_tolerance,
                         symmetry_tolerance) ||
       !out.eri.allclose(pair_order(out.eri), symmetry_tolerance,
                         symmetry_tolerance)))
    fail("eri must obey chemists'-notation permutation symmetry");

  const auto metric = out.overlap.linearalg.eigh();
  if (!metric.values.gt(options.overlap_cutoff).all().item<bool>())
    fail("overlap must be positive definite above overlap_cutoff");
  out.orthogonalizer = metric.vectors.mul(metric.values.rsqrt().unsqueeze(-2))
                           .matmul(metric.vectors.transpose(-2, -1));
  return out;
}

std::pair<Series, Series> orbitals(const Prepared &data, const Series &fock) {
  const Series transformed =
      data.orthogonalizer.transpose(-2, -1).matmul(fock).matmul(
          data.orthogonalizer);
  auto decomposition = transformed.linearalg.eigh();
  return {std::move(decomposition.values),
          data.orthogonalizer.matmul(decomposition.vectors)};
}

Series density_from(const Series &coefficients, std::int64_t occupied) {
  const Series occ = coefficients.slice(-1, 0, occupied, 1);
  return occ.matmul(occ.transpose(-2, -1)).mul(2.0);
}

Series spin_density_from(const Series &coefficients, std::int64_t occupied) {
  if (occupied == 0)
    return coefficients.mul(0.0);
  const Series occ = coefficients.slice(-1, 0, occupied, 1);
  return occ.matmul(occ.transpose(-2, -1));
}

Series coulomb_from(const Prepared &data, const Series &density) {
  const Series weighted = density.unsqueeze(-3).unsqueeze(-3);
  return data.eri.mul(weighted).sum(-1, false).sum(-1, false);
}

Series exchange_from(const Prepared &data, const Series &density) {
  const Series weighted = density.unsqueeze(-3).unsqueeze(-3);
  return data.exchange_eri.mul(weighted).sum(-1, false).sum(-1, false);
}

Series fock_from(const Prepared &data, const Series &density) {
  return data.core.add(coulomb_from(data, density))
      .sub(exchange_from(data, density).mul(0.5));
}

Series electronic_energy(const Prepared &data, const Series &density,
                         const Series &fock) {
  return density.transpose(-2, -1)
      .mul(data.core.add(fock))
      .sum(-1, false)
      .sum(-1, false)
      .mul(0.5);
}

Series density_rms(const Series &next, const Series &previous) {
  return next.sub(previous).square().mean(-1, false).mean(-1, false).sqrt();
}

Series orbital_error(const Prepared &data, const Series &density,
                     const Series &fock) {
  const Series commutator = fock.matmul(density)
                                .matmul(data.overlap)
                                .sub(data.overlap.matmul(density).matmul(fock));
  return data.orthogonalizer.transpose(-2, -1)
      .matmul(commutator)
      .matmul(data.orthogonalizer);
}

Series orbital_gradient_rms(const Series &coefficients, const Series &fock,
                            std::int64_t occupied) {
  const std::int64_t orbitals = coefficients.size(-1);
  if (occupied == orbitals)
    return fock.sum(-1, false).sum(-1, false).mul(0.0);

  const Series occupied_coefficients = coefficients.slice(-1, 0, occupied, 1);
  const Series virtual_coefficients =
      coefficients.slice(-1, occupied, orbitals, 1);
  const Series gradient = virtual_coefficients.transpose(-2, -1)
                              .matmul(fock)
                              .matmul(occupied_coefficients)
                              .mul(2.0);
  return gradient.square().mean(-1, false).mean(-1, false).sqrt();
}

Series stack_last(const std::vector<Series> &values) {
  Series out = values.front().unsqueeze(-1);
  if (values.size() > 1) {
    std::vector<Series> rest;
    rest.reserve(values.size() - 1);
    for (std::size_t index = 1; index < values.size(); ++index)
      rest.push_back(values[index].unsqueeze(-1));
    out = out.unite(rest, -1);
  }
  return out;
}

Series stack_rows(const std::vector<Series> &rows) {
  Series out = rows.front().unsqueeze(-2);
  if (rows.size() > 1) {
    std::vector<Series> rest;
    rest.reserve(rows.size() - 1);
    for (std::size_t index = 1; index < rows.size(); ++index)
      rest.push_back(rows[index].unsqueeze(-2));
    out = out.unite(rest, -2);
  }
  return out;
}

Series diis_weights(const std::vector<Series> &errors,
                    const std::vector<Series> *paired_errors,
                    double rcond) {
  const std::size_t count = errors.size();
  std::vector<std::vector<Series>> gram(
      count, std::vector<Series>(count, pluss::series::undefined()));
  for (std::size_t row = 0; row < count; ++row) {
    for (std::size_t column = row; column < count; ++column) {
      Series entry =
          errors[row].mul(errors[column]).sum(-1, false).sum(-1, false);
      if (paired_errors != nullptr)
        entry = entry.add((*paired_errors)[row]
                              .mul((*paired_errors)[column])
                              .sum(-1, false)
                              .sum(-1, false));
      gram[row][column] = entry;
      gram[column][row] = entry;
    }
  }

  const Series zero = gram.front().front().mul(0.0);
  const Series minus_one = zero.sub(1.0);
  std::vector<Series> rows;
  rows.reserve(count + 1);
  for (std::size_t row = 0; row < count; ++row) {
    std::vector<Series> entries = gram[row];
    entries.push_back(minus_one);
    rows.push_back(stack_last(entries));
  }
  std::vector<Series> constraint(count, minus_one);
  constraint.push_back(zero);
  rows.push_back(stack_last(constraint));
  const Series pulay = stack_rows(rows);

  std::vector<Series> rhs_entries(count, zero);
  rhs_entries.push_back(minus_one);
  const Series rhs = stack_last(rhs_entries);
  return pluss::subsp::linearalg::pinverse(pulay, rcond)
      .matmul(rhs.unsqueeze(-1))
      .squeeze(-1)
      .slice(-1, 0, static_cast<std::int64_t>(count), 1);
}

Series mix_focks(const std::vector<Series> &focks, const Series &weights) {
  Series mixed =
      focks.front().mul(weights.select(-1, 0).unsqueeze(-1).unsqueeze(-1));
  for (std::size_t index = 1; index < focks.size(); ++index)
    mixed = mixed.add(
        focks[index].mul(weights.select(-1, static_cast<std::int64_t>(index))
                             .unsqueeze(-1)
                             .unsqueeze(-1)));
  return mixed;
}

Series diis_fock(const std::vector<Series> &focks,
                 const std::vector<Series> &errors, double rcond) {
  return mix_focks(focks, diis_weights(errors, nullptr, rcond));
}

std::pair<Series, Series> uhf_focks(const Prepared &data,
                                    const Series &alpha_density,
                                    const Series &beta_density) {
  const Series coulomb =
      coulomb_from(data, alpha_density.add(beta_density));
  return {data.core.add(coulomb).sub(exchange_from(data, alpha_density)),
          data.core.add(coulomb).sub(exchange_from(data, beta_density))};
}

Series uhf_energy(const Prepared &data, const Series &alpha_density,
                  const Series &beta_density, const Series &alpha_fock,
                  const Series &beta_fock) {
  const Series alpha = alpha_density.transpose(-2, -1)
                           .mul(data.core.add(alpha_fock))
                           .sum(-1, false)
                           .sum(-1, false);
  const Series beta = beta_density.transpose(-2, -1)
                          .mul(data.core.add(beta_fock))
                          .sum(-1, false)
                          .sum(-1, false);
  return alpha.add(beta).mul(0.5);
}

Series uhf_density_rms(const Series &next_alpha, const Series &alpha,
                       const Series &next_beta, const Series &beta) {
  const double elements =
      static_cast<double>(2 * alpha.size(-2) * alpha.size(-1));
  return next_alpha.sub(alpha)
      .square()
      .sum(-1, false)
      .sum(-1, false)
      .add(next_beta.sub(beta).square().sum(-1, false).sum(-1, false))
      .div(elements)
      .sqrt();
}

std::pair<Series, std::int64_t>
orbital_gradient_square(const Series &coefficients, const Series &fock,
                        std::int64_t occupied) {
  const std::int64_t orbitals = coefficients.size(-1);
  const std::int64_t elements = occupied * (orbitals - occupied);
  if (elements == 0)
    return {fock.sum(-1, false).sum(-1, false).mul(0.0), 0};

  const Series occupied_coefficients = coefficients.slice(-1, 0, occupied, 1);
  const Series virtual_coefficients =
      coefficients.slice(-1, occupied, orbitals, 1);
  const Series gradient = virtual_coefficients.transpose(-2, -1)
                              .matmul(fock)
                              .matmul(occupied_coefficients);
  return {gradient.square().sum(-1, false).sum(-1, false), elements};
}

Series uhf_gradient_rms(const Series &alpha_coefficients,
                        const Series &beta_coefficients,
                        const Series &alpha_fock, const Series &beta_fock,
                        std::int64_t alpha_electrons,
                        std::int64_t beta_electrons) {
  auto alpha = orbital_gradient_square(alpha_coefficients, alpha_fock,
                                       alpha_electrons);
  auto beta = orbital_gradient_square(beta_coefficients, beta_fock,
                                      beta_electrons);
  const std::int64_t count = alpha.second + beta.second;
  if (count == 0)
    return alpha.first.add(beta.first);
  return alpha.first.add(beta.first).div(static_cast<double>(count)).sqrt();
}

Series spin_squared(const Prepared &data, const Series &alpha_coefficients,
                    const Series &beta_coefficients,
                    std::int64_t alpha_electrons,
                    std::int64_t beta_electrons) {
  Series overlap_norm =
      data.core.sum(-1, false).sum(-1, false).mul(0.0);
  if (alpha_electrons > 0 && beta_electrons > 0) {
    const Series alpha =
        alpha_coefficients.slice(-1, 0, alpha_electrons, 1);
    const Series beta = beta_coefficients.slice(-1, 0, beta_electrons, 1);
    overlap_norm = alpha.transpose(-2, -1)
                       .matmul(data.overlap)
                       .matmul(beta)
                       .square()
                       .sum(-1, false)
                       .sum(-1, false);
  }
  const double spin_z =
      0.5 * static_cast<double>(alpha_electrons - beta_electrons);
  const double electron_term =
      0.5 * static_cast<double>(alpha_electrons + beta_electrons);
  return overlap_norm.mul(-1.0).add(spin_z * spin_z + electron_term);
}

Series prepare_guess_density(const Prepared &data, const Series &density,
                             const char *spin) {
  if (!density.defined() || density.dim() < 2 ||
      density.size(-2) != data.nao || density.size(-1) != data.nao ||
      density.scalar_type() != data.core.scalar_type() ||
      density.platform() != data.core.platform())
    fail(std::string(spin) +
         " initial density must match the AO matrix shape, dtype, and platform");
  Series expanded =
      expand_to(density, with_tail(data.batch, {data.nao, data.nao}));
  require_finite(expanded, spin);
  const double tolerance =
      data.core.scalar_type() == ScalarType::Float32 ? 2e-5 : 1e-10;
  if (!expanded.allclose(expanded.transpose(-2, -1), tolerance, tolerance))
    fail(std::string(spin) + " initial density must be symmetric");
  return expanded;
}

UhfResult solve_uhf(const Prepared &data, std::int64_t alpha_electrons,
                    std::int64_t beta_electrons, const UhfGuess *initial,
                    const HfOptions &options) {
  Series alpha_density;
  Series beta_density;
  if (initial != nullptr) {
    alpha_density =
        prepare_guess_density(data, initial->alpha, "alpha");
    beta_density = prepare_guess_density(data, initial->beta, "beta");
  } else {
    const auto [energy, coefficients] = orbitals(data, data.core);
    (void)energy;
    alpha_density = spin_density_from(coefficients, alpha_electrons);
    beta_density = spin_density_from(coefficients, beta_electrons);
  }

  auto initial_fock = uhf_focks(data, alpha_density, beta_density);
  Series alpha_fock = std::move(initial_fock.first);
  Series beta_fock = std::move(initial_fock.second);
  Series electronic = uhf_energy(data, alpha_density, beta_density, alpha_fock,
                                 beta_fock);
  Series total = electronic.add(data.nuclear);
  Series rms = total.mul(0.0).add(std::numeric_limits<double>::infinity());
  Series gradient = rms;
  Series delta = rms;
  Series converged = total.isfinite().logical_and(total.mul(0.0).eq(1.0));
  Series alpha_orbital_energy = pluss::series::undefined();
  Series beta_orbital_energy = pluss::series::undefined();
  Series alpha_coefficients = pluss::series::undefined();
  Series beta_coefficients = pluss::series::undefined();
  std::vector<Series> alpha_fock_history;
  std::vector<Series> beta_fock_history;
  std::vector<Series> alpha_error_history;
  std::vector<Series> beta_error_history;
  if (options.diis_space > 0) {
    const auto space = static_cast<std::size_t>(options.diis_space);
    alpha_fock_history.reserve(space);
    beta_fock_history.reserve(space);
    alpha_error_history.reserve(space);
    beta_error_history.reserve(space);
  }
  std::int64_t iterations = 0;
  std::int64_t diis_steps = 0;

  for (; iterations < options.max_iterations; ++iterations) {
    Series alpha_orbital_fock = alpha_fock;
    Series beta_orbital_fock = beta_fock;
    if (options.diis_space > 0) {
      alpha_fock_history.push_back(alpha_fock);
      beta_fock_history.push_back(beta_fock);
      alpha_error_history.push_back(
          orbital_error(data, alpha_density, alpha_fock));
      beta_error_history.push_back(
          orbital_error(data, beta_density, beta_fock));
      if (static_cast<std::int64_t>(alpha_fock_history.size()) >
          options.diis_space) {
        alpha_fock_history.erase(alpha_fock_history.begin());
        beta_fock_history.erase(beta_fock_history.begin());
        alpha_error_history.erase(alpha_error_history.begin());
        beta_error_history.erase(beta_error_history.begin());
      }
      if (iterations >= options.diis_start &&
          alpha_fock_history.size() >= 2) {
        const Series weights = diis_weights(
            alpha_error_history, &beta_error_history, options.diis_rcond);
        alpha_orbital_fock = mix_focks(alpha_fock_history, weights);
        beta_orbital_fock = mix_focks(beta_fock_history, weights);
        ++diis_steps;
      }
    }

    auto alpha_orbit = orbitals(data, alpha_orbital_fock);
    auto beta_orbit = orbitals(data, beta_orbital_fock);
    alpha_orbital_energy = std::move(alpha_orbit.first);
    alpha_coefficients = std::move(alpha_orbit.second);
    beta_orbital_energy = std::move(beta_orbit.first);
    beta_coefficients = std::move(beta_orbit.second);
    Series next_alpha =
        spin_density_from(alpha_coefficients, alpha_electrons);
    Series next_beta = spin_density_from(beta_coefficients, beta_electrons);
    if (options.damping > 0.0) {
      next_alpha = next_alpha.mul(1.0 - options.damping)
                       .add(alpha_density.mul(options.damping));
      next_beta = next_beta.mul(1.0 - options.damping)
                      .add(beta_density.mul(options.damping));
    }
    auto next_fock = uhf_focks(data, next_alpha, next_beta);
    const Series next_electronic =
        uhf_energy(data, next_alpha, next_beta, next_fock.first,
                   next_fock.second);
    const Series next_total = next_electronic.add(data.nuclear);
    rms = uhf_density_rms(next_alpha, alpha_density, next_beta, beta_density);
    gradient = uhf_gradient_rms(
        alpha_coefficients, beta_coefficients, next_fock.first,
        next_fock.second, alpha_electrons, beta_electrons);
    delta = next_total.sub(total).abs();
    converged = delta.le(options.energy_tolerance)
                    .logical_and(gradient.le(options.gradient_tolerance));

    alpha_density = std::move(next_alpha);
    beta_density = std::move(next_beta);
    alpha_fock = std::move(next_fock.first);
    beta_fock = std::move(next_fock.second);
    electronic = next_electronic;
    total = next_total;
    if (converged.all().item<bool>()) {
      ++iterations;
      break;
    }
  }

  auto final_alpha = orbitals(data, alpha_fock);
  auto final_beta = orbitals(data, beta_fock);
  alpha_orbital_energy = std::move(final_alpha.first);
  alpha_coefficients = std::move(final_alpha.second);
  beta_orbital_energy = std::move(final_beta.first);
  beta_coefficients = std::move(final_beta.second);
  Series spin = spin_squared(data, alpha_coefficients, beta_coefficients,
                             alpha_electrons, beta_electrons);
  return UhfResult{std::move(total),
                   std::move(electronic),
                   std::move(alpha_orbital_energy),
                   std::move(beta_orbital_energy),
                   std::move(alpha_coefficients),
                   std::move(beta_coefficients),
                   std::move(alpha_density),
                   std::move(beta_density),
                   std::move(alpha_fock),
                   std::move(beta_fock),
                   std::move(rms),
                   std::move(gradient),
                   std::move(delta),
                   std::move(spin),
                   std::move(converged),
                   iterations,
                   diis_steps};
}

RhfResult solve(const Prepared &data, std::int64_t electrons,
                const Series *initial_density, const HfOptions &options) {
  const std::int64_t occupied = electrons / 2;
  Series density;
  if (initial_density != nullptr) {
    if (!initial_density->defined() || initial_density->dim() < 2 ||
        initial_density->size(-2) != data.nao ||
        initial_density->size(-1) != data.nao ||
        initial_density->scalar_type() != data.core.scalar_type() ||
        initial_density->platform() != data.core.platform())
      fail("initial density must match the AO matrix shape, dtype, and "
           "platform");
    density = expand_to(*initial_density,
                        with_tail(data.batch, {data.nao, data.nao}));
    require_finite(density, "initial density");
  } else {
    const auto [energy, coefficients] = orbitals(data, data.core);
    (void)energy;
    density = density_from(coefficients, occupied);
  }

  Series fock = fock_from(data, density);
  Series electronic = electronic_energy(data, density, fock);
  Series total = electronic.add(data.nuclear);
  Series rms = total.mul(0.0).add(std::numeric_limits<double>::infinity());
  Series gradient = total.mul(0.0).add(std::numeric_limits<double>::infinity());
  Series delta = rms;
  Series converged = total.isfinite().logical_and(total.mul(0.0).eq(1.0));
  Series orbital_energy = pluss::series::undefined();
  Series coefficients = pluss::series::undefined();
  std::vector<Series> fock_history;
  std::vector<Series> error_history;
  if (options.diis_space > 0) {
    fock_history.reserve(static_cast<std::size_t>(options.diis_space));
    error_history.reserve(static_cast<std::size_t>(options.diis_space));
  }
  std::int64_t iterations = 0;
  std::int64_t diis_steps = 0;

  for (; iterations < options.max_iterations; ++iterations) {
    Series orbital_fock = fock;
    if (options.diis_space > 0) {
      fock_history.push_back(fock);
      error_history.push_back(orbital_error(data, density, fock));
      if (static_cast<std::int64_t>(fock_history.size()) > options.diis_space) {
        fock_history.erase(fock_history.begin());
        error_history.erase(error_history.begin());
      }
      if (iterations >= options.diis_start && fock_history.size() >= 2) {
        orbital_fock =
            diis_fock(fock_history, error_history, options.diis_rcond);
        ++diis_steps;
      }
    }

    auto orbit = orbitals(data, orbital_fock);
    orbital_energy = std::move(orbit.first);
    coefficients = std::move(orbit.second);
    Series next_density = density_from(coefficients, occupied);
    if (options.damping > 0.0)
      next_density = next_density.mul(1.0 - options.damping)
                         .add(density.mul(options.damping));
    const Series next_fock = fock_from(data, next_density);
    const Series next_electronic =
        electronic_energy(data, next_density, next_fock);
    const Series next_total = next_electronic.add(data.nuclear);
    rms = density_rms(next_density, density);
    gradient = orbital_gradient_rms(coefficients, next_fock, occupied);
    delta = next_total.sub(total).abs();
    converged = delta.le(options.energy_tolerance)
                    .logical_and(gradient.le(options.gradient_tolerance));

    density = std::move(next_density);
    fock = next_fock;
    electronic = next_electronic;
    total = next_total;
    if (converged.all().item<bool>()) {
      ++iterations;
      break;
    }
  }

  // Report orbitals of the final Fock matrix, not the matrix from the prior
  // density update. The density remains the converged mixed iterate.
  auto final_orbit = orbitals(data, fock);
  orbital_energy = std::move(final_orbit.first);
  coefficients = std::move(final_orbit.second);
  return RhfResult{std::move(total),
                   std::move(electronic),
                   std::move(orbital_energy),
                   std::move(coefficients),
                   std::move(density),
                   std::move(fock),
                   std::move(rms),
                   std::move(gradient),
                   std::move(delta),
                   std::move(converged),
                   iterations,
                   diis_steps};
}

} // namespace

RhfResult rhf(const AoIntegrals &integrals, std::int64_t electrons,
              const HfOptions &options) {
  if (electrons <= 0 || electrons % 2 != 0)
    fail("RHF requires a positive even electron count");
  const Prepared data = prepare(integrals, options);
  if (electrons > 2 * data.nao)
    fail("RHF electron count cannot exceed 2*nao");
  return solve(data, electrons, nullptr, options);
}

RhfResult rhf(const AoIntegrals &integrals, std::int64_t electrons,
              const Series &initial_density, const HfOptions &options) {
  if (electrons <= 0 || electrons % 2 != 0)
    fail("RHF requires a positive even electron count");
  const Prepared data = prepare(integrals, options);
  if (electrons > 2 * data.nao)
    fail("RHF electron count cannot exceed 2*nao");
  return solve(data, electrons, &initial_density, options);
}

UhfResult uhf(const AoIntegrals &integrals, std::int64_t alpha_electrons,
              std::int64_t beta_electrons, const HfOptions &options) {
  if (alpha_electrons < 0 || beta_electrons < 0 ||
      alpha_electrons + beta_electrons <= 0)
    fail("UHF spin populations must be non-negative and contain an electron");
  const Prepared data = prepare(integrals, options);
  if (alpha_electrons > data.nao || beta_electrons > data.nao)
    fail("each UHF spin population cannot exceed nao");
  return solve_uhf(data, alpha_electrons, beta_electrons, nullptr, options);
}

UhfResult uhf(const AoIntegrals &integrals, std::int64_t alpha_electrons,
              std::int64_t beta_electrons, const UhfGuess &initial_density,
              const HfOptions &options) {
  if (alpha_electrons < 0 || beta_electrons < 0 ||
      alpha_electrons + beta_electrons <= 0)
    fail("UHF spin populations must be non-negative and contain an electron");
  const Prepared data = prepare(integrals, options);
  if (alpha_electrons > data.nao || beta_electrons > data.nao)
    fail("each UHF spin population cannot exceed nao");
  return solve_uhf(data, alpha_electrons, beta_electrons, &initial_density,
                   options);
}

} // namespace pluss::subsp::natsci::chemistry::quantum
